#ifndef _C4_REGEN_HPP_
#define _C4_REGEN_HPP_

#include <cstdio>
#include <vector>
#include <set>
#include <algorithm>

#include <c4/yml/yml.hpp>
#include <c4/yml/std/vector.hpp>
#include <c4/std/string.hpp>
#include <c4/fs/fs.hpp>
#include <c4/tpl/engine.hpp>

#include "c4/ast/ast.hpp"

#include <c4/c4_push.hpp>

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

typedef enum {
    _ENT_NONE,
    ENT_VAR,
    ENT_ENUM,
    ENT_FUNCTION,
    ENT_CLASS,
    ENT_MEMBER,
    ENT_METHOD,
} EntityType_e;


/** a source code entity of interest */
struct Entity
{
    ast::TranslationUnit c$ m_tu{nullptr};
    ast::Index            $ m_index{nullptr};
    ast::Cursor             m_cursor;
    ast::Cursor             m_parent;
    ast::Region             m_region;
    csubstr                 m_str;
    csubstr                 m_name;

    virtual void init(astEntityRef e)
    {
        m_tu = e.tu;
        m_index = e.idx;
        m_cursor = e.cursor;
        m_parent = e.parent;
        m_region.init_region(*e.idx, e.cursor);
        m_str = m_region.get_str(to_csubstr(e.tu->m_contents));
        m_name = _get_display_name();
    }

protected:

    csubstr _get_display_name() const { return to_csubstr(m_cursor.display_name(*m_index)); }
    csubstr _get_spelling() const { return to_csubstr(m_cursor.spelling(*m_index)); }

};



/** A tag used to mark and/or annotate entities. For example:
 *@begincode
 *
 * MY_TAG()
 * struct Foo {};
 *
 * // a tag accepts annotations as (quasi-)YAML code:
 * MY_TAG(x, y, z, a, b: c, d: [e, f, g], h: {i: j, k: l})
 * struct Bar {};
 *@endcode
 *
 */
struct Tag : public Entity
{
    csubstr m_spec_str;
    c4::yml::Tree m_annotations;

    bool empty() const { return m_name.empty(); }

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

/** an entity which is possibly tagged */
struct TaggedEntity : public Entity
{
    EntityType_e m_entity_type;
    Tag m_tag;

    bool is_tagged() const { return ! m_tag.empty(); }
    void set_tag(ast::Cursor tag, ast::Cursor tag_parent)
    {
        ast::Entity e{tag, tag_parent, m_tu, m_index};
        m_tag.init(e);
    }
};


struct DataType : public Entity
{
    csubstr m_type_name;
    CXType m_cxtype;

    virtual void init(astEntityRef e) override
    {
        this->Entity::init(e);
    }

    bool is_integral_signed()   const { return ast::is_integral_signed(m_cxtype.kind); }
    bool is_integral_unsigned() const { return ast::is_integral_unsigned(m_cxtype.kind); }
};



struct Var : public TaggedEntity
{
    DataType m_type;

