#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "sample.h"

void execute_command_node(Node *node) {
    if (node->kind != NODE_COMMAND) return;

    // コマンドの引数
    char **args = node->data.wordlist.words;
    // size_t argc = node->data.wordlist.count;

    // リダイレクションがある場合
    if (node->rhs != NULL && node->rhs->kind == NODE_REDIRECT) {
        char *redir_symbol = node->rhs->data.redirect.symbol;
        char *filename = node->rhs->data.redirect.filename->data.wordlist.words[0];

        if (strcmp(redir_symbol, ">") == 0) {
            // 標準出力をファイルにリダイレクト
            if (freopen(filename, "w", stdout) == NULL) {
                perror("freopen");
                exit(EXIT_FAILURE);
            }
        }
    }

    // コマンドの実行
    pid_t pid = fork();
    if (pid == 0) { // 子プロセス
        execvp(args[0], args);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else if (pid > 0) { // 親プロセス
        waitpid(pid, NULL, 0);
    } else {
        perror("fork");
        exit(EXIT_FAILURE);
    }
}

void execute_pipe_node(Node *node) {
    if (node->kind != NODE_PIPE) return;

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid1 = fork();
    if (pid1 == 0) { // 左側のコマンド
        dup2(pipefd[1], STDOUT_FILENO); // 標準出力をパイプの書き込み側にリダイレクト
        close(pipefd[0]);
        close(pipefd[1]);
        execute_command(node->lhs);
        exit(EXIT_FAILURE);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) { // 右側のコマンド
        dup2(pipefd[0], STDIN_FILENO); // 標準入力をパイプの読み込み側にリダイレクト
        close(pipefd[1]);
        close(pipefd[0]);
        execute_command(node->rhs);
        exit(EXIT_FAILURE);
    }

    // 親プロセス
    close(pipefd[0]);
    close(pipefd[1]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

void execute_and_node(Node *node) {
    if (node->kind != NODE_AND) return;

    pid_t pid = fork();
    if (pid == 0) { // 子プロセス
        execute_command(node->lhs);
        exit(EXIT_SUCCESS);
    } else if (pid > 0) { // 親プロセス
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            execute_command(node->rhs);
        }
    } else {
        perror("fork");
        exit(EXIT_FAILURE);
    }
}

void execute_or_node(Node *node) {
    if (node->kind != NODE_OR) return;

    pid_t pid = fork();
    if (pid == 0) { // 子プロセス
        execute_command(node->lhs);
        exit(EXIT_FAILURE);
    } else if (pid > 0) { // 親プロセス
        int status;
        waitpid(pid, &status, 0);
        if (!(WIFEXITED(status) && WEXITSTATUS(status) == 0)) {
            execute_command(node->rhs);
        }
    } else {
        perror("fork");
        exit(EXIT_FAILURE);
    }
}

void execute_command(Node *node) {
    if (node == NULL) return;

    switch (node->kind) {
        case NODE_COMMAND:
            execute_command_node(node);
            break;
        case NODE_PIPE:
            execute_pipe_node(node);
            break;
        case NODE_AND:
            execute_and_node(node);
            break;
        case NODE_OR:
            execute_or_node(node);
            break;
        default:
            fprintf(stderr, "Unknown node kind: %d\n", node->kind);
    }
}
