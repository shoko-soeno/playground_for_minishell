#include "sample.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

/*
レキサーの動作：
空白文字をスキップ
|, ||, &&, <, >, >>, <<, <> などのリダイレクション記号やコネクタを認識します。
その他の連続する非スペース文字列を単語（WORD）として認識します。
*/

// ヘルパー関数：新しいトークンを作成
static t_token *create_token(TokenType type, const char *value) {
    t_token *token = malloc(sizeof(t_token));
    if (!token) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    token->type = type;
    token->value = value ? strdup(value) : NULL;
    token->next = NULL;
    return token;
}

// レキサー関数
t_token *lexer(const char *input) {
    t_token *head = NULL; // the first token
    t_token *current = NULL; // the last token, which is updated as we add more tokens
    size_t i = 0;
    size_t len = strlen(input);

    while (i < len) {
        // スペースをスキップ
        if (isspace(input[i])) {
            i++;
            continue;
        }

        // 特殊記号の処理
        if (input[i] == '|') {
            if (i + 1 < len && input[i + 1] == '|') {
                t_token *token = create_token(TOKEN_OR, "||");
                i += 2;
                if (!head) {
                    head = current = token; // both head and current are point to the same token (head/current -> [token1] -> NULL)
                } else {
                    current->next = token; // add new token to the end of the list (head-> [token1] -> [token2] -> NULL)
                    current = token; // current -> [token2] -> NULL
                }
            } else {
                t_token *token = create_token(TOKEN_PIPE, "|");
                i += 1;
                if (!head) {
                    head = current = token;
                } else {
                    current->next = token;
                    current = token;
                }
            }
            continue;
        }

        if (input[i] == '&') {
            if (i + 1 < len && input[i + 1] == '&') {
                t_token *token = create_token(TOKEN_AND, "&&");
                i += 2;
                if (!head) {
                    head = current = token; // if head is NULL, set head and current to token
                } else {
                    current->next = token; // set current->next to token so that it can be linked to the next token
                    current = token; // update current to token
                }
            } else {
                // 単一の&は未対応（必要に応じて追加）
                fprintf(stderr, "Unsupported token: &\n");
                exit(EXIT_FAILURE);
            }
            continue;
        }

        if (input[i] == '<') {
            t_token *token = create_token(TOKEN_REDIRECT, ">");
            i += 1;
            if (!head) {
                head = current = token;
            } else {
                current->next = token;
                current = token;
            }
        }

        // 単語（WORD）の処理 isgraph() は空白文字以外の印字可能な文字を認識
        if (isgraph(input[i])) {
            size_t start = i;
            while (i < len && !isspace(input[i]) && !strchr("|&<>", input[i])) i++;
            size_t word_len = i - start;
            char *word = xmalloc(word_len + 1);
            strncpy(word, &input[start], word_len);
            word[word_len] = '\0';
            t_token *token = create_token(TOKEN_WORD, word);
            free(word); // create_token 内で複製しているため解放
            if (!head) {
                head = current = token;
            } else {
                current->next = token;
                current = token;
            }
            continue;
        }

        // 未知の文字
        fprintf(stderr, "Unknown character: %c\n", input[i]);
        exit(EXIT_FAILURE);
    }

    // EOFトークンを追加
    t_token *eof_token = create_token(TOKEN_EOF, NULL);
    if (!head) {
        head = current = eof_token;
    } else {
        current->next = eof_token;
        current = eof_token;
    }

    return head;
}
