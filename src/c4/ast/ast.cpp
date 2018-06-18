#include "c4/ast/ast.hpp"
#include "c4/std/string.hpp"

#include <c4/c4_push.hpp>

namespace c4 {
namespace ast {

thread_local std::vector<const char*> CompilationDb::s_cmd;
thread_local std::vector<std::string> CompilationDb::s_cmd_buf;

Cursor Cursor::first_child() const
{
    struct _fchdata
    {
        Cursor this_;
        Cursor child;
    } data{*this, clang_getNullCursor()};

    visit_children(*this, [](Cursor c, Cursor parent, void *d_){
        auto d = (_fchdata $) d_;
        if(parent.is_same(d->this_) && d->child.is_null())
        {
            d->child = c;
            return CXChildVisit_Break;
        }
        // clang is still calling after returning break, so renew the vows here
        return CXChildVisit_Break;
    }, &data);

    return data.child;
}

Cursor Cursor::next_sibling() const
{
    struct _nsibdata
    {
        Cursor this_;
        Cursor next;
        bool   gotit;
        bool   should_break;
    } data{*this, clang_getNullCursor(), false, false};
    Cursor parent = clang_getCursorLexicalParent(*this);
    if(clang_Cursor_isNull(parent))
    {
        parent = root();
    }
    if(parent.is_same(*this)) return clang_getNullCursor();
    visit_children(parent, [](Cursor c, Cursor parent, void* d_){
        auto d = (_nsibdata $) d_;
        if( ! d->gotit)
        {
            if(c.is_same(d->this_))
            {
                d->gotit = true;
                return CXChildVisit_Continue;
            }
        }
        else if( ! d->should_break)
        {
            d->next = c;
            d->should_break = true; // hack to make sure we break
            return CXChildVisit_Break;
        }
        else if(d->gotit && d->should_break)
        {
            // clang is still calling after returning break, so renew the vows here
            return CXChildVisit_Break;
        }
        return CXChildVisit_Continue;
    }, &data);

    return data.next;
}

void Cursor::print_recursive(const char* msg, unsigned indent) const
{
    print(msg, indent);
    Cursor n = clang_getNullCursor();
    for(Cursor c = first_child(); !c.is_same(n); c = c.next_sibling())
    {
        c.print_recursive(msg, indent+1);
    }
}

void Cursor::print(const char* msg, unsigned indent) const
{
    CXSourceLocation loc = clang_getCursorLocation(*this);
    CXFile f;
    unsigned line, col, offs;
    clang_getExpansionLocation(loc, &f, &line, &col, &offs);
    print_str(clang_getFileName(f));
    if(msg) printf(": %s: ", msg);
    printf(":%u:%u: %*s", line, col, 2*indent, "");
    print_str(clang_getCursorKindSpelling(kind()));
    print_str(clang_getCursorDisplayName(*this), /*skip_empty*/true, ": name='%s'");
    print_str(clang_getTypeSpelling(clang_getCursorType(*this)), /*skip_empty*/true, ": type='%s'");
    print_str(clang_getCursorSpelling(*this), /*skip_empty*/true, ": spell='%s'");
    printf("\n");
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

namespace detail {

struct _visitor_data
{
    visitor_pfn visitor;
    void *data;
    bool same_unit_only;
    CXTranslationUnit transunit;
};

inline CXChildVisitResult _visit_impl(CXCursor cursor, CXCursor parent, CXClientData data)
{
    _visitor_data *C4_RESTRICT vd = reinterpret_cast<_visitor_data*>(data);
    // skip builtin cursors or cursors from other translation units
    if((vd->same_unit_only && (clang_Cursor_getTranslationUnit(cursor) != vd->transunit))
            || clang_Cursor_isMacroBuiltin(cursor))
    {
        return CXChildVisit_Continue;
    }
    // apparently the conditions above are not enough to filter out builtin
    // macros such as __cplusplus or _MSC_VER. So try to catch those here.
    else if(cursor.kind == CXCursor_MacroDefinition)
    {
        // is there a smarter way to do this?
        CXSourceLocation loc = clang_getCursorLocation(cursor);
        CXFile f;
        unsigned line, col, offs;
        clang_getExpansionLocation(loc, &f, &line, &col, &offs);
        CXString s = clang_getFileName(f);
        const char *cs = clang_getCString(s);
        if(cs == nullptr)
        {
            clang_disposeString(s);
            return CXChildVisit_Continue;
        }
        clang_disposeString(s);
    }
    enum CXChildVisitResult ret = vd->visitor(cursor, parent, vd->data);
    return ret;
}

} // namespace detail


void visit_children(Cursor root, visitor_pfn visitor, void *data, bool same_unit_only)
{
    detail::_visitor_data vd{visitor, data, same_unit_only, clang_Cursor_getTranslationUnit(root)};
    clang_visitChildren(root, &detail::_visit_impl, &vd);
}

} // namespace ast
} // namespace c4

#include <c4/c4_pop.hpp>
