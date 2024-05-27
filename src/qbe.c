#include "qbe.h"

#include <stdbool.h>
#include <string.h>

#include "flags.h"
#include "token_types.h"

static enum QbeType getQbeType(const enum TokenType type) {
    switch (type) {
        // case TOKEN_char  : return QBE_Byte;
        case TOKEN_char  : return QBE_Word;
        case TOKEN_int   : return QBE_Word;
        case TOKEN_long  : return QBE_Long;
        case TOKEN_float : return QBE_Single;
        case TOKEN_double: return QBE_Double;
        default: break;
    }
    return QBE_Word;
}

enum GrammarUnit {
    GU_Invalid=0,

    /* Proto-Grammar Chains */
    GU_Adjective_Chain,
    GU_Decl_Chain,

    /* Absolutes */
    GU_Fun_Decl,
    GU_Fun_Defn,
    GU_Var_Decl,
    GU_Var_Defn,
    GU_Ret_Stmt,

    GU_New_Scope,
    GU_End_Scope,

    GU_New_Args,
    GU_End_Args,

    GU_New_Index,
    GU_End_Index,

    GU_Expr_Or_Call,
    GU_Fun_Call,
    GU_Expression,

    GU_Qbe_Call,
};

static const char* strGrammarUnit[] = {
    "Invalid",
    "AdjChain",
    "DeclChain",
    "FunDecl",
    "FunDefn",
    "VarDecl",
    "VarDefn",
    "RetStmt",

    "NewScope",
    "EndScope",

    "NewArgs",
    "EndArgs",

    "NewIndex",
    "EndIndex",

    "ExprOrCall",
    "FunCall",
    "Expression",
    "QbeCall"
};

static enum GrammarUnit predictGrammar(const enum GrammarUnit gu, const pToken lhs, const pToken rhs) {
    const enum TokenType lht = lhs->type, rht = rhs ? rhs->type : TOKEN_invalid;

    /* Start building a chain or no chain necessary */
    if (gu==GU_Invalid) {
        if (strcmp(lhs->text, "{")==0)       return GU_New_Scope;
        if (strcmp(lhs->text, "}")==0)       return GU_End_Scope;
        if (strcmp(lhs->text, "(")==0)       return GU_New_Args;
        if (strcmp(lhs->text, ")")==0)       return GU_End_Args;
        if (strcmp(lhs->text, "[")==0)       return GU_New_Index;
        if (strcmp(lhs->text, "]")==0)       return GU_End_Index;
        if (strcmp(lhs->text, "__qbe__")==0) return GU_Qbe_Call;
        if (isConst(lhs->type))              return GU_Expression;

        if (lht == TOKEN_return)             return GU_Ret_Stmt;
        if (lht == TOKEN_identifier) {
            if (strcmp(rhs->text, "(")==0)   return GU_Fun_Call;
            return GU_Expr_Or_Call;
        }
        if (isAdjective(lht)) return GU_Adjective_Chain;
        if (rhs) {
            if (isAdjective(lht)) {
                if (isAdjective(rht))  return GU_Adjective_Chain;
                if (isIdentifier(rht)) return GU_Decl_Chain;
            }
            if (isType(lht)) {
                if (isIdentifier(rht)) return GU_Decl_Chain;
            }
        }

        return GU_Invalid;
    }
    if (rhs == NULL) return gu;

    /* Chain must exist to proceed */
    switch (gu) {
        default: break;

        case GU_Adjective_Chain:
            if (isAdjective(rht))          return GU_Adjective_Chain;
            if (isType(rht))               return GU_Decl_Chain;
            if (isIdentifier(rht))         return GU_Decl_Chain;
            if (strcmp(rhs->text, "*")==0) return GU_Adjective_Chain;
            return GU_Invalid;

        case GU_Decl_Chain:
            if (strcmp(rhs->text, "*")==0) return GU_Decl_Chain;
            if (strcmp(rhs->text, "(")==0) return GU_Fun_Decl;
            if (strcmp(rhs->text, "[")==0) return GU_Decl_Chain;
            if (strcmp(rhs->text, ";")==0) return GU_Var_Decl;
            if (isAssignmentOperator(rhs->text)) return GU_Var_Defn;
            return GU_Decl_Chain;

        case GU_Var_Defn:
            if (strcmp(lhs->text, "=")==0 && strcmp(rhs->text, ";")==0)
                ERRO(EXIT_FAILURE, "No value provided for variable declaration!");
            return GU_Var_Defn;

        case GU_Expr_Or_Call:
            if (isAssignmentOperator(rhs->text)) return GU_Expression;
            if (strcmp(rhs->text, "(")==0)       return GU_Fun_Call;
            return GU_Expression;
    }

