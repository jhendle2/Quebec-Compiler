#ifndef QUEBEC_LEXER_H
#define QUEBEC_LEXER_H

#include "parse.h"

enum NodeType {
    NT_INVALID=0,
    NT_DECL,
    NT_FN_HEADER,
    NT_VAR_DECL,
    NT_NEW_SCOPE,
    NT_END_SCOPE,
    NT_ARGS_LIST,
    NT_RETURN,
NT_LENGTH
};

__attribute_maybe_unused__ static const char* nodeType2Str[NT_LENGTH] = {
    "Invalid",
    "Decl",
    "Fn Header",
    "Var Decl",
    "New Scope",
    "End Scope",
    "Args List",
    "Return",
};

typedef struct syntax_node_s {
    enum NodeType type;
    pToken tokens;
    struct syntax_node_s *parent, *next, *children;
} *pSyntaxNode;

pSyntaxNode newSyntaxNode(const pToken tokens);
void   delSyntaxNode(pSyntaxNode* head);
void   dumpSyntaxTree(const pSyntaxNode head, const size_t level);
void   appendSyntaxNode(pSyntaxNode* head, pSyntaxNode next);
void   addChildSyntaxNode(pSyntaxNode parent, pSyntaxNode child);

pSyntaxNode buildTreeFromTokens(const pToken tokens);

#endif /* QUEBEC_LEXER_H */