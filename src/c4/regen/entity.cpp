#include "c4/regen/entity.hpp"
#include "c4/yml/parse.hpp"

namespace c4 {
namespace regen {

void Entity::init(astEntityRef e)
{
    m_tu = e.tu;
    m_index = e.idx;
    m_cursor = e.cursor;
    m_parent = e.parent;
    m_region.init_region(*e.idx, e.cursor);
    m_str = m_region.get_str(to_csubstr(e.tu->m_contents));
    m_name = _get_display_name();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline bool is_idchar(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')
        || (c == '_' || c == '-' || c == '~' || c == '$');
}

csubstr find_pair(csubstr s, char open, char close, bool allow_nested)
{
    size_t b = s.find(open);
    if(b == csubstr::npos) return csubstr();
    if( ! allow_nested)
    {
        size_t e = s.find(close, b+1);
        if(e == csubstr::npos) return csubstr();
        return s.range(b, e);
    }
    size_t e, curr = b+1, count = 0;
    const char both[] = {open, close, '\0'};
    while((e = s.first_of(both, curr)) != csubstr::npos)
    {
        if(s[e] == open)
        {
            ++count;
            curr = e+1;
        }
        else if(s[e] == close)
        {
            if(count == 0) return s.range(b, e+1);
            --count;
            curr = e+1;
        }
    }
    return csubstr();
}


void Tag::parse_annotations()
{
    csubstr s = find_pair(m_str, '(', ')', /*allow_nested*/true);
    C4_ASSERT(s.len >= 2 && s.begins_with('(') && s.ends_with(')'));
    m_spec_str = s.range(1, s.len-1).trim(' ');
    m_annotations.clear();
    m_annotations.clear_arena();
    if(m_spec_str.empty()) return;
    substr yml_src = m_annotations.copy_to_arena(m_spec_str);
    c4::yml::parse(yml_src, &m_annotations);
}


} // namespace regen
} // namespace c4
