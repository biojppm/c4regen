#include <c4/regen/regen.hpp>
#include <c4/log/log.hpp>
#include <gtest/gtest.h>

namespace c4 {
namespace ast {

constexpr static const char *default_args[] = {"-x", "c++"};

struct test_unit
{
    Index idx;
    TranslationUnit unit;

    template< size_t SrcSz, size_t CmdSz >
    test_unit(const char (&src)[SrcSz], const char* (&cmd)[CmdSz])
        :
        idx(clang_createIndex(0, 0)),
        unit(idx, to_csubstr(src), cmd, CmdSz)
    {
    }

    template< size_t SrcSz >
    test_unit(const char (&src)[SrcSz])
        :
        idx(clang_createIndex(0, 0)),
        unit(idx, to_csubstr(src), default_args, C4_COUNTOF(default_args))
    {
    }

};

TEST(foo, bar)
{
    test_unit tu("int main() { return 0; }\0");
    tu.unit.visit_children([](Cursor c, Cursor parent, void*){
            std::string type = c.type_spelling();
            std::string cont = c.spelling();
            Location loc = c.location();
            c4::_log("{}:{}: {}({}B): {}", loc.file_c, loc.line, loc.column, loc.offset, c.kind_spelling());
            if( ! type.empty()) c4::_log(": type='{}'", type);
            if( ! cont.empty()) c4::_log(": cont='{}'", cont);
            c4::_print('\n');
            return CXChildVisit_Recurse;
        });
}


} // namespace ast
} // namespace c4
