#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

// dup 関数の挙動確認
// dup 関数は、ファイルディスクリプタをコピーするシステムコール
// プロセスの持つ fd 表の中でエントリ番号が最小であるからのエントリを探し、そこに引き数で渡された fd のエントリのコピーを書き込む
// 下記のコードでは、標準出力をとじ、その後に作成したファイルディスクリプタを書き込んでいる。
// そのため、cat コマンドが内部的に呼ばれる際に、 cat > newfile として呼ばれる。
int	main()
{
	int	fd;

	fd = open("newfile", O_WRONLY | O_CREAT, 0664);
	close(1);
	dup(fd);
	execlp("cat", "cat", (char *)0);
}
