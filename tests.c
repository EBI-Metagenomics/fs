#include "fs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

inline static void print_ctx(char const *func, char const *file, int line)
{
    fprintf(stderr, "\n%s failed at %s:%d\n", func, file, line);
}

#define ASSERT(x)                                                              \
    do                                                                         \
    {                                                                          \
        if (!(x))                                                              \
        {                                                                      \
            print_ctx(__FUNCTION__, __FILE__, __LINE__);                       \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

int main(void)
{
    long size = 0;
    ASSERT(!fs_size("LICENSE", &size));
    ASSERT(size == 1069);

    FILE *fp = fopen("LICENSE", "rb");
    char filepath[1024] = {0};
    ASSERT(!fs_size_fp(fp, &size));
    ASSERT(size == 1069);
    ASSERT(!fs_getpath(fp, sizeof filepath, filepath));
    ASSERT(strlen(filepath) >= strlen("LICENSE"));
    char const *base = &filepath[strlen(filepath) - strlen("LICENSE")];
    ASSERT(!strcmp(base, "LICENSE"));
    fclose(fp);

    int fd = 0;
    ASSERT(!fs_fileno(stdin, &fd));
    ASSERT(fd == STDIN_FILENO);

    ASSERT(!fs_fileno(stdin, &fd));
    ASSERT(fd == STDIN_FILENO);

    ASSERT(!fs_fileno(stdout, &fd));
    ASSERT(fd == STDOUT_FILENO);

    ASSERT(!fs_fileno(stderr, &fd));
    ASSERT(fd == STDERR_FILENO);

    unsigned char *data = NULL;
    ASSERT(!fs_readall("LICENSE", &size, &data));

    ASSERT(!fs_touch("touch.txt"));
    ASSERT(!fs_unlink("touch.txt"));

    ASSERT(!strcmp(fs_strerror(-1), "unknown error"));
    ASSERT(!strcmp(fs_strerror(127), "unknown error"));
    ASSERT(!strcmp(fs_strerror(FS_ENOMEM), "not enough memory"));

    ASSERT(!fs_mkstemp(sizeof filepath, filepath));
    ASSERT(fs_exists(filepath));
    ASSERT(!fs_unlink(filepath));

    char small[6] = {0};
    ASSERT(fs_mkstemp(sizeof small, small) == FS_ETRUNCPATH);

    return 0;
}
