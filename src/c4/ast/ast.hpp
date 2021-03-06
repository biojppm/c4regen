#ifndef _C4_AST_HPP_
#define _C4_AST_HPP_

#include <vector>
#include <string>

#include <clang-c/CXCompilationDatabase.h>
#include <clang-c/Index.h>

#include <c4/error.hpp>
#include <c4/substr.hpp>
#include <c4/fs/fs.hpp>
#include <c4/c4_push.hpp>
#include <c4/std/vector.hpp>

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

namespace c4 {
namespace ast {

inline bool is_integral_unsigned(CXTypeKind t)
{
    return t == CXType_Char_U ||
           t == CXType_UChar  ||
           t == CXType_Char16 ||
           t == CXType_Char32 ||
           t == CXType_UShort ||
           t == CXType_UInt   ||
           t == CXType_ULong  ||
           t == CXType_ULongLong ||
           t == CXType_UInt128;
}

inline bool is_integral_signed(CXTypeKind t)
{
    return t == CXType_Char_S ||
           t == CXType_SChar ||
           t == CXType_WChar ||
           t == CXType_Short ||
           t == CXType_Int ||
           t == CXType_Long ||
           t == CXType_LongLong ||
           t == CXType_Int128;
}


//-----------------------------------------------------------------------------

struct Cursor;
using visitor_pfn = CXChildVisitResult (*)(Cursor c, Cursor parent, void *data);
void visit_children(Cursor root, visitor_pfn visitor, void *data=nullptr, bool same_unit_only=true);


//-----------------------------------------------------------------------------

template<class T>
struct pimpl_handle
{
    T m_handle;

    inline operator       T ()       { return m_handle; }
    inline operator const T () const { return m_handle; }

    T       operator-> ()      { return m_handle; }
    const T operator-> () const{ return m_handle; }

public:

    pimpl_handle() : m_handle(nullptr) {}

    pimpl_handle(T h) : m_handle(h) {}
    pimpl_handle& operator= (T h) { C4_ASSERT(m_handle == nullptr); m_handle = (h); }

protected:

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
        if(build_dir == nullptr) return;
        CXCompilationDatabase_Error err;
        m_handle = clang_CompilationDatabase_fromDirectory(build_dir, &err);
        C4_CHECK_MSG(err == CXCompilationDatabase_NoError,
                     "error constructing compilation database");
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
        C4_CHECK_MSG(m_handle != nullptr, "no compilation database");
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

struct StringCollection
{

    //! The returned csubstr is zero-terminated!
    const char* store(CXString s);

    // use pages to ensure that no string is relocated
    std::vector<csubstr> m_strings;
    std::vector<std::vector<char>> m_pages;
    constexpr static const size_t default_page_size = 1024u;

    C4_NO_COPY_CTOR(StringCollection);
    C4_NO_COPY_ASSIGN(StringCollection);

    StringCollection() : m_strings(), m_pages() {}
    StringCollection(StringCollection &&that) = default; //{ _move(&that); }
    StringCollection& operator= (StringCollection &&that) = default;/*{ _move(&that); return *this; }
    void _move(StringCollection *that)
    {
        print("before: tstr?", that->m_strings.empty());
        print("before: tbuf?", that->m_pages.empty());
        m_strings = std::move(that->m_strings);
        m_pages = std::move(that->m_pages);
        print("after : tstr?", that->m_strings.empty());
        print("after : tbuf?", that->m_pages.empty());
    }*/
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct Index : pimpl_handle<CXIndex>
{
    using pimpl_handle<CXIndex>::pimpl_handle;

    Index() : pimpl_handle(_s_create()), m_strings()
    {
    }

    ~Index()
    {
        _clear();
    }

    void clear()
    {
        _clear();
        m_handle = _s_create();
    }

private:

    static CXIndex _s_create()
    {
        return clang_createIndex(/*excludeDeclarationsFromPCH*/0, /*displayDiagnostics*/1);
    }

    void _clear()
    {
        if(m_handle)
        {
            clang_disposeIndex(m_handle);
            m_handle = nullptr;
        }
    }

public:

    /** This provides a facility for holding strings in the Index. Clients
     * should call this once and store the result, to minimize calls to pairs
     * of clang_*getString()/clang_disposeString(). Attention should be paid
     * so that the lifetime of the Index exceeds the lifetime of clients.
     *
     * @todo avoid string duplication by using a set or hash set
     */
    const char* store_str(CXString s)
    {
        return m_strings.store(s);
    }

    /** move out the string collection for later use */
    StringCollection&& yield_strings()
    {
        return std::move(m_strings);
    }

    StringCollection m_strings;
};

inline void print_str(CXString cxs, bool skip_empty=false, const char *fmt="%s")
{
    const char *s = clang_getCString(cxs);
    if(s && (strlen(s) || ! skip_empty))
    {
        printf(fmt, s);
    }
    clang_disposeString(cxs);
}


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
        file = idx.store_str(clang_getFileName(f));
    }
};

struct Region
{
    const char* m_file;
    LocData     m_start;
    LocData     m_end;

