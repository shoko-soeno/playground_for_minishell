#include <unistd.h>

// 下記のコードではシェルのパイプの動作を模倣する。
int main()
{
	int fd[2];
	char *argv[10];

	pipe(fd);
	if(fork() == 0)
	{
		close(fd[0]);
		close(1);
		dup(fd[1]); // パイプ出力へのリダイレクトを作成
		close(fd[1]);

		argv[0] = "ls";
		argv[1] = "-l";
		argv[2] = (char *)0;
		execvp(argv[0], argv);
	}
	else
	{
		close(fd[1]);
		close(0);
		dup(fd[0]); // pipe 入力のリダイレクトを作成
		close(fd[0]);

		argv[0] = "grep";
		argv[1] = "c$";
		argv[2] = (char *)0;
		execvp(argv[0], argv);
	}
}