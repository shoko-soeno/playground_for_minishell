// https://github.com/pasqualerossi/42-School-Exam-Rank-04/blob/main/microshell.c

#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

// write an error message to stderr
void err(char *str)
{
    while (*str)
        write(2, str++, 1);
}

// change the current working directory
int cd(char **argv, int i)
{
    if (i != 2)
        return err("cd: wrong number of arguments\n"), 1;
    if (chdir(argv[1]) == -1)
        return err("cd: cannot change directory to "), err(argv[1]), err("\n"), 1;
    return 0;
}

// Function to set pipe
// end == 1 sets stdout to act as write end of our pipe
// end == 0 sets stdin to act as read end of our pipe
void setPipe(int has_pipe, int *fd, int end)
{
    if (has_pipe && (dup2(fd[end], end) == -1) || close(fd[0]) == -1 || close(fd[1]) == -1)
        err("dup2 error\n"), exit(1);
}

// Function to execute a command
int exec(char **argv, int i, char **envp)
{
    int has_pipe, fd[2], pid, status;
    has_pipe = argv[i] && !strcmp(argv[i], "|");

    // Function to execute a command
    if (!has_pipe && !strcmp(argv[0], "cd"))
        return cd(argv, i);
    
    // If the command includes a pipe and creating the pipe fails, print error and exit
    if (has_pipe && pipe(fd) == -1)
        err("pipe error\n"), exit(1);

    // If the fork fails, print error and exit
    if ((pid = fork()) == -1)
        err("fork error\n"), exit(1);
    if (!pid)
    {
        argv[i] = 0;
        // If the command includes a pipe, set write end of pipe, if it fail print error and exit
        setPipe(has_pipe, fd, 1);
        if (!strcmp(argv[0], "cd"))
            exit(cd(argv, i));
        execve(argv[0], argv, envp);
        err("error: cannot execute "), err(*argv), err("\n"), exit(1);
        exit(1);
    }
    // Wait for the child process to finish
    waitpid(pid, &status, 0);
    // If the command includes a pipe, set write end of pipe, if it fail print error and exit
    setPipe(has_pipe, fd, 0);
    // Return the exit status of the child process
    return WIFEXITED(status) && WEXITSTATUS(status);
}

int main(int argc, char **argv, char **envp)
{
    (void)argc;
    int i = 0, status = 0;

    // Loop through each following argument
    while (argv[i])
    {
        // Move the pointer to the next argument after the last delimeter / first argument
        argv += i +1;
        i = 0;
        // Loop through each argument until a pipe or semicolon is found
        while (argv[i] && strcmp(argv[i], "|") && strcmp(argv[i], ";"))
            i++;
        // If there are arguments, execute them
        if (i)
            status = exec(argv, i, envp);
    }
    return status;
}