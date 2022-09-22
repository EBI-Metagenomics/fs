#define _POSIX_C_SOURCE 200112L
#define _FILE_OFFSET_BITS 64

#include "xfile.h"
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <unistd.h>

#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#include <fcntl.h>
#include <sys/syslimits.h>
#endif

#define BUFFSIZE (8 * 1024)
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

static char *error_strings[] = {
#define X(_, A) A,
    XFILE_MAP(X)
#undef X
};

int xfile_size(char const *filepath, int64_t *size)
{
    struct stat st = {0};
    if (stat(filepath, &st) == 1) return XFILE_ESTAT;
    *size = st.st_size;
    return XFILE_OK;
}

int xfile_psize(FILE *fp, int64_t *size)
{
    off_t old = ftello(fp);
    if (old < 0) return XFILE_EFTELL;

    if (fseeko(fp, 0, SEEK_END) < 0) return XFILE_EFSEEK;

    *size = ftello(fp);
    if (*size < 0) return XFILE_EFSEEK;

    if (fseeko(fp, old, SEEK_SET) < 0) return XFILE_EFSEEK;

    return XFILE_OK;
}

int xfile_dsize(int fd, int64_t *size)
{
    struct stat st = {0};
    if (fsync(fd) < 0) return XFILE_EFSYNC;
    if (fstat(fd, &st) < 0) return XFILE_EFSTAT;
    *size = st.st_size;
    return XFILE_OK;
}

int xfile_tell(FILE *restrict fp, int64_t *offset)
{
    return (*offset = ftello(fp)) < 0 ? XFILE_EFTELL : XFILE_OK;
}

int xfile_seek(FILE *restrict fp, int64_t offset, int whence)
{
    return fseeko(fp, (off_t)offset, whence) < 0 ? XFILE_EFSEEK : XFILE_OK;
}

int xfile_copy(FILE *restrict dst, FILE *restrict src)
{
    char buffer[BUFFSIZE];
    size_t n = 0;
    while ((n = fread(buffer, sizeof(*buffer), BUFFSIZE, src)) > 0)
    {
        if (n < BUFFSIZE && ferror(src)) return XFILE_EFREAD;

        if (fwrite(buffer, sizeof(*buffer), n, dst) < n) return XFILE_EFWRITE;
    }
    if (ferror(src)) return XFILE_EFREAD;

    return XFILE_OK;
}

int xfile_unlink(char const *filepath)
{
    return unlink(filepath) < 0 ? XFILE_EUNLINK : XFILE_OK;
}

int xfile_rmdir(char const *dirpath)
{
    return rmdir(dirpath) < 0 ? XFILE_ERMDIR : XFILE_OK;
}

static int filepath_from_file_pointer(FILE *fp, char *filepath);

int xfile_refopen(FILE *fp, char const *mode, FILE **out)
{
    char filepath[PATH_MAX] = {0};
    int rc = filepath_from_file_pointer(fp, filepath);
    if (rc) return rc;
    return (*out = fopen(filepath, mode)) ? XFILE_OK : XFILE_EFOPEN;
}

int xfile_fileno(FILE *fp, int *fd)
{
    return (*fd = fileno(fp)) < 0 ? XFILE_EFILENO : XFILE_OK;
}

bool xfile_exists(char const *filepath) { return access(filepath, F_OK) == 0; }

int xfile_touch(char const *filepath)
{
    if (xfile_exists(filepath)) return XFILE_OK;
    FILE *fp = fopen(filepath, "wb");
    if (!fp) return XFILE_EFOPEN;
    return fclose(fp) ? XFILE_EFCLOSE : XFILE_OK;
}

int xfile_readall(char const *filepath, int64_t *size, unsigned char **data)
{
    *size = 0;
    int rc = xfile_size(filepath, size);
    if (rc) return rc;

    FILE *fp = fopen(filepath, "rb");
    if (!fp) return XFILE_EFOPEN;

    if (!(*data = malloc(*size + 1)))
    {
        fclose(fp);
        return XFILE_ENOMEM;
    }

    if (fread(*data, *size, 1, fp) < 1)
    {
        fclose(fp);
        free(*data);
        return XFILE_EFREAD;
    }

    fclose(fp);
    (*data)[*size] = '\0';
    return XFILE_OK;
}

char const *xfile_strerror(int rc)
{
    if (rc < 0 || rc >= (int)ARRAY_SIZE(error_strings)) return "unknown error";
    return error_strings[rc];
}

static int filepath_from_file_pointer(FILE *fp, char *filepath)
{
    int fd = 0;
    int rc = xfile_fileno(fp, &fd);
    if (rc) return rc;

#ifdef __APPLE__
    if (fcntl(fd, F_GETPATH, filepath) < 0) return XFILE_EFCNTL;
#else
    sprintf(filepath, "/proc/self/fd/%d", fd);
#endif

    return XFILE_OK;
}
