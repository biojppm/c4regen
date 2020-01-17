#ifndef _c4_REGEN_WRITER_HPP_
#define _c4_REGEN_WRITER_HPP_

#include <set>
#include "c4/regen/source_file.hpp"

#include <c4/c4_push.hpp>

namespace c4 {
namespace regen {

bool is_hdr(csubstr filename);
bool is_src(csubstr filename);

void add_hdr(const char* ext);
void add_src(const char* ext);


//-----------------------------------------------------------------------------

/** the template used to render each chunk of generated code */
constexpr const char s_default_tpl_chunk[] = R"(
// generator: {{generator.name}}
// entity: {{entity.name}} @ {{entity.file}}:{{entity.line}}
{{gencode}}
)";


//-----------------------------------------------------------------------------

/** the template used to render header files */
constexpr const char s_default_tpl_hdr[] = R"({% if has_hdr != 0 %}
// DO NOT EDIT!
// This was automatically generated with https://github.com/biojppm/c4regen

#ifndef {{hdr.include_guard}}
#define {{hdr.include_guard}}

{{hdr.preamble}}

{% if namespace %}
{{namespace.begin}}
{% endif %}

{{hdr.gencode}}

{% if namespace %}
{{namespace.end}}
{% endif %}

{% if has_inl %}
#include "{{inl.filename}}"
{% endif %}

#endif // {{hdr.include_guard}}
{% endif %})";


//-----------------------------------------------------------------------------

/** the template used to render inline (definition header) files */
constexpr const char s_default_tpl_inl[] = R"({% if has_inl != 0 %}
// DO NOT EDIT!
// This was automatically generated with https://github.com/biojppm/c4regen

#ifndef {{inl.include_guard}}
#define {{inl.include_guard}}

{% if has_hdr %}
#ifndef {{hdr.include_guard}}
#include "{{hdr.filename}}"
#endif
{% endif %}

{{inl.preamble}}

{% if namespace %}
{{namespace.begin}}
{% endif %}

{{inl.gencode}}

{% if namespace %}
{{namespace.end}}
{% endif %}

#endif // {{inl.include_guard}}
{% endif %})";


//-----------------------------------------------------------------------------

/** the template used to render source files */
constexpr const char s_default_tpl_src[] = R"({% if has_src != 0 %}
// DO NOT EDIT!
// This was automatically generated with https://github.com/biojppm/c4regen

{% if has_hdr %}
#ifndef {{hdr.include_guard}}
#include "{{hdr.filename}}"
#endif
{% endif %}

{% if has_inl %}
#ifndef {{inl.include_guard}}
#include "{{inl.filename}}"
#endif
{% endif %}

{{src.preamble}}

{% if namespace %}
{{namespace.begin}}
{% endif %}

{{src.gencode}}

{% if namespace %}
{{namespace.end}}
{% endif %}
{% endif %})";


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct WriterBase
{
    typedef enum {HDR, INL, SRC} Destination_e;
    using CodeStore = CodeInstances<std::string>;
    using Contributors = std::set<Generator const*>;
    using set_type     = std::set<std::string>;

    CodeInstances<Contributors> m_contributors;  ///< generators that contributed code
    CodeInstances<std::string>  m_file_names;
    CodeInstances<std::string>  m_file_preambles;
    CodeInstances<std::string>  m_file_contents;
    CodeInstances<CodeTemplate> m_file_tpl;

    CodeTemplate  m_tpl_chunk;
    c4::yml::Tree m_tpl_ws_tree;
    c4::tpl::Rope m_tpl_ws_rope;
    std::string   m_tpl_ws_str;

    std::string   m_source_root;

public:

    virtual ~WriterBase() = default;
    virtual void load(c4::yml::NodeRef const n);

    void set_source_root(csubstr r) { m_source_root.assign(r.begin(), r.end()); }

    void extract_filenames(csubstr src_file_name, CodeInstances<std::string> $ fn);
    virtual void insert_filenames(csubstr src_file_name, set_type $ filenames)
    {
        extract_filenames(src_file_name, &m_file_names);
        if( ! m_file_names.m_hdr.empty()) filenames->insert(m_file_names.m_hdr);
        if( ! m_file_names.m_inl.empty()) filenames->insert(m_file_names.m_inl);
        if( ! m_file_names.m_src.empty()) filenames->insert(m_file_names.m_src);
    }

    void write(SourceFile c$$ src, set_type $ output_names=nullptr);

    virtual void begin_files() {}
    virtual void end_files() {}

protected:

    virtual void _begin_file(SourceFile c$$ src) { C4_UNUSED(src); }
    virtual void _end_file(SourceFile c$$ src) { C4_UNUSED(src); }

    void _request_preambles(CodeChunk c$$ chunk);
    void _append_preamble(csubstr s, Destination_e dst);
    static void _append_to(csubstr s, Destination_e dst, CodeInstances<std::string> $ code);
    void _append_code_chunk(CodeChunk c$$ ch, c4::tpl::Rope c$$ r, Destination_e dst);
    void _render_files();
    csubstr _incguard(csubstr filename);

    template <class T>
    static void _clear(CodeInstances<T> $ s)
    {
        s->m_hdr.clear();
        s->m_inl.clear();
        s->m_src.clear();
    }

    void _clear()
    {
        _clear(&m_file_names);
        _clear(&m_file_preambles);
        _clear(&m_file_contents);
        _clear(&m_contributors);
    }

};


