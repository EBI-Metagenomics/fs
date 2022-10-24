#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif

#ifdef _FILE_OFFSET_BITS
#undef _FILE_OFFSET_BITS
#endif

#define _POSIX_C_SOURCE 200809L
#define _FILE_OFFSET_BITS 64

#include "fs.h"
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <unistd.h>

#ifdef __APPLE__
#ifdef _DARWIN_C_SOURCE
#undef _DARWIN_C_SOURCE
#endif
#define _DARWIN_C_SOURCE 1
#include <fcntl.h>
#include <sys/param.h>
#endif

#define BUFFSIZE (8 * 1024)
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define LINESIZE BUFFSIZE

static char *error_strings[] = {
#define X(_, A) A,
    FS_MAP(X)
#undef X
};

int fs_size(char const *filepath, long *size)
{
    struct stat st = {0};
    if (stat(filepath, &st) == 1) return FS_ESTAT;
    *size = (long)st.st_size;
    return FS_OK;
}

int fs_size_fp(FILE *fp, long *size)
{
    off_t old = ftello(fp);
    if (old < 0) return FS_EFTELL;

    if (fseeko(fp, 0, SEEK_END) < 0) return FS_EFSEEK;

    *size = ftello(fp);
    if (*size < 0) return FS_EFSEEK;

    if (fseeko(fp, old, SEEK_SET) < 0) return FS_EFSEEK;

    return FS_OK;
}

int fs_size_fd(int fd, long *size)
{
    struct stat st = {0};
    if (fsync(fd) < 0) return FS_EFSYNC;
    if (fstat(fd, &st) < 0) return FS_EFSTAT;
    *size = st.st_size;
    return FS_OK;
}

int fs_tell(FILE *restrict fp, long *offset)
{
    return (*offset = ftello(fp)) < 0 ? FS_EFTELL : FS_OK;
}

int fs_seek(FILE *restrict fp, long offset, int whence)
{
    return fseeko(fp, (off_t)offset, whence) < 0 ? FS_EFSEEK : FS_OK;
}

int fs_copy(FILE *restrict dst, FILE *restrict src)
{
    static _Thread_local char buffer[BUFFSIZE];
    size_t n = 0;
    while ((n = fread(buffer, sizeof(*buffer), BUFFSIZE, src)) > 0)
    {
        if (n < BUFFSIZE && ferror(src)) return FS_EFREAD;

        if (fwrite(buffer, sizeof(*buffer), n, dst) < n) return FS_EFWRITE;
    }
    if (ferror(src)) return FS_EFREAD;

    return FS_OK;
}

int fs_unlink(char const *filepath)
{
    return unlink(filepath) < 0 ? FS_EUNLINK : FS_OK;
}

int fs_rmdir(char const *dirpath)
{
    return rmdir(dirpath) < 0 ? FS_ERMDIR : FS_OK;
}

static int concat_path_file(unsigned size, char *dst, const char *path,
                            const char *filename);

int fs_mkstemp(unsigned size, char *filepath)
{
    filepath[0] = '\0';
    static char const template[] = "tmp.XXXXXXXXXX";
    char const *tmpdir = getenv("TMPDIR");
    if (!tmpdir || tmpdir[0] == '\0') tmpdir = "/tmp";
    int rc = concat_path_file(size, filepath, tmpdir, template);
    if (rc) return rc;
    return mkstemp(filepath) < 0 ? FS_EMKSTEMP : FS_OK;
}

int fs_move(char const *restrict dst, char const *restrict src)
{
    if (rename(src, dst) == 0) return FS_OK;
    FILE *fdst = fopen(dst, "wb");
    if (!fdst) return FS_EFOPEN;

    FILE *fsrc = fopen(src, "rb");
    if (!fsrc)
    {
        fclose(fdst);
        return FS_EFOPEN;
    }

    int rc = fs_copy(fdst, fsrc);
    if (!fclose(fdst) && fclose(fsrc)) fs_unlink(src);
    return rc;
}

int fs_refopen(FILE *fp, char const *mode, FILE **out)
{
    char filepath[FILENAME_MAX] = {0};
    int rc = fs_getpath(fp, sizeof filepath, filepath);
    if (rc) return rc;
    return (*out = fopen(filepath, mode)) ? FS_OK : FS_EFOPEN;
}

int fs_fileno(FILE *fp, int *fd)
{
    return (*fd = fileno(fp)) < 0 ? FS_EFILENO : FS_OK;
}

int fs_getpath(FILE *fp, unsigned size, char *filepath)
{
    int fd = 0;
    int rc = fs_fileno(fp, &fd);
    if (rc) return rc;

#ifdef __APPLE__
    (void)size;
    char pathbuf[MAXPATHLEN] = {0};
    if (fcntl(fd, F_GETPATH, pathbuf) < 0) return FS_EFCNTL;
    if (strlen(pathbuf) >= size) return FS_ETRUNCPATH;
    strcpy(filepath, pathbuf);
#else
    char pathbuf[FILENAME_MAX] = {0};
    sprintf(pathbuf, "/proc/self/fd/%d", fd);
    ssize_t n = readlink(pathbuf, filepath, size);
    if (n < 0) return FS_EREADLINK;
    if (n >= size) return FS_ETRUNCPATH;
    filepath[n] = '\0';
#endif

    return FS_OK;
}

