#!/bin/bash

# test終了後に、使用した一時ファイル（cmpとout）を削除する
cleanup() {
    rm -f cmp out
}

# 指定したコマンドをbashとminishellで実行し、結果と終了ステータスを比較する
# $?: Bashの終了ステータス
assert() {
    # 引数$1に指定されたコマンドを(左詰めで30文字分の幅を確保して)表示
    # '%-30s: 'はシェルで解釈されずそのままprintfに渡される。
    # "\"$1\""　シェルで変数展開。"を文字として出力する
    printf '%-30s: ' "\"$1\""
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

cleanup
echo 'all OK'

