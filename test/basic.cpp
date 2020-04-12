#include <c4/std/vector.hpp>
#include <c4/log/log.hpp>
#include <c4/regen/regen.hpp>
#include <c4/regen/exec.hpp>
#include <c4/yml/yml.hpp>
#include <gtest/gtest.h>

namespace c4 {
namespace ast {

constexpr static const char *default_args[] = {""};

struct test_unit
{
    Index idx;
    TranslationUnit unit;

    template<size_t SrcSz, size_t CmdSz>
    test_unit(const char (&src)[SrcSz], const char* (&cmd)[CmdSz])
        :
        idx(),
        unit(idx, to_csubstr(src), cmd, CmdSz, default_options, /*delete_tmp*/false)
    {
    }

    template<size_t SrcSz>
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
            C4_UNUSED(parent);
            auto &tu_ = *(test_unit*) data;
            const char* name  = c.display_name(tu_.idx);
            const char* type  = c.type_spelling(tu_.idx);
            const char* spell = c.spelling(tu_.idx);
            Location loc = c.location(tu_.idx);
            c4::_log("{}:{}:{}: {}", loc.file, loc.line, loc.column, c.kind_spelling(tu_.idx));
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


//-----------------------------------------------------------------------------

TEST(ast, first_child__next_sibling)
{
    test_unit tu(R"(#define C4_ENUM(...)
C4_ENUM(foo, bar: baz)
typedef enum {FOO, BAR} MyEnum_e;
)");

    /*
    clang_visitChildren(tu.unit.root(), [](CXCursor c, CXCursor parent, void *data) {
        Cursor(c).print("crl");
        return CXChildVisit_Recurse;
    }, nullptr);
    */

    ast::Cursor r = tu.unit.root();
    ast::Cursor c0 = r.first_child();
    ast::Cursor c1 = c0.next_sibling();
    ast::Cursor c2 = c1.next_sibling();
    ast::Cursor c20 = c2.first_child();
    ast::Cursor c21 = c20.next_sibling();
    ast::Cursor c22 = c21.next_sibling();
    ast::Cursor c3 = c2.next_sibling();
    ast::Cursor c30 = c3.first_child();
    ast::Cursor c300 = c30.first_child();
    ast::Cursor c301 = c300.next_sibling();
    ast::Cursor c302 = c301.next_sibling();
    ast::Cursor c31 = c30.next_sibling();
    ast::Cursor c4 = c3.next_sibling();

    EXPECT_EQ(c0.kind(), CXCursor_MacroDefinition); EXPECT_TRUE(c0.first_child().is_null());
    EXPECT_EQ(c1.kind(), CXCursor_MacroExpansion);  EXPECT_TRUE(c1.first_child().is_null());
    EXPECT_EQ(c2.kind(), CXCursor_EnumDecl);
    EXPECT_EQ(c20.kind(), CXCursor_EnumConstantDecl); EXPECT_TRUE(c20.first_child().is_null());
    EXPECT_EQ(c21.kind(), CXCursor_EnumConstantDecl); EXPECT_TRUE(c21.first_child().is_null());
    EXPECT_TRUE(c22.is_null());
    EXPECT_FALSE(c3.is_same(c30));
    EXPECT_FALSE(c30.is_same(c3));
    EXPECT_EQ(c3.kind(), CXCursor_TypedefDecl);
    EXPECT_EQ(c30.kind(), CXCursor_EnumDecl);
    EXPECT_EQ(c300.kind(), CXCursor_EnumConstantDecl); EXPECT_TRUE(c300.first_child().is_null());
    EXPECT_EQ(c301.kind(), CXCursor_EnumConstantDecl); EXPECT_TRUE(c301.first_child().is_null());
    EXPECT_TRUE(c302.is_null());
    EXPECT_EQ(c31.kind(), CXCursor_TypedefDecl); // RECURSION!!!
    EXPECT_TRUE(c4.is_null());

    int i = 0;
    for(Cursor c = r.first_child(); c; c = c.next_sibling(), ++i)
    {
        switch(i)
        {
        case 0: EXPECT_TRUE(c.is_same(c0)); break;
        case 1: EXPECT_TRUE(c.is_same(c1)); break;
        case 2: EXPECT_TRUE(c.is_same(c2)); break;
        case 3: EXPECT_TRUE(c.is_same(c3)); break;
        default: GTEST_FAIL(); break;
        }
    }
    EXPECT_EQ(i, 4);

    i = 0;
    for(Cursor c = c2.first_child(); c; c = c.next_sibling(), ++i)
    {
        switch(i)
        {
        case 0: EXPECT_TRUE(c.is_same(c20)); break;
        case 1: EXPECT_TRUE(c.is_same(c21)); break;
        default: GTEST_FAIL(); break;
        }
    }
    EXPECT_EQ(i, 2);

    i = 0;
    for(Cursor c = c3.first_child(); c; c = c.next_sibling(), ++i)
    {
        switch(i)
        {
        case 0: EXPECT_TRUE(c.is_same(c30)); break;
        case 1: EXPECT_TRUE(c.is_same(c31)); break;
        default: GTEST_FAIL(); break;
        }
    }
    EXPECT_EQ(i, 2);

    i = 0;
    for(Cursor c = c30.first_child(); c; c = c.next_sibling(), ++i)
    {
        switch(i)
        {
        case 0: EXPECT_TRUE(c.is_same(c300)); break;
        case 1: EXPECT_TRUE(c.is_same(c301)); break;
        default: GTEST_FAIL(); break;
        }
    }
    EXPECT_EQ(i, 2);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

using GenStrs = c4::regen::CodeInstances<std::string>;
using Triplet = regen::CodeInstances<const char *>;

struct SrcAndGen
{
    const char *name;
    const char *src;
    Triplet gen;

    SrcAndGen(const char *name_, const char* src_, Triplet t) : name(name_), src(src_), gen(t) {}
    SrcAndGen(const char *name_, const char* src_) : name(name_), src(src_), gen({"", "", ""}) {}
    SrcAndGen(const char *name_, const char* src_, const char *gen_hdr) : name(name_), src(src_), gen({gen_hdr, "", ""}) {}
    SrcAndGen(const char *name_, const char* src_, const char *gen_hdr, const char *gen_src) : name(name_), src(src_), gen({gen_hdr, "", gen_src}) {}
};

struct GenCompare
{
    SrcAndGen const& case_;
    c4::regen::Regen const& rg;
    c4::regen::SourceFile const& src_file;

    GenCompare(SrcAndGen const& case__, c4::regen::Regen const& rg_, c4::regen::SourceFile const&  src_file_)
        : case_(case__), rg(rg_), src_file(src_file_)
    {
    }

    void check(GenStrs *gen_files, GenStrs *gen_code)
    {
        rg.m_writer.m_impl->extract_filenames(src_file.m_name, gen_files);
        c4::fs::file_get_contents(gen_files->m_hdr.c_str(), &gen_code->m_hdr);
        c4::fs::file_get_contents(gen_files->m_inl.c_str(), &gen_code->m_inl);
        c4::fs::file_get_contents(gen_files->m_src.c_str(), &gen_code->m_src);
        compare_hdr(gen_code->m_hdr, case_.gen.m_hdr);
        compare_inl(gen_code->m_inl, case_.gen.m_inl);
        compare_src(gen_code->m_src, case_.gen.m_src);
    }

    void compare_hdr(std::string const& hdr, const char *expected)
    {
        EXPECT_EQ(hdr, expected);
    }

    void compare_inl(std::string const& inl, const char *expected)
    {
        EXPECT_EQ(inl, expected);
    }

    void compare_src(std::string const& src, const char *expected)
    {
        EXPECT_EQ(src, expected);
    }
};



void test_regen_exec(const char *test_name, const char *cfg_yml_buf, std::initializer_list<SrcAndGen> cases)
{
    using arg = std::vector<char>;
    auto putcontents = [](arg const& filename, csubstr contents) {
        arg tmp_ = filename;
        substr dirname = to_substr(tmp_).dirname();
        tmp_[dirname.len] = '\0';
        fs::mkdirs(dirname.data());
        fs::file_put_contents(filename.data(), contents);
    };

    csubstr yml = to_csubstr(cfg_yml_buf);
    arg cwd, tmpdir, casedir, cfgfile, srcfile, srcfilefull;
    std::vector<arg> src_files;
    std::vector<arg> src_file_fullnames;
    // place all test files under this directory
    tmpdir = fs::tmpnam<arg>("test_tmp/XXXXXXXX/");
    catrs(append, &tmpdir, to_csubstr(test_name), "/");
    catrs(&cfgfile, to_csubstr(tmpdir), "c4regen.cfg.yml", '\0');
    cwd = c4::fs::cwd<arg>();
    putcontents(cfgfile, yml);
    std::vector<const char*> args = {
        "--cmd", "generate",
        "--flag", "'-x'",
        "--flag", "c++",
        "--cfg", cfgfile.data(),
        "--",
    };
    for(SrcAndGen sg : cases)
    {
        // use a separate directory for each case
        catrs(&casedir, to_csubstr(tmpdir), sg.name, "/");
        catrs(&srcfile, to_csubstr(casedir), "c4regen.cpp", '\0');
        catrs(&srcfilefull, to_csubstr(cwd), "/", to_csubstr(srcfile)); // we need the full path to the file
        putcontents(srcfile, to_csubstr(sg.src));
        src_files.emplace_back(srcfile);
        src_file_fullnames.emplace_back(srcfilefull);
        args.emplace_back(src_file_fullnames.back().data());
    }

    c4::regen::Regen rg;
    rg.m_save_src_files = true;
    c4::regen::exec(&rg, (int)args.size(), args.data(), /*skip_exe_name*/false);

    GenStrs filenames, generated;

    ASSERT_EQ(cases.size(), rg.m_src_files.size());

    int count = 0;
    for(SrcAndGen case_ : cases)
    {
        GenCompare gc(case_, rg, rg.m_src_files[count++]);
        gc.check(&filenames, &generated);
    }
}


TEST(enums, basic)
{
    test_regen_exec("enums.basic", R"(
# one of: stdout, samefile, genfile, gengroup, singlefile
writer: gengroup
tpl:
  chunk: |
    {{gencode}}
  hdr: |
    #pragma once
    {{hdr.preamble}}
    {{hdr.gencode}}
  src: |
    {% if has_hdr %}
    #include "{{hdr.filename}}"
    {% endif %}
    {{src.preamble}}
generators:
  - name: enum_symbols
    type: enum # one of: enum, class, function
    extract:
      macro: C4_ENUM
    hdr_preamble: |
      #include "enum_pairs.h"
    hdr: |
      template<> const EnumPairs<{{type}}> enum_pairs();
    src: |
      template<> const EnumPairs<{{type}}> enum_pairs()
      {
          static const EnumAndName<{{type}}> vals[] = {
              {% for e in symbols %}
              { {{e.name}}, "{{e.name}}"},
              {% endfor %}
          };
          EnumPairs<{{type}}> r(vals);
          return r;
      }
)",
    {{"empty_sources", "", ""},

     {"basic_enum",
      R"(#define C4_ENUM(...)
C4_ENUM()
typedef enum {FOO, BAR} MyEnum_e;
)",
      /*src*/R"()"},

     {"enum_with_meta",
      R"(#define C4_ENUM(...)
C4_ENUM(aaa, bbb: ccc)
typedef enum {FOO, BAR} MyEnum_e;
)",
      /*src*/R"()"},
    });
}



TEST(classes, basic)
{
    test_regen_exec("classes.basic", R"(
writer: gengroup
generators:
  -
    name: class_members
    type: class # one of: enum, class, function
    extract:
      macro: C4_CLASS
    hdr: |
      void show({{name}} const& obj);
    src_preamble: |
      #include <iostream>
    src: |
      void show({{name}} const& obj);
      {
          {% for m in members %}
          std::cout << "member: '{{m.name}}' of type '{{m.type}}': value=" << obj.{{m.name}} << "\n";
          {% endfor %}
          {% for m in methods %}
          std::cout << "method: '{{m.name}}' of type '{{m.type}}'\n";
          {% endfor %}
      }
)",
    {{"empty", "", ""},
     {"two_classes", R"(#define C4_CLASS(...)
C4_CLASS()
struct foo
{
  int a, b, c;
  float x, y, z;

  void some_method(foo const& that);
};

C4_CLASS()
struct bar
{
  int za, zb, zc;
  float zx, zy, zz;

  void some_other_method(bar const& that);
};
)"}
    });
}

} // namespace ast
} // namespace c4
