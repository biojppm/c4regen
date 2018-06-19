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
    if(is_null()) return clang_getNullCursor();
    struct _nsibdata
    {
        Cursor this_;
        Cursor next;
        bool   gotit;
    } data{*this, clang_getNullCursor(), false};
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
        else
        {
            d->next = c;
            return CXChildVisit_Break;
        }
        return CXChildVisit_Continue;
    }, &data);

    return data.next;
}

void Cursor::print_recursive(const char* msg, unsigned indent) const
{
    struct _visit_data
    {
        const char *msg_;
        unsigned indent_;
        Cursor prev_parent;
        Cursor prev_child;
    } vd{msg, indent, clang_getNullCursor(), *this};
    visit_children(*this, [](Cursor c, Cursor parent, void *data) {
        auto vd = (_visit_data $) data;
        if(parent.is_same(vd->prev_child)) 
        {
            ++vd->indent_;
        }
        else if(c.is_same(vd->prev_parent))
        {
            --vd->indent_;
        }
        c.print(vd->msg_, vd->indent_);
        vd->prev_parent = parent;
        vd->prev_child = c;
        return CXChildVisit_Recurse;
    }, &vd);
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
    CXTranslationUnit transunit;
    bool same_unit_only;
    bool should_break;
};

inline CXChildVisitResult _visit_impl(CXCursor cursor, CXCursor parent, CXClientData data);

} // namespace detail


void visit_children(Cursor root, visitor_pfn visitor, void *data, bool same_unit_only)
{
    detail::_visitor_data vd{visitor, data, clang_Cursor_getTranslationUnit(root), same_unit_only, false};
    clang_visitChildren(root, &detail::_visit_impl, &vd);
}


inline CXChildVisitResult detail::_visit_impl(CXCursor cursor, CXCursor parent, CXClientData data)
{
    _visitor_data *C4_RESTRICT vd = reinterpret_cast<_visitor_data*>(data);
    //printf("fdx: "); Cursor ccc = cursor; ccc.print("before filter");
    if(vd->should_break)
    {
        // clang is still calling after returning break, so renew the vows here
        return CXChildVisit_Break;
    }
    // skip builtin cursors or cursors from other translation units
    if((vd->same_unit_only && (clang_Cursor_getTranslationUnit(cursor) != vd->transunit))
            || clang_Cursor_isMacroBuiltin(cursor))
    {
        //printf("skip diff unit....\n");
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
            //printf("skip builtin....\n");
            return CXChildVisit_Continue;
        }
        clang_disposeString(s);
    }
    //printf("after filter, calling\n");
    auto ret = vd->visitor(cursor, parent, vd->data);
    //printf("call done!\n");
    if(ret == CXChildVisit_Break)
    {
        // hack to make sure we break
        //printf("break this!\n");
        vd->should_break = true;
    }
    return ret;
}

} // namespace ast
} // namespace c4

#include <c4/c4_pop.hpp>