    virtual void init(astEntityRef e) override
    {
        this->TaggedEntity::init(e);
        m_entity_type = ENT_VAR;
    }
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
typedef enum {
    EXTR_ALL, ///< extract all possible entities
    EXTR_TAGGED_MACRO, ///< extract only entities tagged with a macro
    EXTR_TAGGED_MACRO_ANNOTATED, ///< extract only entities with an annotation within a tag macro
} ExtractorType_e;

/** Examines the AST and extracts useful entities.
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

    struct Data
    {
        c4::ast::Cursor cursor, tag;
        bool extracted, has_tag;
        inline operator bool() const { return extracted; }
    };
    Extractor::Data extract(c4::ast::Index &idx, c4::ast::Cursor c) const;
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------


template<class T>
struct CodeInstances
{
    T m_hdr;
    T m_inl;
    T m_src;
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct CodeTemplate
{
    std::shared_ptr<c4::tpl::Engine> engine;
    c4::tpl::Rope parsed_rope;

    bool empty() const { return engine.get() == nullptr; }

    bool load(c4::yml::NodeRef const n, csubstr name, csubstr fallback_tpl={})
    {
        engine.reset();
        csubstr src = fallback_tpl;
        if(n.valid())
        {
            n.get_if(name, &src);
        }
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


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct Generator;


struct CodeChunk : public CodeInstances<c4::tpl::Rope>
{
    Generator c$ m_generator;
    Entity    c$ m_originator;
};


//-----------------------------------------------------------------------------

struct CodePreamble
{
    std::string preamble;

    bool empty() const { return preamble.empty(); }

    bool load(c4::yml::NodeRef const& n, csubstr name)
    {
        csubstr s;
        n.get_if(name, &s);
        if(s.not_empty())
        {
            preamble.assign(s.begin(), s.end());
        }
        return ! empty();
    }
};

struct Generator : public CodeInstances<CodeTemplate>
{
    using Preambles = CodeInstances<CodePreamble>;

    Extractor    m_extractor;
    Preambles    m_preambles;
    EntityType_e m_entity_type;
    csubstr      m_name;
    bool         m_empty;

    Generator() :
        CodeInstances<CodeTemplate>(),
        m_extractor(),
        m_preambles(),
        m_entity_type(),
        m_name(),
        m_empty(true)
    {
    }
    virtual ~Generator() = default;

    Generator(Generator const&) = default;
    Generator& operator= (Generator const&) = default;

    Generator(Generator &&) = default;
    Generator& operator= (Generator &&) = default;

    void load(c4::yml::NodeRef n)
    {
        m_name = n["name"].val();
        m_extractor.load(n["extract"]);
        load_templates(n);
    }

    void generate(Entity c$$ o, c4::yml::NodeRef root, CodeChunk *ch) const
    {
        if(m_empty) return;
        ch->m_generator = this;
        ch->m_originator = &o;
        create_prop_tree(o, root);
        render(root, ch);
    }

    virtual void create_prop_tree(Entity c$$ o, c4::yml::NodeRef root) const = 0;

    void render(c4::yml::NodeRef const properties, CodeChunk *ch) const
    {
        m_hdr.render(properties, &ch->m_hdr);
        m_inl.render(properties, &ch->m_inl);
        m_src.render(properties, &ch->m_src);
    }

    void load_templates(c4::yml::NodeRef const n)
    {
        m_empty  = false;
        m_empty |= m_preambles.m_hdr.load(n, "hdr_preamble");
        m_empty |= m_preambles.m_inl.load(n, "inl_preamble");
        m_empty |= m_preambles.m_src.load(n, "src_preamble");
        m_empty |= m_hdr         .load(n, "hdr");
        m_empty |= m_inl         .load(n, "inl");
        m_empty |= m_src         .load(n, "src");
    }

};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct Function;
struct FunctionParameter : public Entity
{
    Function c$ m_function{nullptr};
    DataType    m_data_type;

    void init_param(astEntityRef e, Function c$ f)
    {
        m_function = f;
        this->Entity::init(e);
    }
};

/** a free-standing function */
struct Function : public TaggedEntity
{
    DataType m_return_type;
    std::vector<FunctionParameter> m_parameters;

    virtual void init(astEntityRef e) override
    {
        m_entity_type = ENT_FUNCTION;
        this->TaggedEntity::init(e);
        int num_args = clang_Cursor_getNumArguments(m_cursor);
        m_parameters.resize(num_args);
        unsigned i = 0;
        for(auto &a : m_parameters)
        {
            ast::Cursor pc = clang_Cursor_getArgument(m_cursor, i++);
            ast::Entity pe{pc, m_cursor, m_tu, m_index};
            a.init_param(pe, this);
        }
    }
};

/** generate code from free standing functions */
struct FunctionGenerator : public Generator
{
    FunctionGenerator() : Generator() { m_entity_type = ENT_FUNCTION; }
    void create_prop_tree(Entity const& o, c4::yml::NodeRef root) const override
    {
        C4_NOT_IMPLEMENTED();
    }
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct Enum;
struct EnumSymbol : public TaggedEntity
{
    Enum    *m_enum;
    csubstr  m_sym;
    csubstr  m_val;
    char     m_val_buf[32];