bool fs_exists(char const *filepath) { return access(filepath, F_OK) == 0; }

int fs_touch(char const *filepath)
{
    if (fs_exists(filepath)) return FS_OK;
    FILE *fp = fopen(filepath, "wb");
    if (!fp) return FS_EFOPEN;
    return fclose(fp) ? FS_EFCLOSE : FS_OK;
}

int fs_readall(char const *filepath, long *size, unsigned char **data)
{
    *size = 0;
    *data = NULL;
    int rc = fs_size(filepath, size);
    if (rc) return rc;

    if (*size == 0) return 0;

    FILE *fp = fopen(filepath, "rb");
    if (!fp) return FS_EFOPEN;

    if (!(*data = malloc(*size)))
    {
        fclose(fp);
        return FS_ENOMEM;
    }

    if (fread(*data, *size, 1, fp) < 1)
    {
        fclose(fp);
        free(*data);
        return FS_EFREAD;
    }

    return fclose(fp) ? FS_EFCLOSE : FS_OK;
}

int fs_writeall(char const *filepath, long size, unsigned char *data)
{
    FILE *fp = fopen(filepath, "wb");
    if (!fp) return FS_EFOPEN;

    if (size <= 0) return fclose(fp) ? FS_EFCLOSE : FS_OK;

    if (fwrite(data, size, 1, fp) < 1)
    {
        fclose(fp);
        return FS_EFWRITE;
    }

    return fclose(fp) ? FS_EFCLOSE : FS_OK;
}

char const *fs_strerror(int rc)
{
    if (rc < 0 || rc >= (int)ARRAY_SIZE(error_strings)) return "unknown error";
    return error_strings[rc];
}

int fs_join(FILE *a, FILE *b, FILE *out)
{
    static _Thread_local char line[LINESIZE] = {0};

    while (fgets(line, sizeof(line), a))
    {
        if (ferror(a)) return FS_EFGETS;
        if (fwrite(line, sizeof(*line), strlen(line), out) < 1)
            return FS_EFWRITE;
    }
    if (ferror(a)) return FS_EFGETS;

    while (fgets(line, sizeof(line), b))
    {
        if (ferror(b)) return FS_EFGETS;
        if (fwrite(line, sizeof(*line), strlen(line), out) < 1)
            return FS_EFWRITE;
    }
    return ferror(b) ? FS_EFGETS : FS_OK;
}

int fs_split(FILE *in, long cut, FILE *a, FILE *b)
{
    static _Thread_local char line[LINESIZE] = {0};

    long i = 0;
    while (fgets(line, sizeof(line), in))
    {
        if (ferror(in)) return FS_EFGETS;
        if (i < cut)
        {
            if (fwrite(line, sizeof(*line), strlen(line), a) < 1)
                return FS_EFWRITE;
        }
        else if (fwrite(line, sizeof(*line), strlen(line), b) < 1)
            return FS_EFWRITE;
        ++i;
    }
    return ferror(in) ? FS_EFGETS : FS_OK;
}

static char *_fs_strdup(char const *str);

static void _fs_readlines_cleanup(long cnt, char *lines[])
{
    for (long i = 0; i < cnt; ++i)
        free(lines[i]);
    free(lines);
}

int fs_readlines(FILE *in, long *cnt, char **lines[])
{
    static _Thread_local char line[LINESIZE] = {0};

    *cnt = 0;
    *lines = NULL;

    while (fgets(line, sizeof(line), in))
    {
        if (ferror(in)) return FS_EFGETS;

        char **ptr = NULL;
        if (*cnt == 0)
        {
            ptr = malloc((*cnt + 1) * sizeof(*lines));
            if (!ptr) return FS_ENOMEM;
        }
        else
        {
            ptr = realloc(*lines, (*cnt + 1) * sizeof(*lines));
            if (!ptr)
            {
                _fs_readlines_cleanup(*cnt, *lines);
                return FS_ENOMEM;
            }
        }
        char *str = _fs_strdup(line);
        if (!str)
        {
            _fs_readlines_cleanup(*cnt, *lines);
            return FS_ENOMEM;
        }
        *lines = ptr;
        (*lines)[*cnt] = str;
        *cnt += 1;
    }
    return ferror(in) ? FS_EFGETS : FS_OK;
}

// ACK: BusyBox
static char *last_char_is(const char *s, int c)
{
    if (!s[0]) return NULL;
    while (s[1])
        s++;
    return (*s == (char)c) ? (char *)s : NULL;
}

// ACK: BusyBox
static int concat_path_file(unsigned size, char *dst, const char *path,
                            const char *filename)
{
    if (!path) path = "";
    char *lc = last_char_is(path, '/');
    while (*filename == '/')
        filename++;

    char const *sep = lc == NULL ? "/" : "";
    if (strlen(path) + strlen(sep) + strlen(filename) >= size)
        return FS_ETRUNCPATH;

    dst[0] = '\0';
    strcat(strcat(strcat(dst, path), sep), filename);
    return FS_OK;
}

static char *_fs_strdup(char const *str)
{
    size_t len = strlen(str) + 1;
    void *new = malloc(len);

    if (new == NULL) return NULL;

    return (char *)memcpy(new, str, len);
}
