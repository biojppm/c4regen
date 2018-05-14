#ifndef _c4_FS_HPP_
#define _c4_FS_HPP_

#include <c4/platform.hpp>
#include <c4/error.hpp>
#include <stdio.h>

#ifdef C4_POSIX
struct dirent;
#endif

namespace c4 {
namespace fs {


typedef enum {
    FILE,
    DIR,
    SYMLINK,
    PIPE,
    SOCK,
    OTHER,
    INVALID
} PathType_e;

constexpr const char* default_access = "wb";


//-----------------------------------------------------------------------------

bool path_exists(const char *pathname);
PathType_e path_type(const char *pathname);

inline bool is_file(const char *pathname) { return path_type(pathname) == FILE; }
inline bool is_dir(const char *pathname) { return path_type(pathname) == DIR; }

void mkdirs(char *pathname);


//-----------------------------------------------------------------------------

struct path_times
{
    uint64_t creation, modification, access;
};

path_times times(const char *pathname);
inline uint64_t ctime(const char *pathname) { return times(pathname).creation; }
inline uint64_t mtime(const char *pathname) { return times(pathname).modification; }
inline uint64_t atime(const char *pathname) { return times(pathname).access; }

//-----------------------------------------------------------------------------

/** get the current working directory */
char *cwd(char *buf, int sz);

//-----------------------------------------------------------------------------

void delete_file(const char *filename);
void delete_path(const char *pathname, bool recursive=false);

//-----------------------------------------------------------------------------

template< class CharContainer >
void file_get_contents(const char *filename, CharContainer *v)
{
    ::FILE *fp = ::fopen(filename, "rb");
    C4_CHECK_MSG(fp != nullptr, "could not open file");
    ::fseek(fp, 0, SEEK_END);
    v->resize(::ftell(fp));
    ::rewind(fp);
    ::fread(&(*v)[0], 1, v->size(), fp);
    ::fclose(fp);
}

template< class CharContainer >
void file_put_contents(const char *filename, CharContainer const& v, const char* access=default_access)
{
    ::FILE *fp = ::fopen(filename, access);
    C4_CHECK_MSG(fp != nullptr, "could not open file");
    ::fwrite(&v[0], 1, v.size(), fp);
    ::fclose(fp);
}


//-----------------------------------------------------------------------------

/** the dtor deletes the temp file */
struct ScopedTempFile
{
    const char* m_name;
    ::FILE* m_file;

    const char* name() const { return m_name; }
    ::FILE* file() const { return m_file; }

    ~ScopedTempFile()
    {
        ::fclose(m_file);
        delete_file(m_name);
        m_file = nullptr;
        m_name = nullptr;
    }

    ScopedTempFile(const char *access=default_access)
        : m_name(::tmpnam(nullptr)), m_file(::fopen(m_name, access))
    {
    }

    ScopedTempFile(const char* contents, size_t sz, const char *access=default_access)
        : ScopedTempFile(access)
    {
        ::fwrite(contents, 1, sz, m_file);
    }

    template< class CharContainer >
    ScopedTempFile(CharContainer const& contents, const char* access=default_access)
        : ScopedTempFile(&contents[0], contents.size())
    {
    }
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct PathVisitorData
{
    PathType_e type;
#ifdef C4_POSIX
    struct dirent* data;
#endif
};

using PathVisitor = int (*)(const char *pathname, PathVisitorData const &info);

/** this function is NOT thread-safe */
int walk(const char *pathname, PathVisitor fn);

} // namespace fs
} // namespace c4

#endif /* _c4_FS_HPP_ */
