#include <c4/regen/regen.hpp>
#include <c4/opt/opt.hpp>
#include <c4/log/log.hpp>
#include <c4/std/std.hpp>


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

enum { UNKNOWN, HELP, CFG, DIR };
const option::Descriptor usage[] =
{
    {UNKNOWN, 0, "" , ""    , c4::opt::none     , "USAGE: regen [options] <source-file> [<more source-files>]\n\nOptions:" },
    {HELP,    0, "h", "help", c4::opt::none     , "  -h, --help  \tPrint usage and exit." },
    {CFG,     0, "c", "cfg" , c4::opt::required, "  -c <cfg-yml>, --cfg=<cfg-yml>  \tThe full path to the regen config YAML file (required)." },
    {DIR,     0, "d", "dir" , c4::opt::required, "  -d <build-dir>, --dir=<build-dir>  \tThe full path to the directory containing the compile_commands.json file (required)." },
    {0,0,0,0,0,0}
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

int main(int argc, const char* argv[])
{
    auto opts = c4::opt::make_parser(usage, argc, argv, HELP, {CFG, DIR});

    using namespace c4::ast;
    using namespace c4::regen;

    Regen rg(opts(CFG));
    CompilationDb db(opts(DIR));
    for(const char* source_file : opts.posn_args())
    {
        Index idx(clang_createIndex(0, 0));
        TranslationUnit unit(idx, source_file, db);
        unit.visit_children([](Cursor c, Cursor parent, void*){
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

    return 0;
}
