#include "c4/regen/enum.hpp"

namespace c4 {
namespace regen {


void EnumSymbol::init_symbol(astEntityRef r, Enum *e)
{
    this->Entity::init(r);
    m_enum = e;
    m_sym = m_name;
    if(e->m_underlying_type.is_integral_signed())
    {
        long long val = clang_getEnumConstantDeclValue(m_cursor);
        m_val_size = to_str(m_val_buf, val);
    }
    else if(e->m_underlying_type.is_integral_unsigned())
    {
        unsigned long long val = clang_getEnumConstantDeclUnsignedValue(m_cursor);
        m_val_size = to_str(m_val_buf, val);
    }
}


void Enum::init(astEntityRef e)
{
    m_entity_type = ENT_ENUM;
    this->TaggedEntity::init(e);
    m_underlying_type.m_cxtype = clang_getEnumDeclIntegerType(m_cursor);

    struct _symbol_visit_data
    {
        Enum *e;
    } vd{this};

    m_cursor.print_recursive();

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

} // namespace regen
} // namespace c4
