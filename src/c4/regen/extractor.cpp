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

Extractor::Data Extractor::extract(c4::ast::Index &idx, c4::ast::Cursor c) const
{
    Extractor::Data ret;
    ret.extracted = false;
    switch(m_type)
    {
    case EXTR_ALL:
        ret.extracted = true;
        ret.cursor = c;
        ret.has_tag = false;
        return ret;
    case EXTR_TAGGED_MACRO:
        if(c.kind() == CXCursor_MacroExpansion)
        {
            if(c.display_name(idx) == m_tag)
            {
                ret.extracted = true;
                ret.cursor = c.next_sibling();
                ret.tag = c;
                ret.has_tag = true;
                return ret;
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
