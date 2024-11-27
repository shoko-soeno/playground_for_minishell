#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <limits.h>
#include <ctype.h>
#include <string.h>
#define PATH_MAX 4096  // 一般的な最大パス長（Linuxなどでは4096が一般的）

struct cmd {
	int argc;
	char **argv;
	int capa; // capacity of argv
	int status; // status of the command (for builtins) or exit status (for external commands)
	int pid; // process id
	struct cmd *next; // next command in the pipeline
};

// set the argc to -1 if the command is a redirection command    
// bool is_redirect(struct cmd *cmd)を作ってもいいかも
#define REDIRECT_P(cmd) ((cmd)->argc == -1)
#define PID_BUILTIN -2 // process id for builtin commands to distinguish from external commands
// if cmd is a builtin command, the program should not fork a child process
// BUILTIN_P(cmd): check if the command is a builtin command
#define BUILTIN_P(cmd) ((cmd)->pid == PID_BUILTIN)

struct builtin {
	char *name; // name of the builtin command
	int (*f)(int argc, char *argv[]); // function pointer to the builtin command
};

static void prompt(void);
static int invoke_commands(struct cmd *cmd);
static void exec_pipeline(struct cmd *cmdhead);
static void redirect_stdout(char *path);
static int wait_pipeline(struct cmd *cmdhead);
static struct cmd* pipeline_tail(struct cmd *cmdhead);
static struct cmd* parse_command_line(char *cmdline);
static void free_cmd(struct cmd *p);
static struct builtin* lookup_builtin(char *name);
static int builtin_cd(int argc, char *argv[]);
static int builtin_pwd(int argc, char *argv[]);
static int builtin_exit(int argc, char *argv[]);
static void* xmalloc(size_t sz);
static void* xrealloc(void *ptr, size_t sz);

static char *program_name;

int
main(int argc, char *argv[])
{
	program_name = argv[0];
	for (;;) { // keep prompting the user for commands
		prompt(); // prompt the user for a command
	}
	exit(0);
}

#define LINEBUF_MAX 2048

static void	prompt(void)
{
	static char buf[LINEBUF_MAX];
	struct cmd *cmd;

	fprintf(stdout, "$ "); // print the prompt
	fflush(stdout); // flush the output buffer
	if (fgets(buf, LINEBUF_MAX, stdin) == NULL) // read a line from the user
		exit(0);
	cmd = parse_command_line(buf); // parse the command line
	if (cmd == NULL) {
		fprintf(stderr, "%s: syntax error\n", program_name);
		return;
	}
	if (cmd->argc > 0)
		invoke_commands(cmd); // invoke the commands
	free_cmd(cmd); // free the command structure
}

static int  invoke_commands(struct cmd *cmdhead)
{
	int st;
	// Save original stdin and stdout
	int original_stdin = dup(0); // duplicate stdin
	int original_stdout = dup(1); // duplicate stdout

	exec_pipeline(cmdhead); // Execute the pipeline
	st = wait_pipeline(cmdhead); // Wait for the pipeline to finish
	// Restore original stdin and stdout
	close(0); dup2(original_stdin, 0); close(original_stdin);
	close(1); dup2(original_stdout, 1); close(original_stdout);

	return st;
}

// cmdhead: head of the command list
#define HEAD_P(cmd) ((cmd) == cmdhead)
#define TAIL_P(cmd) (((cmd)->next == NULL) || REDIRECT_P((cmd)->next))
// REDERECT_P(cmd): check if the command is a redirection command

static void
exec_pipeline(struct cmd *cmdhead)
{
	struct cmd *cmd; // current command
	int fds_previous[2] = {-1, -1}; // file descriptors for the previous command
	int fds_current[2] = {-1, -1}; // file descriptors for the current command

	// Execute the pipeline
	for (cmd = cmdhead; cmd && !REDIRECT_P(cmd); cmd = cmd->next) {
		fds_previous[0] = fds_current[0]; // fd[0] is the read end of the pipe
		fds_previous[1] = fds_current[1]; // fd[1] is the write end of the pipe
		if (! TAIL_P(cmd)) { // if the command is not the last command
			if (pipe(fds_current) < 0) {
				perror("pipe");
				exit(3);
			}
		}
		// Check if the command is a builtin
		if (lookup_builtin(cmd->argv[0]) != NULL) {
			cmd->pid = PID_BUILTIN; // set the pid to indicate that the command is a builtin
		}
		else {
			cmd->pid = fork(); // fork a child process if cmd is not a builtin
			if (cmd->pid < 0) {
				perror("fork");
				exit(3);
			}
			if (cmd->pid > 0) { // parent process (shell) continues to the next command
				if (fds_previous[0] != -1) close(fds_previous[0]);
				if (fds_previous[1] != -1) close(fds_previous[1]);
				continue;
			}
		}
		if (! HEAD_P(cmd)) { // if the command is not the first command
			close(0); dup2(fds_previous[0], 0); close(fds_previous[0]); // redirect stdin to the read end of the pipe
			close(fds_previous[1]); // close the write end of the pipe
		}
		if (! TAIL_P(cmd)) { // if the command is not the last command
			close(fds_current[0]); // close the read end of the pipe
			close(1); dup2(fds_current[1], 1); close(fds_current[1]); // redirect stdout to the write end of the pipe
		}
		if ((cmd->next != NULL) && REDIRECT_P(cmd->next)) { // if the next command is a redirection command
			redirect_stdout(cmd->next->argv[0]);
		}
		if (!BUILTIN_P(cmd)) {
			execvp(cmd->argv[0], cmd->argv); // execute the command
			fprintf(stderr, "%s: command not found: %s\n", // if execvp fails to execute the command
					program_name, cmd->argv[0]); // print an error message
			exit(1);
		}
	}
}