    return gu;
}

static enum GrammarUnit predictGrammarTokens(FILE* fp, const pToken tokens) {
    pToken lhs = tokens;
    if (lhs==NULL) return GU_Invalid;
    if (global_VERBOSE) dumpFileLine(lhs->origin);

    pToken rhs = tokens->next;
    enum GrammarUnit possible_grammar = GU_Invalid;
    while (rhs) {
        possible_grammar = predictGrammar(possible_grammar, lhs, rhs);
        
        if (global_VERBOSE) printf("[DEBG] Prediction: %s [%s] vs [%s]\n", strGrammarUnit[possible_grammar], lhs->text, rhs->text);

        lhs = rhs;
        rhs = rhs->next;
    }
    possible_grammar = predictGrammar(possible_grammar, lhs, NULL);
    if (global_VERBOSE) printf("[DEBG] Final Prediction: %s [%s]\n\n", strGrammarUnit[possible_grammar], lhs->text);
    return possible_grammar;
}

#define DATA_BLOCK_LENGTH 512
typedef char Block[DATA_BLOCK_LENGTH];

static uint global_ConstCounter = 0;
static void compileInlineQbe(FILE* fp, const pToken tokens, const enum GrammarUnit grammar, Block data_seg) {
    /* First pass to pull out data segment constants */

    pToken temp = tokens->next; /* Skip the `__qbe__` keyword */
    pToken builtin = tokens->next;
    uint   fmt_id  = 0;
    while (temp) {
        if (temp->type == TOKEN_stringConst) {
            if (!fmt_id) fmt_id = global_ConstCounter; /* Only the first one could be a `printf` format string */
            char buf[64] = {0};
            sprintf(buf, "data $s_const_%u = { b %s, b 0 }\n", global_ConstCounter++, temp->text);
            strncat(data_seg, buf, 64);
            break;
        }
        temp = temp->next;
    }

    if (strcmp(builtin->text, "printf")==0) {
        fprintf(fp, "\tcall $printf(l $s_const_%u, ...)\n", global_ConstCounter); 
    }
}

