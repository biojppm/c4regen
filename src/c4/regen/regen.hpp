#ifndef _C4_REGEN_HPP_
#define _C4_REGEN_HPP_

#include <cstdio>
#include <vector>
#include <set>

#include <c4/yml/yml.hpp>
#include <c4/yml/std/vector.hpp>
#include <c4/std/string.hpp>

#include <c4/tpl/engine.hpp>

#include "c4/ast/ast.hpp"

namespace c4 {
namespace regen {

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void file_get_contents(const char *filename, std::string *v)
{
    printf("opening file: %s\n", filename);
    std::FILE *fp = std::fopen(filename, "rb");
    C4_CHECK_MSG(fp != nullptr, "could not open file");
    std::fseek(fp, 0, SEEK_END);
    v->resize(std::ftell(fp));
    std::rewind(fp);
    std::fread(&(*v)[0], 1, v->size(), fp);
    std::fclose(fp);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct CodeEntity : public ast::Region
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


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
typedef enum {
    EXTR_ALL, ///< extract all possible entities
    EXTR_TAGGED_MACRO, ///< extract only entities tagged with a macro
    EXTR_TAGGED_MACRO_ANNOTATED, ///< extract only entities with an annotation within a tag macro
} ExtractorType_e;

/** YAML examples:
 * for EXTR_ALL:
 *
 * @begincode
 * extract: all
 * @endcode
 *
 * for EXTR_TAGGED_MACRO:
 *
 * @begincode
 * extract:
 *   macro: C4_ENUM
 * @endcode
 *
 * for EXTR_TAGGED_MACRO_ANNOTATED:
 *
 * @begincode
 * extract:
 *   macro: C4_PROPERTY
 *   attr: gui
 * @endcode
 */
struct Extractor
{
    void load(c4::yml::NodeRef n)
    {
        C4_CHECK(n.valid());
        m_type = EXTR_ALL;
        m_tag.clear();
        m_attr.clear();

        if(!n.valid()) return;
        C4_CHECK(n.key() == "extract");
        if(n.is_keyval())
        {
            csubstr val = n.val();
            if(n.val() == "all")
            {
                m_type = EXTR_ALL;
            }
            else
            {
                C4_ERROR("unknown extractor");
            }
        }
        else
        {
            c4::yml::NodeRef m = n["macro"];
            C4_CHECK(m.valid());
            m_tag.assign(m.val().str, m.val().len);
            m_type = EXTR_TAGGED_MACRO;

            c4::yml::NodeRef a = n.find_child("attr");
            if(a.valid())
            {
                m_type = EXTR_TAGGED_MACRO_ANNOTATED;
                m_attr.assign(a.val().str, a.val().len);
            }
        }
    }

    ExtractorType_e m_type{EXTR_ALL};
    std::string m_tag;
    std::string m_attr;

    bool must_extract(c4::ast::Cursor c) const
    {
        switch(m_type)
        {
        case EXTR_ALL:
            return true;
        case EXTR_TAGGED_MACRO:
        case EXTR_TAGGED_MACRO_ANNOTATED:
            C4_NOT_IMPLEMENTED();
            break;
        default:
            C4_NOT_IMPLEMENTED();
        }
        return false;
    }

};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct Generator;

struct Tpl
{
    std::shared_ptr<c4::tpl::Engine> engine;
    c4::tpl::Rope parsed_rope;

    bool empty() const { return engine.get() == nullptr; }

    bool load(c4::yml::NodeRef const& n, csubstr name)
    {
        engine.reset();
        csubstr src;
        n.get_if(name, &src);
        if( ! src.empty())
        {
            engine = std::make_shared< c4::tpl::Engine >();
            engine->parse(src, &parsed_rope);
        }
        return ! empty();
    }

    void render(c4::yml::NodeRef & properties, c4::tpl::Rope *r) const
    {
        engine->render(properties, r);
    }
};

template< class T >
struct CodeInstances
{
    T m_hdr_preamble;
    T m_inl_preamble;
    T m_src_preamble;
    T m_hdr;
    T m_inl;
    T m_src;
};

struct CodeChunk : public CodeInstances< c4::tpl::Rope >
{
    Generator const* m_generator;
    CodeEntity m_originator;
};

struct Generator
{
    csubstr  m_name;
    Extractor m_extractor;

    bool m_empty;

    CodeInstances<Tpl> m_templates;

    Generator() :
        m_name(),
        m_extractor(),
        m_empty(true),
        m_templates()
    {
    }
    virtual ~Generator() = default;

    Generator(Generator const&) = default;
    Generator& operator= (Generator const&) = default;

    Generator(Generator &&) = default;
    Generator& operator= (Generator &&) = default;

    void load(c4::yml::NodeRef const& n)
    {
        m_name = n["name"].val();
        m_extractor.load(n["extract"]);
        load_templates(n);
    }

    void generate(Originator const& o, c4::yml::NodeRef *root, CodeChunk *ch) const
    {
        if(m_empty) return;
        ch->m_generator = this;
        ch->m_originator = o;
        create_prop_tree(o, root);
        render(*root, ch);
    }

    virtual void create_prop_tree(Originator const& o, c4::yml::NodeRef *root) const = 0;

    void render(c4::yml::NodeRef & properties, CodeChunk *ch) const
    {
        m_templates.m_hdr_preamble.render(properties, &ch->m_hdr_preamble);
        m_templates.m_inl_preamble.render(properties, &ch->m_inl_preamble);
        m_templates.m_src_preamble.render(properties, &ch->m_src_preamble);
        m_templates.m_hdr.render(properties, &ch->m_hdr);
        m_templates.m_inl.render(properties, &ch->m_inl);
        m_templates.m_src.render(properties, &ch->m_src);
    }

    void load_templates(c4::yml::NodeRef const& n)
    {
        m_empty = false;
        m_empty |= m_templates.m_hdr_preamble.load(n, "hdr_preamble");
        m_empty |= m_templates.m_inl_preamble.load(n, "inl_preamble");
        m_empty |= m_templates.m_src_preamble.load(n, "src_preamble");
        m_empty |= m_templates.m_hdr         .load(n, "hdr");
        m_empty |= m_templates.m_inl         .load(n, "inl");
        m_empty |= m_templates.m_src         .load(n, "src");
    }

};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct Enum : public Originator
{
    std::vector< EnumSymbol > m_symbols;
};

struct EnumGenerator : public Generator
{
    void create_prop_tree(Originator const& o, c4::yml::NodeRef *root) const override
    {

    }
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct Class : public Originator
{
    std::vector< Member > m_members;
};

struct ClassGenerator : public Generator
{
    void create_prop_tree(Originator const& o, c4::yml::NodeRef *root) const override
    {

    }
};



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct DataType : public CodeEntity
{
};

struct FunctionParameter : public CodeEntity
{
    DataType m_data_type;
};

struct Function : public Originator
{
    DataType m_return_type;
    std::vector<FunctionParameter> m_parameters;
};

struct FunctionGenerator : public Generator
{
    void create_prop_tree(Originator const& o, c4::yml::NodeRef *root) const override
    {
        C4_NOT_IMPLEMENTED();
    }
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

typedef enum {
    ENT_ENUM,
    ENT_CLASS,
    ENT_FUNCTION
} EntityType_e;

struct SourceFile : public CodeEntity
{
    template<class EntityT, class GeneratorT>
    struct entity_collection
    {
        std::vector<EntityT> m_entities;

        void gencode(SourceFile *sf, GeneratorT const& g)
        {
            c4::yml::Tree props;
            c4::yml::NodeRef root = props.rootref();

            size_t count = 0;
            for(auto const& p : sf->m_pos)
            {
                if(p.entity_type == ENT_CLASS)
                {
                    g.generate(m_entities[p.pos], &root, &sf->m_chunks[count]);
                }
                ++count;
            }
        }

    };

public:

    bool m_is_header; ///< is this a header file?

    // originator entities extracted from this source file. Given a
    // generator, these entities will originate the code chunks.

    entity_collection< Enum, EnumGenerator > m_enums;
    entity_collection< Class, ClassGenerator > m_classes;
    entity_collection< Function, FunctionGenerator > m_functions;

    // make sure that the generated code chunks are in the
    // same order as in the source file
    struct OriginatorPos
    {
        EntityType_e entity_type;
        size_t pos;
    };
    std::vector< OriginatorPos > m_pos; ///< the map to the chunks array
    std::vector< CodeChunk > m_chunks;  ///< the chunks from the originated source code

public:

    SourceFile()
    {
    }

    void setup()
    {
        auto f = to_csubstr(file);
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

    void prepare_extraction(Generator const* beg, Generator const* end)
    {

    }

    void gencode(ClassGenerator const& g)
    {
        m_classes.gencode(this, g);
    }

    void gencode(EnumGenerator const& g)
    {
        m_enums.gencode(this, g);
    }

    void gencode(FunctionGenerator const& g)
    {
        m_functions.gencode(this, g);
    }

public:

    struct const_iterator
    {
        const_iterator(SourceFile const* s, size_t pos) : s(s), pos(pos) {}
        SourceFile const* s;
        size_t pos;

        using value_type = Originator const;

        value_type& operator*  () const { C4_ASSERT(pos >= 0 && pos < s->m_pos.size()); return *s->resolve(s->m_pos[pos]); }
        value_type* operator-> () const { C4_ASSERT(pos >= 0 && pos < s->m_pos.size()); return  s->resolve(s->m_pos[pos]); }
    };

    const_iterator begin() const { return const_iterator(this, 0); }
    const_iterator end  () const { return const_iterator(this, m_pos.size()); }

    Originator const* resolve(OriginatorPos const& p) const
    {
        switch(p.entity_type)
        {
        case ENT_ENUM: return &m_enums.m_entities[p.pos];
        case ENT_CLASS: return &m_classes.m_entities[p.pos];
        case ENT_FUNCTION: return &m_functions.m_entities[p.pos];
        default:
            C4_ERROR("unknown entity type");
            break;
        }
        return nullptr;
    }
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

    Writer()
    {
    }

    void load(c4::yml::NodeRef const& n)
    {
        csubstr s;
        n.get_if("writer", &s, csubstr("stdout"));
        m_type = str2type(s);
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
        case SAMEFILE: output_names->insert(src.file); return;
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
    c4::yml::Tree m_config_data;

    std::vector< EnumGenerator     > m_enum_gens;
    std::vector< ClassGenerator    > m_class_gens;
    std::vector< FunctionGenerator > m_function_gens;
    std::vector< Generator*        > m_all_gens;

    std::vector< SourceFile > m_files;

    Writer m_writer;

public:

    Regen(const char* config_file)
    {
        load_config_file(config_file);
    }

    void load_config_file(const char* file_name)
    {
        // read the yml config and parse it
        m_config_file_name = file_name;
        file_get_contents(m_config_file_name.c_str(), &m_config_file_yml);
        c4::yml::parse(to_csubstr(m_config_file_name), to_substr(m_config_file_yml), &m_config_data);
        c4::yml::NodeRef n, r = m_config_data.rootref();

        m_writer.load(r);

        m_enum_gens.clear();
        m_class_gens.clear();
        m_function_gens.clear();
        m_all_gens.clear();
        n = r.find_child("generators");
        if( ! n.valid()) return;
        for(auto const& ch : n.children())
        {
            csubstr gtype = ch["type"].val();
            if(gtype == "enum")
            {
                _loadgen(ch, &m_enum_gens);
            }
            else if(gtype == "class")
            {
                _loadgen(ch, &m_class_gens);
            }
            else if(gtype == "function")
            {
                _loadgen(ch, &m_function_gens);
            }
            else
            {
                C4_ERROR("unknown generator type");
            }
        }
    }

    void generate_code()
    {
        for(auto & sf : m_files)
        {
            for(auto const& eg : m_enum_gens)
            {
                sf.gencode(eg);
            }
            for(auto const& cg : m_class_gens)
            {
                sf.gencode(cg);
            }
            for(auto const& fg : m_function_gens)
            {
                sf.gencode(fg);
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

    bool empty() { return m_all_gens.empty(); }

    void print_output_file_names() const
    {
        Writer::set_type ofn;
        for(auto const& sf : m_files)
        {
            m_writer.get_output_file_names(sf, &ofn);
        }
    }

    template< class GeneratorT >
    void _loadgen(c4::yml::NodeRef const& n, std::vector< GeneratorT > *gens)
    {
        gens->emplace_back();
        auto &g = gens->back();
        g.load(n);
        m_all_gens.push_back(&g);
    }

};

} // namespace regen
} // namespace c4


#endif /* _C4_REGEN_HPP_ */
