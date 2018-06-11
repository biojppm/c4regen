#include "c4/regen/function.hpp"

#include <c4/c4_push.hpp>

namespace c4 {
namespace regen {

void FunctionParameter::init_param(astEntityRef e, Function c$ f)
{
    this->Entity::init(e);
}

void Function::init(astEntityRef e)
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


} // namespace regen
} // namespace c4

#include <c4/c4_pop.hpp>