    void init_symbol(astEntityRef r, Enum *e);

};

/** an enumeration type */
struct Enum : public TaggedEntity
{
    std::vector<EnumSymbol> m_symbols;
    DataType m_underlying_type;

    virtual void init(astEntityRef e) override
    {
        m_entity_type = ENT_ENUM;
        this->TaggedEntity::init(e);
        m_underlying_type.m_cxtype = clang_getEnumDeclIntegerType(m_cursor);
    }
};

struct EnumGenerator : public Generator
{
    EnumGenerator() : Generator() { m_entity_type = ENT_ENUM; }
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

struct Class;
struct Member : public TaggedEntity
{
    Class c$ m_class;

    virtual void init(astEntityRef e) override
    {
        this->TaggedEntity::init(e);
        m_entity_type = ENT_MEMBER;
        C4_NOT_IMPLEMENTED();
    }
};

struct Method : public Function
{
    Class c$ m_class;

    virtual void init(astEntityRef e) override
    {
        this->Function::init(e);
        m_entity_type = ENT_METHOD;
        C4_NOT_IMPLEMENTED();
    }
};

struct Class : public TaggedEntity
{
    std::vector<Member> m_members;
    std::vector<Method> m_methods;

    virtual void init(astEntityRef e) override
    {
        this->TaggedEntity::init(e);
        m_entity_type = ENT_CLASS;
        C4_NOT_IMPLEMENTED();
    }
};

struct ClassGenerator : public Generator
{
    ClassGenerator() : Generator() { m_entity_type = ENT_CLASS; }
    void create_prop_tree(Entity const& o, c4::yml::NodeRef root) const override
    {
        C4_NOT_IMPLEMENTED();
    }
};




//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct SourceFile : public Entity
{
public:

    // originator entities extracted from this source file. Given a
    // generator, these entities will originate the code chunks.

    std::vector<Enum>     m_enums;      ///< enum originator entities
    std::vector<Class>    m_classes;    ///< class originator entities
    std::vector<Function> m_functions;  ///< function originator entities

    /// this is used to make sure that the generated code chunks are in the
    /// same order as given in the source file
    struct EntityPos
    {
        Generator c$ generator;
        EntityType_e entity_type;
        size_t pos;
    };
    std::vector<EntityPos> m_pos;    ///< the map to the chunks array
    std::vector<CodeChunk> m_chunks; ///< the code chunks originated from the source code

public:

    void init_source_file(ast::Index $$ idx, ast::TranslationUnit c$$ tu)
    {
        ast::Entity e{tu.root(), {}, &tu, &idx};
        this->Entity::init(e);
    }

    void clear()
    {
        m_enums.clear();
        m_classes.clear();
        m_functions.clear();
        m_pos.clear();
        m_chunks.clear();
    }

    size_t extract(Generator c$ c$ gens, size_t num_gens)
    {
        size_t num_chunks = m_pos.size();

        // extract all entities
        for(size_t i = 0; i < num_gens; ++i)
        {
            auto c$ g_ = gens[i];
            switch(g_->m_entity_type)
            {
            case ENT_CLASS:    _extract(&m_classes  , ENT_CLASS   , *g_); break;
            case ENT_ENUM:     _extract(&m_enums    , ENT_ENUM    , *g_); break;
            case ENT_FUNCTION: _extract(&m_functions, ENT_FUNCTION, *g_); break;
            default:
                C4_NOT_IMPLEMENTED();
            }
        }

        // reorder the chunks so that they are in the same order as the originating entities
        std::sort(m_pos.begin(), m_pos.end(), [this](EntityPos c$$ l_, EntityPos c$$ r_){
            return this->resolve(l_)->m_region < this->resolve(r_)->m_region;
        });

        num_chunks = m_pos.size() - num_chunks;

        return num_chunks;
    }

    void gencode(Generator c$ c$ gens, size_t num_gens, c4::yml::NodeRef workspace)
    {
        for(size_t i = 0; i < num_gens; ++i)
        {
            auto c$ g_ = gens[i];
            switch(g_->m_entity_type)
            {
            case ENT_CLASS:    _gencode(&m_classes  , ENT_CLASS   , *g_, workspace); break;
            case ENT_ENUM:     _gencode(&m_enums    , ENT_ENUM    , *g_, workspace); break;
            case ENT_FUNCTION: _gencode(&m_functions, ENT_FUNCTION, *g_, workspace); break;
            default:
                C4_NOT_IMPLEMENTED();
            }
        }
    }

private:

    template<class EntityT>
    void _extract(std::vector<EntityT> $ entities, EntityType_e type, Generator c$$ g)
    {
        struct _visitor_data
        {
            SourceFile $ sf;
            std::vector<EntityT> $ entities;
            EntityType_e type;
            Generator c$ gen;
        } vd{this, entities, type, &g};

        auto visitor = [](ast::Cursor c, ast::Cursor parent, void *data)
        {
            auto vd = (_visitor_data $)data;
            Extractor::Data ret;
            if((ret = vd->gen->m_extractor.extract(*vd->sf->m_index, c)) == true)
            {
                EntityPos pos{vd->gen, vd->type, vd->entities->size()};
                vd->sf->m_pos.emplace_back(pos);
                vd->sf->m_chunks.emplace_back();
                vd->entities->emplace_back();
                EntityT $$ e = vd->entities->back();
                ast::Entity ae;
                ae.tu = vd->sf->m_tu;
                ae.idx = vd->sf->m_index;
                ae.cursor = ret.cursor;
                ae.parent = parent;
                e.init(ae);
                if(ret.has_tag)
                {
                    e.set_tag(ret.tag, parent);
                }
            }
            return CXChildVisit_Recurse;
        };

        m_tu->visit_children(visitor, &vd);
    }

    template<class EntityT>
    void _gencode(std::vector<EntityT> $ entities, EntityType_e type, Generator c$$ g, c4::yml::NodeRef workspace)
    {
        C4_ASSERT(workspace.is_root());
        for(size_t i = 0, e = m_pos.size(); i < e; ++i)
        {
            auto c$$ p = m_pos[i];
            if(p.generator == &g && p.entity_type == type)
            {
                g.generate((*entities)[p.pos], workspace, &m_chunks[i]);
            }
        }
    }

public:

    struct const_iterator
    {
        const_iterator(SourceFile c$ s, size_t pos) : s(s), pos(pos) {}
        SourceFile c$ s;
        size_t pos;

        using value_type = Entity const;

        value_type $$ operator*  () const { C4_ASSERT(pos >= 0 && pos < s->m_pos.size()); return *s->resolve(s->m_pos[pos]); }
        value_type  $ operator-> () const { C4_ASSERT(pos >= 0 && pos < s->m_pos.size()); return  s->resolve(s->m_pos[pos]); }
    };

    const_iterator begin() const { return const_iterator(this, 0); }
    const_iterator end  () const { return const_iterator(this, m_pos.size()); }

public:

    Entity c$ resolve(EntityPos c$$ p) const
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

constexpr const char s_default_tpl_chunk[] = R"(
// generator: {{generator.name}}
// entity: {{entity.name}} @ {{entity.file}}:{{entity.line}}
{{gencode}}
)";

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
    virtual void load(c4::yml::NodeRef const n)
    {
        auto ntpl = n.find_child("tpl");
        m_tpl_chunk.load(ntpl, "chunk", s_default_tpl_chunk);
        m_file_tpl.m_hdr.load(ntpl, "hdr", s_default_tpl_hdr);
        m_file_tpl.m_inl.load(ntpl, "inl", s_default_tpl_inl);
        m_file_tpl.m_src.load(ntpl, "src", s_default_tpl_src);
    }

