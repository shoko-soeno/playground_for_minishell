#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h>

// __attribute__((noreturn)) は関数が戻り値を返さない
// つまりこの関数が呼ばれたらプログラムが終了することを示す
void	fatal_error (const char *msg) __attribute__((noreturn));

void	fatal_error (const char *msg)
{
	dprintf(STDERR_FILENO, "Fatal Error: %s\n", msg);
	exit(1);
}

int	interpret(char *line)
{
	extern char	**environ; //グローバル変数environ(環境変数のリストを指すポインタ)を参照
	//argvの最初の要素として line を設定し、最後に NULL（リストの終了）を加える
	char		*argv[] = {line, NULL};
	pid_t		pid;
	int			wstatus;

	pid = fork();
	if (pid < 0)  //forkが失敗したらエラーを表示して終了
		fatal_error("fork");
	else if (pid == 0)
	{
		//子プロセス
		//実行するプログラムのパス、プログラム名のリスト、環境変数のリスト
		execve(line, argv, environ);
		fatal_error("execve");
	} else {
		//子プロセスが終了するまで親プロセスをブロック
		//終了時にステータスをwstatusに格納する
		//wstatusはビットでフラグを管理している
		wait(&wstatus);
		//WEXITSTATUS マクロは、wstatus からプロセスの終了コードのみを抽出
		return (WEXITSTATUS(wstatus)); 
	}
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