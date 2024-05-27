#ifndef QUEBEC_TYPES_H
#define QUEBEC_TYPES_H

#include "parse.h"

#define isType(TYPE) (\
    TYPE == TOKEN_char ||\
    TYPE == TOKEN_double ||\
    TYPE == TOKEN_float ||\
    TYPE == TOKEN_int ||\
    TYPE == TOKEN_long ||\
    TYPE == TOKEN_short ||\
    TYPE == TOKEN_void\
)

#define isAdjective(TYPE) (\
    TYPE == TOKEN_const ||\
    TYPE == TOKEN_enum ||\
    TYPE == TOKEN_extern ||\
    TYPE == TOKEN_inline ||\
    TYPE == TOKEN_register ||\
    TYPE == TOKEN_signed ||\
    TYPE == TOKEN_static ||\
    TYPE == TOKEN_struct ||\
    TYPE == TOKEN_unsigned ||\
    TYPE == TOKEN_volatile\
)

#define isConst(TYPE) (\
    TYPE == TOKEN_hexConst    ||\
    TYPE == TOKEN_intConst    ||\
    TYPE == TOKEN_floatConst  ||\
    TYPE == TOKEN_doubleConst ||\
    TYPE == TOKEN_charConst   ||\
    TYPE == TOKEN_stringConst\
)

#define isIdentifier(TYPE) (TYPE == TOKEN_identifier)

#define isAssignmentOperator(S) (\
    strcmp(rhs->text,   "=")==0 ||\
    strcmp(rhs->text,  "+=")==0 ||\
    strcmp(rhs->text,  "-=")==0 ||\
    strcmp(rhs->text,  "*=")==0 ||\
    strcmp(rhs->text,  "/=")==0 ||\
    strcmp(rhs->text,  "%=")==0 ||\
    strcmp(rhs->text,  "|=")==0 ||\
    strcmp(rhs->text,  "&=")==0 ||\
    strcmp(rhs->text,  "^=")==0 ||\
    strcmp(rhs->text, "<<=")==0 ||\
    strcmp(rhs->text, ">>=")==0\
)

#endif /* QUEBEC_TYPES_H */