    Region(){}
    Region(Index &idx, CXCursor c) { init_region(idx, c); }

    void init_region(Index &idx, CXCursor c)
    {
        CXSourceRange  ext = clang_getCursorExtent(c);
        CXSourceLocation s = clang_getRangeStart(ext);
        CXSourceLocation e = clang_getRangeEnd(ext);
        CXFile f;
        clang_getExpansionLocation(s, &f, &m_start.line, &m_start.column, &m_start.offset);
        clang_getExpansionLocation(e, nullptr, &m_end.line, &m_end.column, &m_end.offset);
        m_file = idx.store_str(clang_getFileName(f));
    }

    csubstr get_str(csubstr file_contents) const
    {
        return file_contents.range(m_start.offset, m_end.offset);
    }

    bool operator< (Region c$$ that) const
    {
        if(this == &that) return false;

        if(m_start.offset < that.m_start.offset) return true;
        else if(m_start.offset > that.m_start.offset) return false;

        if(m_end.offset < that.m_end.offset) return true;

        return false;
    }
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct Cursor : public CXCursor
{
    inline Cursor() : CXCursor() {}
    inline Cursor(CXCursor c) : CXCursor(c) {}

    Cursor root() const { CXTranslationUnit unit = clang_Cursor_getTranslationUnit(*this); return clang_getTranslationUnitCursor(unit); }
    Cursor semantic_parent() const { return Cursor(clang_getCursorSemanticParent(*this)); }
    Cursor lexical_parent() const { return Cursor(clang_getCursorLexicalParent(*this)); }

    Cursor first_child() const;
    Cursor next_sibling() const;

    Location location(Index &idx) const { return Location(idx, *this); }
    Region region(Index &idx) const { return Region(idx, *this); }

    inline operator bool() const { return ! is_null(); }
    bool is_null() const { return clang_equalCursors(*this, clang_getNullCursor()); }
    bool is_same(const Cursor c) const { return clang_hashCursor(*this) == clang_hashCursor(c); }

    unsigned hash() const { return clang_hashCursor(*this); }
    CXCursorKind kind() const { return clang_getCursorKind(*this); }
    CXType type() const { return clang_getCursorType(*this); }
    CXType canonical_type() const { return clang_getCanonicalType(type()); }
    CXType result_type() const { return clang_getCursorResultType(*this); }
    CXType named_type() const { return clang_Type_getNamedType(type()); }

    bool is_declaration()   const { return clang_isDeclaration(kind())   != 0; }
    bool is_reference()     const { return clang_isReference(kind())     != 0; }
    bool is_expression()    const { return clang_isExpression(kind())    != 0; }
    bool is_statement()     const { return clang_isStatement(kind())     != 0; }
    bool is_preprocessing() const { return clang_isPreprocessing(kind()) != 0; }
    bool is_attribute()     const { return clang_isAttribute(kind())     != 0; }
    bool has_attrs()        const { return clang_Cursor_hasAttrs(*this)  != 0; }

    // template info
    bool is_tpl() const { CXCursorKind k = kind(); return k == CXCursor_FunctionTemplate || k == CXCursor_ClassTemplate || k == CXCursor_ClassTemplatePartialSpecialization; }
    bool is_tpl_class() const { return kind() == CXCursor_ClassTemplate; }
    bool is_tpl_function() const { return kind() == CXCursor_FunctionTemplate; }
    bool is_tpl_specialization() const { return kind() == CXCursor_ClassTemplatePartialSpecialization; }
    unsigned num_tpl_args() const { return (unsigned)clang_Cursor_getNumTemplateArguments(*this); }
    enum CXTemplateArgumentKind tpl_arg_kind(int i) const { return clang_Cursor_getTemplateArgumentKind(*this, i); }
    CXType tpl_arg_type(unsigned i) const { return clang_Cursor_getTemplateArgumentType(*this, i); }
    long long tpl_arg_ival(unsigned i) const { return clang_Cursor_getTemplateArgumentValue(*this, i); }
    unsigned long long tpl_arg_uval(unsigned i) const { return clang_Cursor_getTemplateArgumentUnsignedValue(*this, i); }

    // these utility functions are expensive because of the allocations.
    // They should be called once and the results should be stored.
    const char*  display_name(Index &idx) const { return idx.store_str(clang_getCursorDisplayName(*this)); }
    const char*      spelling(Index &idx) const { return idx.store_str(clang_getCursorSpelling(*this)); }
    const char* type_spelling(Index &idx) const { return idx.store_str(clang_getTypeSpelling(type())); }
    const char* kind_spelling(Index &idx) const { return idx.store_str(clang_getCursorKindSpelling(kind())); }
    const char*   raw_comment(Index &idx) const { return idx.store_str(clang_Cursor_getRawCommentText(*this)); }
    const char* brief_comment(Index &idx) const { return idx.store_str(clang_Cursor_getBriefCommentText(*this)); }

    void print_recursive(const char* msg=nullptr, unsigned indent=0) const;
    void print(const char* msg=nullptr, unsigned indent=0) const;

    Cursor tag_subject() const;
    csubstr tag_annotations(csubstr file_contents) const;
};



void visit_children(Cursor root, visitor_pfn visitor, void *data, bool same_unit_only);


//-----------------------------------------------------------------------------

struct CursorMatcher
{
    CXCursorKind kind;
    csubstr name;