    virtual void extract_filenames(csubstr src_file, set_type $ filenames) = 0;

    void write(SourceFile c$$ src, set_type $ output_names=nullptr)
    {
C4_ERROR("asdfjkhasdkjhasdkjh");
        _begin_file(src);

        for(auto c$$ chunk : src.m_chunks)
        {
            _request_preambles(chunk);
        }

        // header code
        for(auto c$ gen : m_contributors.m_hdr)
        {
            _append_preamble(to_csubstr(gen->m_preambles.m_hdr.preamble), HDR);
        }
        for(auto c$$ chunk : src.m_chunks)
        {
            _append_code_chunk(chunk, chunk.m_hdr, HDR);
        }

        // inline code
        for(auto c$ gen : m_contributors.m_inl)
        {
            _append_preamble(to_csubstr(gen->m_preambles.m_inl.preamble), INL);
        }
        for(auto c$$ chunk : src.m_chunks)
        {
            _append_code_chunk(chunk, chunk.m_inl, INL);
        }

        // source code
        for(auto c$ gen : m_contributors.m_src)
        {
            _append_preamble(to_csubstr(gen->m_preambles.m_src.preamble), SRC);
        }
        for(auto c$$ chunk : src.m_chunks)
        {
            _append_code_chunk(chunk, chunk.m_src, SRC);
        }

        _render_files();

        _end_file(src);
    }

