#include "c4/regen/extractor.hpp"

#include <c4/std/string.hpp>
#include <c4/std/vector.hpp>


namespace c4 {
namespace regen {

//-----------------------------------------------------------------------------

void Extractor::load(c4::yml::NodeRef n)
{
    C4_CHECK(n.valid());
    m_type = EXTR_ALL;
    m_tag.clear();
    m_attr.clear();

    if(!n.valid()) return;
    C4_CHECK(n.key() == "extract");
    if(n.is_keyval())
    {
        if(n.val() == "all")
        {
            m_type = EXTR_ALL;
        }
        else
        {
            C4_ERROR("unknown extractor");
        }
    }
    else
    {
        c4::yml::NodeRef m = n["macro"];
        C4_CHECK(m.valid());
        m_tag.assign(m.val().str, m.val().len);
        m_type = EXTR_TAGGED_MACRO;

        c4::yml::NodeRef a = n.find_child("attr");
        if(a.valid())
        {
            m_type = EXTR_TAGGED_MACRO_ANNOTATED;
            m_attr.assign(a.val().str, a.val().len);
        }
    }
}


//-----------------------------------------------------------------------------

void Extractor::set_kinds(std::initializer_list<CXCursorKind> il)
{
    m_cursor_kinds.clear();
    for(auto k : il)
    {
        m_cursor_kinds.emplace_back(k);
    }
}

bool Extractor::kind_matches(CXCursorKind k) const
{
    for(auto ck : m_cursor_kinds)
    {
        if(ck == k) return true;
    }
    return false;
}

//-----------------------------------------------------------------------------

Extractor::Data Extractor::extract(c4::ast::Index &idx, c4::ast::Cursor c) const
{
    Extractor::Data ret;
    ret.extracted = false;
    switch(m_type)
    {
    case EXTR_ALL:
        if(kind_matches(c.kind()))
        {
            ret.extracted = true;
            ret.cursor = c;
            ret.has_tag = false;
            return ret;
        }
    case EXTR_TAGGED_MACRO:
        if(c.kind() == CXCursor_MacroExpansion)
        {
            if(c.display_name(idx) == m_tag)
            {
                ast::Cursor subj = c.tag_subject();
                if(kind_matches(subj.kind()))
                {
                    ret.extracted = true;
                    ret.cursor = subj;
                    ret.tag = c;
                    ret.has_tag = true;
                    C4_CHECK( ! ret.cursor.is_null());
                    C4_CHECK( ! ret.cursor.is_same(ret.tag));
                    return ret;
                }
            }
        }
        return ret;
    case EXTR_TAGGED_MACRO_ANNOTATED:
    default:
        C4_NOT_IMPLEMENTED();
    }
    return ret;
}

} // namespace regen
} // namespace c4
