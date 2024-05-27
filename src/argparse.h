#ifndef TUCKY_PARSER_H
#define TUCKY_PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct tucky_arg_s {
    const char*  txt;
    struct tucky_arg_s* next;
} *TuckyArg;
#define TUCKY_NO_ARGS NULL

enum TuckyArgumentStatus {
    OPTIONAL=0,
    REQUIRED=1
};

typedef struct tucky_argument_s {
    enum TuckyArgumentStatus status;
    bool     enabled;
    char     flag;
    uint     nargs;
    const char*    keyword;
    const char*    help;
    TuckyArg args;
    struct tucky_argument_s* next;
} *TuckyArgument;

typedef struct {
    int     argc;
    char**  argv;
    char*   invoker;
    TuckyArgument args;
} TuckyArgParser;

/****************************************************************/

static inline void addArgument(
    TuckyArgParser* parser,
    const char flag, const char* keyword, 
    const uint nargs, 
    const enum TuckyArgumentStatus status,
    const char* help);

static inline void delArgParser(TuckyArgParser parser);

static inline void printArguments(const TuckyArgParser parser);
static inline void dumpArgs(const TuckyArgParser parser);

static inline TuckyArgument getArgumentFromFlag   (TuckyArgParser parser, const char flag);
static inline TuckyArgument getArgumentFromKeyword(TuckyArgParser parser, const char* keyword);

static inline TuckyArgParser newArgParser(int argc, char** argv);
static inline void parseArgs(TuckyArgParser parser);

#define STORE_TRUE 0
#define AS_MANY   -1
#define NARGS(N)   N

/****************************************************************/

static inline TuckyArg newArg(const char* txt) {
    TuckyArg arg = (TuckyArg)malloc(sizeof(struct tucky_arg_s));
    arg->txt  = txt;
    arg->next = NULL;
    return arg;
}

static inline void appendArg(TuckyArg* head_ptr, TuckyArg arg) {
    if (head_ptr==NULL || (*head_ptr)==NULL) {
        (*head_ptr) = arg;
        return;
    }

    if ( (*head_ptr)->next == NULL) {
        (*head_ptr)->next = arg;
        return;
    }

    appendArg( &((*head_ptr)->next), arg);
}

static inline void printArg(const TuckyArg arg) {
    if (arg) {
        printf("(%s) ", arg->txt);
        printArg(arg->next);
    }
}

static inline void delArg(TuckyArg* arg_ptr) {
    if (arg_ptr==NULL || *arg_ptr==NULL) return;
    if ((*arg_ptr)->next) delArg(&((*arg_ptr)->next));
    free(*arg_ptr);
    (*arg_ptr) = NULL;
}

static inline TuckyArgument newArgument(
    const char flag, const char* keyword, 
    const uint nargs,
    const enum TuckyArgumentStatus status,
    const char* help) {
        
    TuckyArgument argument = (TuckyArgument)malloc(sizeof(struct tucky_argument_s));
    argument->flag    = flag;
    argument->nargs   = nargs;
    argument->keyword = keyword;
    argument->help    = help;
    argument->status  = status;
    argument->enabled = false;
    argument->args    = TUCKY_NO_ARGS;
    argument->next    = NULL;

    return argument;
}

static inline void appendTuckyArgument(TuckyArgument* head_ptr, TuckyArgument argument) {
    if (head_ptr==NULL || (*head_ptr)==NULL) {
        (*head_ptr) = argument;
        return;
    }

    if ( (*head_ptr)->next == NULL) {
        (*head_ptr)->next = argument;
        return;
    }

    appendTuckyArgument( &((*head_ptr)->next), argument);
}

static inline void addArgument(
    TuckyArgParser* parser,
    const char flag, const char* keyword, 
    const uint nargs, 
    const enum TuckyArgumentStatus status,
    const char* help) {

    TuckyArgument argument = newArgument(flag, keyword, nargs, status, help);
    appendTuckyArgument(&(parser->args), argument);

    return;
}

static inline void delArgument(TuckyArgument* arg_ptr) {
    if (arg_ptr==NULL || *arg_ptr==NULL) return;
    delArgument(&((*arg_ptr)->next));
    delArg(&((*arg_ptr)->args));
    free(*(arg_ptr));
    (*arg_ptr) = NULL;
}

static inline void delArgParser(TuckyArgParser parser) {
    delArgument(&(parser.args));
}

static inline void printArguments(const TuckyArgParser parser) {
    printf("==== %s ====\n", parser.invoker);
    TuckyArgument temp = parser.args;
    while (temp) {
        if (temp->nargs == AS_MANY)
             printf("  -%c --%-10s [∞] : %s\n",
                temp->flag, temp->keyword,
                             temp->help);
        else printf("  -%c --%-10s [%u] : %s\n",
                temp->flag, temp->keyword,
                temp->nargs, temp->help);
        temp = temp->next;
    }
}
static inline void dumpArgs(const TuckyArgParser parser) {
    printf("==== DUMP %s ====\n", parser.invoker);
    TuckyArgument temp = parser.args;
    while (temp) {
        printf("%10s : ", temp->keyword);
        if (temp->nargs > 0) printArg(temp->args);
        else printf("%s", temp->enabled?"ENABLED":"DISABLED");
        printf("\n");
        temp = temp->next;
    }
}

