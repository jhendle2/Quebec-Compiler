#ifndef QUEBEC_PARSER_H
#define QUEBEC_PARSER_H

#include "common.h"

typedef struct file_line_s {
    uint  line_num;
    const char* file_path;
    char* text;
    struct file_line_s* next; 
} *pFileLine;

pFileLine newFileLine(const char* file_path, const char* text, uint line_num);
void      delFileLine(pFileLine* flp);
void      dumpFileLine(const pFileLine flp);

pFileLine readFileAsLines(const char* file_path);

/************************************************************/

#define CAT(A,B) A##B
#define QUOTE(S) #S

enum TokenType {
    TOKEN_invalid=0,
    #define TOKEN_TYPE(NAME) CAT(TOKEN_,NAME),
        #include "tokens.h"
    #undef TOKEN_TYPE
NUM_TOKEN_TYPES
};

typedef struct token_s {
    uint offset;
    enum TokenType type;
    pFileLine origin;
    char* text;
    struct token_s* next;
} *pToken;

pToken newToken(const pFileLine origin, const char* text, const uint offset);
void   delToken(pToken* tp);
void   dumpToken(const pToken tp);
void   listTokens(const pToken tp);
void   appendToken(pToken* head, pToken next);

void   fprintfFileLine(FILE* fp, const pFileLine flp);
void   fprintfToken    (FILE* fp, const pToken    tp);

pToken tokenizeLine(const pFileLine flp);
pToken splitTokens(pToken head);
pToken pluckToken(pToken token);

#endif /* QUEBEC_PARSER_H */