#ifndef _c4_REGEN_WRITER_HPP_
#define _c4_REGEN_WRITER_HPP_

#include <set>
#include "c4/regen/source_file.hpp"

#include <c4/c4_push.hpp>

namespace c4 {
namespace regen {


//-----------------------------------------------------------------------------

/** the template used to render each chunk */
constexpr const char s_default_tpl_chunk[] = R"(
// generator: {{generator.name}}
// entity: {{entity.name}} @ {{entity.file}}:{{entity.line}}
{{gencode}}
)";


//-----------------------------------------------------------------------------

/** the template used to render header files */
constexpr const char s_default_tpl_hdr[] = R"({% if has_hdr %}
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
constexpr const char s_default_tpl_inl[] = R"({% if has_inl %}
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
constexpr const char s_default_tpl_src[] = R"({% if has_src %}
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
    using Contributors = std::set<Generator c$>;
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

public:

    virtual ~WriterBase() = default;
    virtual void load(c4::yml::NodeRef const n);

    virtual void extract_filenames(csubstr src_file, set_type $ filenames) = 0;

    void write(SourceFile c$$ src, set_type $ output_names=nullptr);

    virtual void begin_files() {}
    virtual void end_files() {}

protected:

    virtual void _begin_file(SourceFile c$$ src) {}
    virtual void _end_file(SourceFile c$$ src) {}

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
        _clear();
    }
    void _end_file(SourceFile c$$ src) override
    {
        printf("%.*s\n", (int)m_file_preambles.m_hdr.size(), m_file_preambles.m_hdr.data());
        printf("%.*s\n", (int)m_file_contents .m_hdr.size(), m_file_contents .m_hdr.data());

        printf("%.*s\n", (int)m_file_preambles.m_inl.size(), m_file_preambles.m_inl.data());
        printf("%.*s\n", (int)m_file_contents .m_inl.size(), m_file_contents .m_inl.data());

        printf("%.*s\n", (int)m_file_preambles.m_src.size(), m_file_preambles.m_src.data());
        printf("%.*s\n", (int)m_file_contents .m_src.size(), m_file_contents .m_src.data());
    }

    void extract_filenames(csubstr src_file, set_type $ filenames) override
    {
        // nothing to do here
    }

};



//-----------------------------------------------------------------------------

/** */
struct WriterGenFile : public WriterBase
{

    void extract_filenames(csubstr src_file, set_type $ filenames) override
    {
    }

};


//-----------------------------------------------------------------------------

struct WriterGenGroup : public WriterBase
{

    void extract_filenames(csubstr src_file, set_type $ filenames) override
    {
    }

};


//-----------------------------------------------------------------------------

struct WriterSameFile : public WriterBase
{

    void extract_filenames(csubstr src_file, set_type $ filenames) override
    {
    }

};


//-----------------------------------------------------------------------------

struct WriterSingleFile : public WriterBase
{

    void extract_filenames(csubstr src_file, set_type $ filenames) override
    {

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

    void extract_filenames(csubstr src_file, set_type *workspace)
    {
        m_impl->extract_filenames(src_file, workspace);
    }

    void begin_files() { m_impl->begin_files(); }
    void end_files() { m_impl->end_files(); }

public:

    void load(c4::yml::NodeRef const n)
    {
        csubstr s;
        n.get_if("writer", &s, csubstr("stdout"));
        m_type = str2type(s);
        switch(m_type)
        {
        case STDOUT:
            m_impl.reset(new WriterStdout());
            break;
        case GENFILE:
            m_impl.reset(new WriterGenFile());
            break;
        case GENGROUP:
            m_impl.reset(new WriterGenGroup());
            break;
        case SAMEFILE:
            m_impl.reset(new WriterSameFile());
            break;
        case SINGLEFILE:
            m_impl.reset(new WriterSingleFile());
            break;
        default:
            C4_ERROR("unknown writer type");
        }
        m_impl->load(n);
    }

    static Type_e str2type(csubstr type_name)
    {
        if(type_name == "stdout")
        {
            return STDOUT;
        }
        else if(type_name == "genfile")
        {
            return GENFILE;
        }
        else if(type_name == "gengroup")
        {
            return GENGROUP;
        }
        else if(type_name == "samefile")
        {
            return SAMEFILE;
        }
        else if(type_name == "singlefile")
        {
            return SINGLEFILE;
        }
        else
        {
            C4_ERROR("unknown writer type");
        }
        return STDOUT;
    }

};

} // namespace regen
} // namespace c4

#include <c4/c4_pop.hpp>

#endif /* _c4_REGEN_WRITER_HPP_ */
