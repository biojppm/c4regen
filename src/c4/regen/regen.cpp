#include "c4/regen/regen.hpp"

namespace c4 {
namespace regen {

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
    size_t e, curr = b, count = 0;
    const char both[] = {open, close};
    while((e = s.first_of(both, curr)) != csubstr::npos)
    {
        if(s[e] == open)
        {
            ++count;
            curr = e+1;
        }
        else if(s[e] == close)
        {
            if(count == 0) return s.range(b, e);
            --count;
            curr = e+1;
        }
    }
    return csubstr();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
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

size_t Extractor::extract(CXCursorKind kind, c4::ast::TranslationUnit const& tu, std::vector<ast::Entity> *out) const
{
    switch(m_type)
    {
    case EXTR_ALL:
    {
        ast::CursorMatcher matcher{kind, ""};
        return tu.select(matcher, out);
    }
    case EXTR_TAGGED_MACRO:
    case EXTR_TAGGED_MACRO_ANNOTATED:
    {
        // select all macro expansions
        ast::CursorMatcher matcher{CXCursor_MacroExpansion, to_csubstr(m_tag)};
        size_t sz = out->size();
        size_t ret = tu.select(matcher, out);
        // filter in the specified kind
        size_t curr = 0;
        C4_NOT_IMPLEMENTED();
        /*
        for(size_t i = 0; i < ret; ++i)
        {
            ast::Cursor c = (*out)[sz + i].next_sibling();
            if(c.kind() == kind)
            {
                (*out)[sz + curr] = c;
                ++curr;
            }
        }
         */
        if(m_type == EXTR_TAGGED_MACRO_ANNOTATED)
        {
            // filter in only annotated occurrences
            C4_NOT_IMPLEMENTED();
        }
        return curr;
    }
    default:
        C4_NOT_IMPLEMENTED();
    }
    return false;
}

Extractor::Data Extractor::extract(c4::ast::Index &idx, c4::ast::Cursor c) const
{
    Extractor::Data ret;
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
                ret.cursor = c.next_sibling();
                ret.tag = c;
                ret.extracted = true;
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


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct FileType
{
    std::vector<std::string> m_extensions;
    FileType(std::initializer_list<const char*> exts);
    void add(const char* extension);
    bool matches(csubstr filename) const;
};

FileType s_hdr_exts = {".h", ".hpp", ".hxx", ".h++", ".hh", ".inl"};
FileType s_src_exts = {".c", ".cpp", ".cxx", ".c++", ".cc"};

FileType::FileType(std::initializer_list<const char*> exts)
{
    m_extensions.reserve(exts.size());
    for(auto e : exts)
    {
        add(e);
    }
}

void FileType::add(const char* extension)
{
    m_extensions.push_back(extension);
}

bool FileType::matches(csubstr filename) const
{
    for(auto const& sext : m_extensions)
    {
        auto ext = to_csubstr(sext);
        if(filename.ends_with(ext))
        {
            return true;
        }
    }
    return false;
}


//-----------------------------------------------------------------------------

bool is_hdr(csubstr filename) { return s_hdr_exts.matches(filename); }
bool is_src(csubstr filename) { return s_src_exts.matches(filename); }

void add_hdr(const char* ext) { s_hdr_exts.add(ext); }
void add_src(const char* ext) { s_src_exts.add(ext); }

} // namespace regen
} // namespace c4
