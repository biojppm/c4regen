#include "c4/regen/entity.hpp"
#include "c4/yml/parse.hpp"

namespace c4 {
namespace regen {

void TemplateArg::init(Entity *e, unsigned i)
{
    m_kind = e->m_cursor.tpl_arg_kind(i);
    m_type = e->m_cursor.tpl_arg_type(i);
}


//-----------------------------------------------------------------------------

void Entity::init(astEntityRef e)
{
    m_tu = e.tu;
    m_index = e.idx;
    m_cursor = e.cursor;
    m_parent = e.parent;
    m_region.init_region(*e.idx, e.cursor);
    m_str = m_region.get_str(to_csubstr(e.tu->m_contents));
    m_name = to_csubstr(m_cursor.display_name(*m_index));
    m_spelling = to_csubstr(m_cursor.spelling(*m_index));
    m_type = to_csubstr(m_cursor.type_spelling(*m_index));
    m_brief_comment = to_csubstr(m_cursor.brief_comment(*m_index));
    m_raw_comment = to_csubstr(m_cursor.raw_comment(*m_index));
    if(m_name.empty()) m_name = _get_spelling();

    m_tpl_args.clear();
    if(m_cursor.is_tpl())
    {
        for(unsigned i = 0; i < m_cursor.num_tpl_args(); ++i)
        {
            m_tpl_args.emplace_back();
            m_tpl_args.back().init(this, i);
        }
    }
}

void Entity::create_prop_tree(c4::yml::NodeRef n) const
{
    n |= yml::MAP;
    n["name"] = m_name;
    n["spelling"] = m_spelling;
    n["kind"] = m_kind;
    n["type"] = m_type;
    n["brief_comment"] = m_brief_comment;
    n["raw_comment"] = m_raw_comment;

    if(m_cursor.is_tpl())
    {
        n["is_tpl"] = "1";
        if(m_cursor.is_tpl_class()) n["is_tpl_class"] = "1";
        if(m_cursor.is_tpl_function()) n["is_tpl_function"] = "1";
        for(auto const& tpl_arg : m_tpl_args)
        {

        }
    }

    n["region"] |= yml::MAP;
    n["region"]["file"] = to_csubstr(m_region.m_file);
    n["region"]["start"] |= yml::MAP;
    n["region"]["start"]["line"] << m_region.m_start.line;
    n["region"]["start"]["column"] << m_region.m_start.column;
    n["region"]["start"]["offset"] << m_region.m_start.offset;
    n["region"]["end"] |= yml::MAP;
    n["region"]["end"]["line"] << m_region.m_end.line;
    n["region"]["end"]["column"] << m_region.m_end.column;
    n["region"]["end"]["offset"] << m_region.m_start.offset;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool is_idchar(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')
        || (c == '_' || c == '-' || c == '~' || c == '$');
}

csubstr val2keyval_get_key(csubstr s)
{
    s = s.trim(' ');
    C4_CHECK(s[0] != '\'' && s[0] != '"');
    auto pos = s.find(": "); //! @todo: this may be inside quotes
    if(pos != csubstr::npos)
    {
        // assume it's a keyval
        return csubstr();
    }
    // ok, assume no key, so it's a val
    for(const char c : s)
    {
        C4_CHECK(is_idchar(c));
    }
    return s;
}

void Tag::_parse_annotations()
{
    csubstr s = s.pair_range_nested('(', ')');
    C4_ASSERT(s.len >= 2 && s.begins_with('(') && s.ends_with(')'));
    m_spec_str = s.range(1, s.len-1).trim(' ');
    m_annotations.clear();
    m_annotations.clear_arena();
    if(m_spec_str.empty()) return;
    substr yml_src = _normalize_map_str(m_spec_str);
    m_spec_str = yml_src;
    c4::yml::parse(yml_src, &m_annotations);
}

/** convert the code with a relaxed map to a strict YAML map so that it can be parsed */
substr Tag::_normalize_map_str(csubstr s)
{
    size_t prev = 0;
    bool needs_brackets = ! s.begins_with('{');
    if(needs_brackets) m_annotations.copy_to_arena("{");
    for(size_t i = 0; i < s.len; ++i)
    {
        char c = s[i];
        if(c == '\'' || c == '"')
        {
            csubstr ss = s.sub(i).pair_range_esc(c, '\\');
            C4_CHECK(!ss.empty());
            i += ss.len;
            substr ws = m_annotations.alloc_arena(ss.len);
            memcpy(ws.str, ss.str, ss.len);
            prev = i;
        }
        else if(c == '(')
        {
            csubstr ss = s.sub(i).pair_range_nested('(', ')');
            C4_CHECK(!ss.empty());
            i += ss.len;
            substr ws = m_annotations.alloc_arena(ss.len);
            memcpy(ws.str, ss.str, ss.len);
            prev = i;
        }
        else if(c == '[')
        {
            csubstr ss = s.sub(i).pair_range_nested('[', ']');
            C4_CHECK(!ss.empty());
            i += ss.len;
            substr ws = m_annotations.alloc_arena(ss.len);
            memcpy(ws.str, ss.str, ss.len);
            prev = i;
        }
        else if(c == ',') // skip commas within special regions
        {
            C4_ASSERT(i != 0);
            csubstr ss = s.range(prev, i);
            if(ss.empty()) break;
            ss = val2keyval_get_key(ss);
            prev = i+1;
            if(ss.empty()) continue;
            substr ws = m_annotations.alloc_arena(ss.len + 2 + 1 + 1);
            cat(ws, ss, ": 1,");
        }
    }
    if(prev < s.len)
    {
        substr ws = m_annotations.alloc_arena(s.len - prev);
        memcpy(ws.str, s.str + prev, s.len - prev);
    }
    if(needs_brackets) m_annotations.copy_to_arena("}");
    return m_annotations.arena();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void TaggedEntity::create_prop_tree(c4::yml::NodeRef n) const
{
    n |= yml::MAP;
    if(is_tagged())
    {
        m_tag.create_prop_tree(n["tag"]);
        if(m_tag.m_spec_str.not_empty())
        {
            auto a = n["annot"];
            a |= yml::MAP;
            m_tag.m_annotations.rootref().duplicate_children(a, a.last_child());
        }
    }
    Entity::create_prop_tree(n);
}


} // namespace regen
} // namespace c4
