#ifndef _c4_REGEN_ENTITY_HPP_
#define _c4_REGEN_ENTITY_HPP_

#include <c4/std/string.hpp>
#include <c4/std/vector.hpp>
#include <c4/yml/node.hpp>

#include "c4/ast/ast.hpp"
#include <c4/c4_push.hpp>

namespace c4 {
namespace regen {

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

//-----------------------------------------------------------------------------

struct Entity;

struct TemplateArg
{
    CXTemplateArgumentKind m_kind;
    CXType                 m_type;
    size_t                 m_val_size;
    char                   m_val_buf[64];

    void init(Entity *e, unsigned i);
};


//-----------------------------------------------------------------------------

/** a source code entity of interest */
struct Entity
{
    ast::TranslationUnit  c$ m_tu{nullptr};
    ast::Index             $ m_index{nullptr};
    ast::Cursor              m_cursor;
    ast::Cursor              m_parent;
    ast::Region              m_region;
    csubstr                  m_str;
    csubstr                  m_name;

    bool                     m_is_tpl;
    std::vector<TemplateArg> m_tpl_args;

public:

    virtual void init(astEntityRef e);
    
    virtual void create_prop_tree(c4::yml::NodeRef root) const;

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
        _parse_annotations();
    }

    void _parse_annotations();
    void _normalize_map_str();
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

    virtual void create_prop_tree(c4::yml::NodeRef root) const override;
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


} // namespace regen
} // namespace c4

#include <c4/c4_pop.hpp>

#endif /* _c4_REGEN_ENTITY_HPP_ */
