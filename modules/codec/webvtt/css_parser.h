/*****************************************************************************
 * css_parser.h : CSS parser interface
 *****************************************************************************
 * Copyright (C) 2017 VideoLabs, VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/
#ifndef CSS_PARSER_H
#define CSS_PARSER_H

//#define YYDEBUG 1
//#define CSS_PARSER_DEBUG
#define YYSTYPE CSSSTYPE

typedef struct heimdall_css_parser_t heimdall_css_parser_t;
typedef struct heimdall_css_selector_t heimdall_css_selector_t;
typedef struct heimdall_css_declaration_t heimdall_css_declaration_t;
typedef struct heimdall_css_rule_t heimdall_css_rule_t;
typedef struct heimdall_css_expr_t heimdall_css_expr_t;

typedef struct
{
    float val;
    char *psz;
    heimdall_css_expr_t *function;
    enum
    {
        TYPE_INVALID,
        TYPE_NONE = 0,
        TYPE_EMS,
        TYPE_EXS,
        TYPE_PIXELS,
        TYPE_POINTS,
        TYPE_MILLIMETERS,
        TYPE_PERCENT,
        TYPE_MILLISECONDS,
        TYPE_HERTZ,
        TYPE_DEGREES,
        TYPE_DIMENSION,
        TYPE_STRING = 0x20,
        TYPE_FUNCTION,
        TYPE_IDENTIFIER,
        TYPE_HEXCOLOR,
        TYPE_UNICODERANGE,
        TYPE_URI,
    } type;
} heimdall_css_term_t;

struct heimdall_css_expr_t
{
    struct
    {
        char op;
        heimdall_css_term_t term;
    } *seq;
    size_t i_alloc;
    size_t i_count;
};

struct heimdall_css_declaration_t
{
    char *psz_property;
    heimdall_css_expr_t *expr;
    heimdall_css_declaration_t *p_next;
};

enum heimdall_css_match_e
{
    MATCH_EQUALS,
    MATCH_INCLUDES,
    MATCH_DASHMATCH,
    MATCH_BEGINSWITH,
    MATCH_ENDSWITH,
    MATCH_CONTAINS,
};

enum heimdall_css_relation_e
{
   RELATION_SELF = 0,
   RELATION_DESCENDENT = ' ',
   RELATION_DIRECTADJACENT = '+',
   RELATION_INDIRECTADJACENT = '~',
   RELATION_CHILD = '>',
};

struct heimdall_css_selector_t
{
    char *psz_name;
    enum
    {
        SELECTOR_SIMPLE = 0,
        SELECTOR_PSEUDOCLASS,
        SELECTOR_PSEUDOELEMENT,
/*        SELECTOR_PSEUDONTHCHILD,
        SELECTOR_PSEUDONTHTYPE,
        SELECTOR_PSEUDONTHLASTCHILD,
        SELECTOR_PSEUDONTHLASTTYPE,*/
        SPECIFIER_ID,
        SPECIFIER_CLASS,
        SPECIFIER_ATTRIB,
    } type;
    struct
    {
        heimdall_css_selector_t *p_first;
        heimdall_css_selector_t **pp_append;
    } specifiers;

    enum heimdall_css_match_e match;
    heimdall_css_selector_t *p_matchsel;

    enum heimdall_css_relation_e combinator;
    heimdall_css_selector_t *p_next;
};

struct heimdall_css_rule_t
{
    bool b_valid;
    heimdall_css_selector_t *p_selectors;
    heimdall_css_declaration_t *p_declarations;

    heimdall_css_rule_t *p_next;
};

struct heimdall_css_parser_t
{
    struct
    {
        heimdall_css_rule_t *p_first;
        heimdall_css_rule_t **pp_append;
    } rules;
};

#define CHAIN_APPEND_DECL(n, t) void n( t *p_a, t *p_b )

void heimdall_css_term_Clean( heimdall_css_term_t a );
bool heimdall_css_expression_AddTerm( heimdall_css_expr_t *p_expr, char op, heimdall_css_term_t a );
void heimdall_css_expression_Delete( heimdall_css_expr_t *p_expr );
heimdall_css_expr_t * heimdall_css_expression_New( heimdall_css_term_t term );

CHAIN_APPEND_DECL(heimdall_css_declarations_Append, heimdall_css_declaration_t);
void heimdall_css_declarations_Delete( heimdall_css_declaration_t *p_decl );
heimdall_css_declaration_t * heimdall_css_declaration_New( const char *psz );

CHAIN_APPEND_DECL(heimdall_css_selector_Append, heimdall_css_selector_t);
void heimdall_css_selector_AddSpecifier( heimdall_css_selector_t *p_sel, heimdall_css_selector_t *p_spec );
void heimdall_css_selectors_Delete( heimdall_css_selector_t *p_sel );
heimdall_css_selector_t * heimdall_css_selector_New( int type, const char *psz );

void heimdall_css_rules_Delete( heimdall_css_rule_t *p_rule );
heimdall_css_rule_t * heimdall_css_rule_New( void );

void heimdall_css_parser_AddRule( heimdall_css_parser_t *p_parser, heimdall_css_rule_t *p_rule );
void heimdall_css_parser_Debug( const heimdall_css_parser_t *p_parser );
void heimdall_css_parser_Clean( heimdall_css_parser_t *p_parser );
void heimdall_css_parser_Init( heimdall_css_parser_t *p_parser );

bool heimdall_css_parser_ParseBytes( heimdall_css_parser_t *p_parser, const uint8_t *, size_t );
bool heimdall_css_parser_ParseString( heimdall_css_parser_t *p_parser, const char * );

void heimdall_css_unescape( char *psz );
char * heimdall_css_unquoted( const char *psz );
char * heimdall_css_unescaped( const char *psz );
char * heimdall_css_unquotedunescaped( const char *psz );

#   ifdef CSS_PARSER_DEBUG

void css_selector_Debug( const heimdall_css_selector_t *p_sel );
void css_rule_Debug( const heimdall_css_rule_t *p_rule );

#   endif

#endif
