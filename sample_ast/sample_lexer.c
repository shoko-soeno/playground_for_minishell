#include "sample.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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

t_token *lexer(const char *input) {
    t_token *head = NULL;
    t_token *current = NULL;

    while (*input) {
        if (isspace(*input)) {
            input++;
            continue;
        }

        t_token *new_token = NULL;

        if (*input == '|') {
            if (*(input + 1) == '|') {
                new_token = create_token(TOKEN_OR, "||");
                input += 2;
            } else {
                new_token = create_token(TOKEN_PIPE, "|");
                input++;
            }
        } else if (*input == '&') {
            if (*(input + 1) == '&') {
                new_token = create_token(TOKEN_AND, "&&");
                input += 2;
            } else {
                fprintf(stderr, "Unsupported token: &\n");
                exit(EXIT_FAILURE);
            }
        } else if (*input == '<') {
            new_token = create_token(TOKEN_REDIRECT_IN, "<");
            input++;
        } else if (*input == '>') {
            if (*(input + 1) == '>') {
                new_token = create_token(TOKEN_REDIRECT_OUT, ">>");
                input += 2;
            } else {
                new_token = create_token(TOKEN_REDIRECT_OUT, ">");
                input++;
            }
        } else {
            const char *start = input;
            while (*input && !isspace(*input) && !strchr("|&<>", *input)) input++;
            new_token = create_token(TOKEN_WORD, strndup(start, input - start));
        }

        if (!head) {
            head = current = new_token;
        } else {
            current->next = new_token;
            current = new_token;
        }
    }

    if (!head) {
        head = create_token(TOKEN_EOF, NULL);
    } else {
        current->next = create_token(TOKEN_EOF, NULL);
    }

    return head;
}

void free_token_list(t_token *token) {
    while (token) {
        t_token *next = token->next;
        if (token->value) free(token->value);
        free(token);
        token = next;
    }
}
