#include "c4/ast/ast.hpp"
#include "c4/std/string.hpp"

#include <c4/c4_push.hpp>

namespace c4 {
namespace ast {

thread_local std::vector< const char* > CompilationDb::s_cmd;
thread_local std::vector< std::string > CompilationDb::s_cmd_buf;

Cursor Cursor::next_sibling() const
{
    struct _nsibdata
    {
        Cursor c;
        Cursor next;
        bool   gotit;
    } data{*this, clang_getNullCursor(), false};
    Cursor parent = clang_getCursorLexicalParent(*this);
    if(clang_Cursor_isNull(parent))
    {
        parent = root();
    }
    visit_children(parent, [](Cursor c, Cursor parent, void* d_){
        auto d = (_nsibdata $) d_;
        if( ! d->gotit)
        {
            if(c.is_same(d->c))
            {
                d->gotit = true;    
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

Cursor Cursor::first_child() const
{
    struct _fchdata
    {
        Cursor c;
        Cursor child;
    } data{*this, clang_getNullCursor()};

    visit_children(*this, [](Cursor c, Cursor parent, void *d_){
        auto d = (_fchdata $) d_;
        if(parent.is_same(d->c))
        {
            d->child = c;
            return CXChildVisit_Break;
        }
        // not sure if this will ever be hit
        return CXChildVisit_Continue;
    }, &data);

    return data.child;
}
} // namespace ast
} // namespace c4

#include <c4/c4_pop.hpp>
