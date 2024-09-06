#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

// 下記のコードではpipe()システムコールの挙動を確認する。
int main()
{
	int fd[2], rfd;
	char c;

	pipe(fd); //配列の一番目に、pipe の読み出し用 fd 、pipe 配列の 2 番目に書き込み用 fd が用意される。
	if (fork() == 0)
	{
		close(fd[0]);
		rfd = open("huge_file", O_RDONLY);
		while (read(rfd, &c, 1) != 0) //oldfile 空文字を読み込む。
			write(fd[1], &c, 1); //1 文字をpipe の書き込み用ファイルディスクリプタに書き込む
		close(fd[1]);
		close(rfd);
	}
	else
	{
		close(fd[1]);
		while (read(fd[0], &c, 1) != 0) // pipe の読み出し用ファイルディスクリプタから読み込む
			write(1, &c, 1); // 標準出力に書き込む
		close(fd[0]);
	}
}