    virtual void begin_files() {}
    virtual void end_files() {}

protected:

    virtual void _begin_file(SourceFile c$$ src) {}
    virtual void _end_file(SourceFile c$$ src) {}

    void _request_preambles(CodeChunk c$$ chunk)
    {
        if( ! chunk.m_hdr.empty())
        {
            m_contributors.m_hdr.insert(chunk.m_generator);
        }
        if( ! chunk.m_inl.empty())
        {
            m_contributors.m_inl.insert(chunk.m_generator);
        }
        if( ! chunk.m_src.empty())
        {
            m_contributors.m_src.insert(chunk.m_generator);
        }
    }

    void _append_preamble(csubstr s, Destination_e dst)
    {
        _append_to(s, dst, &m_file_preambles);
    }

    static void _append_to(csubstr s, Destination_e dst, CodeInstances<std::string> $ code)
    {
        switch(dst)
        {
        case HDR: code->m_hdr.append(s.str, s.len); break;
        case INL: code->m_inl.append(s.str, s.len); break;
        case SRC: code->m_src.append(s.str, s.len); break;
        default: C4_ERROR("unknown destination");
        }
    }

    void _append_code_chunk(CodeChunk c$$ ch, c4::tpl::Rope c$$ r, Destination_e dst)
    {
        // linearize the chunk's rope into a temporary buffer
        r.chain_all_resize(&m_tpl_ws_str);

        // now render the chunk template using that buffer
        m_tpl_ws_tree.clear();
        m_tpl_ws_tree.clear_arena();
        c4::yml::NodeRef root = m_tpl_ws_tree.rootref();
        root |= c4::yml::MAP;
        c4::yml::NodeRef gen = root["generator"];
        gen |= c4::yml::MAP;
        gen["name"] = ch.m_generator->m_name;
        c4::yml::NodeRef ent = root["entity"];
        ent |= c4::yml::MAP;
        ent["name"] = ch.m_originator->m_name;
        ent["file"] = ch.m_originator->m_region.m_file;
        ent["line"] << ch.m_originator->m_region.m_start.line;
        root["gencode"] = to_csubstr(m_tpl_ws_str);
        m_tpl_chunk.render(root, &m_tpl_ws_rope);

        // now append the templated chunk to the file contents
        for(csubstr entry : m_tpl_ws_rope.entries())
        {
            _append_to(entry, dst, &m_file_contents);
        }
    }

