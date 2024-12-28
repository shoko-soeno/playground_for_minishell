#include <stdio.h>
#include <stdbool.h>
#include "sample.h"

#include <stdio.h>

// ヘルパー関数：インデントを印字
static void print_indent(int depth) {
    for (int i = 0; i < depth; i++) {
        printf("  ");
    }
}

// ASTの表示関数
void print_ast(Node *node, int depth) {
    if (node == NULL) {
        print_indent(depth);
        printf("NULL\n");
        return;
    }

    print_indent(depth);
    switch (node->kind) {
        case NODE_COMMAND:
            printf("Command\n");
            if (node->data.wordlist.count > 0) {
                print_indent(depth + 1);
                printf("Words: ");
                for (size_t i = 0; i < node->data.wordlist.count; i++) {
                    printf("%s ", node->data.wordlist.words[i]);
                }
                printf("\n");
            }
            if (node->rhs != NULL && node->rhs->kind == NODE_REDIRECT) {
                print_ast(node->rhs, depth + 1);
            }
            break;
        case NODE_PIPE:
            printf("Pipe |\n");
            print_ast(node->lhs, depth + 1);
            print_ast(node->rhs, depth + 1);
            break;
        case NODE_AND:
            printf("Logical AND &&\n");
            print_ast(node->lhs, depth + 1);
            print_ast(node->rhs, depth + 1);
            break;
        case NODE_OR:
            printf("Logical OR ||\n");
            print_ast(node->lhs, depth + 1);
            print_ast(node->rhs, depth + 1);
            break;
        case NODE_REDIRECT:
            printf("Redirection %s\n", node->data.redirect.symbol);
            print_ast(node->data.redirect.filename, depth + 1);
            break;
        case NODE_WORDLIST:
            printf("WordList (%zu)\n", node->data.wordlist.count);
            for (size_t i = 0; i < node->data.wordlist.count; i++) {
                print_indent(depth + 1);
                printf("WORD: %s\n", node->data.wordlist.words[i]);
            }
            break;
        case NODE_EOF:
            printf("EOF\n");
            break;
        default:
            printf("Unknown node type\n");
    }
}

