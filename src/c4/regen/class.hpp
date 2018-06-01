#ifndef _c4_REGEN_CLASS_HPP_
#define _c4_REGEN_CLASS_HPP_

#include "c4/regen/entity.hpp"
#include "c4/regen/generator.hpp"
#include "c4/regen/function.hpp"

#include <c4/c4_push.hpp>

namespace c4 {
namespace regen {


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


//-----------------------------------------------------------------------------

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


//-----------------------------------------------------------------------------

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


//-----------------------------------------------------------------------------

struct ClassGenerator : public Generator
{
    ClassGenerator() : Generator() { m_entity_type = ENT_CLASS; }
    void create_prop_tree(Entity const& o, c4::yml::NodeRef root) const override
    {
        C4_NOT_IMPLEMENTED();
    }
};


} // namespace regen
} // namespace c4

#endif /* _c4_REGEN_CLASS_HPP_ */
