#ifndef _c4_REGEN_ENUM_HPP_
#define _c4_REGEN_ENUM_HPP_

#include "c4/regen/entity.hpp"
#include "c4/regen/generator.hpp"

#include <c4/c4_push.hpp>

namespace c4 {
namespace regen {


struct Enum;
struct EnumSymbol : public TaggedEntity
{
    Enum    *m_enum;
    csubstr  m_sym;
    char     m_val_buf[32];
    size_t   m_val_size;

    void init_symbol(astEntityRef r, Enum *e);
    void create_prop_tree(c4::yml::NodeRef n) const override;
};


//-----------------------------------------------------------------------------

/** an enumeration type */
struct Enum : public TaggedEntity
{
    std::vector<EnumSymbol> m_symbols;
    DataType m_underlying_type;

    virtual void init(astEntityRef e) override;
    virtual void create_prop_tree(c4::yml::NodeRef n) const override;
};


//-----------------------------------------------------------------------------

struct EnumGenerator : public Generator
{
    EnumGenerator() : Generator() { m_entity_type = ENT_ENUM; }
};



} // namespace regen
} // namespace c4

#include <c4/c4_pop.hpp>

#endif /* _c4_REGEN_ENUM_HPP_ */