    void _render_files()
    {
        m_tpl_ws_tree.clear();
        m_tpl_ws_tree.clear_arena();
        c4::yml::NodeRef root = m_tpl_ws_tree.rootref();
        root |= c4::yml::MAP;

        root["has_hdr"] << ( ! m_file_contents.m_hdr.empty());
        root["has_inl"] << ( ! m_file_contents.m_inl.empty());
        root["has_src"] << ( ! m_file_contents.m_src.empty());
        c4::yml::NodeRef hdr = root["hdr"];
        c4::yml::NodeRef inl = root["inl"];
        c4::yml::NodeRef src = root["src"];
        hdr |= c4::yml::MAP;
        inl |= c4::yml::MAP;
        src |= c4::yml::MAP;
        hdr["preamble"] = to_csubstr(m_file_preambles.m_hdr);
        inl["preamble"] = to_csubstr(m_file_preambles.m_inl);
        src["preamble"] = to_csubstr(m_file_preambles.m_src);
        hdr["gencode"] = to_csubstr(m_file_contents.m_hdr);
        inl["gencode"] = to_csubstr(m_file_contents.m_inl);
        src["gencode"] = to_csubstr(m_file_contents.m_src);
        hdr["include_guard"] = _incguard(to_csubstr(m_file_names.m_hdr));
        inl["include_guard"] = _incguard(to_csubstr(m_file_names.m_inl));
        //src["include_guard"] = _incguard(to_csubstr(m_file_names.m_src));
        hdr["filename"] = to_csubstr(m_file_names.m_hdr);
        inl["filename"] = to_csubstr(m_file_names.m_inl);
        src["filename"] = to_csubstr(m_file_names.m_src);

        // note that the properties are using the file contents
        // strings, so we can't chain the rope directly into those
        // strings. That's the reason for chaining into the temp
        // string and then copying it to the file contents strings.

        m_file_tpl.m_hdr.render(root, &m_tpl_ws_rope);
        m_tpl_ws_rope.chain_all_resize(&m_tpl_ws_str);
        m_file_contents.m_hdr = m_tpl_ws_str;

        m_file_tpl.m_inl.render(root, &m_tpl_ws_rope);
        m_tpl_ws_rope.chain_all_resize(&m_tpl_ws_str);
        m_file_contents.m_inl = m_tpl_ws_str;

        m_file_tpl.m_src.render(root, &m_tpl_ws_rope);
        m_tpl_ws_rope.chain_all_resize(&m_tpl_ws_str);
        m_file_contents.m_src = m_tpl_ws_str;
    }

    csubstr _incguard(csubstr filename)
    {
        substr incg = m_tpl_ws_tree.alloc_arena(filename.len + 7);
        cat(incg, filename, "_GUARD_");
        for(auto $$ c : incg)
        {
            if(c == '.' || c == '/' || c == '\\') c = '_';
            else c = std::toupper(c);
        }
        return incg;
    }

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

    Writer m_writer;

public:

    Regen(const char* config_file)
    {
        _load_config_file(config_file);
    }

    bool empty() const { return m_gens_all.empty(); }

public:

    template <class SourceFileNameCollection>
    void gencode(SourceFileNameCollection c$$ collection, const char* db_dir=nullptr)
    {
        ast::CompilationDb db(db_dir);
        yml::Tree workspace;
        yml::NodeRef wsroot = workspace.rootref();
        SourceFile sf;

        m_writer.begin_files();
        for(const char* source_file : collection)
        {
            ast::Index idx;
            ast::TranslationUnit unit(idx, source_file, db);
            sf.clear();
            sf.init_source_file(idx, unit);

            sf.extract(m_gens_all.data(), m_gens_all.size());
            sf.gencode(m_gens_all.data(), m_gens_all.size(), workspace);
            m_writer.write(sf);
        }
        m_writer.end_files();
    }


    template <class SourceFileNameCollection>
    void print_output_filenames(SourceFileNameCollection c$$ collection)
    {
        Writer::set_type workspace;
        for(const char* source_file : collection)
        {
            m_writer.extract_filenames(to_csubstr(source_file), &workspace);
        }
        for(auto c$$ name : workspace)
        {
            printf("%s\n", name.c_str());
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

#include <c4/c4_pop.hpp>

#endif /* _C4_REGEN_HPP_ */
