#ifndef QUEBEC_QBE_H
#define QUEBEC_QBE_H

#include "lexer.h"

/* https://c9x.me/compile/doc/il.html#Simple-Types
   BASETY := 'w' | 'l' | 's' | 'd' # Base types
   EXTTY  := BASETY | 'b' | 'h'    # Extended types */
enum QbeType {
    QBE_Word,      /* 32-bit integer        */
    QBE_Long,      /* 64-bit integer        */
    QBE_Single,    /* 32-bit floating point */
    QBE_Double,    /* 64-bit floating point */
    QBE_Byte,      /* 8-bit  integer        */
    QBE_HalfWorld, /* 16-bit integer        */
QBE_TYPE_LENGTH
};
__attribute_maybe_unused__ static const char* qbeType2str[QBE_TYPE_LENGTH] = {
    "w",
    "l",
    "s",
    "d",
    "b",
    "h"
};

void compileFile(const char* output_path, pSyntaxNode master);

#endif /* QUEBEC_QBE_H */