//-----------------------------------------------------------------------------

struct WriterStdout : public WriterBase
{

    void _begin_file(SourceFile c$$ src) override
    {
        C4_UNUSED(src);
        _clear();
        extract_filenames(src.m_name, &m_file_names);
    }
    void _end_file(SourceFile c$$ src) override
    {
        C4_UNUSED(src);
#define _c4prfile(which) if( ! m_file_contents.which.empty()) { printf("%.*s\n", (int)m_file_contents.which.size(), m_file_contents.which.data()); }
        _c4prfile(m_hdr)
        _c4prfile(m_inl)
        _c4prfile(m_src)
#undef _c4prfile
    }

    void insert_filenames(csubstr src_file, set_type $ filenames) override
    {
        C4_UNUSED(src_file);
        C4_UNUSED(filenames);
        // nothing to do here
    }

};



//-----------------------------------------------------------------------------

/** */
struct WriterGenFile : public WriterBase
{
};


//-----------------------------------------------------------------------------

struct WriterGenGroup : public WriterBase
{

    void _begin_file(SourceFile c$$ src) override
    {
        C4_UNUSED(src);
        _clear();
        extract_filenames(src.m_name, &m_file_names);
    }
    void _end_file(SourceFile c$$ src) override
    {
        C4_UNUSED(src);
#define _c4svfile(which) c4::fs::file_put_contents(m_file_names.which.c_str(), m_file_contents.which.data(), m_file_contents.which.size());
        _c4svfile(m_hdr)
        _c4svfile(m_inl)
        _c4svfile(m_src)
#undef _c4svfile
    }

};


//-----------------------------------------------------------------------------

struct WriterSameFile : public WriterBase
{

    void insert_filenames(csubstr src_file, set_type $ filenames) override
    {
        C4_UNUSED(src_file);
        C4_UNUSED(filenames);
    }

};


//-----------------------------------------------------------------------------

struct WriterSingleFile : public WriterBase
{

    void insert_filenames(csubstr src_file, set_type $ filenames) override
    {
        C4_UNUSED(src_file);
        C4_UNUSED(filenames);
    }

};


//-----------------------------------------------------------------------------

struct Writer
{

    typedef enum {
        STDOUT,     ///< @see WriterStdout
        GENFILE,    ///< @see WriterGenFile
        GENGROUP,   ///< @see WriterGenGroup
        SAMEFILE,   ///< @see WriterSameFile
        SINGLEFILE, ///< @see WriterSingleFile
    } Type_e;

    using set_type = WriterBase::set_type;

public:

    Type_e m_type;
    std::unique_ptr<WriterBase> m_impl;

public:

    void write(SourceFile c$$ src)
    {
        m_impl->write(src);
    }

    void insert_filenames(csubstr src_file, set_type *workspace)
    {
        m_impl->insert_filenames(src_file, workspace);
    }

    void begin_files() { m_impl->begin_files(); }
    void end_files() { m_impl->end_files(); }

public:

    void load(c4::yml::NodeRef const n);
    static Type_e str2type(csubstr type_name);

};

} // namespace regen
} // namespace c4

#include <c4/c4_pop.hpp>

#endif /* _c4_REGEN_WRITER_HPP_ */
