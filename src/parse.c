#include "parse.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "grammar.h"

pFileLine newFileLine(const char* file_path, const char* text, uint line_num) {
    pFileLine flp = malloc(sizeof(*flp));
    *flp = (struct file_line_s){
        .line_num  = line_num,
        .file_path = file_path,
        .text      = strdup(text),
        .next      = NULL
    };
    return flp;
}

void delFileLine(pFileLine* flp) {
    if (flp==NULL || *flp==NULL) return;
    if ((*flp)->text) free((*flp)->text);
    if ((*flp)->next) delFileLine( &((*flp)->next) );
    free(*flp);
}

void fprintfFileLine(FILE* fp, const pFileLine flp) {
    if (flp == NULL) fprintf(fp, "(null)\n");
    else fprintf(fp, "%s:%u: %s\n", flp->file_path, flp->line_num, flp->text);
}
void dumpFileLine(const pFileLine flp) {
    fprintfFileLine(stdout, flp);
}

static void appendFileLine(pFileLine* head, pFileLine next) {
    if (head==NULL || *head==NULL)   *head        = next;
    else if ((*head)->next == NULL) (*head)->next = next;
    else appendFileLine( &((*head)->next), next );
}

/************************************************************/

pFileLine readFileAsLines(const char* file_path) {
    pFileLine file_as_lines = NULL;

    FILE* fp = fopen(file_path, "r");
    if (fp == NULL) {
        WARN("File (%s) does not exist", file_path);
        return NULL;
    }

    uint line_num = 0;
    char text[256] = {0};
    for (; fgets(text, 256, fp) != NULL; ) {
        // fgets(text, 256, fp);
        ++line_num;
        const uint len = strlen(text);
        if (len == 1) continue;
        if (text[len-1] == '\n') text[len-1] = 0;
        pFileLine flp = newFileLine(file_path, text, line_num);
        appendFileLine(&file_as_lines, flp);
    }

    pFileLine flp = newFileLine(file_path, text, line_num);
    appendFileLine(&file_as_lines, flp);

    fclose(fp);

    return file_as_lines;
}

/************************************************************/

static const char* strTokenType[NUM_TOKEN_TYPES] = {
    "invalid",
    #define TOKEN_TYPE(NAME) QUOTE(NAME),
        #include "tokens.h"
    #undef TOKEN_TYPE
};

static bool isDecDigit(const char c) {
    return c>='0' && c<='9';
}
static bool isHexDigit(const char c) {
    return
        (c>='A' && c<='F') ||
        (c>='a' && c<='f') ||
        isDecDigit(c);
}
static bool isAlpha(const char c) {
    return
        (c>='A' && c<='Z') ||
        (c>='a' && c<='z') ||
        c=='_';
}

static bool isIntConst(const char* s) {
    const char* t = s;
    if (*t == '-') t++;
    while (*t) {
        if (!isDecDigit(*t)) return false;
        t++;
    } return true;
}
static bool isHexConst(const char* s) {
    if (strlen(s)==1) return false;
    if (strlen(s)>2 && s[0]=='0' && s[1] == 'x') {
        const char* t = s+2;
        while (*t) {
            if (!isHexDigit(*t)) return false;
            t++;
        } return true;
    } return false;
}

#define DOUBLE_DECIMAL_COUNT 7
// #define DOUBLE_DECIMAL_COUNT 8
static bool isFloatConst(const char* s) {
    if (strlen(s)==1) return false;
    short after_dot_count = 0;
    short dot_count = 0;
    const char* t = s;
    if (*t == '-') t++;
    while (*t) {
        if (*t == '.') dot_count++;
        else if (*t=='f' && *(t+1)==0) break;
        else if (!isDecDigit(*t)) return false;
        t++;
        if (dot_count) after_dot_count++;
    }
    return dot_count == 1 && after_dot_count<=DOUBLE_DECIMAL_COUNT;
}
static bool isDoubleConst(const char* s) {
    if (strlen(s)==1) return false;
    short after_dot_count = 0;
    short dot_count = 0;
    const char* t = s;
    if (*t == '-') t++;
    while (*t) {
        if (*t == '.') dot_count++;
        else if (*t=='f' && *(t+1)==0) break;
        else if (!isDecDigit(*t)) return false;
        t++;
        if (dot_count) after_dot_count++;
    }
    return dot_count == 1 && after_dot_count>DOUBLE_DECIMAL_COUNT;
}

static bool isPossibleIdentifier(const char* s) {
    const char* t = s;
    if (!isAlpha(*t)) return false;
    t++;
    while (*t) {
        if (!isAlpha(*t)) return false;
        t++;
    } return true;
}

static enum TokenType tokenTypeFromStr(const char* s) {
    for (enum TokenType token_type = 0; token_type<NUM_TOKEN_TYPES; token_type++) {
        if (strcmp(s, strTokenType[token_type])==0) return token_type;
    }
    if (isPossibleIdentifier(s)) return TOKEN_identifier;
    return TOKEN_invalid;
}

