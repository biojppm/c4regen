#ifndef _c4_REGEN_EXEC_HPP_
#define _c4_REGEN_EXEC_HPP_

#include <c4/regen/regen.hpp>
#include <c4/opt/opt.hpp>
#include <c4/log/log.hpp>
#include <c4/std/std.hpp>

namespace c4 {
namespace regen {


enum { UNKNOWN, HELP, CMD, CFG, DIR, FLAGS };
const option::Descriptor usage[] =
{
    {UNKNOWN, 0, "" , ""     , c4::opt::none    , "USAGE: regen generate [options] <source-file> [<more source-files>]\n\nOptions:" },
    {HELP   , 0, "h", "help" , c4::opt::none    , "  -h, --help  \tPrint usage and exit." },
    {CMD    , 0, "x", "cmd"  , c4::opt::required, "  -x <cmd>, --cmd=<cmd>  \t(required) The command to execute. Must be one of [generate,outfiles]." },
    {CFG    , 0, "c", "cfg"  , c4::opt::required, "  -c <cfg-yml>, --cfg=<cfg-yml>  \t(required) The full path to the regen config YAML file." },
    {DIR    , 0, "d", "dir"  , c4::opt::nonempty, "  -d <build-dir>, --dir=<build-dir>  \t(required) The full path to the directory containing the compile_commands.json file." },
    {FLAGS  , 0, "f", "flag" , c4::opt::nonempty, "  -f <compiler-flag>, --flag=<compiler-flag>  \t(required) The full path to the directory containing the compile_commands.json file." },
    {0,0,0,0,0,0}
};

bool valid_cmd(csubstr cmd)
{
    return cmd == "generate" || cmd == "outfiles";
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int exec(int argc, const char *argv[])
{
    using namespace c4;

    auto opts = opt::make_parser(usage, argc, argv, HELP, {CFG});
    regen::Regen rg(opts(CFG));
    if(rg.empty()) return 0;
    const char* dir = opts[DIR].arg;
    ast::CompilationDb db(dir);
    csubstr cmd = to_csubstr(opts(CMD));
    C4_CHECK(valid_cmd(cmd));

    yml::Tree workspace;
    yml::NodeRef wsroot = workspace.rootref();
    regen::SourceFile sf;
    regen::Writer::set_type outfiles;
    for(const char* source_file : opts.posn_args())
    {
        ast::Index idx;
        ast::TranslationUnit unit(idx, source_file, db);
        sf.clear();
        sf.init(idx, unit);
        if(cmd == "generate")
        {
            rg.extract(&sf);
            rg.gencode(&sf, wsroot);
            rg.outcode(&sf);
        }
        else if(cmd == "outfiles")
        {
            rg.extract(&sf);
            rg.print_output_file_names(sf, &outfiles);
        }
    }

    return 0;
}


} // namespace regen
} // namespace c4

#endif /* _c4_REGEN_EXEC_HPP_ */
