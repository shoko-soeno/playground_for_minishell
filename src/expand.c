#include <stdlib.h>
#include "minishell.h"
#include <string.h>

void    append_char(char **s, char c)
{
    size_t size;
    char *new;

    size = 2;
    if (*s)
        size += strlen(*s);
    new = malloc(size);
    if (new == NULL)
        fatal_error("malloc");
    if (*s)
        my_strlcpy(new, *s, size);
    new[size - 2] = c;
    new[size - 1] = '\0';
    if (*s)
        free(*s);
    *s = new;
}

void    quote_removal(t_token *tok)
{
    char *new_word;
    char *p;
    
    if (tok == NULL || tok->kind == TK_EOF || tok->word == NULL)
        return;
    p = tok->word;
    new_word = NULL;
    while(*p && !is_metacharacter(*p))
    {
        if (*p == SINGLE_QUOTE_CHAR)
        {
            p++; //skip quote
            while (*p && *p != SINGLE_QUOTE_CHAR)
            {
                if (*p == '\0')
                    assert_error("Unclosed single quote");
                append_char(&new_word, *p);
                p++;
            }
            p++;
        }
        else if (*p == DOUBLE_QUOTE_CHAR)
        {
            p++; //skip quote
            while (*p && *p != DOUBLE_QUOTE_CHAR)
            {
                if (*p == '\0')
                    assert_error("Unclosed double quote");
                append_char(&new_word, *p);
                p++;
            }
            p++; //skip quote
        }
        else
        {
            append_char(&new_word, *p);
            p++;
        }
    }
    free(tok->word);
    tok->word = new_word;
    quote_removal(tok->next);
}

void    expand(t_token *tok)
{
    quote_removal(tok);
}