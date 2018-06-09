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
    csubstr  m_val;
    char     m_val_buf[32];

    void init_symbol(astEntityRef r, Enum *e);

};


//-----------------------------------------------------------------------------

/** an enumeration type */
struct Enum : public TaggedEntity
{
    std::vector<EnumSymbol> m_symbols;
    DataType m_underlying_type;

    virtual void init(astEntityRef e) override;

};


//-----------------------------------------------------------------------------

struct EnumGenerator : public Generator
{
    EnumGenerator() : Generator() { m_entity_type = ENT_ENUM; }
    void create_prop_tree(Entity const& o, c4::yml::NodeRef root) const override
    {
        C4_NOT_IMPLEMENTED();
    }
};



} // namespace regen
} // namespace c4

#include <c4/c4_pop.hpp>

#endif /* _c4_REGEN_ENUM_HPP_ */
