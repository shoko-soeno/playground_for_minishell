#include "sample.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static t_token *current_token;

static void parse_error(const char *message) {
    fprintf(stderr, "Parse error: %s\n", message);
    exit(EXIT_FAILURE);
}

static void advance() {
    if (current_token->type != TOKEN_EOF) {
        current_token = current_token->next;
    }
}

static bool expect_token(TokenType type) {
    return current_token->type == type;
}

Node *parse_wordlist() {
    if (!expect_token(TOKEN_WORD)) {
        parse_error("Expected WORD in wordlist");
    }

    size_t capacity = 4;
    size_t count = 0;
    char **words = malloc(sizeof(char *) * capacity);

    while (expect_token(TOKEN_WORD)) {
        if (count >= capacity) {
            capacity *= 2;
            words = realloc(words, sizeof(char *) * capacity);
        }
        words[count++] = strdup(current_token->value);
        advance();
    }

    Node *wordlist = create_wordlist_node(words, count);
    for (size_t i = 0; i < count; i++) {
        free(words[i]);
    }
    free(words);

    return wordlist;
}

Node *parse_redirect() {
    if (!expect_token(TOKEN_REDIRECT_IN) && !expect_token(TOKEN_REDIRECT_OUT)) {
        parse_error("Expected redirection token");
    }

    char *symbol = strdup(current_token->value);
    advance();

    if (!expect_token(TOKEN_WORD)) {
        parse_error("Expected filename after redirection");
    }

    char *filename = strdup(current_token->value);
    advance();

    return create_redirect_node(symbol, filename);
}

Node *parse_command() {
    Node *wordlist = parse_wordlist();
    Node *redirect = NULL;

    printf("Wordlist count: %zu\n", wordlist->data.wordlist.count);
    for (size_t i = 0; i < wordlist->data.wordlist.count; i++) {
        printf("Word: %s\n", wordlist->data.wordlist.words[i]);
    }

    while (expect_token(TOKEN_REDIRECT_IN) || expect_token(TOKEN_REDIRECT_OUT)) {
        Node *new_redirect = parse_redirect();
        if (!redirect) {
            redirect = new_redirect;
        } else {
            redirect->rhs = new_redirect;
        }
    }

    return create_command_node(wordlist->data.wordlist.words, wordlist->data.wordlist.count, redirect);
}

Node *parse_pipe() {
    Node *left = parse_command();

    while (expect_token(TOKEN_PIPE)) {
        advance();
        Node *right = parse_command();
        left = new_node(NODE_PIPE, left, right);
    }

    return left;
}

Node *parse_logical() {
    Node *left = parse_pipe();

    while (expect_token(TOKEN_AND) || expect_token(TOKEN_OR)) {
        NodeKind kind = current_token->type == TOKEN_AND ? NODE_AND : NODE_OR;
        advance();
        Node *right = parse_pipe();
        left = new_node(kind, left, right);
    }

    return left;
}

Node *parse_start(t_token *token_list) {
    current_token = token_list;
    Node *ast = parse_logical();

    if (current_token->type != TOKEN_EOF) {
        parse_error("Unexpected tokens after command");
    }

    return ast;
}

void free_ast(Node *node) {
    if (!node) return;

    free_ast(node->lhs);
    free_ast(node->rhs);

    if (node->kind == NODE_WORDLIST || node->kind == NODE_COMMAND) {
        for (size_t i = 0; i < node->data.wordlist.count; i++)
            free(node->data.wordlist.words[i]);
        free(node->data.wordlist.words);
    } else if (node->kind == NODE_REDIRECT) {
        for (size_t i = 0; i < node->data.redirect.count; i++) {
            free(node->data.redirect.symbols[i]);
            free(node->data.redirect.filenames[i]);
        }
        free(node->data.redirect.symbols);
        free(node->data.redirect.filenames);
    }

    free(node);
}
