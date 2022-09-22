#include "xfile.h"

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
    int64_t size = 0;
    ASSERT(!xfile_size("LICENSE", &size));
    ASSERT(size == 1069);

    FILE *fp = fopen("LICENSE", "rb");
    ASSERT(!xfile_psize(fp, &size));
    ASSERT(size == 1069);
    fclose(fp);

    int fd = 0;
    ASSERT(!xfile_fileno(stdin, &fd));
    ASSERT(fd == STDIN_FILENO);

    ASSERT(!xfile_fileno(stdin, &fd));
    ASSERT(fd == STDIN_FILENO);

    ASSERT(!xfile_fileno(stdout, &fd));
    ASSERT(fd == STDOUT_FILENO);

    ASSERT(!xfile_fileno(stderr, &fd));
    ASSERT(fd == STDERR_FILENO);

    unsigned char *data = NULL;
    ASSERT(!xfile_readall("LICENSE", &size, &data));

    ASSERT(!xfile_touch("touch.txt"));
    ASSERT(!xfile_unlink("touch.txt"));

    ASSERT(!strcmp(xfile_strerror(-1), "unknown error"));
    ASSERT(!strcmp(xfile_strerror(127), "unknown error"));
    ASSERT(!strcmp(xfile_strerror(XFILE_ENOMEM), "not enough memory"));

    return 0;
}
