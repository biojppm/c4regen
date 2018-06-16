#ifndef _c4_REGEN_CLASS_HPP_
#define _c4_REGEN_CLASS_HPP_

#include "c4/regen/entity.hpp"
#include "c4/regen/generator.hpp"
#include "c4/regen/function.hpp"

#include <c4/c4_push.hpp>

namespace c4 {
namespace regen {

struct Class;


//-----------------------------------------------------------------------------

struct Member : public Var
{
    Class c$ m_class;

    void init_member(astEntityRef e, Class c$ c);
};


//-----------------------------------------------------------------------------

struct Method : public Function
{
    Class c$ m_class;

    void init_method(astEntityRef e, Class c$ c);
};


//-----------------------------------------------------------------------------

struct Class : public TaggedEntity
{
    std::vector<Member> m_members;
    std::vector<Method> m_methods;

    void init(astEntityRef e) override;
    void create_prop_tree(c4::yml::NodeRef n) const override;
};


//-----------------------------------------------------------------------------

struct ClassGenerator : public Generator
{
    ClassGenerator() : Generator() { m_entity_type = ENT_CLASS; }
};


} // namespace regen
} // namespace c4

#endif /* _c4_REGEN_CLASS_HPP_ */
