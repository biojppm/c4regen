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


struct SrcAndGen
{
    const char *src;
    const char *gen;
};

void test_regen_exec(const char *cfg_yml_buf, std::initializer_list<SrcAndGen> sources)
{
    auto yml = to_csubstr(cfg_yml_buf);
    auto cfg_file = c4::fs::ScopedTmpFile(yml.str, yml.len, "c4regen.tmp-XXXXXXXX.cfg.yml", "wb", /*do_delete*/false);
    std::vector<const char*> args = {"-x", "generate", "-c", cfg_file.m_name, "-f", "'-x'", "-f", "'c++'"};
    std::vector<c4::fs::ScopedTmpFile> src_files;
    std::vector<std::string> src_file_fullnames;
    std::string cwd;
    fs::cwd(&cwd);
    for(SrcAndGen sg : sources)
    {
        src_files.emplace_back(sg.src, strlen(sg.src), "c4regen.tmp-XXXXXXXX.cpp", "wb", /*do_delete*/false);
        // we need the full path to the file
        src_file_fullnames.emplace_back();
        src_files.back().full_path(&src_file_fullnames.back());
        args.emplace_back(src_file_fullnames.back().c_str());
    }
    c4::regen::exec((int)args.size(), args.data());
}


TEST(foo, baz)
{
    test_regen_exec(R"(
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
      // {{name}}
      template<> const EnumPairs<{{enum.type}}> enum_pairs();
    src: |
      // {{name}}
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
)",
    {{"", ""},
     {R"(#define C4_ENUM(...)
C4_ENUM(foo, bar: baz)
typedef enum {FOO, BAR} MyEnum_e;
)"}
    });
}

} // namespace ast
} // namespace c4