static void
redirect_stdout(char *path)
{
	int fd;

	close(1); // close stdout so that the next output will be written to the file
	// O_WRONLY: open the file for writing only
	// O_TRUNC: delete the contents of the file if it exists
	// O_CREAT: create the file if it does not exist
	fd = open(path, O_WRONLY|O_TRUNC|O_CREAT, 0666);
	if (fd < 0) {
		perror(path);
		return;
	}
	if (fd != 1) {
		dup2(fd, 1); // stdout is redirected to the fd so that the next output will be written to the file
		close(fd); // close fd
	}
}

// Wait for the pipeline to finish
// if the command is a builtin command, execute the builtin command
static int
wait_pipeline(struct cmd *cmdhead)
{
	struct cmd *cmd;

	// if cmd is not NULL, and cmd is not a redirection command, cmd is not the last command
	for (cmd = cmdhead; cmd && !REDIRECT_P(cmd); cmd = cmd->next) {
		if (BUILTIN_P(cmd))
			cmd->status = lookup_builtin(cmd->argv[0])->f(cmd->argc, cmd->argv);
		else
			waitpid(cmd->pid, &cmd->status, 0); // wait for the child process to finish
	}
	return pipeline_tail(cmdhead)->status; // return the exit status of the last command
}

static struct cmd*
pipeline_tail(struct cmd *cmdhead) // get the last command in the pipeline
{
	struct cmd *cmd;

	for (cmd = cmdhead; !TAIL_P(cmd); cmd = cmd->next)
		;
	return cmd;
}

#define INIT_ARGV 8 // initial capacity of cmd->argv
#define IDENT_CHAR_P(c) (!isspace((int)c) && ((c) != '|') && ((c) != '>'))
// check if c is a character that can be part of an identifier
// space, pipe and redirect are not part of an identifier

static struct cmd*
parse_command_line(char *p)
{
	struct cmd *cmd;

	// initialize the command structure
	cmd = xmalloc(sizeof(struct cmd)); // allocate memory for the command structure
	cmd->argc = 0; // initialize the number of arguments to 0
	cmd->argv = xmalloc(sizeof(char*) * INIT_ARGV); // allocate memory for the arguments(8 bytes)
	cmd->capa = INIT_ARGV; // set the capacity of the arguments to 8
	cmd->next = NULL;
	while (*p) { // parse the command line
		while (*p && isspace((int)*p)) // skip leading spaces
			*p++ = '\0'; // replace spaces with null characters
		if (! IDENT_CHAR_P(*p))
			break;
		if (*p && IDENT_CHAR_P(*p)) {
			if (cmd->capa <= cmd->argc) { // if argc exceeds the capacity
				cmd->argv = xrealloc(cmd->argv, cmd->capa);
			}
			cmd->argv[cmd->argc] = p; // set the start of the argument to p
			cmd->argc++; // increment the number of arguments
		}
		while (*p && IDENT_CHAR_P(*p))
			p++;
	}
	if (cmd->capa <= cmd->argc) {
		cmd->capa += 1;
		cmd->argv = xrealloc(cmd->argv, cmd->capa);
	}
	cmd->argv[cmd->argc] = NULL; // set the last argument to NULL

	if (*p == '|' || *p == '>') {
		if (cmd == NULL || cmd->argc == 0) goto parse_error;
		cmd->next = parse_command_line(p + 1); // parse the next command recursively
		if (cmd->next == NULL || cmd->next->argc == 0) goto parse_error;
		if (*p == '>') {
			if (cmd->next->argc != 1) goto parse_error;
			cmd->next->argc = -1;
		}
		*p = '\0';
	}

	return cmd;

  parse_error:
	if (cmd) free_cmd(cmd);
	return NULL;
}

static void
free_cmd(struct cmd *cmd)
{
	if (cmd->next != NULL)
		free_cmd(cmd->next);
	free(cmd->argv);
	free(cmd);
}

struct builtin builtins_list[] = {
	{"cd",      builtin_cd},
	{"pwd",     builtin_pwd},
	{"exit",    builtin_exit},
	{NULL,      NULL}
};

static struct builtin*
lookup_builtin(char *cmd)
{
	struct builtin *p;

	for (p = builtins_list; p->name; p++) {
		if (strcmp(cmd, p->name) == 0)
			return p; // return the builtin command
	}
	return NULL;
}

static int
builtin_cd(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "%s: wrong argument\n", argv[0]);
		return 1;
	}
	if (chdir(argv[1]) < 0) {
		perror(argv[1]);
		return 1;
	}
	return 0;
}

static int
builtin_pwd(int argc, char *argv[])
{
	char buf[PATH_MAX];

	if (argc != 1) {
		fprintf(stderr, "%s: wrong argument\n", argv[0]);
		return 1;
	}
	if (!getcwd(buf, PATH_MAX)) {
		fprintf(stderr, "%s: cannot get working directory\n", argv[0]);
		return 1;
	}
	printf("%s\n", buf);
	return 0;
}

static int
builtin_exit(int argc, char *argv[])
{
	if (argc != 1) {
		fprintf(stderr, "%s: too many arguments\n", argv[0]);
		return 1;
	}
	exit(0);
}

static void*
xmalloc(size_t sz)
{
	void *p;

	p = calloc(1, sz);
	if (!p)
		exit(3);
	return p;
}

static void*
xrealloc(void *ptr, size_t sz)
{
	void *p;

	if (!ptr) return xmalloc(sz);
	p = realloc(ptr, sz);
	if (!p)
		exit(3);
	return p;
}