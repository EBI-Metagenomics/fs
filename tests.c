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

static void test_cksum(void);

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

    test_cksum();

    return 0;
}

static void test_cksum(void)
{
    static char *files[] = {
        "assets/a.txt", "assets/b.txt", "assets/c.txt",       "assets/d.txt",
        "assets/e.txt", "assets/f.txt", "assets/unsorted.txt"};
    static long chsums[] = {0, 2570, 27707, 27707, 19317, 19317, 46689};

    for (int i = 0; i < 7; ++i)
    {
        long chk = 0;
        if (fs_exists("output.txt")) fs_unlink("output.txt");
        ASSERT(!fs_copy("output.txt", files[i]));
        ASSERT(!fs_setperm("output.txt", FS_OWNER, FS_WRITE, true));
        ASSERT(!fs_sort("output.txt"));
        ASSERT(!fs_cksum("output.txt", FS_FLETCHER16, &chk));
        ASSERT(chsums[i] == chk);
    }
}
