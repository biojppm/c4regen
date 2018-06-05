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
    {DIR    , 0, "d", "dir"  , c4::opt::nonempty, "  -d <build-dir>, --dir=<build-dir>  \tThe full path to the directory containing the compile_commands.json file." },
    {FLAGS  , 0, "f", "flag" , c4::opt::nonempty, "  -f <compiler-flag>, --flag=<compiler-flag>  \tThe full path to the directory containing the compile_commands.json file." },
    {0,0,0,0,0,0}
};

inline bool valid_cmd(csubstr cmd)
{
    return cmd == "generate" || cmd == "outfiles";
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline int exec(int argc, const char *argv[], bool with_exe_name=false)
{
    using namespace c4;

    if(with_exe_name)
    {
        ++argc;
        ++argv;
    }

    auto opts = opt::make_parser(usage, argc, argv, HELP, {CFG});
    regen::Regen rg(opts(CFG));
    if(rg.empty()) return 0;

    csubstr cmd = to_csubstr(opts(CMD));
    C4_CHECK(valid_cmd(cmd));

    if(cmd == "generate")
    {
        if(opts[DIR])
        {
            rg.gencode(opts.posn_args(), opts[DIR].arg);
        }
        else
        {
            std::vector<const char*> flags;
            for(auto const& f : opts.opts(FLAGS))
            {
                flags.push_back(f.arg);
            }
            rg.gencode(opts.posn_args(), nullptr, flags.data(), flags.size());
        }
    }
    else if(cmd == "outfiles")
    {
        rg.print_output_filenames(opts.posn_args());
    }

    return 0;
}

template <size_t N>
inline int exec(const char *(&argv)[N], bool with_exe_name=false)
{
    return exec(N, argv, with_exe_name);
}

inline int exec(std::initializer_list<const char*> il, bool with_exe_name=false)
{
    // [support.initlist] 18.9/1 specifies that for std::initializer_list<T>
    // iterator must be T*
    auto argv = const_cast<const char**>(il.begin());
    int  argc = static_cast<int>(il.size());
    return exec(argc, argv, with_exe_name);
}

} // namespace regen
} // namespace c4

#endif /* _c4_REGEN_EXEC_HPP_ */
