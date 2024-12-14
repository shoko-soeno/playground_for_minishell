#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

void simulate_abnormal_exit() {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // 子プロセス
        printf("Child: Running...\n");
        sleep(1); // 少し待機して親がシグナルを送るのを待つ
        printf("Child: Exiting abnormally...\n");
        while (1); // シグナルを受け取るためループ
    } else {
        // 親プロセス
        int status;

        // 子プロセスに SIGSEGV を送信
        sleep(1); // 子プロセスが準備されるのを待つ
        kill(pid, SIGSEGV); // SIGSEGV を送信
        printf("Parent: Sent SIGSEGV to child\n");

        // 子プロセスの終了を待機
        wait(&status);

        // ステータスを表示
        printf("Parent: Raw status: 0x%04X\n", status);

        if (WIFSIGNALED(status)) {
            printf("Parent: Child terminated by signal %d\n", WTERMSIG(status));
            if (WCOREDUMP(status)) {
                printf("Parent: Core dumped\n");
            }
        }
    }
}

int main() {
    simulate_abnormal_exit();
    return 0;
}
