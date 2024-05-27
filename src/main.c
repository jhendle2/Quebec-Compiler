#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common.h"
#include "flags.h"
#include "parse.h"
#include "lexer.h"
#include "qbe.h"

#define TUCKY_INFO_OVERRIDE
    #define TUCKY_APP       "Quebec C-Compiler"
    #define TUCKY_VERSION   "0.1.0"
    #define TUCKY_AUTHOR    "Jonah Hendler"
    #define TUCKY_DATE      "May 26th, 2024"

#include "argparse.h"

bool global_VERBOSE;

int main(int argc, char** argv) {
    /****************************************************/
    TuckyArgParser parser = newArgParser(argc, argv);

    addArgument(&parser, 'f', "file"   ,    AS_MANY, REQUIRED, "Input file path");
    addArgument(&parser, 'o', "out"    ,          1, REQUIRED, "Output file path");
    addArgument(&parser, 'v', "verbose", STORE_TRUE, OPTIONAL, "Enable verbose output");
    addArgument(&parser, 'r', "run"    , STORE_TRUE, OPTIONAL, "After compilation, immediately run the program");

    parseArgs(parser);

    const char* file_path    = getArgumentFromFlag(parser, 'f')->args->txt;
    const char* outfile_path = getArgumentFromFlag(parser, 'o')->args->txt;
    const bool  run_immed    = getArgumentFromFlag(parser, 'r')->enabled;
    
    global_VERBOSE           = getArgumentFromFlag(parser, 'v')->enabled;
    if (global_VERBOSE) { printf("[DEBG] Verbose output enabled\n"); }

    uint       step      = 0;
    const uint max_steps = run_immed ? 4 : 3;

    /****************************************************/
    INFO("Parsing...    STEP (%d/%d)", ++step, max_steps);
    pFileLine file_as_lines = readFileAsLines(file_path);
    if (file_as_lines == NULL) return EXIT_FAILURE;

    pFileLine temp = file_as_lines;
    pToken file_as_tokens = NULL;
    while (temp) {
        if (global_VERBOSE) { printf("[DEBG] "); dumpFileLine(temp); }
        pToken line_as_tokens = tokenizeLine(temp);
        if (line_as_tokens == NULL) goto next;

        appendToken(&file_as_tokens, line_as_tokens);
next:
        temp = temp->next;
    }

    /****************************************************/
    pSyntaxNode master = buildTreeFromTokens(file_as_tokens);
    if (global_VERBOSE) { printf("[DEBG]"); dumpSyntaxTree(master, 0); }

    INFO("Assembling... STEP (%d/%d)", ++step, max_steps);
    compileFile(outfile_path, master);

    /****************************************************/
    
    INFO("Compiling...  STEP (%d/%d)", ++step, max_steps);
    char cmd_buf[256] = {0};
    sprintf(cmd_buf, "qbe -o temp.s temp.ssa && cc -o %s temp.s\n", outfile_path);
    int ret = system(cmd_buf);
    if (global_VERBOSE) printf("[DEBG] QBE ret code = %d\n", ret);
    
    if (ret != EXIT_SUCCESS) {
        WARN("QBE did not compile successfully!\n");
        goto cleanup;
    }

    /****************************************************/
    
    if (run_immed) {
        INFO("Running...    STEP (%d/%d)", ++step, max_steps);
        sprintf(cmd_buf, "./%s", outfile_path);
        ret = system(cmd_buf);
        if (global_VERBOSE) printf("[DEBG] Run ret code = %d\n", ret);
    }

    /****************************************************/
cleanup:
    delSyntaxNode(&master);
    delFileLine(&file_as_lines);
    delArgParser(parser);

    INFO("All Done!\n");
    return EXIT_SUCCESS;
}