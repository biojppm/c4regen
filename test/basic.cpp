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

using GenStrs = regen::CodeInstances<std::string>;
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



void test_regen_exec(const char *test_name, const char *cfg_yml_buf, SrcAndGen sg)
{
    SCOPED_TRACE(sg.name);

    using arg = std::vector<char>;
    auto putcontents = [](arg const& filename, csubstr contents) {
        arg tmp_ = filename;
        substr dirname = to_substr(tmp_).dirname();
        if(dirname.ends_with('/') || dirname.ends_with('\\'))
        {
            dirname = dirname.first(dirname.len - 1);
        }
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

    // use a separate directory for each case
    catrs(&casedir, to_csubstr(tmpdir), sg.name, "/");
    catrs(&srcfile, to_csubstr(casedir), "c4regen.cpp", '\0');
    catrs(&srcfilefull, to_csubstr(cwd), "/", to_csubstr(srcfile)); // we need the full path to the file
    putcontents(srcfile, to_csubstr(sg.src));
    src_files.emplace_back(srcfile);
    src_file_fullnames.emplace_back(srcfilefull);
    args.emplace_back(src_file_fullnames.back().data());

    c4::regen::Regen rg;
    rg.m_save_src_files = true;
    c4::regen::exec(&rg, (int)args.size(), args.data(), /*skip_exe_name*/false);

    GenStrs filenames, generated;

    int count = 0;
    GenCompare gc(sg, rg, rg.m_src_files[count++]);
    gc.check(&filenames, &generated);
}

csubstr get_writer(regen::Writer::Type_e type)
{
    using I = std::underlying_type<regen::Writer::Type_e>::type;
    static const std::map<I, csubstr> specs = {
        {regen::Writer::STDOUT, "writer: stdout"},
        {regen::Writer::GENGROUP, "writer: gengroup"},
        {regen::Writer::GENFILE, "writer: genfile"},
        {regen::Writer::SAMEFILE, "writer: samefile"},
        {regen::Writer::SINGLEFILE, "writer: singlefile"},
    };
    auto it = specs.find(type);
    C4_CHECK(it != specs.end());
    return it->second;
}

typedef enum {
    TPL_DEFAULT,
} TemplateType_e;
csubstr get_tpl(TemplateType_e type)
{
    using I = std::underlying_type<TemplateType_e>::type;
    static const std::map<I, csubstr> specs = {
        {
            TPL_DEFAULT,
            R"(
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
)"
        },
    };
    auto it = specs.find(type);
    C4_CHECK(it != specs.end());
    return it->second;
}


typedef enum {
    GEN_ENUM_SYMBOLS,
    GEN_CLASS_MEMBERS,
} GeneratorType_e;
csubstr get_generator(GeneratorType_e type)
{
    using I = std::underlying_type<GeneratorType_e>::type;
    static const std::map<I, csubstr> specs = {
        {
            GEN_ENUM_SYMBOLS,
            R"(
name: enum_symbols
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
)"
        },
        {
            GEN_CLASS_MEMBERS,
            R"(
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
)"
        },
    };
    auto it = specs.find(type);
    C4_CHECK(it != specs.end());
    return it->second;
}

constexpr const char basic_enums_cfg[] = R"(
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
    #include "{{hdr.filename}}"
    
    {{src.gencode}}
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
          static constexpr const EnumAndName<{{type}}> vals[] = {
              {% for e in symbols %}
              { {{e.name}}, "{{e.name}}"},
              {% endfor %}
          };
          {% if meta %}{% if meta.aaa %}/* has meta.aaa: {{meta.aaa}} */{% endif %}
          {% if meta.aaa != 0 %}/* meta.aaa is not 0 */{% endif %}
          {% if meta.aaa == 1 %}/* meta.aaa is 1 */{% endif %}
          {% if meta.bbb %}/* has meta.bbb: {{meta.bbb}} */{% endif %}
          {% if meta.bbb == ccc %}/* meta.bbb is ccc */{% endif %}
          {% if meta.bbb == "ccc" %}/* meta.bbb is "ccc" */{% endif %}{% endif %}
          EnumPairs<{{type}}> r(vals);
          return r;
      }
)";

TEST(enums_basic, empty_sources)
{
    test_regen_exec("enums.basic", basic_enums_cfg, {
        "empty_sources",
        /*read*/R"()",
        /*hdr*/R"()",
        /*src*/R"()",
    });
}

