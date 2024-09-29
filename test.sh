#!/bin/bash

# # test終了後に、使用した一時ファイル（cmpとout）を削除する
# cleanup() {
#     rm -f cmp out
# }

cat <<EOF | clang -xc -o a.out -
#include <stdio.h>
int main() { printf("hello from a.out\n"); }
EOF

cat <<EOF | clang -xc -o print_args -
#include <stdio.h>
int main(int argc, char *argv[]){ 
    for (int i = 0; argv[i]; i++)
        printf("argv[%d]: %s\n", i, argv[i]); 
}
EOF
# cat <<EOF ... EOF:
# Here Documentと呼ばれる構文
# EOF までの間に書かれたテキストをそのまま標準入力として扱う
# つまり、cat コマンドにCプログラムのコードを渡している
# gcc -xc -o a.out -:
# -xc 入力をC言語のコードとして解釈する
# -o a.out 実行ファイル生成
# -標準入力からソースコードを読み取る。

cleanup() {
    rm -f cmp out a.out print_args
}

# 指定したコマンドをbashとminishellで実行し、結果と終了ステータスを比較する
# $?: Bashの終了ステータス
assert() {
    # 引数$1に指定されたコマンドを(左詰めで30文字分の幅を確保して)表示
    # '%-30s: 'はシェルで解釈されずそのままprintfに渡される。
    # "\"$1\""　シェルで変数展開。"を文字として出力する
    # printf '%-30s: ' "\"$1\""
    printf '%-50s: ' "[$1]"

    # exit status
    # $1の値をBashにパイプで渡す。エスケープシーケンスで解釈しつつ（-e）改行なし(-n)で。
    # bashでの実行結果をcmpファイルに保存
    # 2>&- 標準エラー出力（ファイルディスクリプタ2）を閉じて（&-）エラーメッセージが出ないようにする
    echo -n -e "$1" | bash >cmp 2>&-
    expected=$?
    # 同じコマンドをminishellでも実行し、その結果をoutファイルに保存
    echo -n -e "$1" | ./minishell >out 2>&-
    actual=$?

    diff cmp out >/dev/null && echo -n ' diff OK' || echo -n ' diff NG'

    if [ "$actual" = "$expected" ]; then
        echo -n ' exit status OK'
    else
        echo -n " exit status NG, expected $expected but got $actual"
    fi
    echo
}

# empty line (EOF)
assert ''

# Absolute path commands without args 
assert '/bin/pwd'
assert '/bin/echo'
assert '/bin/ls'

# search command path without args
assert 'pwd'
assert 'echo'
assert 'ls'
assert './a.out'

# ## no such command
assert 'a.out'
assert 'nosuchfile'

# Tokenize
## unquoted word
assert 'ls /'
assert 'echo hello    world     '
assert 'nosushfile'

## single quote
assert "./print_args 'hello  world' '42 Tokyo'"
assert "echo 'hello  world' '42Tokyo'"
assert "echo '\"hello world\"' '42Tokyo'"

## double quote
assert './print_args "hello  world" "42 Tokyo"'
assert 'echo "hello  world" "42Tokyo"'
assert "echo \"'hello   world'\" \"42Tokyo\""

## combination 
assert "echo hello'     world'"
assert "echo hello'   world '\" 42Tokyo  \""

cleanup
echo 'all OK'