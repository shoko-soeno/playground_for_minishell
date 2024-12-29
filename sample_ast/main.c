#include "sample.h"
#include <stdio.h>

void test_lexer() {
    const char *input = "echo hello && grep world || cat file.txt >> output.txt";
    t_token *tokens = lexer(input);

    printf("Tokens:\n");
    while (tokens) {
        printf("Type: %d, Value: %s\n", tokens->type, tokens->value);
        tokens = tokens->next;
    }

    free_token_list(tokens);
}

void test_parser() {
    const char *input = "echo hello && grep world || cat file.txt > output.txt";
    t_token *tokens = lexer(input);

    Node *ast = parse_start(tokens);
    printf("AST:\n");
    print_ast(ast, 0);

    free_ast(ast);
    free_token_list(tokens);
}

void test_execution(char *envp[]) {
    const char *input = "echo hello && echo success || echo failure";
    t_token *tokens = lexer(input);

    Node *ast = parse_start(tokens);
    print_ast(ast, 0);
    printf("Executing commands:\n");
    execute_command(ast, envp);

    free_ast(ast);
    free_token_list(tokens);
}

int main(int argc, char *argv[], char *envp[]) {
    // printf("=== Testing Lexer ===\n");
    // test_lexer();

    // printf("\n=== Testing Parser ===\n");
    // test_parser();

    (void) argc;
    (void) argv;
    // while (*envp) {
    //     printf("DEBUG %s\n", *envp++);
    // }
    printf("\n=== Testing Execution ===\n");
    test_execution(envp);

    return 0;
}