    bool operator() (Index &C4_RESTRICT idx, Cursor c) const
    {
        if(c.kind() != kind && kind != 0)
        {
            return false;
        }
        if(name.not_empty() && name.compare(to_csubstr(c.display_name(idx))))
        {
            return false;
        }
        return true;
    }

};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct Token : public CXToken
{
    Token() : CXToken() {}
    Token(CXToken t) : CXToken(t) {}

    CXTokenKind kind() const { return clang_getTokenKind(*this); }

    // these utility functions are expensive because of the allocations.
    // They should be called once and the results should be stored.
    const char* spelling(Index &idx, CXTranslationUnit const& tu) const { return idx.store_str(clang_getTokenSpelling(tu, *this)); }
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


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
struct TranslationUnit;
struct Entity;

size_t select(TranslationUnit &tu, Cursor root, CursorMatcher m, std::vector<ast::Entity> *out, bool same_unit_only);
size_t select(Index &idx         , Cursor root, CursorMatcher m, std::vector<ast::Entity> *out, bool same_unit_only);


constexpr const unsigned default_options = CXTranslationUnit_DetailedPreprocessingRecord;

struct TranslationUnit : pimpl_handle<CXTranslationUnit>
{
    Index *m_index;
    std::vector<char> m_contents;

public:

    ~TranslationUnit()
    {
        clear();
    }

    // inherit base ctors
    TranslationUnit() : pimpl_handle<CXTranslationUnit>() {}
    using pimpl_handle<CXTranslationUnit>::pimpl_handle;

    TranslationUnit(Index &idx, csubstr src, const char * const* cmds, size_t cmds_sz, unsigned options=default_options, bool delete_tmp=true)
        :
          pimpl_handle<CXTranslationUnit>()
    {
        reset(idx, src, cmds, cmds_sz, options, delete_tmp);
    }

    TranslationUnit(Index &idx, const char *filename, const char * const* cmds, size_t cmds_sz, unsigned options=default_options)
        :
          pimpl_handle<CXTranslationUnit>()
    {
        reset(idx, filename, cmds, cmds_sz, options);
    }

    TranslationUnit(Index &idx, const char *filename, CompilationDb const& db, unsigned options=default_options)
        :
          pimpl_handle<CXTranslationUnit>()
    {
        reset(idx, filename, db, options);
    }

public:

    void clear()
    {
        m_contents.clear();
        if(m_handle)
        {
            clang_disposeTranslationUnit(m_handle);
            m_handle = nullptr;
        }

    }

    void reset(Index &idx, csubstr src, const char * const* cmds, size_t cmds_sz, unsigned options=default_options, bool delete_tmp=true)
    {
        clear();
        m_index = &idx;
        m_contents.assign(src.begin(), src.end());
        auto tmp = c4::fs::ScopedTmpFile(src.str, src.len, "c4regen.tmp-XXXXXXXX.cpp");
        tmp.do_delete(delete_tmp);
        this->_parse2(idx, tmp.m_name, cmds, cmds_sz, options);
    }

    void reset(Index &idx, const char *filename, const char * const* cmds, size_t cmds_sz, unsigned options=default_options)
    {
        clear();
        m_index = &idx;
        c4::fs::file_get_contents(filename, &m_contents);
        this->_parse_argv(idx, filename, cmds, cmds_sz, options);
    }

    void reset(Index &idx, const char *filename, CompilationDb const& db, unsigned options=default_options)
    {
        clear();
        m_index = &idx;
        c4::fs::file_get_contents(filename, &m_contents);
        auto c$$ cmd = db.get_cmd(filename);
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

    void visit_children(visitor_pfn visitor, void *data=nullptr, bool same_unit_only=true) const
    {
        c4::ast::visit_children(root(), visitor, data, same_unit_only);
    }

public:

    size_t select(CursorMatcher m, std::vector<Entity> *v, bool same_unit_only=true) const
    {
        return c4::ast::select(*m_index, root(), m, v, same_unit_only);
    }

    size_t select_tagged(csubstr tag, std::vector<Entity> *v, bool same_unit_only=true) const
    {
        CursorMatcher matcher;
        matcher.kind = CXCursor_MacroExpansion;
        matcher.name = tag;
        return select(matcher, v, same_unit_only);
    }
};


//-----------------------------------------------------------------------------

struct Entity
{
    Cursor  cursor;
    Cursor  parent;
    TranslationUnit c$ tu;
    Index $ idx;
};
using EntityRef = Entity const& C4_RESTRICT;

//-----------------------------------------------------------------------------

namespace detail {

struct SelectData
{
    TranslationUnit *C4_RESTRICT tu;
    Index *C4_RESTRICT idx;
    CursorMatcher *C4_RESTRICT matcher;
    std::vector<ast::Entity> *C4_RESTRICT out;
};

inline CXChildVisitResult _select_impl(Cursor c, Cursor parent, void *data_)
{
    auto *C4_RESTRICT data = reinterpret_cast<SelectData *C4_RESTRICT>(data_);
    if((*data->matcher)(*data->idx, c))
    {
        data->out->emplace_back();
        auto &C4_RESTRICT b = data->out->back();
        b.cursor = c;
        b.parent = parent;
        b.tu = data->tu;
        b.idx = data->idx;
    }
    return CXChildVisit_Recurse;
}

} // namespace detail

inline size_t select(TranslationUnit &tu, Cursor root, CursorMatcher m, std::vector<ast::Entity> *out, bool same_unit_only)
{
    detail::SelectData data{&tu, tu.m_index, &m, out};
    size_t sz = out->size();
    visit_children(root, &detail::_select_impl, &data, same_unit_only);
    return out->size() - sz;
}

inline size_t select(Index &idx, Cursor root, CursorMatcher m, std::vector<ast::Entity> *out, bool same_unit_only)
{
    detail::SelectData data{nullptr, &idx, &m, out};
    size_t sz = out->size();
    visit_children(root, &detail::_select_impl, &data, same_unit_only);
    return out->size() - sz;
}


} // namespace ast
} // namespace c4

#include <c4/c4_pop.hpp>

#endif // _C4_AST_HPP_
