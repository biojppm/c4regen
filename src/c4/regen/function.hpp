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
    DataType m_data_type;

    void init_param(astEntityRef e, Function c$ f);
};


//-----------------------------------------------------------------------------

/** a free-standing function */
struct Function : public TaggedEntity
{
    DataType m_return_type;
    std::vector<FunctionParameter> m_parameters;

    virtual void init(astEntityRef e) override;
};


//-----------------------------------------------------------------------------

/** generate code from free standing functions */
struct FunctionGenerator : public Generator
{
    FunctionGenerator() : Generator() { m_entity_type = ENT_FUNCTION; }
};


} // namespace regen
} // namespace c4

#include <c4/c4_pop.hpp>

#endif /* _c4_REGEN_FUNCTION_HPP_ */
