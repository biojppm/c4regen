#ifndef _C4_REGEN_HPP_
#define _C4_REGEN_HPP_

#include <cstdio>
#include <vector>
#include <set>

#include <c4/yml/yml.hpp>
#include <c4/yml/std/vector.hpp>
#include <c4/std/string.hpp>

#include <c4/tpl/engine.hpp>

namespace c4 {
namespace regen {

using DataNode = c4::yml::NodeRef;
using DataTree = c4::yml::Tree;

using Engine   = c4::tpl::Engine;
using Rope     = c4::tpl::Rope;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void file_get_contents(const char *filename, std::string *v)
{
    std::FILE *fp = std::fopen(filename, "rb");
    C4_ERROR_IF(!fp, "could not open file");
    std::fseek(fp, 0, SEEK_END);
    v->resize(std::ftell(fp));
    std::rewind(fp);
    std::fread(&(*v)[0], 1, v->size(), fp);
    std::fclose(fp);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct Region
{
    struct Pos { size_t offset, line, col; };

    const char * m_file;
    Pos          m_start;
    Pos          m_end;
};

struct CodeEntity : public Region
{
};

struct Annotation : public CodeEntity
{
    csubstr m_key;
    csubstr m_val;
};

struct Tag : public CodeEntity
{
    std::vector< Annotation > m_annotations;
};

struct Var : public CodeEntity
{
    csubstr m_name;
    csubstr m_type_name;
};

struct Class;
struct Member : public Var
{
    Class *m_class;

};

struct Enum;
struct EnumSymbol : public CodeEntity
{
    Enum *m_enum;

    csubstr m_sym;
    csubstr m_val;
};

/** an entity which will originate code; ie, cause code to be generated */
struct Originator : public CodeEntity
{

};

struct Class : public Originator
{
    std::vector< Member > m_members;
};

struct Enum : public Originator
{
    std::vector< EnumSymbol > m_symbols;
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct Generator;

struct CodeChunk
{
    Generator const* m_generator;
    CodeEntity m_originator;
    Rope m_hdr_preamble;
    Rope m_inl_preamble;
    Rope m_src_preamble;
    Rope m_hdr;
    Rope m_inl;
    Rope m_src;
};

struct Generator
{
    csubstr  m_name;
    csubstr  m_tag;

    std::shared_ptr<Engine> m_hdr_preamble;
    std::shared_ptr<Engine> m_inl_preamble;
    std::shared_ptr<Engine> m_src_preamble;
    std::shared_ptr<Engine> m_hdr;
    std::shared_ptr<Engine> m_inl;
    std::shared_ptr<Engine> m_src;

    bool m_empty;

    Rope m_parsed_rope;

    Generator() :
        m_name(),
        m_tag(),
        m_hdr_preamble(),
        m_inl_preamble(),
        m_src_preamble(),
        m_hdr(),
        m_inl(),
        m_src(),
        m_empty(true)
    {
    }
    virtual ~Generator() = default;

    Generator(Generator const&) = default;
    Generator& operator= (Generator const&) = default;

    Generator(Generator &&) = default;
    Generator& operator= (Generator &&) = default;

    void load(DataNode const& n)
    {
        m_name = n.key();
        m_tag = n["tag"].val();
        m_hdr_preamble = load_tpl(n, "hdr_preamble");
        m_inl_preamble = load_tpl(n, "inl_preamble");
        m_src_preamble = load_tpl(n, "src_preamble");
        m_hdr = load_tpl(n, "hdr");
        m_inl = load_tpl(n, "inl");
        m_src = load_tpl(n, "src");
    }

    std::shared_ptr<Engine> load_tpl(DataNode const& n, csubstr const& name)
    {
        auto eng = std::make_shared< Engine >();
        csubstr src;
        n.get_if(name, &src);
        if(!src.empty())
        {
            eng->parse(src, &m_parsed_rope);
            m_empty = false;
        }
        return eng;
    }

    void generate(Originator const& o, DataNode *root, CodeChunk *ch) const
    {
        if(m_empty) return;
        ch->m_generator = this;
        ch->m_originator = o;
        create_prop_tree(o, root);
        render(*root, ch);
    }

    virtual void create_prop_tree(Originator const& o, DataNode *root) const = 0;

    void render(DataNode & properties, CodeChunk *ch) const
    {
        m_hdr_preamble->render(properties, &ch->m_hdr_preamble);
        m_inl_preamble->render(properties, &ch->m_inl_preamble);
        m_src_preamble->render(properties, &ch->m_src_preamble);
        m_hdr->render(properties, &ch->m_hdr);
        m_inl->render(properties, &ch->m_inl);
        m_src->render(properties, &ch->m_src);
    }

};

struct EnumGenerator : public Generator
{
    void create_prop_tree(Originator const& o, DataNode *root) const override final
    {

    }
};

struct ClassGenerator : public Generator
{
    void create_prop_tree(Originator const& o, DataNode *root) const override final
    {

    }
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
struct SourceFile : public CodeEntity
{
    struct OriginatorPos
    {
        bool is_class;
        size_t pos;
    };

    std::vector< OriginatorPos > m_pos;
    std::vector< Enum > m_enums;
    std::vector< Class > m_classes;

    std::vector< CodeChunk > m_chunks;
    bool m_is_header;

    SourceFile()
    {
    }

    void setup()
    {
        auto f = to_csubstr(m_file);
        m_is_header = false;
        std::initializer_list< csubstr > hdr_exts = {".h", ".hpp", ".hxx", ".h++", ".hh"};
        for(auto &ext : hdr_exts)
        {
            if(f.ends_with(ext))
            {
                m_is_header = true;
                break;
            }
        }
        if( ! m_is_header)
        {
            bool gotit = false;
            std::initializer_list< csubstr > src_exts = {".c", ".cpp", ".cxx", ".c++", ".cc"};
            for(auto &ext : src_exts)
            {
                if(f.ends_with(ext))
                {
                    gotit = true;
                    break;
                }
            }
            C4_ASSERT(gotit);
        }
    }

    void generate_class_code(ClassGenerator const& g)
    {
        DataTree props;
        DataNode root = props.rootref();

        m_chunks.resize(m_pos.size());
        size_t count = 0;
        for(auto const& p : m_pos)
        {
            if(p.is_class)
            {
                g.generate(m_classes[p.pos], &root, &m_chunks[count]);
            }
            ++count;
        }
    }

    void generate_enum_code(EnumGenerator const& g)
    {
        DataTree props;
        DataNode root = props.rootref();

        m_chunks.resize(m_pos.size());
        size_t count = 0;
        for(auto const& p : m_pos)
        {
            if( ! p.is_class)
            {
                g.generate(m_enums[p.pos], &root, &m_chunks[count]);
            }
            ++count;
        }
    }

public:

    struct const_iterator
    {
        const_iterator(SourceFile const* s, size_t pos) : s(s), pos(pos) {}
        SourceFile const* s;
        size_t pos;

        using value_type = Originator const;

        value_type& operator*  () const { C4_ASSERT(pos >= 0 && pos < s->m_pos.size()); auto &p = s->m_pos[pos]; return p.is_class ?  (Originator const&)s->m_classes[p.pos] :  (Originator const&)s->m_enums[p.pos]; }
        value_type* operator-> () const { C4_ASSERT(pos >= 0 && pos < s->m_pos.size()); auto &p = s->m_pos[pos]; return p.is_class ? &(Originator const&)s->m_classes[p.pos] : &(Originator const&)s->m_enums[p.pos]; }
    };

    const_iterator begin() const { return const_iterator(this, 0); }
    const_iterator end  () const { return const_iterator(this, m_pos.size()); }
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------


struct Writer
{

    typedef enum {
        STDOUT,
        GENFILE,
        GENGROUP,
        SAMEFILE,
        SINGLEFILE,
    } Type_e;

    static Type_e str2type(csubstr type_name)
    {
        if(type_name == "stdout")
        {
            return STDOUT;
        }
        else if(type_name == "samefile")
        {
            return SAMEFILE;
        }
        else if(type_name == "genfile")
        {
            return GENFILE;
        }
        else if(type_name == "gengroup")
        {
            return GENGROUP;
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

    Type_e m_type;

    Writer(csubstr type_name) : m_type(str2type(type_name))
    {
    }

    void write(SourceFile const& src) const
    {
        std::set< std::string > output_names;
    }

    using set_type = std::set< std::string >;

    void get_output_file_names(SourceFile const& src, set_type *output_names) const
    {
        switch(m_type)
        {
        case STDOUT: break;
        case SAMEFILE: output_names->insert(src.m_file); return;
        case GENFILE: output_names->clear(); return;
        case GENGROUP: output_names->clear(); return;
        case SINGLEFILE: output_names->clear(); return;
        }
        C4_ERROR("unknown writer type");
    }

    set_type get_output_file_names(SourceFile const& src) const
    {
        set_type ofn;
        get_output_file_names(src, &ofn);
        return ofn;
    }

};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct Regen
{
    std::string m_config_file_name;
    std::string m_config_file_yml;
    DataTree m_config_data;

    std::vector<  EnumGenerator > m_enum_gens;
    std::vector< ClassGenerator > m_class_gens;

    std::vector< SourceFile > m_files;

    csubstr m_writer_type;
    Writer m_writer;

    void load_config_file(std::string const& file_name)
    {
        m_config_file_name = file_name;
        file_get_contents(m_config_file_name.c_str(), &m_config_file_yml);
        parse(to_csubstr(m_config_file_name), to_substr(m_config_file_yml), &m_config_data);

        DataNode n, r = m_config_data.rootref();

        r.get_if("writer", &m_writer_type, csubstr("stdout"));
        m_writer = Writer(m_writer_type);

        n = r.find_child("enum");
        if(n.valid())
        {
            for(auto const& ch : n.children())
            {
                m_enum_gens.emplace_back();
                auto &g = m_enum_gens.back();
                g.load(ch);
            }
        }

        n = r.find_child("class");
        if(n.valid())
        {
            for(auto const& ch : n.children())
            {
                m_class_gens.emplace_back();
                auto &g = m_class_gens.back();
                g.load(ch);
            }
        }
    }

    void generate_code()
    {
        for(auto & sf : m_files)
        {
            for(auto const& eg : m_enum_gens)
            {
                sf.generate_enum_code(eg);
            }
            for(auto const& cg : m_class_gens)
            {
                sf.generate_class_code(cg);
            }
        }
    }

    void output_code() const
    {
        for(auto const& sf : m_files)
        {
            m_writer.write(sf);
        }
    }

    bool empty()
    {
        return m_enum_gens.size() == 0 && m_class_gens.size() == 0;
    }

    void print_output_file_names() const
    {
        Writer::set_type ofn;
        for(auto const& sf : m_files)
        {
            m_writer.get_output_file_names(sf, &ofn);
        }
    }
};

} // namespace regen
} // namespace c4


#endif /* _C4_REGEN_HPP_ */
