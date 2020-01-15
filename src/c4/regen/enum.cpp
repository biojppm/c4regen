#include "c4/regen/enum.hpp"

namespace c4 {
namespace regen {



void Enum::init(astEntityRef e)
{
    m_entity_type = ENT_ENUM;
    this->TaggedEntity::init(e);
    m_name = m_type;
    m_underlying_type.m_cxtype = clang_getEnumDeclIntegerType(m_cursor);

    //m_cursor.print_recursive();

    struct _symbol_visit_data
    {
        Enum *e;
    } vd{this};

    auto fn = [](ast::Cursor c, ast::Cursor parent, void *data) {
        auto vd = (_symbol_visit_data const*) data;
        if(c.kind() == CXCursor_EnumConstantDecl)
        {
            ast::Entity ae;
            ae.cursor = c;
            ae.parent = parent;
            ae.idx = vd->e->m_index;
            ae.tu = vd->e->m_tu;
            vd->e->m_symbols.emplace_back();
            vd->e->m_symbols.back().init_symbol(ae, vd->e);
        }
        return CXChildVisit_Continue;
    };
    ast::visit_children(m_cursor, fn, &vd);
}


void Enum::create_prop_tree(c4::yml::NodeRef n) const
{
    auto es = n["symbols"];
    es |= yml::SEQ;
    for(auto const& s : m_symbols)
    {
        auto sn = es.append_child();
        sn |= yml::MAP;
        s.create_prop_tree(sn);
    }
    TaggedEntity::create_prop_tree(n);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void EnumSymbol::init_symbol(astEntityRef r, Enum *e)
{
    this->Entity::init(r);
    m_enum = e;
    m_sym = m_name;
    if(e->m_underlying_type.is_integral_signed())
    {
        long long val = clang_getEnumConstantDeclValue(m_cursor);
        m_val_size = to_chars(m_val_buf, val);
    }
    else if(e->m_underlying_type.is_integral_unsigned())
    {
        unsigned long long val = clang_getEnumConstantDeclUnsignedValue(m_cursor);
        m_val_size = to_chars(m_val_buf, val);
    }
}

void EnumSymbol::create_prop_tree(c4::yml::NodeRef n) const
{
    n["symbol"] = m_sym;
    n["value"] = csubstr(m_val_buf, m_val_size);
    TaggedEntity::create_prop_tree(n);
}

} // namespace regen
} // namespace c4
