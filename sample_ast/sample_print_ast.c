#include "sample.h"
#include <stdio.h>

void print_indent(int depth) {
    for (int i = 0; i < depth; i++) {
        printf("  ");
    }
}

void print_ast(Node *node, int depth) {
    if (!node) return;

    print_indent(depth);
    switch (node->kind) {
        case NODE_COMMAND:
            printf("COMMAND: ");
            if (node->lhs && node->lhs->kind == NODE_WORDLIST) {
                for (size_t i = 0; i < node->lhs->data.wordlist.count; i++) {
                    printf("%s ", node->lhs->data.wordlist.words[i]);
                }
            }
            printf("\n");
            if (node->rhs) {
                print_ast(node->rhs, depth + 1);
            }
            break;
        case NODE_PIPE:
            printf("PIPE\n");
            print_ast(node->lhs, depth + 1);
            print_ast(node->rhs, depth + 1);
            break;
        case NODE_AND:
            printf("AND\n");
            print_ast(node->lhs, depth + 1);
            print_ast(node->rhs, depth + 1);
            break;
        case NODE_OR:
            printf("OR\n");
            print_ast(node->lhs, depth + 1);
            print_ast(node->rhs, depth + 1);
            break;
        case NODE_REDIRECT:
            printf("REDIRECT: ");
            for (size_t i = 0; i < node->data.redirect.count; i++) {
                printf("%s -> %s ", node->data.redirect.symbols[i], node->data.redirect.filenames[i]);
            }
            printf("\n");
            break;
        case NODE_WORDLIST:
            printf("WORDLIST: ");
            for (size_t i = 0; i < node->data.wordlist.count; i++) {
                printf("%s ", node->data.wordlist.words[i]);
            }
            printf("\n");
            break;
        default:
            printf("UNKNOWN NODE\n");
            break;
    }
}
