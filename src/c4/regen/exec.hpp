#ifndef _c4_REGEN_EXEC_HPP_
#define _c4_REGEN_EXEC_HPP_

#include <c4/regen/regen.hpp>
#include <c4/opt/opt.hpp>
#include <c4/log/log.hpp>
#include <c4/std/std.hpp>

namespace c4 {
namespace regen {


enum { UNKNOWN, HELP, CMD, CFG, DIR };
const option::Descriptor usage[] =
{
    {UNKNOWN, 0, "" , ""    , c4::opt::none    , "USAGE: regen generate [options] <source-file> [<more source-files>]\n\nOptions:" },
    {HELP,    0, "h", "help", c4::opt::none    , "  -h, --help  \tPrint usage and exit." },
    {CMD,     0, "x", "cmd" , c4::opt::required, "  -x <cmd>, --cmd=<cmd>  \t(required) The command to execute. Must be one of [generate,outfiles,showast]." },
    {CFG,     0, "c", "cfg" , c4::opt::required, "  -c <cfg-yml>, --cfg=<cfg-yml>  \t(required) The full path to the regen config YAML file." },
    {DIR,     0, "d", "dir" , c4::opt::required, "  -d <build-dir>, --dir=<build-dir>  \t(required) The full path to the directory containing the compile_commands.json file." },
    {0,0,0,0,0,0}
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int exec(int argc, const char *argv[])
{
    auto opts = c4::opt::make_parser(usage, argc, argv, HELP, {CFG, DIR});

    using namespace c4;

    regen::Regen rg(opts(CFG));
    if(rg.empty()) return 0;


    ast::CompilationDb db(opts(DIR));
    for(const char* source_file : opts.posn_args())
    {
        ast::Index idx;
        ast::TranslationUnit unit(idx, source_file, db);
        unit.visit_children([](ast::Cursor c, ast::Cursor parent, void* data){
            ast::Index &idx = *(ast::Index*)data;
            const char* type = c.type_spelling(idx);
            const char* cont = c.spelling(idx);
            ast::Location loc = c.location(idx);
            c4::_log("{}:{}: {}({}B): {}", loc.file, loc.line, loc.column, loc.offset, c.kind_spelling(idx));
            if(strlen(type)) c4::_log(": type='{}'", type);
            if(strlen(cont)) c4::_log(": cont='{}'", cont);
            c4::_print('\n');
            return CXChildVisit_Recurse;
        });
    }

    return 0;
}


} // namespace regen
} // namespace c4

#endif /* _c4_REGEN_EXEC_HPP_ */
