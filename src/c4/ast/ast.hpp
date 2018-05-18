#ifndef _C4_AST_HPP_
#define _C4_AST_HPP_

#include <vector>
#include <string>

#include <clang-c/CXCompilationDatabase.h>
#include <clang-c/Index.h>

#include <c4/error.hpp>
#include <c4/substr.hpp>
#include <c4/fs/fs.hpp>

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

namespace c4 {
namespace ast {

struct Cursor;
using visitor_pfn = CXChildVisitResult (*)(Cursor c, Cursor parent, void *data);


//-----------------------------------------------------------------------------

template <class T>
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

#ifndef NDEBUG
    virtual ~pimpl_handle()
    {
        C4_ASSERT(m_handle == nullptr);
    }
#endif
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct CompilationDb : pimpl_handle<CXCompilationDatabase>
{

    CompilationDb(const char* build_dir) : pimpl_handle<CXCompilationDatabase>()
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

    std::vector<const char*> const& get_cmd(const char* full_file_name) const
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

    thread_local static std::vector<const char*> s_cmd;
    thread_local static std::vector<std::string> s_cmd_buf;
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct Index : pimpl_handle<CXIndex>
{
    using pimpl_handle<CXIndex>::pimpl_handle;

    Index() : pimpl_handle(clang_createIndex(/*excludeDeclarationsFromPCH*/0, /*displayDiagnostics*/1))
    {
    }

    ~Index()
    {
        for(auto &s : m_strings)
        {
            clang_disposeString(s);
        }
        m_strings.clear();
        if(m_handle)
        {
            clang_disposeIndex(m_handle);
            m_handle = nullptr;
        }
    }

    std::vector<CXString> m_strings;
    const char* get_string(CXString s)
    {
        m_strings.push_back(s);
        return clang_getCString(s);
    }

    template< class StrGetterPfn, class ...Args >
    const char* to_str(StrGetterPfn fn, Args&&... args)
    {
        CXString cs = fn(std::forward<Args>(args)...);
        return get_string(cs);
    }

};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct LocData
{
    unsigned offset, line, column;
};

struct Location : public LocData
{
    const char* file;

    Location(){}
    Location(Index &idx, CXCursor c)
    {
        CXSourceLocation loc = clang_getCursorLocation(c);
        CXFile f;
        clang_getExpansionLocation(loc, &f, &line, &column, &offset);
        file = idx.to_str(&clang_getFileName, f);
    }
};

struct Region
{
    const char* m_file;
    LocData     m_start;
    LocData     m_end;

    Region(){}
    Region(Index &idx, CXCursor c)
    {
        CXSourceRange  ext = clang_getCursorExtent(c);
        CXSourceLocation s = clang_getRangeStart(ext);
        CXSourceLocation e = clang_getRangeEnd(ext);
        CXFile f;
        clang_getExpansionLocation(s, &f, &m_start.line, &m_start.column, &m_start.offset);
        clang_getExpansionLocation(e, nullptr, &m_end.line, &m_end.column, &m_end.offset);
        m_file = idx.to_str(&clang_getFileName, f);
    }
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct Cursor : public CXCursor
{
    using CXCursor::CXCursor;

    inline Cursor(CXCursor c) : CXCursor(c) {}

    Location location(Index &idx) const { return Location(idx, *this); }
    Region region(Index &idx) const { return Region(idx, *this); }

    Cursor semantic_parent() const { return Cursor(clang_getCursorSemanticParent(*this)); }
    Cursor lexical_parent() const { return Cursor(clang_getCursorLexicalParent(*this)); }

    CXCursorKind kind() const { return clang_getCursorKind(*this); }
    CXType type() const { return clang_getCursorType(*this); }
    CXType canonical_type() const { return clang_getCanonicalType(type()); }
    CXType result_type() const { return clang_getCursorResultType(*this); }
    CXType named_type() const { return clang_Type_getNamedType(type()); }

    // these utility functions are expensive because of the allocations.
    // They should be called once and the results should be stored.
    const char*      spelling(Index &idx) const { return idx.to_str(&clang_getCursorSpelling, *this); }
    const char* type_spelling(Index &idx) const { return idx.to_str(&clang_getTypeSpelling, type()); }
    const char* kind_spelling(Index &idx) const { return idx.to_str(&clang_getCursorKindSpelling, kind()); }
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/*
struct Token : public CXToken
{
    using CXToken::CXToken;

    Token(CXToken t) : CXToken(t) {}

    CXTokenKind kind() const { return clang_getTokenKind(*this); }

    // these utility functions are expensive because of the allocations.
    // They should be called once and the results should be stored.
    inline std::string spelling() const { return to_str(&clang_getTokenSpelling, *this); }
    inline const char* spelling_c() const { return to_c_str(&clang_getTokenSpelling, *this); }
};

struct CursorTokens
{
    Cursor const* m_cursor{nullptr};
    CXToken *m_tokens{nullptr};
    unsigned int m_numtokens{0};

    CursorTokens(Cursor const& cursor) : m_cursor(&cursor)
    {
        CXSourceRange range = clang_getCursorExtent(cursor);
        auto tu = clang_Cursor_getTranslationUnit(cursor);
        clang_tokenize(tu, range, &m_tokens, &m_numtokens);
    }

    ~CursorTokens()
    {
        if(m_tokens)
        {
            auto tu = clang_Cursor_getTranslationUnit(*m_cursor);
            clang_disposeTokens(tu, m_tokens, m_numtokens);
        }
    }
};
*/


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

constexpr const unsigned default_options = CXTranslationUnit_DetailedPreprocessingRecord;

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

    TranslationUnit(Index &idx, csubstr src, const char * const* cmds, size_t cmds_sz, unsigned options=default_options, bool delete_tmp=true)
    {
        auto tmp = c4::fs::ScopedTmpFile(src.str, src.len, "c4trunittmp.XXXXXXXX.cpp");
        tmp.do_delete(delete_tmp);
        this->_parse2(idx, tmp.m_name, cmds, cmds_sz, options);
    }

    TranslationUnit(Index &idx, const char *filename, const char * const* cmds, size_t cmds_sz, unsigned options=default_options)
    {
        this->_parse_argv(idx, filename, cmds, cmds_sz, options);
    }

    TranslationUnit(Index &idx, const char *filename, CompilationDb const& db, unsigned options=default_options)
    {
        auto const& cmd = db.get_cmd(filename);
        C4_CHECK(cmd.size() > 1);
        this->_parse_argv(idx, nullptr, cmd.data(), cmd.size(), options);
    }

private:

    /** @param filename nullptr informs that the filename is in the args */
    void _parse_argv(Index &idx, const char *filename, const char * const* cmds, size_t cmds_sz, unsigned options)
    {
        C4_ASSERT(fs::path_exists(filename));
        CXErrorCode err = clang_parseTranslationUnit2FullArgv(idx,
                                    filename, //nullptr informs that the filename is in the args
                                    cmds, (unsigned)cmds_sz,
                                    nullptr, 0,
                                    options,
                                    &m_handle);
        check_err(err, m_handle);
    }

    void _parse2(Index &idx, const char *filename, const char * const* cmds, size_t cmds_sz, unsigned options)
    {
        C4_ASSERT(fs::path_exists(filename));
        CXErrorCode err = clang_parseTranslationUnit2(idx,
                                    filename, //nullptr informs that the filename is in the args
                                    cmds, (unsigned)cmds_sz,
                                    nullptr, 0,
                                    options,
                                    &m_handle);
        check_err(err, m_handle);
    }

public:

    static void check_err(CXErrorCode err, CXTranslationUnit translation_unit)
    {
        switch(err)
        {
        case CXError_Failure:          C4_ERROR("A generic error code, no further details are available."); break;
        case CXError_Crashed:          C4_ERROR("libclang crashed while performing the requested operation."); break;
        case CXError_InvalidArguments: C4_ERROR("function detected that the arguments violate the function contract."); break;
        case CXError_ASTReadError:     C4_ERROR("An AST deserialization error has occurred."); break;
        case CXError_Success:
        default:
            break;
        }
        C4_CHECK(translation_unit != nullptr);
    }

public:

    Cursor root() const
    {
        return clang_getTranslationUnitCursor(m_handle);
    }

    struct _visitor_data
    {
        visitor_pfn visitor;
        void *data;
        bool same_unit_only;
    };

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
