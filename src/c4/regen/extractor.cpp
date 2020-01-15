#include "c4/regen/extractor.hpp"

#include "c4/regen/source_file.hpp"
#include <c4/std/string.hpp>
#include <c4/std/vector.hpp>

#include <c4/c4_push.hpp>


namespace c4 {
namespace regen {

//-----------------------------------------------------------------------------

void Extractor::load(c4::yml::NodeRef n)
{
    C4_CHECK(n.valid());
    m_type = EXTR_ALL;
    m_macro.clear();
    m_entry.clear();

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
        m_macro.assign(m.val().str, m.val().len);
        m_type = EXTR_TAGGED_MACRO;

        c4::yml::NodeRef a = n.find_child("attr");
        if(a.valid())
        {
            m_type = EXTR_TAGGED_MACRO_ANNOTATED;
            m_entry.assign(a.val().str, a.val().len);
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

bool Extractor::has_true_annotation(csubstr annot, csubstr entry) const
{
    auto pos = annot.find(entry);
    if(pos == csubstr::npos) return false;
    csubstr val = annot.sub(pos, entry.len);
    if( ! val.begins_with(": ")) return true;
    val = val.sub(2);
    if(val.begins_with('"')) // value is delimited with double quotes
    {
        val = val.pair_range_esc('"');
        val = val.trim('"');
    }
    else if(val.begins_with('\'')) // value is delimited with single quotes
    {
        val = val.pair_range_esc('"');
        val = val.trim('\'');
    }
    else
    {
        pos = val.find(',');
        if(pos != csubstr::npos) // value is delimited with the next comma
        {
            val = val.left_of(pos);
        }
        else
        {
            pos = val.find(')'); // assume value is delimited with the next parens
            val = val.left_of(pos);
        }
    }
    C4_CHECK_MSG(val.not_empty(), "could not parse annotation entry");
    val = val.first_uint_span();
    C4_CHECK_MSG(val.not_empty(), "could not parse annotation entry");
    uint32_t v;
    bool ok = from_chars(val, &v);
    C4_CHECK_MSG(ok, "could not parse annotation entry");
    return v != 0;
}

//-----------------------------------------------------------------------------

Extractor::Data Extractor::extract(SourceFile c$$ sf, c4::ast::Cursor c) const
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
    case EXTR_TAGGED_MACRO_ANNOTATED:
        if(c.kind() == CXCursor_MacroExpansion)
        {
            if(c.display_name(*sf.m_index) == m_macro)
            {
                ast::Cursor subj = c.tag_subject();
                if(kind_matches(subj.kind()))
                {
                    bool annotation_ok;
                    if(m_type == EXTR_TAGGED_MACRO)
                    {
                        annotation_ok = true;
                    }
                    else if(m_type == EXTR_TAGGED_MACRO_ANNOTATED)
                    {
                        csubstr annotations = c.tag_annotations(sf.m_str);
                        annotation_ok = has_true_annotation(annotations, to_csubstr(m_entry));
                    }
                    else
                    {
                        C4_NOT_IMPLEMENTED();
                    }
                    if(annotation_ok)
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
        }
        return ret;
    default:
        C4_NOT_IMPLEMENTED();
    }
    return ret;
}

} // namespace regen
} // namespace c4

#include <c4/c4_pop.hpp>
