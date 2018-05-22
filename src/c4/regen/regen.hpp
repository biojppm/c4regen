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
#include "c4/fs/fs.hpp"

namespace c4 {
namespace regen {


//-----------------------------------------------------------------------------

inline bool is_idchar(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')
        || (c == '_' || c == '-' || c == '~' || c == '$');
}

csubstr find_pair(csubstr s, char open, char close, bool allow_nested=false);


//-----------------------------------------------------------------------------

bool is_hdr(csubstr filename);
bool is_src(csubstr filename);

void add_hdr(const char* ext);
void add_src(const char* ext);

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
struct Tag;

using astEntityRef = ast::EntityRef;

struct Entity : public ast::Region
{
    ast::Index const *m_index{nullptr};
    ast::Cursor       m_cursor;
    ast::Cursor       m_parent;
    csubstr           m_str;
    Tag        const *m_tag{nullptr};

    virtual void init(astEntityRef e)
    {
        init_region(*e.idx, e.cursor);
        m_index = e.idx;
        m_cursor = e.cursor;
        m_parent = e.parent;
        m_str = get_str(e.file_contents);
    }

    void set_tag(Tag const* t) { m_tag = t; }

};



/** A tag used to mark and/or annotate entities. Examples:
 *
 *@begincode
 * C4_TAG()
 * // accepts annotations as YAML code:
 * C4_TAG(a, b: c, d: [e, f, g], h: {i: j, k: l})
 *@endcode
 *
 */
struct Tag : public Entity
{
    csubstr m_name;
    csubstr m_spec_str;
    c4::yml::Tree m_annotations;

    void init(astEntityRef e) override final
    {
        this->Entity::init(e);
        parse_annotations();
    }

    void parse_annotations()
    {
        csubstr s = find_pair(m_str, '(', ')', /*allow_nested*/true);
        C4_ASSERT(s.len >= 2 && s.begins_with('(') && s.ends_with(')'));
        m_spec_str = s.range(1, s.len-1).trim(' ');
        m_annotations.clear();
        m_annotations.clear_arena();
        if(m_spec_str.empty()) return;
        substr yml_src = m_annotations.copy_to_arena(m_spec_str);
        c4::yml::parse(yml_src, &m_annotations);
    }
};

struct DataType : public Entity
{
    csubstr m_type_name;
    CXType m_cxtype;
    bool is_integral_signed()   const { return ast::is_integral_signed(m_cxtype.kind); }
    bool is_integral_unsigned() const { return ast::is_integral_unsigned(m_cxtype.kind); }
};

struct Var : public Entity
{
    csubstr  m_name;
    DataType m_type;
};

struct Class;
struct Member : public Var
{
    Class *m_class;
};

struct Function;
struct FunctionParameter : public Entity
{
    Function *m_function;
    DataType  m_data_type;
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
typedef enum {
    EXTR_ALL, ///< extract all possible entities
    EXTR_TAGGED_MACRO, ///< extract only entities tagged with a macro
    EXTR_TAGGED_MACRO_ANNOTATED, ///< extract only entities with an annotation within a tag macro
} ExtractorType_e;

/** A class which examines the AST and extracts useful entities.
 *
 * YAML config examples:
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
    ExtractorType_e m_type{EXTR_ALL};
    std::string m_tag;
    std::string m_attr;

    void load(c4::yml::NodeRef n);
    size_t extract(CXCursorKind kind, c4::ast::TranslationUnit const& tu, std::vector<ast::Entity> *out) const;
    bool must_extract(c4::ast::Index &idx, c4::ast::Cursor c) const;
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct Generator;

struct CodeTemplate
{
    std::shared_ptr<c4::tpl::Engine> engine;
    c4::tpl::Rope parsed_rope;

    bool empty() const { return engine.get() == nullptr; }

    bool load(c4::yml::NodeRef const& n, csubstr name)
    {
        engine.reset();
        csubstr src;
        n.get_if(name, &src);
        if(src.not_empty())
        {
            engine = std::make_shared<c4::tpl::Engine>();
            engine->parse(src, &parsed_rope);
        }
        return ! empty();
    }

    void render(c4::yml::NodeRef properties, c4::tpl::Rope *r) const
    {
        engine->render(properties, r);
    }
};


template<class T>
struct CodeInstances
{
    T m_hdr_preamble;
    T m_inl_preamble;
    T m_src_preamble;
    T m_hdr;
    T m_inl;
    T m_src;
};


struct CodeChunk : public CodeInstances<c4::tpl::Rope>
{
    Generator const* m_generator;
    Entity const* m_originator;
};


struct Generator : public CodeInstances<CodeTemplate>
{
    csubstr   m_name;
    Extractor m_extractor;
    bool      m_empty;

    Generator() :
        CodeInstances<CodeTemplate>(),
        m_name(),
        m_extractor(),
        m_empty(true)
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

    void generate(Entity const& o, c4::yml::NodeRef root, CodeChunk *ch) const
    {
        if(m_empty) return;
        ch->m_generator = this;
        ch->m_originator = &o;
        create_prop_tree(o, root);
        render(root, ch);
    }

    virtual void create_prop_tree(Entity const& o, c4::yml::NodeRef root) const = 0;

    void render(c4::yml::NodeRef const properties, CodeChunk *ch) const
    {
        m_hdr_preamble.render(properties, &ch->m_hdr_preamble);
        m_inl_preamble.render(properties, &ch->m_inl_preamble);
        m_src_preamble.render(properties, &ch->m_src_preamble);
        m_hdr.render(properties, &ch->m_hdr);
        m_inl.render(properties, &ch->m_inl);
        m_src.render(properties, &ch->m_src);
    }

    void load_templates(c4::yml::NodeRef const n)
    {
        m_empty  = false;
        m_empty |= m_hdr_preamble.load(n, "hdr_preamble");
        m_empty |= m_inl_preamble.load(n, "inl_preamble");
        m_empty |= m_src_preamble.load(n, "src_preamble");
        m_empty |= m_hdr         .load(n, "hdr");
        m_empty |= m_inl         .load(n, "inl");
        m_empty |= m_src         .load(n, "src");
    }

};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

/** a free-standing function */
struct Function : public Entity
{
    DataType                       m_return_type;
    std::vector<FunctionParameter> m_parameters;
};

/** generate code from free standing functions */
struct FunctionGenerator : public Generator
{
    void create_prop_tree(Entity const& o, c4::yml::NodeRef root) const override
    {
        C4_NOT_IMPLEMENTED();
    }
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct Enum;
struct EnumSymbol : public Entity
{
    Enum    *m_enum;
    csubstr  m_sym;
    csubstr  m_val;
    char     m_val_buf[32];

