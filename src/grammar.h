#ifndef QUEBEC_GRAMMAR_H
#define QUEBEC_GRAMMAR_H

#include <stdbool.h>

static inline bool isDelim(const char c) {
    switch (c) {
        case '~': case '!': case '%': case '^': case '&':
        case '*': case '(': case ')': case '-': case '+':
        case '=': case '{': case '}': case '[': case ']':
        case '|': case '\\': case ':': case ';': case '\"':
        case '\'': case '<': case ',': case '>': case '.':
        case '/': case '?': case '#': return true;
        default:            return false;
    }
}

static inline bool isOperator(const char c1, const char c2) {
    return (
        (c1=='=' && c2=='=') ||
        (c1=='!' && c2=='=') ||
        (c1=='<' && c2=='=') ||
        (c1=='>' && c2=='=') ||
        (c1=='~' && c2=='=') ||
        (c1=='+' && c2=='=') ||
        (c1=='-' && c2=='=') ||
        (c1=='*' && c2=='=') ||
        (c1=='/' && c2=='=') ||
        (c1=='%' && c2=='=') ||
        (c1=='-' && c2=='>') ||
        (c1=='<' && c2=='<') ||
        (c1=='>' && c2=='>') ||
        0
    );
}

#endif /* QUEBEC_GRAMHAR_H */