#include "c4/regen/class.hpp"

namespace c4 {
namespace regen {


void Member::init_member(astEntityRef e, Class c$ c)
{
    this->Var::init(e);
    m_entity_type = ENT_MEMBER;
    m_class = c;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void Method::init_method(astEntityRef e, Class c$ c)
{
    this->Function::init(e);
    m_entity_type = ENT_METHOD;
    m_class = c;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------


void Class::init(astEntityRef e)
{
    this->TaggedEntity::init(e);
    m_entity_type = ENT_CLASS;

    struct _visit_data
    {
        Class *c;
    } vd{this};

    auto fn = [](ast::Cursor c, ast::Cursor parent, void *data) {
        auto vd = (_visit_data const*) data;
        if(c.kind() == CXCursor_FieldDecl)
        {
            ast::Entity ae;
            ae.cursor = c;
            ae.parent = parent;
            ae.idx = vd->c->m_index;
            ae.tu = vd->c->m_tu;
            vd->c->m_members.emplace_back();
            vd->c->m_members.back().init_member(ae, vd->c);
        }
        else if(c.kind() == CXCursor_CXXMethod)
        {
            ast::Entity ae;
            ae.cursor = c;
            ae.parent = parent;
            ae.idx = vd->c->m_index;
            ae.tu = vd->c->m_tu;
            vd->c->m_methods.emplace_back();
            vd->c->m_methods.back().init_method(ae, vd->c);
        }
        return CXChildVisit_Continue;
    };
    ast::visit_children(m_cursor, fn, &vd);
}


void Class::create_prop_tree(c4::yml::NodeRef n) const
{
    auto members = n["members"];
    members |= yml::SEQ;
    for(auto const& s : m_members)
    {
        auto sn = members.append_child();
        sn |= yml::MAP;
        s.create_prop_tree(sn);
    }
    
    auto methods = n["methods"];
    methods |= yml::SEQ;
    for(auto const& s : m_methods)
    {
        auto sn = methods.append_child();
        sn |= yml::MAP;
        s.create_prop_tree(sn);
    }

    TaggedEntity::create_prop_tree(n);
}


} // namespace regen
} // namespace c4