static void compileGrammar(FILE* fp, const pToken tokens, const enum GrammarUnit grammar, Block data_seg) {
    /* Example:
        function w $add(w %a, w %b) {              # Define a function add
        @start
            %c =w add %a, %b                   # Adds the 2 arguments
            ret %c                             # Return the result
        }
        export function w $main() {                # Main function
        @start
            %r =w call $add(w 1, w 1)          # Call add(1, 1)
            call $printf(l $fmt, ..., w %r)    # Show the result
            ret 0
        }
        data $fmt = { b "One and one make %d!\n", b 0 }
    */
    static bool needs_auto_ret = true;   // FIXME: Doesn't seem to work
    static pToken ret_type_token = NULL; // FIXME: Doesn't seem to work
    pToken temp = tokens;
    char assignment[64] = {0};
    switch (grammar) {
        default: break;

        case GU_Fun_Decl: {
            enum TokenType ret_type = TOKEN_invalid;
            const char* identifier = "";
            const char* args       = "";

            while (temp) {
                if (isType(temp->type)) {
                    ret_type_token = temp;
                    ret_type   = temp->type;
                }
                if (isIdentifier(temp->type)) identifier = temp->text;
                temp = temp->next;
            }

            if (ret_type != TOKEN_void) {
                needs_auto_ret = false; // FIXME: Fails if you declare functions in a scope? Is this even common?
            }

            if (strcmp(identifier, "main")==0) fprintf(fp, "export ");
            fprintf(fp, "function %s $%s(%s) {\n",
                qbeType2str[getQbeType(ret_type)],
                identifier,
                args
            );
            fprintf(fp, "@start\n");
            break;
        }

        case GU_Fun_Call: {
            const char* identifier = temp->text;
            const char* args = "";
            fprintf(fp, "\tcall $%s(%s)\n", identifier, args);
            break;
        }

        case GU_Var_Defn:
        case GU_Var_Decl: {
            enum TokenType var_type = TOKEN_invalid;
            const char* identifier = "";
            bool using_data_seg = false;
            sprintf(assignment, (grammar == GU_Var_Decl) ? "0" : "");

            while (temp) {
                if (isType(temp->type))       var_type   = temp->type;
                if (isIdentifier(temp->type)) identifier = temp->text;
                if (isConst(temp->type))      {
                    switch (temp->type) {
                        default: break;
                        case TOKEN_intConst   : sprintf(assignment, "%ld", strtol(temp->text, NULL, 10)); break;
                        case TOKEN_hexConst   : sprintf(assignment, "%ld", strtol(temp->text, NULL, 16)); break;
                        case TOKEN_floatConst : sprintf(assignment,  "s_%f", strtof(temp->text, NULL    )); break;
                        case TOKEN_doubleConst: {
                            if (var_type == TOKEN_float) ERRO(EXIT_FAILURE, "Replace `float` with `double` to store this amount of precision"); 
                            sprintf(assignment,  "d_%f", strtod(temp->text, NULL    ));
                            break;
                        }
                        case TOKEN_charConst  : sprintf(assignment,  "%d", temp->text[1]); break;
                        case TOKEN_stringConst: {
                            using_data_seg = true;
                            char buf[64] = {0};
                            sprintf(buf, "data $s_const_%u = { b %s, b 0 }\n", global_ConstCounter++, temp->text);
                            strncat(data_seg, buf, 64);
                            break;
                        }
                    }
                }
                temp = temp->next;
            }

            if (!using_data_seg)
                fprintf(fp, "\t%%%s =%s sub 0, %s\n", identifier, qbeType2str[getQbeType(var_type)], assignment);
            break;
        }

        case GU_Expression: {
            /* First pass to pull out data segment constants */

            while (temp) {
                if (temp->type == TOKEN_stringConst) {
                    char buf[64] = {0};
                    sprintf(buf, "data $s_const_%u = { b %s, b 0 }\n", global_ConstCounter++, temp->text);
                    strncat(data_seg, buf, 64);
                    break;
                }
                temp = temp->next;
            }
            break;
        }

        case GU_Qbe_Call: {
            compileInlineQbe(fp, tokens, grammar, data_seg);
            break;
        }

        case GU_Ret_Stmt: {
            needs_auto_ret = false;
            ret_type_token = NULL; // FIXME: Dirty hack

            const char* ret_val = "0";
            fprintf(fp, "\tret %s\n", ret_val);
            // FIXME: Type match checking with return value in function header!
            break;
        }

        case GU_End_Scope: {
            if (needs_auto_ret || ret_type_token) { // FIXME: Will crap out for nested scopes like if/while/for/etc.
                // if (ret_type_token) {
                //     WARN("Missing return statement around function end:");
                //     dumpFileLine(tokens->origin);
                //     printf("\n");
                // }

                fprintf(fp, "\tret 0\n");
                needs_auto_ret = true;
                ret_type_token = NULL;
            }
            fprintf(fp, "}\n\n");
            break;
        }
    }
}

void compileSyntaxNode(FILE* fp, const pSyntaxNode snode, Block data_seg) {
    if (snode->tokens == NULL) return; /* Master node for file has no tokens  */
    if (strcmp(snode->tokens->text, ";")==0) return; /* Extraneous semicolons */

    static uint line_to_print = 1;
    if (snode->tokens) {
        const pFileLine curr_line = snode->tokens->origin;
        if (curr_line->line_num >= line_to_print) {
            fprintf(fp, "# "); fprintfFileLine(fp, snode->tokens->origin);
            line_to_print = curr_line->line_num+1;
        }
    }

    const enum GrammarUnit grammar = predictGrammarTokens(fp, snode->tokens);
    if (grammar == GU_Invalid) ERRO(EXIT_FAILURE, "Syntax Error");
    compileGrammar(fp, snode->tokens, grammar, data_seg);
}

void compileTree(FILE* fp, const pSyntaxNode head, Block data_seg) {
    if (head == NULL) return;
    compileSyntaxNode(fp, head, data_seg);
    compileTree(fp, head->children, data_seg);
    compileTree(fp, head->next, data_seg);
}

void compileFile(const char* output_path, pSyntaxNode master) {
    Block data_seg = {0};
    
    FILE* fp = fopen("temp.ssa", "w");
    compileTree(fp, master, data_seg);

    /* Dump out data segment at very bottom */
    if (data_seg[0] != 0) fprintf(fp, "\n# Data Segment\n%s\n", data_seg); /* O(1) vs O(n) `strlen` */
    if (fp) fclose(fp);
}