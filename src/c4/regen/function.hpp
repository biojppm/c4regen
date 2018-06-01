#ifndef _c4_REGEN_FUNCTION_HPP_
#define _c4_REGEN_FUNCTION_HPP_

#include "c4/regen/entity.hpp"
#include "c4/regen/generator.hpp"

#include <c4/c4_push.hpp>

namespace c4 {
namespace regen {

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


//-----------------------------------------------------------------------------

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


//-----------------------------------------------------------------------------

/** generate code from free standing functions */
struct FunctionGenerator : public Generator
{
    FunctionGenerator() : Generator() { m_entity_type = ENT_FUNCTION; }
    void create_prop_tree(Entity const& o, c4::yml::NodeRef root) const override
    {
        C4_NOT_IMPLEMENTED();
    }
};


} // namespace regen
} // namespace c4

#include <c4/c4_pop.hpp>

#endif /* _c4_REGEN_FUNCTION_HPP_ */
