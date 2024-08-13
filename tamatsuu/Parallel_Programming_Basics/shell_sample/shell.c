#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
/*Linux による並行プログラミング入門 p16 より引用*/

int	main()
{
	int		argc;
	char	cmd[80];	/* 80 文字のコマンド入力バッファ*/
	char	*cmdp;	/*作業用ポインタ*/
	char	*argv[10];	/*トークン入れる配列*/
	int		status;
	for(;;)
	{
		printf("\nplease type command\n");
		if(fgets(cmd, 80, stdin) == NULL)
			exit(0);
		cmd[strlen(cmd) - 1] = '\0';
		cmdp = cmd;
		for (argc = 0; ; argc++) // コマンドをトークンに分解
		{
			if ((argv[argc] = strtok(cmdp, " ")) == NULL)
				break;
			cmdp = NULL;
		}
		if (fork() == 0) // 子プロセスの作成
		{
			execvp(argv[0], argv); //子プロセスをコマンドに変身させる
			printf("comannd not found\n"); //コマンド内で、exit を呼ぶため、エラーの場合以外この行に来ない
			exit(1);
		}
		else
		{
			wait (&status);
		}
	}
}