static inline TuckyArgument getArgumentFromFlag   (TuckyArgParser parser, const char flag) {
    TuckyArgument argument = parser.args;
    while (argument) {
        if (argument->flag==flag) return argument;
        argument = argument->next;
    }
    return NULL;
}

static inline TuckyArgument getArgumentFromKeyword(TuckyArgParser parser, const char* keyword) {
    TuckyArgument argument = parser.args;
    while (argument) {
        if (strcmp(argument->keyword, keyword)==0) return argument;
        argument = argument->next;
    }
    return NULL;
}

static TuckyArgParser TUCKY_ACTIVE_PARSER;

static inline void TUCKY_EXIT() {
    delArgParser(TUCKY_ACTIVE_PARSER);
    exit(1);
}
#define TUCKY_EXIT_MSG(...) { printf(__VA_ARGS__); printf("\n"); TUCKY_EXIT(); }

static bool TUCKY_HAS_A_PARSER = false;
TuckyArgParser newArgParser(int argc, char** argv) {
    if (TUCKY_HAS_A_PARSER)
        TUCKY_EXIT_MSG("TuckyAllocError: Only one TuckyParser may exist in a program!\n");
    TUCKY_HAS_A_PARSER=true;

    TUCKY_ACTIVE_PARSER = (TuckyArgParser){
        .argc=argc,
        .argv=argv,
        .invoker=argv[0],
        .args=TUCKY_NO_ARGS
    };
    addArgument(&TUCKY_ACTIVE_PARSER, 'h', "help"   , 0, OPTIONAL, "Display the help information");
    return TUCKY_ACTIVE_PARSER;
}

#ifndef TUCKY_INFO_OVERRIDE
#define TUCKY_INFO_OVERRIDE
    #define TUCKY_APP       "YourAppName"
    #define TUCKY_VERSION   "0.0.0"
    #define TUCKY_AUTHOR    "John A. Smith"
    #define TUCKY_DATE      "Jan 01, 1989"
#endif /* TUCKY_INFO_OVERRIDE */

static inline void help(const TuckyArgParser parser);
#ifndef TUCKY_HELP_OVERRIDE
    static inline void help(const TuckyArgParser parser) {
        printf("#######################################\n");
        printf("# %s v%s\n", TUCKY_APP, TUCKY_VERSION);
        printf("# Author: %s\n", TUCKY_AUTHOR);
        printf("# Last Modified: %s\n", TUCKY_DATE);
        printf("#######################################\n");
        printArguments(parser);
    }
#endif /* TUCKY_HELP_OVERRIDE */

static inline void parseArgs(TuckyArgParser parser) {
    TuckyArgument last_arg   = NULL;
    TuckyArg      string_arg = NULL;

    #define pushToArgument() {\
        if (last_arg) {\
            if ((last_arg->nargs==AS_MANY || last_arg->nargs>0) && string_arg) {\
                appendArg(&(last_arg->args), string_arg);\
                string_arg = NULL;\
            } else {\
                last_arg->enabled = true;\
            }\
        }\
    }

    for (int i = 1; i<parser.argc; i++) { /* Start at 1 to skip executable name */
        const char* arg = parser.argv[i];

        if (arg[0] == '-') {
            pushToArgument();
            if (arg[1] == '-') {
                /* Keyword */
                last_arg = getArgumentFromKeyword(parser, arg+2);
                if (last_arg == NULL)
                    TUCKY_EXIT_MSG("TuckyBadArgument: `--%s`", arg+2);
                continue;
            }

            /* Flag */
            last_arg = getArgumentFromFlag(parser, arg[1]);
            if (last_arg == NULL)
                TUCKY_EXIT_MSG("TuckyBadArgument: `-%c`", arg[1]);
            continue;
        }

        if (last_arg != NULL) {
            TuckyArg temp_string_arg = newArg(arg);
            appendArg(&string_arg, temp_string_arg);
        }
    }
    pushToArgument();

    const bool help_set = getArgumentFromFlag(parser, 'h')->enabled;
    if (help_set) {
        help(parser);
        TUCKY_EXIT();
    }

    TuckyArgument temp = parser.args;
    while (temp) {
        if (
            temp->status == REQUIRED &&
            temp->nargs  >  0        &&
            temp->args   == NULL
        ) {
            TUCKY_EXIT_MSG("TuckyMissingValue: Missing value for argument `--%s`", temp->keyword);
        }
        temp = temp->next;
    }
}

#define TUCKY_FOREACH(ARG, ARGS) for (TuckyArg ARG = ARGS; ARG!=NULL; ARG=ARG->next)

#endif /* TUCKY_PARSER_H */