static enum TokenType deduceTokenType(const char* s) {
    const size_t len = strlen(s);
    if      (len>2 && s[0]=='\"' && s[len-1]=='\"') return TOKEN_stringConst;
    else if (len>2 && s[0]=='\'' && s[len-1]=='\'') return TOKEN_charConst;
    else if (isIntConst(s))    return TOKEN_intConst;
    else if (isHexConst(s))    return TOKEN_hexConst;
    else if (isDoubleConst(s)) return TOKEN_doubleConst;
    else if (isFloatConst(s))  return TOKEN_floatConst;
    else if (s[0]=='#')        return TOKEN_macro;
    else if (isDelim(s[0]))    return TOKEN_operator;
    else if (strcmp(s,"NULL")==0)    return TOKEN_null;
    else if (strcmp(s,"__qbe__")==0) return TOKEN_qbe;
    return tokenTypeFromStr(s);
}

/************************************************************/

pToken newToken(const pFileLine origin, const char* text, const uint offset) {
    pToken ptok = malloc(sizeof(*ptok));
    *ptok = (struct token_s) {
        .offset = offset - strlen(text) + 1,
        .type   = deduceTokenType(text),
        .origin = origin,
        .text   = strdup(text),
        .next   = NULL
    };
    if (strlen(text)==1) ptok->offset += 1; /* Fixes weird bug w/ size 1 tokens */
    return ptok;
}
void delToken(pToken* tp) {
    if (tp==NULL || *tp==NULL) return;
    if ((*tp)->text) free((*tp)->text);
    if ((*tp)->next) delToken( &((*tp)->next) );
    free(*tp);
}
void fprintfToken(FILE* fp, const pToken tp) {
    if (tp == NULL) fprintf(fp, "(null)\n");
    // else printf("%s:%u:%u: %s [%s]\n",
    else fprintf(fp, "%s:%u:%u: (%-10s) %s\n",
        tp->origin->file_path, tp->origin->line_num, tp->offset,
        strTokenType[tp->type], tp->text
    );
}
void dumpToken(const pToken tp) {
    fprintfToken(stdout, tp);
}
void listTokens(const pToken tp) {
    if (tp != NULL) {
        printf("`%s` ", tp->text);
        listTokens(tp->next);
    }
}

void appendToken(pToken* head, pToken next) {
    if (head==NULL || *head==NULL)   *head        = next;
    else if ((*head)->next == NULL) (*head)->next = next;
    else appendToken( &((*head)->next), next );
}   

pToken splitTokens(pToken head) {
    pToken next = head->next;
    head->next = NULL;
    return next;
}
pToken pluckToken(pToken token) {
    pToken next = token->next;
    token->next = NULL;
    return next;
}

pToken tokenizeLine(const pFileLine flp) {
    if (flp == NULL || strlen(flp->text)==0) return NULL;

    pToken tokens = NULL;

    uint buffer_index =  0;
    char buffer[256]  = {0};
    #define pushToken(OFFSET) {\
        if (buffer_index) {\
            buffer[buffer_index] = 0;\
            appendToken(&tokens,\
                newToken(\
                    flp, buffer, OFFSET\
                )\
            );\
            buffer_index = 0;\
        }\
    }

    #define pushChar(C) { buffer[buffer_index++]=C; }

    uint offset;
    bool in_char   = false;
    bool in_string = false;
    for (offset = 0; offset<strlen(flp->text); offset++) {
        const char c1 = flp->text[offset];
        const char c2 = flp->text[offset+1];

        if (in_string) {
            pushChar(c1);
            if (c1 == '\"') {
                in_string = false;
                pushToken(offset);
            }
            continue;
        }
        if (in_char) {
            pushChar(c1);
            if (c1 == '\'') {
                in_char = false;
                pushToken(offset);
            }
            continue;
        }
        if (c1 == '\"') {
            pushToken(offset);
            in_string = true;
            pushChar(c1);
            continue;
        }
        if (c1 == '\'') {
            pushToken(offset);
            in_char = true;
            pushChar(c1);
            continue;
        }
        if (c1 == '/' && c2 == '/') {
            break; /* Break on comments */
        }
        if (c1 == ' ') {
            pushToken(offset);
            continue;
        }
        if (c1 == '.' && (isDecDigit(c2) || c2=='f')) {
            pushChar(c1);
            continue;
        }
        if (c1 == '-' && isDecDigit(c2)) {
            pushChar(c1);
            continue;
        }
        if (isOperator(c1, c2)) {
            pushToken(offset);
            pushChar(c1);
            pushChar(c2);
            pushToken(offset);
            offset++;
            continue;
        }
        if (isDelim(c1)) {
            pushToken(offset);
            pushChar(c1);
            pushToken(offset);
            continue;
        }

        pushChar(c1);
    }
    pushToken(offset);

    return tokens;
}