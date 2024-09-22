#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h>
#include <string.h>
#define PATH_MAX 4096

// __attribute__((noreturn)) は関数が戻り値を返さない
// つまりこの関数が呼ばれたらプログラムが終了することを示す
void	fatal_error (const char *msg) __attribute__((noreturn));
void	err_exit (const char *location, const char *msg, int status) __attribute((noreturn));

void	fatal_error (const char *msg)
{
	dprintf(STDERR_FILENO, "Fatal Error: %s\n", msg);
	exit(1);
}

void	err_exit (const char *location, const char *msg, int status)
{
	dprintf(STDERR_FILENO, "minishell: %s: %s\n", location, msg);
	exit(status);
}

size_t strlcpy(char *dst, const char *src, size_t size)
{
    size_t srclen = strlen(src);
    if (size > 0) {
        size_t copylen = srclen < size - 1 ? srclen : size - 1;
        memcpy(dst, src, copylen);
        dst[copylen] = '\0';
    }
    return srclen;
}

size_t strlcat(char *dst, const char *src, size_t size)
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
			strlcpy(path, value, PATH_MAX);
		strlcat(path, "/", PATH_MAX); //pathに"/bin"が追加されていたら、"/bin/"にする
		strlcat(path, filename, PATH_MAX);
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

void vaidate_access(const char *path, const char *filename)
{
	if (path == NULL)
		error_exit(filename, "command not found", 127);
	if (access(path, F_OK) < 0)
		error_exit(filename, "command not found", 127);
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

int	interpret(char *const line)
{
	// extern char	**environ; //グローバル変数environ(環境変数のリストを指すポインタ)を参照
	// argvの最初の要素として line を設定し、最後に NULL（リストの終了）を加える
	char		*argv[] = {line, NULL};
	// pid_t		pid;
	// int			wstatus;
	int status;

	// pid = fork();
	// if (pid < 0)  //forkが失敗したらエラーを表示して終了
	// 	fatal_error("fork");
	// else if (pid == 0)
	// {
	// 	//子プロセス
	// 	//実行するプログラムのパス、プログラム名のリスト、環境変数のリスト
	// 	execve(line, argv, environ);
	// 	fatal_error("execve");
	// } else {
	// 	//子プロセスが終了するまで親プロセスをブロック
	// 	//終了時にステータスをwstatusに格納する
	// 	//wstatusはビットでフラグを管理している
	// 	wait(&wstatus);
	// 	//WEXITSTATUS マクロは、wstatus からプロセスの終了コードのみを抽出
	// 	return (WEXITSTATUS(wstatus)); 
	// }

	status = extec(argv);
	return (status);
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
		status = interpret(line);
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