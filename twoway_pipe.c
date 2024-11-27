#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

int main(int argc, char *argv[])
{
    int p1[2];
    if (pipe(p1) < 0) {
        perror("pipe");
        exit(1);
    }
    int pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }
    if (pid == 0) {
        //child process
        int x;
        if (read(p1[0], &x, sizeof(x)) == -1) {
            perror("read");
            exit(1);
        }
        printf("Received x %d\n", x);
        x *= 4;
        if (write(p1[1], &x, sizeof(x)) == -1) {
            perror("write");
            exit(1);
        }
        printf("Wrote x * 4 %d\n", x);
    } else {
        //parent process
        srand(time(NULL));
        int y = rand() % 10;
        if(write(p1[1], &y, sizeof(y)) == -1) {
            perror("write");
            exit(1);
        }
        if (read(p1[0], &y, sizeof(y)) == -1) {
            perror("read");
            exit(1);
        }
        printf("Received y %d\n", y);
    }
    close(p1[0]);
    close(p1[1]);
    return 0;
}