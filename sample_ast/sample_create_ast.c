#include "sample.h"
#include <stdlib.h>
#include <string.h>

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = malloc(sizeof(Node));
    if (!node) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    memset(&node->data, 0, sizeof(node->data));
    return node;
}

Node *create_wordlist_node(char **words, size_t count) {
    Node *node = new_node(NODE_WORDLIST, NULL, NULL);
    node->data.wordlist.words = malloc(count * sizeof(char *));
    for (size_t i = 0; i < count; i++) {
        node->data.wordlist.words[i] = strdup(words[i]);
    }
    node->data.wordlist.count = count;
    return node;
}

Node *create_redirect_node(char *symbol, char *filename) {
    Node *node = new_node(NODE_REDIRECT, NULL, NULL);
    node->data.redirect.symbols = malloc(sizeof(char *));
    node->data.redirect.filenames = malloc(sizeof(char *));
    node->data.redirect.symbols[0] = strdup(symbol);
    node->data.redirect.filenames[0] = strdup(filename);
    node->data.redirect.count = 1;
    return node;
}

Node *create_command_node(char **words, size_t count, Node *redirect) {
    Node *wordlist = create_wordlist_node(words, count);
    char **args = malloc((count + 1) * sizeof(char *));
    for (size_t i = 0; i < count; i++) {
        args[i] = strdup(words[i]);
    }
    args[count] = NULL;

    Node *command = new_node(NODE_COMMAND, wordlist, redirect);
    command->data.wordlist.words = args;
    command->data.wordlist.count = count;
    return command;
}