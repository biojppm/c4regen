#include <c4/regen/regen.hpp>
#include <c4/log/log.hpp>
#include <gtest/gtest.h>

namespace c4 {
namespace ast {

constexpr static const char *default_args[] = {""};

struct test_unit
{
    Index idx;
    TranslationUnit unit;

    template< size_t SrcSz, size_t CmdSz >
    test_unit(const char (&src)[SrcSz], const char* (&cmd)[CmdSz])
        :
        idx(),
        unit(idx, to_csubstr(src), cmd, CmdSz, default_options, /*delete_tmp*/false)
    {
    }

    template< size_t SrcSz >
    test_unit(const char (&src)[SrcSz])
        :
        idx(),
        unit(idx, to_csubstr(src), default_args, C4_COUNTOF(default_args), default_options, /*delete_tmp*/false)
    {
    }
};

TEST(foo, bar)
{
    test_unit tu(R"(
#define C4_ENUM()
C4_ENUM()
using TestEnum_e = enum {
    FOO,
    BAR
};
int main() { return 0; }
)");
    tu.unit.visit_children([](Cursor c, Cursor parent, void* data){
            auto &tu = *(test_unit*) data;
            const char* type = c.type_spelling(tu.idx);
            const char* cont = c.spelling(tu.idx);
            Location loc = c.location(tu.idx);
            c4::_log("{}:{}: {}({}B): {}", loc.file, loc.line, loc.column, loc.offset, c.kind_spelling(tu.idx));
            if(strlen(type)) c4::_log(": type='{}'", type);
            if(strlen(cont)) c4::_log(": cont='{}'", cont);
            c4::_print('\n');
            if(c.kind() == CXCursor_MacroExpansion)
            {
                c4::log("{}:{}: GOTIT!", loc.file, loc.line);
            }
            return CXChildVisit_Recurse;
        }, &tu);
}


} // namespace ast
} // namespace c4
