#include "sample.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

// グローバルなトークンポインタ
static t_token *current_token;

// エラーハンドリング関数
static void parse_error(const char *msg) {
    fprintf(stderr, "Parse error: %s\n", msg);
    exit(EXIT_FAILURE);
}

// トークンを進める関数
static void advance() {
    if (current_token->type != TOKEN_EOF) {
        current_token = current_token->next;
    }
}

// 現在のトークンが期待するタイプか確認
static bool expect_token(TokenType type) {
    return current_token->type == type;
}

// トークンの値を比較し、マッチすればトークンを進める
static bool match_token(TokenType type, const char *value) {
    if (current_token->type == type) {
        if (value == NULL || (current_token->value && strcmp(current_token->value, value) == 0)) {
            advance();
            return true;
        }
    }
    return false;
}

// parse_filename: WORD
Node *parse_filename() {
    if (expect_token(TOKEN_WORD)) {
        Node *filename_node = create_wordlist_node(&current_token->value, 1);
        advance();
        return filename_node;
    }
    parse_error("Expected filename (WORD)");
    return NULL; // 実際には到達しない
}

// parse_redirection
Node *parse_redirection()
{
    char *symbol = strdup(">");
    advance(); // リダイレクション記号を進める
    // ファイル名を解析
    Node *filename = parse_filename();

    // 作成
    Node *redir = create_redirect_node(symbol, filename);
    free(symbol);
    return redir;
}

// parse_wordlist: WORD | wordlist WORD
Node *parse_wordlist() {
    if (!expect_token(TOKEN_WORD)) {
        parse_error("Expected WORD in wordlist");
    }

    // 単語を収集
    size_t capacity = 4;
    size_t count = 0;
    char **words = xmalloc(sizeof(char*) * capacity);

    while (expect_token(TOKEN_WORD)) {
        if (count >= capacity) {
            capacity *= 2;
            words = realloc(words, sizeof(char*) * capacity);
            if (!words) {
                perror("realloc");
                exit(EXIT_FAILURE);
            }
        }
        words[count++] = strdup(current_token->value);
        advance();
    }

    Node *wordlist = create_wordlist_node(words, count);

    // メモリ解放
    for (size_t i = 0; i < count; i++) {
        free(words[i]);
    }
    free(words);

    return wordlist;
}

Node *parse_simple_command() {
    // リダイレクションの前置き
    char *redirection_symbol = NULL;
    char *filename = NULL;
    if (expect_token(TOKEN_REDIRECT)) {
        Node *redir = parse_redirection();
    }

    // wordlist
    char *words[10];
    size_t count = 0;
    while (expect_token(TOKEN_WORD) && count < 10) {
        words[count++] = strdup(current_token->value);
        advance();
    }

    // リダイレクションの後置き
    if (expect_token(TOKEN_REDIRECT)) {
        Node *redir = parse_redirection();
    }
    return create_command_node(words, count, redirection_symbol, filename);
}


/*
 * parse_pipe:
 * パイプ演算子 | を処理する
 * パイプはシンプルコマンドの連結を表す
 */
Node *parse_pipe() {
    Node *left = parse_simple_command();

    while (current_token->type == TOKEN_PIPE) {
        advance(); // パイプを進める
        Node *right = parse_simple_command();
        left = new_node(NODE_PIPE, left, right);
    }
    return left;
}

/*
 * parse_logical:
 * 論理演算子 && と || を処理する
 * 論理演算子はパイプよりも低い優先順位を持つ
 */
Node *parse_logical() {
    Node *left = parse_pipe();

    while (current_token->type == TOKEN_AND || current_token->type == TOKEN_OR) {
        NodeKind op_type;
        if (current_token->type == TOKEN_AND) {
            op_type = NODE_AND;
        } else {
            op_type = NODE_OR;
        }

        advance(); // 演算子を進める

        Node *right = parse_pipe();
        left = new_node(op_type, left, right);
    }

    return left;
}

/*
 * parse_start:
 * パースのエントリーポイント
 */
Node *parse_start(t_token *token_list) {
    current_token = token_list;
    Node *ast = parse_logical();
    if (current_token->type != TOKEN_EOF) {
        parse_error("Unexpected tokens after command");
    }
    return ast;
}


/*------------------ Memory Management Functions ------------------*/

#include <stdlib.h>

// トークンリストの解放
void free_token_list(t_token *token) {
    while (token) {
        t_token *next = token->next;
        if (token->value) free(token->value);
        free(token);
        token = next;
    }
}

// ASTの解放関数
void free_ast(Node *node) {
    if (!node) return;
    switch (node->kind) {
        case NODE_COMMAND:
            if (node->data.wordlist.words) {
                for (size_t i = 0; i < node->data.wordlist.count; i++) {
                    free(node->data.wordlist.words[i]);
                }
                free(node->data.wordlist.words);
            }
            if (node->rhs && node->rhs->kind == NODE_REDIRECT) {
                free_ast(node->rhs);
            }
            break;
        case NODE_PIPE:
        case NODE_AND:
        case NODE_OR:
            free_ast(node->lhs);
            free_ast(node->rhs);
            break;
        case NODE_REDIRECT:
            if (node->data.redirect.symbol) free(node->data.redirect.symbol);
            free_ast(node->data.redirect.filename);
            break;
        case NODE_WORDLIST:
            for (size_t i = 0; i < node->data.wordlist.count; i++) {
                free(node->data.wordlist.words[i]);
            }
            free(node->data.wordlist.words);
            break;
        case NODE_EOF:
            // 特別な処理は不要
            break;
        default:
            break;
    }
    free(node);
}
