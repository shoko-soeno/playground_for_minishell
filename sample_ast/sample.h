#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

/*
this parser will create AST based on below eBNF.
    start: command
	command: simple_command command_tail
	command_tail: "|" simple_command command_tail 
            | ("&&" | "||") simple_command command_tail
            | redirection
			|
	simple_command: redirection* wordlist redirection* | redirection+
    wordlist: WORD | wordlist WORD
    redirection: ">" filename
    filename: WORD
    WORD: /[a-zA-Z0-9_"]+/
*/

typedef enum {
    NODE_COMMAND,       // シンプルコマンド
    NODE_PIPE,          // パイプ（|）
    NODE_AND,           // 論理AND（&&）
    NODE_OR,            // 論理OR（||）
    NODE_REDIRECT,   // リダイレクション
    NODE_WORDLIST,      // ワードリスト
    NODE_EOF    // End of File ノード
} NodeKind;

typedef struct Node {
    NodeKind kind;
    struct Node *lhs;
    struct Node *rhs;
    union {
        struct {
            char **words;
            size_t count;
        } wordlist;
        struct {
            char *symbol;
            struct Node *filename;
        } redirect;
    } data;
} Node;

/*
echo hello | grep h && cat < input.txt
Step 1:
    echo hello -> NODE_COMMAND
    grep h -> NODE_COMMAND
    cat < input.txt -> NODE_COMMAND - NODE_REDIRECTION
Step 2:
    echo hello | grep h -> NODE_PIPE
        lhs - echo hello
        rhs - grep h
Step 3:
    echo hello | grep h && cat < input.txt -> NODE_AND
        lhs - echo hello | grep h
        rhs - cat < input.txt

Logical AND &&
├── lhs-  Pipe |
│     ├── lhs - Command: echo hello
│     └── rhs - Command: grep h
└── rhs Command: cat < input.txt
*/

// トークン関連の定義（既存のものを維持）
typedef enum {
    TOKEN_WORD,
    TOKEN_PIPE,             // |
    TOKEN_AND,              // &&
    TOKEN_OR,               // ||
    TOKEN_REDIRECT,      // <
    TOKEN_EOF
} TokenType;

typedef struct Token {
    TokenType type;
    char *value;
    struct Token *next;
} t_token;

// レキサー関数のプロトタイプ
t_token *lexer(const char *input);

void *xmalloc(size_t size)
{
    void *p = malloc(size);
    if (!p) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    return p;
}

/*------------------ ASTNode Creation Functions------------------*/

// sample_ast/sample_create_ast.c
Node *new_node(NodeKind kind, Node *lhs, Node *rhs);
Node *create_wordlist_node(char **words, size_t count);
Node *create_redirect_node(char *symbol, Node *filename);
Node *create_command_node(char **words, size_t count, const char *redirect_symbol, char *filename);

// sample_ast/sample_parser.c
Node *parse_start(t_token *token_list);
void free_token_list(t_token *token);
void free_ast(Node *node);

// sample_ast/sample_execute.c
void execute_command_node(Node *node);
void execute_pipe_node(Node *node);
void execute_and_node(Node *node);
void execute_or_node(Node *node);
void execute_command(Node *node);

// sample_ast/sample_print_ast.c
void print_ast(Node *node, int depth);
