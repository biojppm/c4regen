#ifndef _C4_AST_HPP_
#define _C4_AST_HPP_

#include <vector>
#include <string>

#include <clang-c/CXCompilationDatabase.h>
#include <clang-c/Index.h>

#include "c4/error.hpp"

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

namespace c4 {
namespace ast {

template< class T >
struct pimpl_handle
{
    T m_handle;

    inline operator       T ()       { return m_handle; }
    inline operator const T () const { return m_handle; }

    T       operator-> ()      { return m_handle; }
    const T operator-> () const{ return m_handle; }

    pimpl_handle(T h) : m_handle(h) {}
    pimpl_handle& operator= (T h) { C4_ASSERT(m_handle == nullptr); m_handle = (h); }

protected:

    pimpl_handle() : m_handle(nullptr) {}

    pimpl_handle(pimpl_handle const&) = delete;
    pimpl_handle(pimpl_handle && that) { this->__move(&that); }

    pimpl_handle& operator= (pimpl_handle const&) = delete;
    pimpl_handle& operator= (pimpl_handle && that) { this->__move(&that); return *this; }

    void __move(pimpl_handle *that)
    {
        C4_ASSERT(m_handle == nullptr);
        m_handle = that->m_handle;
        that->m_handle = nullptr;
    }

    ~pimpl_handle()
    {
        C4_ASSERT(m_handle == nullptr);
    }
};



//-----------------------------------------------------------------------------+
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct CompilationDb : pimpl_handle< CXCompilationDatabase >
{

    CompilationDb(const char* build_dir) : pimpl_handle< CXCompilationDatabase >()
    {
        CXCompilationDatabase_Error err;
        m_handle = clang_CompilationDatabase_fromDirectory(build_dir, &err);
        C4_CHECK_MSG(err == CXCompilationDatabase_NoError, "error constructing compilation database");
    }

    ~CompilationDb()
    {
        if(m_handle)
        {
            clang_CompilationDatabase_dispose(m_handle);
            m_handle = nullptr;
        }
    }

    std::vector< const char* > const& get_cmd(const char* full_file_name) const
    {
        CXCompileCommands cmds = clang_CompilationDatabase_getCompileCommands(m_handle, full_file_name);
        unsigned sz = clang_CompileCommands_getSize(cmds);
        C4_CHECK_MSG(sz > 0, "no compilation commands found for file %s. Is it a full path?", full_file_name);
        for(unsigned ic = 0; ic < sz; ++ic)
        {
            CXCompileCommand cmd = clang_CompileCommands_getCommand(cmds, ic);
            unsigned nargs = clang_CompileCommand_getNumArgs(cmd);
            if(nargs > (unsigned)s_cmd_buf.size())
            {
                s_cmd_buf.resize(nargs);
            }
            s_cmd.resize(nargs);
            for(unsigned i = 0; i < nargs; ++i)
            {
                CXString arg = clang_CompileCommand_getArg(cmd, i);
                s_cmd_buf[i].assign(clang_getCString(arg));
                s_cmd[i] = s_cmd_buf[i].c_str();
            }
            // there may be several compilations of the same source file.
            // just return the first
            ////if(ic == 0) break;
        }
        clang_CompileCommands_dispose(cmds);
        return s_cmd;
    }

    thread_local static std::vector< const char* > s_cmd;
    thread_local static std::vector< std::string > s_cmd_buf;
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct Index : pimpl_handle< CXIndex >
{
    using pimpl_handle< CXIndex >::pimpl_handle;

    ~Index()
    {
        if(m_handle)
        {
            clang_disposeIndex(m_handle);
            m_handle = nullptr;
        }
    }

};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct Cursor : public CXCursor
{
    using CXCursor::CXCursor;

    inline Cursor(CXCursor c) : CXCursor(c) {}

    // these utility functions are expensive because of the allocations.
    // They should be called once and the results should be stored.
    inline std::string spelling() const { return to_str(&clang_getCursorSpelling, *this); }
    inline CXCursorKind kind() const { return clang_getCursorKind(*this); }
    inline std::string kind_spelling() const { return to_str(&clang_getCursorKindSpelling, kind()); }

    template< class StrGetterPfn, class ...Args >
    static std::string to_str(StrGetterPfn fn, Args&&... args)
    {
        std::string s;
        CXString cs = fn(std::forward< Args >(args)...);
        s.assign(clang_getCString(cs));
        clang_disposeString(cs);
        return s;
    }
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct TranslationUnit : pimpl_handle< CXTranslationUnit >
{
    using pimpl_handle< CXTranslationUnit >::pimpl_handle;

    ~TranslationUnit()
    {
        if(m_handle)
        {
            clang_disposeTranslationUnit(m_handle);
            m_handle = nullptr;
        }
    }

    TranslationUnit(Index &idx, CompilationDb const& db, const char *filename, unsigned options=CXTranslationUnit_DetailedPreprocessingRecord)
    {
        auto const& cmd = db.get_cmd(filename);
        C4_CHECK(cmd.size() > 1);
        auto err = clang_parseTranslationUnit2FullArgv(idx,
                                    nullptr, //nullptr informs that the filename is in the args
                                    cmd.data(), (unsigned)cmd.size(),
                                    nullptr, 0,
                                    options,
                                    &m_handle);
        switch(err)
        {
        case CXError_Failure: C4_ERROR("A generic error code, no further details are available."); break;
        case CXError_Crashed: C4_ERROR("libclang crashed while performing the requested operation."); break;
        case CXError_InvalidArguments: C4_ERROR("function detected that the arguments violate the function contract."); break;
        case CXError_ASTReadError: C4_ERROR("An AST deserialization error has occurred."); break;
        case CXError_Success:
        default:
            break;
        }
        C4_CHECK(m_handle != nullptr);
    }

    Cursor root() const
    {
        return clang_getTranslationUnitCursor(m_handle);
    }

public:

    using visitor_pfn = CXChildVisitResult (*)(Cursor c, Cursor parent, void *data);

    struct _visitor_data
    {
        visitor_pfn visitor;
        void *data;
        bool same_unit_only;
    };

    void visit_children(visitor_pfn visitor, void *data=nullptr, bool same_unit_only=true)
    {
        _visitor_data this_{visitor, data, same_unit_only};
        clang_visitChildren(root(), s_visit, &this_);
    }

    void visit_children(visitor_pfn visitor, void *data=nullptr, bool same_unit_only=true) const
    {
        _visitor_data this_{visitor, data, same_unit_only};
        clang_visitChildren(root(), s_visit, &this_);
    }

    static CXChildVisitResult s_visit(CXCursor cursor, CXCursor parent, CXClientData data)
    {
        _visitor_data * this_ = reinterpret_cast< _visitor_data* >(data);
        if(this_->same_unit_only && ! clang_Location_isFromMainFile(clang_getCursorLocation(cursor)))
        {
            return CXChildVisit_Continue;
        }
        return this_->visitor(Cursor(cursor), Cursor(parent), this_->data);
    }

};


} // namespace ast
} // namespace c4


#endif // _C4_AST_HPP_