    void init_symbol(astEntityRef r, Enum *e);

};

/** an enumeration type */
struct Enum : public DataType
{
    std::vector<EnumSymbol> m_symbols;
    DataType m_underlying_type;

    virtual void init(astEntityRef e) override
    {
        this->DataType::init(e);
        m_underlying_type.m_cxtype = clang_getEnumDeclIntegerType(m_cursor);
    }
};

struct EnumGenerator : public Generator
{
    void create_prop_tree(Entity const& o, c4::yml::NodeRef root) const override
    {
        C4_NOT_IMPLEMENTED();
    }
};


inline void EnumSymbol::init_symbol(astEntityRef r, Enum *e)
{
    this->Entity::init(r);
    m_enum = e;
    if(e->m_underlying_type.is_integral_signed())
    {
        long long val = clang_getEnumConstantDeclValue(m_cursor);
    }
    else if(e->m_underlying_type.is_integral_unsigned())
    {
        unsigned long long val = clang_getEnumConstantDeclUnsignedValue(m_cursor);
    }
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct Method : public Function
{
    Class *m_class;
};

struct Class : public Entity
{
    std::vector<Member> m_members;
    std::vector<Method> m_methods;
};

struct ClassGenerator : public Generator
{
    void create_prop_tree(Entity const& o, c4::yml::NodeRef root) const override
    {
        C4_NOT_IMPLEMENTED();
    }
};




//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

typedef enum {
    ENT_ENUM,
    ENT_CLASS,
    ENT_FUNCTION
} EntityType_e;

struct SourceFile : public Entity
{
public:

    // originator entities extracted from this source file. Given a
    // generator, these entities will originate the code chunks.

    std::vector<Enum>     m_enums;      ///< enum originator entities
    std::vector<Class>    m_classes;    ///< class originator entities
    std::vector<Function> m_functions;  ///< function originator entities

    /// make sure that the generated code chunks are in the
    /// same order as given in the source file
    struct EntityPos
    {
        EntityType_e entity_type;
        size_t pos;
    };
    std::vector<EntityPos> m_pos;    ///< the map to the chunks array
    std::vector<CodeChunk> m_chunks; ///< the code chunks originated from the source code

public:

    SourceFile()
    {
    }

    void extract(ClassGenerator const& g)
    {

    }

    void gencode(ClassGenerator const& g, c4::yml::NodeRef workspace)
    {
        _gencode(m_classes, ENT_CLASS, g, workspace);
    }

    void gencode(EnumGenerator const& g, c4::yml::NodeRef workspace)
    {
        _gencode(m_enums, ENT_ENUM, g, workspace);
    }

    void gencode(FunctionGenerator const& g, c4::yml::NodeRef workspace)
    {
        _gencode(m_functions, ENT_FUNCTION, g, workspace);
    }

    template<class Entity>
    void _gencode(std::vector<Entity> &entities, EntityType_e type, Generator const& g, c4::yml::NodeRef workspace)
    {
        C4_ASSERT(workspace.is_root());
        size_t count = 0;
        for(auto const& p : m_pos)
        {
            if(p.entity_type == type)
            {
                g.generate(entities[p.pos], workspace, &m_chunks[count]);
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

        using value_type = Entity const;

        value_type& operator*  () const { C4_ASSERT(pos >= 0 && pos < s->m_pos.size()); return *s->resolve(s->m_pos[pos]); }
        value_type* operator-> () const { C4_ASSERT(pos >= 0 && pos < s->m_pos.size()); return  s->resolve(s->m_pos[pos]); }
    };

    const_iterator begin() const { return const_iterator(this, 0); }
    const_iterator end  () const { return const_iterator(this, m_pos.size()); }

    Entity const* resolve(EntityPos const& p) const
    {
        switch(p.entity_type)
        {
        case ENT_ENUM:     return &m_enums    [p.pos];
        case ENT_CLASS:    return &m_classes  [p.pos];
        case ENT_FUNCTION: return &m_functions[p.pos];
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

public:

    Type_e m_type;

public:

    Writer()
    {
    }

    void load(c4::yml::NodeRef const& n)
    {
        csubstr s;
        n.get_if("writer", &s, csubstr("stdout"));
        m_type = str2type(s);
    }

    using set_type = std::set<std::string>;

    void write(SourceFile const& src, set_type *C4_RESTRICT output_names=nullptr, bool get_filenames_only=false) const
    {
        C4_NOT_IMPLEMENTED();
        switch(m_type)
        {
        case STDOUT:
            break;
        case SAMEFILE:
            if(output_names) output_names->insert(src.m_file);
            break;
        case GENFILE:
            break;
        case GENGROUP:
            break;
        case SINGLEFILE:
            break;
        default:
            C4_ERROR("unknown writer type");
        }
    }

    void get_output_file_names(SourceFile const& src, set_type *output_names) const
    {
        write(src, output_names, /*get_filenames_only*/true);
    }

};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct Regen
{
    std::string   m_config_file_name;
    std::string   m_config_file_yml;
    c4::yml::Tree m_config_data;

    std::vector<EnumGenerator    > m_gens_enum;
    std::vector<ClassGenerator   > m_gens_class;
    std::vector<FunctionGenerator> m_gens_function;
    std::vector<Generator*       > m_gens_all;

    std::vector<SourceFile> m_files;

    Writer m_writer;

public:

    Regen(const char* config_file)
    {
        _load_config_file(config_file);
    }

    bool empty() { return m_gens_all.empty(); }

    void gencode()
    {
        for(auto & sf : m_files)
        {
            for(auto *g : m_gens_all)
            {
                //sf.prepare_extraction(g);
            }
        }

        c4::yml::Tree worktree;
        for(auto & sf : m_files)
        {
            for(auto const& eg : m_gens_enum)
            {
                sf.gencode(eg, worktree.rootref());
            }
        }
        for(auto & sf : m_files)
        {
            for(auto const& cg : m_gens_class)
            {
                sf.gencode(cg, worktree.rootref());
            }
        }
        for(auto & sf : m_files)
        {
            for(auto const& fg : m_gens_function)
            {
                sf.gencode(fg, worktree.rootref());
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

    void print_output_file_names() const
    {
        Writer::set_type ofn;
        for(auto const& sf : m_files)
        {
            m_writer.get_output_file_names(sf, &ofn);
        }
    }

private:

    void _load_config_file(const char* file_name)
    {
        // read the yml config and parse it
        m_config_file_name = file_name;
        fs::file_get_contents(m_config_file_name.c_str(), &m_config_file_yml);
        c4::yml::parse(to_csubstr(m_config_file_name), to_substr(m_config_file_yml), &m_config_data);
        c4::yml::NodeRef r = m_config_data.rootref();
        c4::yml::NodeRef n;

        m_writer.load(r);

        m_gens_all.clear();
        m_gens_enum.clear();
        m_gens_class.clear();
        m_gens_function.clear();

        n = r.find_child("generators");
        if( ! n.valid()) return;
        for(auto const& ch : n.children())
        {
            csubstr gtype = ch["type"].val();
            if(gtype == "enum")
            {
                _loadgen(ch, &m_gens_enum);
            }
            else if(gtype == "class")
            {
                _loadgen(ch, &m_gens_class);
            }
            else if(gtype == "function")
            {
                _loadgen(ch, &m_gens_function);
            }
            else
            {
                C4_ERROR("unknown generator type");
            }
        }
    }

    template< class GeneratorT >
    void _loadgen(c4::yml::NodeRef const& n, std::vector<GeneratorT> *gens)
    {
        gens->emplace_back();
        GeneratorT &g = gens->back();
        g.load(n);
        m_gens_all.push_back(&g);
    }
};

} // namespace regen
} // namespace c4


#endif /* _C4_REGEN_HPP_ */
