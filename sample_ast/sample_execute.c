#include "sample.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>

char *resolve_command_path(const char *command, char **envp)
{
    if (!command || !*command) {
        fprintf(stderr, "ERROR: Invalid command\n");
        return NULL;
    }
    printf("DEBUG envp[0] = %s\n", envp[0]);
    printf("DEBUG Resolving path for command: %s\n", command);
    char *path_env = NULL;
    for (size_t i = 0; envp[i]; i++) {
        if (strncmp(envp[i], "PATH=", 5) == 0) {
            path_env = envp[i] + 5;
            break;
        }
    }

    if (!path_env) {
        fprintf(stderr, "PATH environment variable is not set\n");
        return NULL;
    }
    printf("DEBUG: PATH = %s\n", path_env);

    char *paths = strdup(path_env);
    char *token = strtok(paths, ":");
    while (token) {
        char *path = malloc(strlen(token) + strlen(command) + 2);
        sprintf(path, "%s/%s", token, command);
        printf("DEBUG Checking path: %s\n", path);
        struct stat sb;
        if (stat(path, &sb) == 0 && (sb.st_mode & S_IXUSR)) {
            free(paths);
            return path;
        }
        free(path);
        token = strtok(NULL, ":");
    }
    free(paths);
    return NULL;
}

void execute_command_node(Node *node, char **envp)
{
    if (node->kind != NODE_COMMAND) return;

    char **args = node->data.wordlist.words;

    if (node->rhs && node->rhs->kind == NODE_REDIRECT) {
        execute_redirect(node->rhs);
    }

    char *command_path = resolve_command_path(args[0], envp);
    if (!command_path) {
        fprintf(stderr, "Command not found: %s\n", args[0]);
        return;
    }

    printf("DEUG Executing command: %s with args:\n", command_path);
    for (size_t i = 0; args[i]; i++) {
        printf("DEBUG args[%zu] = %s\n", i, args[i]);
    }
    
    pid_t pid = fork();
    if (pid == 0) {
        execve(command_path, args, envp);
        perror("execve");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        waitpid(pid, NULL, 0);
    } else {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    free(command_path);
}

void execute_pipe_node(Node *node, char **envp) {
    if (node->kind != NODE_PIPE) return;

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid1 = fork();
    if (pid1 == 0) {
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        execute_command(node->lhs, envp);
        exit(EXIT_FAILURE);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[1]);
        close(pipefd[0]);
        execute_command(node->rhs, envp);
        exit(EXIT_FAILURE);
    }

    close(pipefd[0]);
    close(pipefd[1]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

void execute_logical_node(Node *node, char **envp) {
    if (node->kind != NODE_AND && node->kind != NODE_OR) return;

    pid_t pid = fork();
    if (pid == 0) {
        execute_command(node->lhs, envp);
        exit(EXIT_SUCCESS);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        if (node->kind == NODE_AND) {
            if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                execute_command(node->rhs, envp);
            }
        } else if (node->kind == NODE_OR) {
            if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
                execute_command(node->rhs, envp);
        }
    } else {
        perror("fork");
        exit(EXIT_FAILURE);
    }
}

void execute_redirect(Node *node) {
    if (node->kind != NODE_REDIRECT) return;

    for (size_t i = 0; i < node->data.redirect.count; i++) {
        const char *symbol = node->data.redirect.symbols[i];
        const char *filename = node->data.redirect.filenames[i];

        if (strcmp(symbol, ">") == 0) {
            int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror("open");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        } else if (strcmp(symbol, ">>") == 0) {
            int fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd < 0) {
                perror("open");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        } else if (strcmp(symbol, "<") == 0) {
            int fd = open(filename, O_RDONLY);
            if (fd < 0) {
                perror("open");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
    }
}

void execute_command(Node *node, char **envp) {
    if (!node) return;

    switch (node->kind) {
        case NODE_COMMAND:
            execute_command_node(node, envp);
            break;
        case NODE_PIPE:
            execute_pipe_node(node, envp);
            break;
        case NODE_AND:
        case NODE_OR:
            execute_logical_node(node, envp);
            break;
        default:
            fprintf(stderr, "Unknown node kind: %d\n", node->kind);
    }
}
