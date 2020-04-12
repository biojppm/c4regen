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
    {FLAGS  , 0, "f", "flag" , c4::opt::nonempty, "  -f <compiler-flag>, --flag=<compiler-flag>  \tAdd a flag to pass to the compiler, generally --flag '-x' --flag 'c++' should be used." },
    {0,0,0,0,0,0}
};

inline bool valid_cmd(csubstr cmd)
{
    return cmd == "generate" || cmd == "outfiles";
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline int exec(regen::Regen *rg, int argc, const char *argv[], bool skip_exe_name=false)
{
    using namespace c4;

    if(skip_exe_name)
    {
        ++argc;
        ++argv;
    }

    auto opts = opt::make_parser(usage, argc, argv, HELP, {CFG});

    rg->load_config(opts(CFG));

    if(rg->empty()) return 0;

    csubstr cmd = to_csubstr(opts(CMD));
    C4_CHECK(valid_cmd(cmd));

    if(cmd == "generate")
    {
        if(opts[DIR])
        {
            rg->gencode(opts.posn_args(), opts[DIR].arg);
        }
        else
        {
            std::vector<const char*> flags;
            for(auto const& f : opts.opts(FLAGS))
            {
                flags.push_back(f.arg);
            }
            rg->gencode(opts.posn_args(), nullptr, flags.data(), flags.size());
        }
    }
    else if(cmd == "outfiles")
    {
        rg->print_output_filenames(opts.posn_args());
    }
    else
    {
        C4_ERROR("unknown command: .*%s", (int)cmd.len, cmd.str);
    }

    return 0;
}

inline int exec(int argc, const char *argv[], bool skip_exe_name=false)
{
    regen::Regen results;
    return exec(&results, argc, argv, skip_exe_name);
}

template <size_t N>
inline int exec(regen::Regen *results, const char *(&argv)[N], bool skip_exe_name=false)
{
    return exec(results, N, argv, skip_exe_name);
}
template <size_t N>
inline int exec(const char *(&argv)[N], bool skip_exe_name=false)
{
    regen::Regen results;
    return exec(&results, N, argv, skip_exe_name);
}

inline int exec(regen::Regen *results, std::initializer_list<const char*> il, bool skip_exe_name=false)
{
    int  argc = static_cast<int>(il.size());
    // [support.initlist] 18.9/1 specifies that for std::initializer_list<T>
    // iterator must be T*
    auto argv = const_cast<const char**>(il.begin());
    return exec(results, argc, argv, skip_exe_name);
}
inline int exec(std::initializer_list<const char*> il, bool skip_exe_name=false)
{
    regen::Regen results;
    return exec(&results, il, skip_exe_name);
}

} // namespace regen
} // namespace c4

#endif /* _c4_REGEN_EXEC_HPP_ */