TEST(enums_basic, vanilla)
{
    test_regen_exec("enums.basic", basic_enums_cfg, {
        "vanilla",
        /*read*/R"(#define C4_ENUM(...)
C4_ENUM()
typedef enum {FOO, BAR} MyEnum_e;
)",
        /*hdr*/R"(#pragma once
#include "enum_pairs.h"

template<> const EnumPairs<MyEnum_e> enum_pairs();


)",
        /*src*/R"(#include "c4regen.c4gen.hpp"

template<> const EnumPairs<MyEnum_e> enum_pairs()
{
    static constexpr const EnumAndName<MyEnum_e> vals[] = {
                { FOO, "FOO"},
                { BAR, "BAR"},
        
    };
    
    EnumPairs<MyEnum_e> r(vals);
    return r;
}


)"
    });
}


TEST(enums_basic, vanilla_with_annotations)
{
    
    test_regen_exec("enums_basic", basic_enums_cfg, {
        "vanilla_with_meta",
        /*read*/R"(#define C4_ENUM(...)
C4_ENUM(aaa, bbb: ccc)
typedef enum {FOO, BAR} MyEnum_e;
)",
        /*hdr*/R"(#pragma once
#include "enum_pairs.h"

template<> const EnumPairs<MyEnum_e> enum_pairs();


)",
        /*src*/R"(#include "c4regen.c4gen.hpp"

template<> const EnumPairs<MyEnum_e> enum_pairs()
{
    static constexpr const EnumAndName<MyEnum_e> vals[] = {
                { FOO, "FOO"},
                { BAR, "BAR"},
        
    };
    /* has meta.aaa: 1 */
    /* meta.aaa is not 0 */
    /* meta.aaa is 1 */
    /* has meta.bbb: ccc */
    
    /* meta.bbb is "ccc" */
    EnumPairs<MyEnum_e> r(vals);
    return r;
}


)"
    });
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

constexpr const char basic_classes_cfg[] = R"(
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
)";


TEST(classes, empty_sources)
{
    test_regen_exec("classes.basic", basic_classes_cfg, {
        "empty",
        /*read*/R"()",
        /*hdr*/R"()",
        /*src*/R"()",
    });
}


TEST(classes, basic)
{
    test_regen_exec("classes.basic", basic_classes_cfg, {
        "two_classes",
        /*read*/R"(#define C4_CLASS(...)
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
)",
        /*hdr*/R"(// DO NOT EDIT!
// This was automatically generated with c4regen: https://github.com/biojppm/c4regen

#ifndef C4REGEN_C4GEN_HPP_GUARD_
#define C4REGEN_C4GEN_HPP_GUARD_





// generator: class_members
// entity: foo @ two_classes/c4regen.cpp:3
void show(foo const& obj);


// generator: class_members
// entity: bar @ two_classes/c4regen.cpp:12
void show(bar const& obj);




#include "c4regen.c4gen.def.hpp"

#endif // C4REGEN_C4GEN_HPP_GUARD_

+)",
        /*src*/R"(// DO NOT EDIT!
// This was automatically generated with c4regen: https://github.com/biojppm/c4regen

#ifndef C4REGEN_C4GEN_HPP_GUARD_
#include "c4regen.c4gen.hpp"
#endif

#ifndef C4REGEN_C4GEN_DEF_HPP_GUARD_
#include "c4regen.c4gen.def.hpp"
#endif

#include <iostream>




// generator: class_members
// entity: foo @ two_classes/c4regen.cpp:3
void show(foo const& obj);
{
        std::cout << "member: 'a' of type 'int': value=" << obj.a << "\n";
        std::cout << "member: 'b' of type 'int': value=" << obj.b << "\n";
        std::cout << "member: 'c' of type 'int': value=" << obj.c << "\n";
        std::cout << "member: 'x' of type 'float': value=" << obj.x << "\n";
        std::cout << "member: 'y' of type 'float': value=" << obj.y << "\n";
        std::cout << "member: 'z' of type 'float': value=" << obj.z << "\n";
        
        std::cout << "method: 'some_method(const foo &)' of type 'void (const foo &)'\n";
        
}


// generator: class_members
// entity: bar @ two_classes/c4regen.cpp:12
void show(bar const& obj);
{
        std::cout << "member: 'za' of type 'int': value=" << obj.za << "\n";
        std::cout << "member: 'zb' of type 'int': value=" << obj.zb << "\n";
        std::cout << "member: 'zc' of type 'int': value=" << obj.zc << "\n";
        std::cout << "member: 'zx' of type 'float': value=" << obj.zx << "\n";
        std::cout << "member: 'zy' of type 'float': value=" << obj.zy << "\n";
        std::cout << "member: 'zz' of type 'float': value=" << obj.zz << "\n";
        
        std::cout << "method: 'some_other_method(const bar &)' of type 'void (const bar &)'\n";
        
}


)",
    });
}

} // namespace ast
} // namespace c4
