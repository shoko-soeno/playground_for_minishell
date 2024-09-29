#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h>
#include <string.h>
#include "minishell.h"
#define PATH_MAX 4096


size_t	my_strlcpy(char *dst, const char *src, size_t size)
{
    size_t srclen = strlen(src);
    if (size > 0) {
        size_t copylen = srclen < size - 1 ? srclen : size - 1;
        memcpy(dst, src, copylen);
        dst[copylen] = '\0';
    }
    return srclen;
}

size_t	my_strlcat(char *dst, const char *src, size_t size)
{
    size_t dstlen = strlen(dst);
    size_t srclen = strlen(src);
    if (dstlen < size) {
        size_t copylen = (srclen < size - dstlen - 1) ? srclen : size - dstlen - 1;
        memcpy(dst + dstlen, src, copylen);
        dst[dstlen + copylen] = '\0';
    }
    return dstlen + srclen;
}

char	*search_path(const char *filename)
{
	char	path[PATH_MAX];
	char	*value;
	char	*end;

	value = getenv("PATH");
	while (*value)
	{
		bzero(path, PATH_MAX); //path配列の初期化
		end = strchr(value, ':');
		if (end)
			strncpy(path, value, end - value);
		else //最後のディレクトリパス
			my_strlcat(path, value, PATH_MAX);
		my_strlcat(path, "/", PATH_MAX); //pathに"/bin"が追加されていたら、"/bin/"にする
		my_strlcat(path, filename, PATH_MAX);
		if (access(path, X_OK) == 0)
		{
			char *dup;

			//pathはローカル変数なので、strdupでヒープメモリを確保してコピーする
			dup = strdup(path);
			if (dup == NULL)
				fatal_error("strdup");
			return (dup); //実行可能ファイルのパスを返す
		}
		if (end == NULL)
			return (NULL);
		value = end + 1;
	}
	return (NULL);
}
//strncpyはNULL終端を保証しない。
//strlcpy、strlcatはnull終端を保証するが、バッファサイズが十分でないとコピーされる文字列の一部が失われる

void validate_access(const char *path, const char *filename)
{
	if (path == NULL)
		err_exit(filename, "command not found", 127);
	if (access(path, F_OK) < 0)
		err_exit(filename, "command not found", 127);
}

int exec(char *argv[])
{
	extern char	**environ;
	const char	*path = argv[0];
	pid_t		pid;
	int			wstatus;

	pid = fork();
	if (pid < 0)
		fatal_error("fork");
	else if (pid == 0)
	{
		if (strchr(path, '/') == NULL)
			path = search_path(path);
		validate_access(path, argv[0]);
		execve(path, argv, environ);
		fatal_error("execve");
	} else {
		wait(&wstatus);
		return (WEXITSTATUS(wstatus));
	}
}

void	interpret(char *line, int *stat_loc)
{
	t_token	*tok;
	char 	**argv;

	tok = tokenize(line);
	if (tok->kind == TK_EOF)
		;
	else if (syntax_error)
	{
		*stat_loc = ERROR_TOKENIZE;
	}	
	else
	{
		expand(tok);
		argv = token_list_to_argv(tok);
		*stat_loc = exec(argv);
		free_argv(argv);
	}
	free_tok(tok);
}

/*
rl_outstream
GNU Readlineライブラリの内部で使用されるファイルポインタ
Readlineが出力を行う際に使用するストリームを指している
デフォルトでは、このストリームは標準出力（stdout）
必要に応じて他のストリーム（たとえば標準エラー出力のstderr）に変更できる

シェルプロンプト (minishell$ ) はユーザーにコマンドを入力させるためのもの
*/

int main(void)
{
	int		status;
	char	*line;

	rl_outstream = stderr; //コマンド結果とプロンプトを明確に区別するため
	status = 0;
	while (1)
	{
		line = readline("minishell$ ");
		if (line == NULL)	// EOF
			break;
		if (*line)
			add_history(line);
		interpret(line, &status);
		free(line);
	}
	exit(status);
}
/*
readline
minishell$ を表示してユーザの入力を待つ
ユーザーが入力してenterを押すと、入力された文字列がlineに格納される
ヒープメモリで割り当てられたものなのであとでfreeする必要がある
EOFは通常Ctrl + Dで送信される
ユーザがシェルを終了する意図をを示すものであり、この場合入力待ちせずNULLを返す

add_history(line)
GNU Readlineライブラリの関数
ユーザーが入力したコマンドを履歴に追加するために使用
次の入力時に↑や↓で呼び出せる
履歴はセッション中は保持される
*/