#include "lexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

pSyntaxNode newSyntaxNode(const pToken tokens) {
    pSyntaxNode snp = malloc(sizeof(*snp));
    snp->type     = NT_INVALID;
    snp->tokens   = tokens;
    snp->parent   = NULL;
    snp->next     = NULL;
    snp->children = NULL;
    return snp;
}
void delSyntaxNode(pSyntaxNode* head) {
    if (head == NULL || *head == NULL) return;
    delToken( &((*head)->tokens) );
    delSyntaxNode( &((*head)->next) );
    delSyntaxNode( &((*head)->children) );
    free(*head);
    *head = NULL;
}
void dumpSyntaxTree(const pSyntaxNode head, const size_t level) {
    if (head == NULL) return;
    for (size_t i = 0; i<level; i++) printf(" * ");
    listTokens(head->tokens);
    printf("\n");
    // printf("(%s)\n", nodeType2Str[head->type]);
    dumpSyntaxTree(head->children, level+1);
    dumpSyntaxTree(head->next, level);
}
void appendSyntaxNode(pSyntaxNode* head, pSyntaxNode next) {
    if (head==NULL || *head==NULL)   *head        = next;
    else if ((*head)->next == NULL) (*head)->next = next;
    else appendSyntaxNode( &((*head)->next), next );
}
void addChildSyntaxNode(pSyntaxNode parent, pSyntaxNode child) {
    child->parent = parent;
    appendSyntaxNode( &(parent->children), child );
}

static bool isUpToken(const pToken token) {
    return (
        strcmp(token->text, "}")==0 ||
        strcmp(token->text, "]")==0 ||
        strcmp(token->text, ")")==0
    );
}

static bool isDownToken(const pToken token) {
    return (
        strcmp(token->text, "{")==0 ||
        strcmp(token->text, "[")==0 ||
        strcmp(token->text, "(")==0
    );
}

static bool isStatementToken(const pToken token) {
    return strcmp(token->text, ";")==0;
}

pSyntaxNode buildTreeFromTokens(const pToken tokens) {
    pSyntaxNode master  = newSyntaxNode(NULL);
    pSyntaxNode current = master;

    pToken token = tokens;
    pSyntaxNode temp = newSyntaxNode(NULL);
    while (token) {

        pToken next = pluckToken(token);
        appendToken( &(temp->tokens), token);

        if (isDownToken(token)) {
            addChildSyntaxNode(current, temp);
            current = temp;
            temp    = newSyntaxNode(NULL);
        }
        if (isUpToken(token)) {
            addChildSyntaxNode(current, temp);
            current = current->parent;
            temp    = newSyntaxNode(NULL);
        }
        if (isStatementToken(token)) {
            addChildSyntaxNode(current, temp);
            temp = newSyntaxNode(NULL);
        }

        token = next;
    }
    if (temp != NULL) delSyntaxNode(&temp); /* We clearly didn't use this so delete it */

    return master;
}