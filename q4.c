#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int i;
    int fd;

    for (i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], ">")==0)
            break;
    }
    if (i != argc)
    {
        argv[i] = (char *)0;
        unlink(argv[i+1]);
        fd = creat(argv[i+1], 0644);
        close(1);
        dup(fd);
    }
    execlp("cat", "cat", (char *)0);
}