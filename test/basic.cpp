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
    test_unit tu(R"(#define C4_ENUM(...)
C4_ENUM(x, y, z, a: b, c: [d, e, f], h: {i: j, k: 'l'})
using TestEnum_e = enum {
    FOO,
    BAR
};
int main(int argc, char *argv[]) { return 0; }
)");
    tu.unit.visit_children([](Cursor c, Cursor parent, void* data){
            auto &tu = *(test_unit*) data;
            const char* name  = c.display_name(tu.idx);
            const char* type  = c.type_spelling(tu.idx);
            const char* spell = c.spelling(tu.idx);
            Location loc = c.location(tu.idx);
            c4::_log("{}:{}: {}({}B): {}", loc.file, loc.line, loc.column, loc.offset, c.kind_spelling(tu.idx));
            if(strlen(name)) c4::_log(": name='{}'", name);
            if(strlen(type)) c4::_log(": type='{}'", type);
            if(strlen(spell)) c4::_log(": spell='{}'", spell);
            c4::_print('\n');
            if(c.kind() == CXCursor_MacroExpansion)
            {
                c4::log("{}:{}: GOTIT!", loc.file, loc.line);
                EXPECT_STREQ(name, "C4_ENUM");
            }
            return CXChildVisit_Recurse;
        }, &tu);
    std::vector<ast::Entity> ws;

    tu.unit.select_tagged("C4_ENUM", &ws);
    EXPECT_EQ(ws.size(), 1);
}


} // namespace ast
} // namespace c4
