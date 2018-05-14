#include "c4/fs.hpp"

#include <c4/substr.hpp>

#ifdef C4_POSIX
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#endif


namespace c4 {
namespace fs {

int _exec_stat(const char *pathname, struct stat *s)
{
#ifdef C4_WIN
    /* If path contains the location of a directory, it cannot contain
     * a trailing backslash. If it does, -1 will be returned and errno
     * will be set to ENOENT. */
    size_t len = strlen(pathname);
    C4_ASSERT(len > 0);
    C4_ASSERT(pathname[len] == '\0');
    C4_ASSERT(pathname[len-1] != '/' && pathname[len-1] != '\\');
    return ::stat(pathname, s);
#endif
#ifdef C4_POSIX
    return ::stat(pathname, s);
#endif
}


bool path_exists(const char *pathname)
{
#if defined(C4_POSIX) || defined(C4_WIN)
    struct stat s;
    return _exec_stat(pathname, &s) == 0;
#else
    C4_NOT_IMPLEMENTED();
    return false;
#endif
}


PathType_e path_type(const char *pathname)
{
#if defined(C4_POSIX) || defined(C4_WIN)
#   ifdef C4_WIN
#      define _c4is(what) (s.st_mode & _S_IF##what)
#   else
#      define _c4is(what) (S_IS##what(s.st_mode))
#   endif
    struct stat s;
    C4_CHECK(_exec_stat(pathname, &s) == 0);
    // https://www.gnu.org/software/libc/manual/html_node/Testing-File-Type.html
    if(_c4is(REG))
    {
        return FILE;
    }
    else if(_c4is(DIR))
    {
        return DIR;
    }
    else if(_c4is(LNK))
    {
        return SYMLINK;
    }
    else if(_c4is(FIFO))
    {
        return PIPE;
    }
    else if(_c4is(SOCK))
    {
        return SOCK;
    }
    else //if(_c4is(BLK) || _c4is(CHR))
    {
        return OTHER;
    }
#   undef _c4is
#else
    C4_NOT_IMPLEMENTED();
    return OTHER;
#endif
}

path_times times(const char *pathname)
{
    path_times t;
#if defined(C4_POSIX) || defined(C4_WIN)
    struct stat s;
    _exec_stat(pathname, &s);
    t.creation = s.st_ctime;
    t.modification = s.st_mtime;
    t.access = s.st_atime;
#else
    C4_NOT_IMPLEMENTED();
#endif
    return t;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

int _exec_mkdir(const char *dirname)
{
#if defined(C4_POSIX)
    return ::mkdir(dirname, 0755);
#elif defined(C4_WIN) || defined(C4_XBOX)
    return ::_mkdir(dirname);
#else
    C4_NOT_IMPLEMENTED();
    return 0;
#endif
}

void mkdirs(char *pathname)
{
    // add 1 to len because we know that the buffer has a null terminator
    c4::substr dir, buf = {pathname, 1 + strlen(pathname)};
    size_t start_pos;
    while(buf.next_split('/', &start_pos, &dir))
    {
        char ctmp = buf[start_pos];
        buf[start_pos] = '\0';
        _exec_mkdir(buf.str);
        buf[start_pos] = ctmp;
    }
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

char *cwd(char *buf, int sz)
{
    C4_ASSERT(sz > 0);
#ifdef C4_POSIX
    return ::getcwd(&buf[0], sz);
#else
    C4_NOT_IMPLEMENTED();
    return nullptr;
#endif
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void delete_file(const char *filename)
{
#ifdef C4_POSIX
    ::unlink(filename);
#else
    C4_NOT_IMPLEMENTED();
#endif
}

void delete_path(const char *pathname, bool recursive)
{
#ifdef C4_POSIX
    if( ! recursive)
    {
        ::remove(pathname);
    }
    else
    {
        C4_NOT_IMPLEMENTED();
    }
#else
    C4_NOT_IMPLEMENTED();
#endif
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

namespace detail {
#ifdef C4_POSIX
struct DirVisitor
{
    ::DIR *m_dir;
    DirVisitor(const char* dirname)
    {
        m_dir = ::opendir(dirname);
        C4_CHECK(m_dir != nullptr);
    }
    ~DirVisitor()
    {
        ::closedir(m_dir);
    }
};
#endif
} // namespace detail

int walk(const char *pathname, PathVisitor fn)
{
#ifdef C4_POSIX
    C4_CHECK(is_dir(pathname));
    detail::DirVisitor dv(pathname);
    struct dirent *de = ::readdir(dv.m_dir);
#else
    C4_NOT_IMPLEMENTED();
#endif
    return 0;
}

} // namespace fs
} // namespace c4
