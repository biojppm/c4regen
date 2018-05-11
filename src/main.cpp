#include <c4/regen/regen.hpp>
#include <c4/opt/opt.hpp>

#include <iostream>


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
    auto opts = c4::opt::make_parser(usage, argc, argv);
    if(opts[HELP])
    {
        opts.help();
        return 0;
    }
    opts.check_mandatory({CFG, DIR});

    using namespace c4::ast;
    using namespace c4::regen;

    Regen rg(opts(CFG));
    CompilationDb db(opts(DIR));
    for(const char* source_file : opts.posn_args())
    {
        Index idx(clang_createIndex(0, 0));
        TranslationUnit unit(idx, source_file, db);
        unit.visit_children([](Cursor c, Cursor parent, void*){
            std::string kind = c.kind_spelling();
            std::string cont = c.spelling();
            std::string type = c.type_spelling();
            Location loc = c.location();
            std::cout << loc.file << ":" << loc.line << ": " << loc.col << "(" << loc.offset << "B)";
            std::cout << ": " << kind;
            if( ! type.empty()) std::cout << ": type='" << type << "'";
            if( ! cont.empty()) std::cout << ": cont='" << cont << "'";
            std::cout << "\n";
            return CXChildVisit_Recurse;
        });
    }

    return 0;
}
