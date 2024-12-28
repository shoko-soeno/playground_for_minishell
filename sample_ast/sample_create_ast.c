#include <stdio.h>
#include <stdbool.h>
#include "sample.h"

static void *xmalloc(size_t size)
{
    void *p = malloc(size);
    if (!p) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    return p;
}

/*------------------ ASTNode Creation Functions------------------*/

Node *new_node(NodeKind kind, Node *lhs, Node *rhs)
{
    Node *node = calloc(1, sizeof(Node));
    if (!node) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    memset(&node->data, 0, sizeof(node->data)); // Clear the data union
    return node;
}

Node *create_wordlist_node(char **words, size_t count)
{
    Node *node = new_node(NODE_WORDLIST, NULL, NULL);
    node->data.wordlist.words = malloc(count * sizeof(char *));
    for (size_t i = 0; i < count; i++) {
        node->data.wordlist.words[i] = strdup(words[i]);
    }
    node->data.wordlist.count = count;
    return node;
}

Node *create_redirect_node(const char *symbol, Node *filename)
{
    Node *node = new_node(NODE_REDIRECT, NULL, filename);
    node->data.redirect.symbol = strdup(symbol);
    return node;
}

Node *create_command_node(char **words, size_t count, const char *redirect_symbol, char *filename)
{
    Node *wordlist = create_wordlist_node(words, count);
    Node *redirect = NULL;
    if (redirect_symbol && filename) {
        Node *filename_node = create_redirect_node(&filename, 1);
        redirect = create_redirect_node(redirect_symbol, filename_node);
    }
    Node *command = new_node(NODE_COMMAND, wordlist, redirect);
    return command;
}


/*------------------ Main Function ------------------*/

int main() {
    // レキサーを呼び出してトークンリストを取得（既存の実装を使用）
    t_token *tokens = lexer("echo hello | grep h && cat < input.txt");

    // パーサーを呼び出してASTを構築
    Node *ast = parse_start(tokens);

    // ASTを表示
    print_ast(ast, 0);

    // ASTを基にコマンドを実行
    execute_command(ast);

    // メモリを解放
    free_ast(ast);
    free_token_list(tokens);

    return 0;
}


