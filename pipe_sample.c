# include <stdlib.h>
# include <unistd.h>
# include <stdio.h>
# include <readline/readline.h>
# include <readline/history.h>
# include <limits.h>
# include <stdbool.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <sys/wait.h>
# include <fcntl.h>
# include <string.h>

struct cmds {
	int			argc;
	char		**cmd;
	pid_t		pid;
	int			exit_status;
	struct cmds	*next;
};

int		invoke_commands(struct cmds *cmdhead);
void	exec_pipeline(struct cmds *cmdhead, int cp_fd[2]);
int		wait_pipeline(struct cmds *cmdhead);
struct cmds*	pipeline_tail(struct cmds *cmdhead);
void    handle_parent_process(int *pfd_pre, int *pfd);
void   handle_child_process(struct cmds *cur_cmd,
    int *pfd_pre, int *pfd, int cp_fd[2]);

void	d_throw_error(char *func_name, char *error_msg)
{
	printf("Error: function name: %s, Error message: %s", func_name, error_msg);
	exit(EXIT_FAILURE);
}

int	invoke_commands(struct cmds *cmdhead)
{
	int	exit_status;
	int	original_stdin;
	int	original_stdout;
	int	cp_fd[2];

	original_stdin = dup(STDIN_FILENO);
	original_stdout = dup(STDOUT_FILENO);
	// cp_fd[0] = original_stdin;
	// cp_fd[1] = original_stdout;
	// exec_pipeline(cmdhead, cp_fd);
    exec_pipeline(cmdhead, NULL);
	exit_status = wait_pipeline(cmdhead);
	
	close(STDIN_FILENO);
	dup2(original_stdin, STDIN_FILENO);
	close(original_stdin);
	close(STDOUT_FILENO);
	dup2(original_stdout, STDOUT_FILENO);
	close(original_stdout);
	return (exit_status);
}

void	exec_pipeline(struct cmds *cmdhead, int cp_fd[2])
{
	struct cmds	*cur_cmd;
	int			pfd_pre[2];
	int			pfd[2];

	cur_cmd = cmdhead;
	// ft_memset(pfd_pre, -1, sizeof(pfd_pre));
	// ft_memset(pfd, -1, sizeof(pfd));
	pfd_pre[0] = -1;
	pfd_pre[1] = -1;
	pfd[0] = -1;
	pfd[1] = -1;
	while (cur_cmd)
	{
		pfd_pre[0] = pfd[0];
		pfd_pre[1] = pfd[1];
		if (cur_cmd->next != NULL && pipe(pfd) < 0)
			d_throw_error("exec_pipeline", "failed to create pipe");
		cur_cmd->pid = fork();
		if (cur_cmd->pid < 0)
			d_throw_error("exec_pipeline", "failed to fork");
		if (cur_cmd->pid > 0)
		{
			handle_parent_process(pfd_pre, pfd);
			cur_cmd = cur_cmd->next;
			continue ;
		}
		handle_child_process(cur_cmd, pfd_pre, pfd, cp_fd);
	}
}

/*
parent process
	- does NOT read or write to the pipe.
	- close the write end of the pipe(pfd[1])
*/
void	handle_parent_process(int *pfd_pre, int *pfd)
{
	if (pfd_pre[0] != -1)
		close(pfd_pre[0]);
	if (pfd_pre[1] != -1)
		close(pfd_pre[1]);
	if (pfd[1] != -1)
		close(pfd[1]);
}

/*
child process
	- FIRST command
		- close the read end of the pipe(pfd[0])
		- redirect the write end of the pipe to STDOUT
	- MIDDLE command
		- redirect the read end of the previous pipe to STDIN
		- redirect the write end of the pipe to STDOUT
	- LAST command
		- redirect the read end of the previous pipe to STDIN
		- does not write to the pipe
*/
void	handle_child_process(struct cmds *cur_cmd,
	int *pfd_pre, int *pfd, int cp_fd[2])
{
    (void) cp_fd;
	if (pfd_pre[1] != -1)
	{
		close(STDIN_FILENO);
		dup2(pfd_pre[0], STDIN_FILENO);
		close(pfd_pre[0]);
		close(pfd_pre[1]);
		// close(pfd[0]);
		// close(pfd[1]);
	}
	if (cur_cmd->next != NULL)
	{
		close(pfd[0]);
		close(STDOUT_FILENO);
		dup2(pfd[1], STDOUT_FILENO);
		close(pfd[1]);
		// close(pfd_pre[0]);
		// close(pfd_pre[1]);
	}
	// close(cp_fd[0]);
	// close(cp_fd[1]);
	execvp(cur_cmd->cmd[0], cur_cmd->cmd);
	d_throw_error("exec_pipeline", "failed to execvp");
}
/*
子プロセスの中でexecvpを実行する
ファイルディスクリプタを閉じる
の2つに関数を分割したほうがよい。
*/

/*
if cmd is a builtin
	call the function and return exit status
if cmd is not a builtin
	wait for the child process to finish
	and return the exit status
*/
int	wait_pipeline(struct cmds *cmdhead)
{
	struct cmds	*cmd;

	cmd = cmdhead;
	while (cmd->next)
	{
		waitpid(cmd->pid, &cmd->exit_status, 0);
		cmd = cmd->next;
	}
	return (pipeline_tail(cmdhead)->exit_status);
}

struct cmds	*pipeline_tail(struct cmds *cmdhead)
{
	struct cmds	*cmd;

	cmd = cmdhead;
	while (cmd->next)
		cmd = cmd->next;
	return (cmd);
}
/*
libftのft_lstlastと同じ機能
lexer, parserで連結リストの操作関数を使っているなら代替可能
*/

struct cmds *create_test_commands()
{
    struct cmds *cmd1 = malloc(sizeof(struct cmds));
    struct cmds *cmd2 = malloc(sizeof(struct cmds));
    
    if (!cmd1 || !cmd2)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    // cmd1 (echo hello)
    cmd1->argc = 2;
    cmd1->cmd = malloc(3 * sizeof(char *));
    cmd1->cmd[0] = strdup("echo");
    cmd1->cmd[1] = strdup("hello");
    cmd1->cmd[2] = NULL;
    cmd1->exit_status = 0;
    cmd1->next = cmd2;

    // cmd2 (wc -l)
    cmd2->argc = 2;
    cmd2->cmd = malloc(3 * sizeof(char *));
    cmd2->cmd[0] = strdup("wc");
    cmd2->cmd[1] = strdup("-l");;
	cmd2->cmd[2] = NULL;
    cmd2->exit_status = 0;
    cmd2->next = NULL;

    return cmd1;
}

int	main(void)
{
	int exit_status;
	struct cmds *cmdhead;
	
	cmdhead = create_test_commands();
	exit_status = invoke_commands(cmdhead);
	printf("test done...exit status: %d\n", exit_status);

	// free after use
	struct cmds *tmp;
	while (cmdhead)
	{
		tmp = cmdhead;
		cmdhead = cmdhead->next;
		int i = 0;
		while (i < tmp->argc)
			free(tmp->cmd[i++]);
		free(tmp->cmd);
		free(tmp);
	}
	return (0);
}