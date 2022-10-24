#ifndef FS_H
#define FS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define FS_MAP(X)                                                              \
    X(OK, "not an error")                                                      \
    X(EFCLOSE, "fclose failed")                                                \
    X(EFCNTL, "fcntl failed")                                                  \
    X(EFGETS, "fgets failed")                                                  \
    X(EFILENO, "fileno failed")                                                \
    X(EFOPEN, "fopen failed")                                                  \
    X(EFREAD, "fread failed")                                                  \
    X(EFSEEK, "fseek failed")                                                  \
    X(EFSTAT, "fstat failed")                                                  \
    X(EFSYNC, "fsync failed")                                                  \
    X(EFTELL, "ftell failed")                                                  \
    X(EFWRITE, "fwrite failed")                                                \
    X(EMKSTEMP, "mkstemp failed")                                              \
    X(ENOMEM, "not enough memory")                                             \
    X(EREADLINK, "readlink failed")                                            \
    X(ERMDIR, "rmdir failed")                                                  \
    X(ESTAT, "stat failed")                                                    \
    X(ETRUNCPATH, "truncated path")                                            \
    X(EUNLINK, "unlink failed")

enum fs_rc
{
#define X(A, _) FS_##A,
    FS_MAP(X)
#undef X
};

int fs_size(char const *filepath, long *size);
int fs_size_fp(FILE *fp, long *size);
int fs_size_fd(int fd, long *size);

int fs_tell(FILE *restrict fp, long *offset);
int fs_seek(FILE *restrict fp, long offset, int whence);

int fs_copy(FILE *restrict dst, FILE *restrict src);
int fs_unlink(char const *filepath);
int fs_rmdir(char const *dirpath);
int fs_mkstemp(unsigned size, char *filepath);
int fs_move(char const *restrict dst, char const *restrict src);

int fs_refopen(FILE *fp, char const *mode, FILE **out);
int fs_fileno(FILE *fp, int *fd);
int fs_getpath(FILE *fp, unsigned size, char *filepath);

bool fs_exists(char const *filepath);
int fs_touch(char const *filepath);

int fs_readall(char const *filepath, long *size, unsigned char **data);
int fs_writeall(char const *filepath, long size, unsigned char *data);

char const *fs_strerror(int rc);

int fs_join(FILE *a, FILE *b, FILE *out);
int fs_split(FILE *in, long cut, FILE *a, FILE *b);

#endif
