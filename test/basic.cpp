#include <c4/regen/regen.hpp>
#include <c4/regen/exec.hpp>
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
            c4::_log("{}:{}:{}: {}", loc.file, loc.line, loc.column, c.kind_spelling(tu.idx));
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


TEST(foo, baz)
{
    const char cfg_yml_buf[] = R"(

writer: stdout # one of: stdout, samefile, genfile, gengroup, singlefile

generators:
  -
    name: enum_symbols
    type: enum # one of: enum, class, function
    extract:
      macro: C4_ENUM
    hdr_preamble: |
      #include "enum_pairs.h"
    hdr: |
      template<> const EnumPairs<{{enum.type}}> enum_pairs();
    src: |
      template<> const EnumPairs<{{enum.type}}> enum_pairs()
      {
          static const EnumAndName<{{enum.type}}> vals[] = {
              {% for e in enum.symbols %}
              { {{e.name}}, "{{e.name}}"},
              {% endfor %}
          };
          EnumPairs<{{enum.type}}> r(vals);
          return r;
      }
)";
    csubstr yml = cfg_yml_buf;
    auto cfg = c4::fs::ScopedTmpFile(yml.str, yml.len, "c4regen.tmp-XXXXXXXX.cfg.yml");
    cfg.do_delete(false);
    c4::regen::exec({"-x", "generate", "-c", cfg.m_name});
}

} // namespace ast
} // namespace c4
