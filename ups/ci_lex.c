/* ci_lex.c - lexical analyser for ups expressions */

/*  Copyright 1991 Mark Russell, University of Kent at Canterbury.
 *  All Rights Reserved.
 *
 *  This file is part of UPS.
 *
 *  UPS is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  UPS is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with UPS; if not, write to the Free Software Foundation, Inc., 59 Temple
 *  Place, Suite 330, Boston, MA  02111-1307  USA
 */


/* @(#)ci_lex.c    1.25 21 Jun 1995 (UKC) */
char ups_ci_lex_c_rcsid[] = "$Id$";

#define DEBUG

#include <mtrprog/ifdefs.h>

#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#if HAVE_LIMITS_H
#include <limits.h>
#endif

#include <local/ukcprog.h>

#include "ups.h"
#include "symtab.h"
#include "ci.h"
#include "ci_parse.h"
#include "ci_util.h"
#include "ci_lex.h"
#include "ci_tokens.h"

static bool Want_debugging_output;

typedef struct message_action_entryst {
    ci_message_action_t ma_action;
    const char *ma_pat;
    struct message_action_entryst *ma_next;
} message_action_entry_t;

#ifndef __STDC__
extern double strtod();
extern char *memchr();
#endif

#if WANT_LL
typedef ulong_long_t number_t;
#else
typedef unsigned long number_t;
#endif

static bool convert_number PROTO((const char *str, int radix,
		  char **p_endstr, number_t *p_val));


#ifdef DEBUG
static const char *tokname PROTO((token_t token));
#endif
static const char *parse_hash_directive PROTO((const char *line, lex_env_t *le));
static lexinfo_t *make_lexinfo PROTO((lex_env_t *le, const char *line));
static const char *skip_whitespace PROTO((lex_env_t *le, const char *line));
static int get_float_constant PROTO((lex_env_t *le, const char *line,
		    const char **p_end, constant_t **p_co));
static const char *get_line PROTO((lex_env_t *le));
static int get_string PROTO((lex_env_t *le, const char *line, string_const_t *sc));
static bool is_aggr_type_specifier PROTO((token_t token));
static bool is_basic_type_specifier PROTO((token_t token));
static bool is_storage_class_or_qualifier PROTO((token_t token));
static bool is_decl_specifier PROTO((token_t token));

static message_action_entry_t *Message_action_list;

static struct {
    const char *name;
    token_t token;
    bool need_lexinfo;
} Keytab[] = {
    { "auto",    AUTO,	FALSE    },
    { "break",    BREAK,	TRUE    },
    { "case",    CASE,	FALSE    },
    { "char",    CHAR,	FALSE    },
    { "const",    CONST,	FALSE    },
    { "continue",    CONTINUE,    TRUE    },
    { "default",    DEFAULT,    FALSE    },
    { "do",	DO,	FALSE    },
    { "double",    DOUBLE,	FALSE    },
    { "else",    ELSE,	FALSE    },
    { "enum",    ENUM,	FALSE    },
    { "extern",    EXTERN,	FALSE    },
    { "float",    FLOAT,	FALSE    },
    { "for",    FOR,	TRUE    },
    { "goto",    GOTO,	FALSE    },
    { "if",	IF,	FALSE    },
    { "int",    INT,	FALSE    },
    { "long",    LONG,	FALSE    },
    { "register",    REGISTER,    FALSE    },
    { "return",    RETURN,	TRUE    },
    { "short",    SHORT,	FALSE    },
    { "signed",    SIGNED,	FALSE    },
    { "sizeof",    SIZEOF,	FALSE    },
    { "static",    STATIC,	FALSE    },
    { "struct",    STRUCT,	FALSE    },
    { "switch",    SWITCH,	FALSE    },
    { "typedef",    TYPEDEF,    FALSE    },
    { "union",    UNION,	FALSE    },
    { "unsigned",    UNSIGNED,    FALSE    },
    { "void",    VOID,	FALSE    },
    { "volatile",    VOLATILE,    FALSE    },
    { "while",    WHILE,	FALSE    },
};
#define NKEYS (sizeof Keytab / sizeof *Keytab)

lex_env_t *Lex_env;

#if !HAVE_STRSTR
/*  BUG: this should be in a library somewhere.
 */
char *
strstr(s, pat)
const char *s, *pat;
{
    int len, s_len, i;

    len = strlen(pat);
    s_len = strlen(s);    /* RGA don't look beyond string */
    for (i = 0; i <= s_len - len && *s != '\0'; ++s, ++i)
	if (*s == *pat && memcmp(s, pat, len) ==  0)
	    return (char *)s; /* UGH */
    return NULL;
}
#endif

void
yyerror(s)
const char *s;
{
    diagf(ET_ERROR, (lexinfo_t *)NULL, "%s", s);
}

void
ci_add_message_action(pat, action)
const char *pat;
ci_message_action_t action;
{
    message_action_entry_t *ma;

    ma = (message_action_entry_t *)e_malloc(sizeof(message_action_entry_t));
    ma->ma_pat = pat;
    ma->ma_action = action;
    ma->ma_next = Message_action_list;
    Message_action_list = ma;
}

void
ci_lex_error(arg, errtype, lx, mesg)
char *arg;
errtype_t errtype;
lexinfo_t *lx;
const char *mesg;
{
    static bool last_mesg_suppressed = FALSE;
    lex_env_t *le;
    char *wmesg;
    bool abort_parse;

    le = (lex_env_t *)arg;

    if (le->le_abort_parse || (errtype == ET_MORE && last_mesg_suppressed))
	return;

    if (errtype == ET_WARNING || errtype == ET_ERROR) {
	message_action_entry_t *ma;

	for (ma = Message_action_list; ma != NULL; ma = ma->ma_next)
	    if (ma->ma_pat == NULL || strstr(mesg, ma->ma_pat) != NULL)
		break;
	if (ma == NULL) {
	    if (errtype == ET_WARNING)
		errtype = ET_ERROR;
	}
	else {
	    if (errtype == ET_ERROR) {
		if (ma->ma_pat != NULL)
		    errf("Tried to ignore error \"%s\" with pattern \"%s\"",
				mesg, ma->ma_pat);
	    }
	    else {
		switch (ma->ma_action) {
		case MA_IGNORE:
		    errtype = ET_IGNORE;
		    break;
		case MA_WARNING_ONLY:
		    errtype = ET_WARNING;
		    break;
		case MA_DEFAULT:
		    break;
		default:
		    ci_panic("bad message action in le");
		}
	    }
	}
    }

    last_mesg_suppressed = errtype == ET_IGNORE;
    if (last_mesg_suppressed)
	return;

    if (errtype == ET_WARNING) {
	wmesg = strf("Warning: %s", mesg);
	mesg = wmesg;
    }
    else
	wmesg = NULL;

    if (errtype == ET_MORE) {
	abort_parse = (*le->le_report_func)((lexinfo_t *)NULL, mesg);
    }
    else {
	lexinfo_t cur_lexinfo;

	if (lx == NULL) {
	    cur_lexinfo.lx_filename = le->le_filename;
	    cur_lexinfo.lx_lnum = le->le_lnum + 1;
	    cur_lexinfo.lx_cnum = le->le_lptr - le->le_line;
	    lx = &cur_lexinfo;
	}
	abort_parse = (*le->le_report_func)(lx, mesg);
    }

    if (wmesg != NULL)
	free(wmesg);

    if (errtype == ET_ERROR)
	le->le_had_error = TRUE;

    if (abort_parse) {
	le->le_abort_parse = TRUE;
	le->le_lptr = "";
    }
}

static const char *
parse_hash_directive(line, le)
const char *line;
lex_env_t *le;
{
    int lnum, nitems;
    char name[256];

    for (; isspace(*line) && *line != '\0'; ++line)
	;
    if (*line == '\0')
	return line;

    if (strncmp(line, "pragma", 6) == 0 && isspace(line[6])) {
	for (line += 7; *line != '\0' && isspace(*line); ++line)
	    ;
	diagf(ET_WARNING, (lexinfo_t *)NULL, "#pragma `%.*s' ignored",
			   (int)strlen(line) - 1, line);
	return line + strlen(line);
    }
    if (strncmp(line, "ident", 5) == 0 && isspace(line[5])) {
	for (line += 6; *line != '\0' && isspace(*line); ++line)
	    ;
	return line + strlen(line);
    }

    nitems = sscanf(line, "%d \"%[^\"]\"", &lnum, name);
    if (nitems < 1) {
	diagf(ET_ERROR, (lexinfo_t *)NULL,
		    "Bad # directive \"%s\"", line);
	return "";
    }
    if (nitems == 2) {
	char *buf;
	size_t len;

	len = strlen(name);
	buf = alloc(Parse_apool, len + 1);
	(void) memcpy(buf, name, len + 1);
	le->le_filename = buf;
    }

    /*  Subtract 1 because we number internally from 0,
     *  and 1 because we are just about to bump the
     *  line number.
     */
    le->le_lnum = lnum - 2;

    return line + strlen(line);
}

static lexinfo_t *
make_lexinfo(le, line)
lex_env_t *le;
const char *line;
{
    lexinfo_t *lx;

    lx = NEW(lexinfo_t);
    lx->lx_filename = le->le_filename;
    lx->lx_lnum = le->le_lnum + 1;
    lx->lx_cnum = line - le->le_line;
    return lx;
}

const char *
ci_translate_escape(s, p_res)
const char *s;
int *p_res;
{
    static const char hexdigits[] = "0123456789abcdefABCDEF";
    const char *pos, *save_s;
    int ch;

    switch (*s) {
    case 'n':
	ch = '\n';
	break;
    case 't':
	ch = '\t';
	break;
    case 'v':
	ch = '\v';
	break;
    case 'b':
	ch = '\b';
	break;
    case 'r':
	ch = '\r';
	break;
    case 'f':
	ch = '\f';
	break;
    case 'a':
	ch = '\007';
	break;
    case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7':
	ch = 0;
	for (save_s = s; isdigit(*s) && *s < '8' && s - save_s < 3; ++s)
	    ch = ch * 8 + *s - '0';
	--s;
	break;
    case 'x':
	ch = 0;
	for (; *s != '\0' && (pos = strchr(hexdigits, *s)) != NULL; ++s) {
	    if (pos >= hexdigits + 16)
		pos -= 6;
	    ch = ch * 16 + pos - hexdigits;
	}
	break;
    default:
	ch = *s;
	break;
    }
    /* Dibyendu : 11/1/99
     * Fixed problem of sign extension - '\377' is now -1 and not 255
     */
    *p_res = (int)(char)ch;
    return s;
}

/*  Based on K&P's hoc follow() function.
 */
#define follow(s, ch, ifyes, ifno) ((*(s) == (ch)) ? (++(s), (ifyes)) : (ifno))

static const char *
get_line(le)
lex_env_t *le;
{
    if (le->le_abort_parse)
	return NULL;

    ++le->le_lnum;
    return le->le_line = (*le->le_getline)(le->le_getline_arg);
}

/*  Skip white space and comments.
 */
static const char *
skip_whitespace(le, line)
lex_env_t *le;
const char *line;
{
#ifdef DEBUG
    bool read_another_line;
#endif
    bool incomment;

    incomment = FALSE;
#ifdef DEBUG
    read_another_line = FALSE;
#endif
    for (;;) {
	for(;;) {
	    const char *s;

	    while (*line != '\0' && isspace(*line))
		++line;
	    if (*line != '\0')
		break;

	    if ((line = get_line(le)) == NULL)
		break;
#ifdef DEBUG
	    read_another_line = TRUE;
#endif
	    for (s = line; isspace(*s); ++s)
		;
	    if (*s == '#')
		line = parse_hash_directive(s + 1, le);
	}

	if (incomment) {
	    if (line == NULL) {
		diagf(ET_ERROR, (lexinfo_t *)NULL,
			"Hit EOF while in a comment");
		break;
	    }
	    else if (*line == '*' && line[1] == '/') {
		line += 2;
		incomment = FALSE;
	    }
	    else
		++line;
	}
	else {
	    if (line != NULL && *line == '/' && line[1] == '*') {
		line += 2;
		incomment = TRUE;
	    }
	    else
		break;
	}
    }

#ifdef DEBUG
    if (Want_debugging_output && read_another_line) {
	putchar('\n');
	printf("\n\"%s\", %d: %s", le->le_filename, le->le_lnum, line);
    }
#endif
    return line;
}

static bool
convert_number(str, radix, p_endstr, p_val)
const char *str;
int radix;
char **p_endstr;
number_t *p_val;
{
    number_t val, maxval;
    static const char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    bool failed;
    const char *startstr, *pos;

    errno = 0;
    startstr = str;

    if (radix == 0) {
	if (*str == '0') {
	    ++str;
	    if (*str == 'x' || *str == 'X') {
		++str;
		radix = 16;
	    }
	    else {
		radix = 8;
	    }
	}
	else {
	    radix = 10;
	}
    }

    if (radix < 2 || radix > 36) {
	diagf(ET_ERROR, (lexinfo_t *)NULL,
	      "Illegal radix %d - not in range 2..36", radix);
	return FALSE;
    }

    val = 0;
    failed = FALSE;
    maxval = ~(number_t)0 / radix;

    while ((pos = memchr(digits, tolower(*str), (size_t)radix)) != NULL) {
	number_t newval;

	if (val > maxval)
	    failed = TRUE;

	newval = val * radix + (pos - digits);

	if (newval < val)
	    failed = TRUE; /* wrapped */
	val = newval;

	++str;
    }

    /* Dibyendu : 19/12/98 : add support for lLuU suffixes */
    if (isalnum(*str) && (strchr("eElLuU", *str) == NULL)) {
	diagf(ET_ERROR, (lexinfo_t *)NULL,
	      "Illegal digit `%c' in integer constant `%.*s'",
	      *str, (str + 1 - startstr), startstr);
	return FALSE;
    }

    *p_endstr = (char *)str;

    if (failed) {
	/* Dibyendu : 11/1/99 : ignore overflow if we are looking at a
	   floating point constant. */
	if (strchr("eElLuU.", *str) == NULL) {
	    diagf(ET_ERROR, (lexinfo_t *)NULL,
		  "Integer constant `%.*s' out of range",
		  str - startstr, startstr);
	    return FALSE;
	}
    }

    *p_val = val;
    return TRUE;
}

static bool
is_aggr_type_specifier(token)
token_t token;
{
    if (token == STRUCT
    ||  token == UNION
    ||  token == ENUM)
	return TRUE;
    return FALSE;
}

static bool
is_basic_type_specifier(token)
token_t token;
{
    if (token == INT
    ||  token == UNSIGNED
    ||  token == SIGNED
    ||  token == LONG
    ||  token == SHORT
    ||  token == VOID
    ||  token == CHAR
    ||  token == FLOAT
    ||  token == DOUBLE)
	return TRUE;
    return FALSE;
}

static bool
is_storage_class_or_qualifier(token)
token_t token;
{
    if (token == STATIC
    ||  token == EXTERN
    ||  token == TYPEDEF
    ||  token == AUTO
    ||  token == REGISTER
    ||  token == CONST
    ||  token == VOLATILE)
	return TRUE;
    return FALSE;
}

static bool
is_decl_specifier(token)
token_t token;
{
    return is_storage_class_or_qualifier(token)
    ||     is_basic_type_specifier(token)
    ||     is_aggr_type_specifier(token)
    ||     token == TYPEDEF_NAME;
}

token_t
yylex()
{
#ifdef DEBUG
    static int pos = -1;
#endif
    lex_env_t *le;
    token_t token;
    const char *line;

    /* Dibyendu : following flags help decide whether a name
     * is a TYPEDEF or an IDENTIFIER.  29/12/98.
     */
    static token_t prev_token = 0;	/* remember last token */
    static bool in_decl_specifier = FALSE; /* parsing declaration specifiers ? */
    static bool seen_type_specifier = FALSE; /* Have we seen a type a type specifier ? */

    /* This is defined in ci_parse.y - we do not declare it in ci_parse.h
     * because although technically public (extern), its use is private.
     */
    extern bool ci_labels_allowed PROTO((void));
    extern bool Expecting_declarator;	/* 18/1/99 */

    le = Lex_env;

#ifdef DEBUG
    if (pos == -1) {
	Want_debugging_output = getenv("LEX_DEBUG") != NULL;
	pos = 0;
    }
#endif
    if (le == NULL) {
#ifdef DEBUG
	if (Want_debugging_output)
	    puts("\n");
#endif
	return 0;
    }

    if ((line = skip_whitespace(le, le->le_lptr)) == NULL) {
	le->le_lptr = line;
	return 0;    /* EOF */
    }

    switch (*line++) {
    case '_': case '$':

    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
    case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
    case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
    case 'v': case 'w': case 'x': case 'y': case 'z':

    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z':
	{
	    const char *s;
	    char *buf;
	    identifier_t *id;
	    size_t len;
	    int i;

	    --line;
            for (s = line; isalnum(*s) || *s == '_' || *s == '$'; ++s)
                ;
            if (*s == ':' && s[1] == ':'
                && (isalnum(s[2]) || s[2] == '_' || s[2] == '$')) {
                s += 3;
                while (isalnum(*s) || *s == '_' || *s == '$')
                    s++;
            }
	    /* RGA Hack to allow breakpoint code to call ostream::operator<<
	      or istream::operator>> */
            if ((*s == '<' && s[1] == '<' && s[2] == '(') ||
		(*s == '>' && s[1] == '>' && s[2] == '('))
	      s += 2;

	    len = s - line;

	    for (i = 0; i < NKEYS; ++i)
		if (memcmp(Keytab[i].name, line, len) == 0 &&
			Keytab[i].name[len] == '\0')
		    break;
	    if (i < NKEYS) {
		token = Keytab[i].token;

		/* Because a TYPEDEF name can appear in a declaration
		 * specifier as either a type name or a declarator, we
		 * need to determine which.  If a typedef name appears
		 * and we have not seen any type specifier yet, then use
		 * the typedef name as the type specifier, otherwise,
		 * treat it like an identifier.
		 * The flags below are set so that this information can
		 * be used later when we encounter the typedef name.
		 */
		if (is_aggr_type_specifier(token)) {
		    /* Any name after this has to be an identifier.
		     */
		    in_decl_specifier = TRUE;
		    seen_type_specifier = TRUE;
		}
		else if (is_storage_class_or_qualifier(token)) {
		    if (!in_decl_specifier) {
			in_decl_specifier = TRUE;
			seen_type_specifier = FALSE;
			/* A name after this has to be a typedef name ...
			 * unless we see a type specifier first.
			 */
		    }
		}
		else if (is_basic_type_specifier(token)) {
		    if (!in_decl_specifier) {
			in_decl_specifier = TRUE;
			seen_type_specifier = TRUE;
		    }
		    else {
			seen_type_specifier = TRUE;
		    }
		    /* Either way, typedef names can no longer appear -
		     * so any name will be treated as a simple IDENTIFIER.
		     */
		}

		line += len;
		if (Keytab[i].need_lexinfo)
		    yylval.lexinfo = make_lexinfo(le, line);
		else
		    yylval.lexinfo = NULL; /* for safety */
		break;
	    }

	    buf = alloc(Parse_apool, len + 1);
	    (void) memcpy(buf, line, len);
	    buf[len] = '\0';

	    id = NEW(identifier_t);
	    id->id_name = buf;
	    id->id_lexinfo = make_lexinfo(le, s);

	    line = skip_whitespace(le, s);

	    id->id_lparen_follows = *line == '(';
	    id->id_semi_follows = (*line == ';' || *line == '}'); /* Dibyendu */
	    yylval.identifier = id;

	    /* The parser provides the function name_type() which is
	     * called here to determine whether a name is a potential
	     * TYPEDEF name.
	     */
	    token = name_type(buf);
	    if (token != IDENTIFIER) {
		/* Dibyendu 29/12/98.
		 * The parser has identified this name as a potential TYPEDEF
		 * name.  Check if the context allows a TYPEDEF name.
		 */
		if (prev_token == GOTO
		||  prev_token == DOT
		||  prev_token == ARROW) {
		    /* Can't be a TYPEDEF name.
		     */
		    token = IDENTIFIER;
		}
		else if (in_decl_specifier) {
		    /* A typedef name can appear only if we have not seen
		     * a type specifier yet.  Also, any more typedef names
		     * automatically become ordinary identifiers for the
		     * same reason.
		     */
		    if (seen_type_specifier)
			token = IDENTIFIER;
		    else
			seen_type_specifier = TRUE;
		}
		else if (*line == ':'
			&& (prev_token == SEMI || prev_token == RBRACE)
			&& ci_labels_allowed()) {
		    /* A statement label.
		     */
		    token = IDENTIFIER;
		}
                else if (Expecting_declarator) {
                    /* 18/1/99 */
                    token = IDENTIFIER;
                }
		else {
		    /* The typedef name is used as the type specifier.
		     */
		    in_decl_specifier = TRUE;
		    seen_type_specifier = TRUE;
		}
	    }
	    /* NOTE: There are four more cases where a TYPEDEF name is
	     * invalid, but these are handled within the parser itself :
	     *    an enum constant
	     *    a function parameter name
	     *    primary expression
	     *    declarator
	     */
	}
	break;

    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
	{
	    char *end;
	    number_t val;
	    const char *startline;

	    /*  BUG: handle signed/unsigned and types correctly.
	     *  Mostly fixed now. 13/1/99 Dibyendu.
	     */
	    startline = line-1;
	    if (!convert_number(line - 1, 0, &end, &val)) {
		token = BADTOK;
	    }
	    else if (*end == 'e' || *end == 'E' || *end == '.') {
		token = get_float_constant(le, line-1, &line,
					   &yylval.constant);
	    }
	    else {
		constant_t *co;
		int i;
		bool is_unsigned, is_long, is_longlong;

		line = end;
		/* IDE/Dibyendu (19/12/98) recognise 'lLuU' */
		is_unsigned = FALSE; is_long = FALSE; is_longlong = FALSE;
		for (i = 0; i < 2; i++) {
		    if (*line == 'l' || *line == 'L') {
			line++;
			is_long = TRUE;
			if (is_long) is_longlong = TRUE;
		    }
		    if (*line == 'u' || *line == 'U') {
			line++;
			is_unsigned = TRUE;
		    }
		}
		/* Dibyendu : 13/1/99 : integer constants bigger than INT_MAX
		 * now have 'unsigned' type.
		 */
#if WANT_LL
		if (!is_unsigned && !is_longlong && (val > LLONG_MAX)) {
		    is_unsigned = TRUE;
		    is_longlong = TRUE;
		}
		if (!is_longlong && (val > UINT_MAX))
		    is_longlong = TRUE;
#endif
		if (!is_unsigned && (val > INT_MAX))
		    is_unsigned = TRUE;

		co = NEW(constant_t);
		co->co_lexinfo = make_lexinfo(le, line);
#if WANT_UINT_CONST
		/* Dibyendu : 13/1/99 : Support for unsigned constants */
		co->co_is_unsigned = is_unsigned;
		co->co_unsigned_val = val;
#else
		co->co_integer_val = val;
#endif
#if WANT_LL
		co->co_is_longlong = is_longlong;
#endif
		yylval.constant = co;
		token = INTEGER_CONSTANT;
#if WANT_UINT_CONST
		/* Dibyendu : 13/1/99 : Support for unsigned constants */
#else
		if (is_unsigned || is_long)
		    diagf(ET_WARNING, co->co_lexinfo,
			  "Integer suffix ignored in %.*s",
			  line - startline, startline);
#endif
	    }
	}
	break;
    case '!':
	token = follow(line, '=', NOTEQ, NOT);
	break;
    case '=':
	token = follow(line, '=', EQEQ, EQUALS);
	break;
    case '%':
	token = follow(line, '=', PERCENT_EQUALS, PERCENT);
	break;
    case '/':
	token = follow(line, '=', SLASH_EQUALS, SLASH);
	break;
    case '^':
	token = follow(line, '=', XOR_EQUALS, XOR);
	break;
    case '*':
	token = follow(line, '=', STAR_EQUALS, STAR);
	break;
    case '[':
	token = LBRAC;
	break;
    case ']':
	token = RBRAC;
	break;
    case '{':
	token = LBRACE;
	break;
    case '}':
	token = RBRACE;
	break;
    case '(':
	token = LPAREN;
	break;
    case ')':
	token = RPAREN;
	break;
    case ',':
	token = COMMA;
	break;
    case ';':
	token = SEMI;
	break;
    case '?':
	token = QUERY;
	break;
    case ':':
	token = COLON;
	break;
    case '\'': {
	/*  BUG: no escapes etc.
	 */
	int val;

	if (*line == '\\')
	    line = ci_translate_escape(line + 1, &val);
	else
	    val = *line;
	++line;

	if (*line != '\'') {
	    le->le_lptr = line;
	    diagf(ET_ERROR, (lexinfo_t *)NULL,
		    "Unterminated char constant");
	    token = BADTOK;
	}
	else {
	    constant_t *co;

	    co = NEW(constant_t);
	    co->co_lexinfo = make_lexinfo(le, line);
	    co->co_integer_val = val;
#if WANT_UINT_CONST
	    co->co_is_unsigned = FALSE;
#endif
#if WANT_LL
	    co->co_is_longlong = FALSE;
#endif
	    yylval.constant = co;
	    token = CHARACTER_CONSTANT;
	    ++line;
	}
	break;
    }
    case '"': {
	constant_t *co;

	co = NEW(constant_t);
	co->co_lexinfo = make_lexinfo(le, line);
#if WANT_UINT_CONST
	co->co_is_unsigned = FALSE;
#endif
	token = get_string(le, line, &co->co_string_val);
	yylval.constant = co;
	line = le->le_lptr;
	break;
    }
    case '.':
	if (*line == '.' && line[1] == '.') {
	    line += 2;
	    token = ELLIPSIS;
	}
	else if (isdigit(*line))
	    token = get_float_constant(le, line-1, &line, &yylval.constant);
	else
	    token = DOT;
	break;
    case '~':
	token = TILDE;
	break;
    case '+':
	if (*line == '+')
	    token = PLUSPLUS;
	else if (*line == '=')
	    token = PLUS_EQUALS;
	else {
	    token = PLUS;
	    --line;
	}
	++line;
	break;
    case '-':
	if (*line == '>')
	    token = ARROW;
	else if (*line == '-')
	    token = MINUSMINUS;
	else if (*line == '=')
	    token = MINUS_EQUALS;
	else {
	    token = MINUS;
	    --line;
	}
	++line;
	break;
    case '|':
	if (*line == '|')
	    token = OROR;
	else if (*line == '=')
	    token = OR_EQUALS;
	else {
	    --line;
	    token = OR;
	}
	++line;
	break;
    case '&':
	if (*line == '&')
	    token = ANDAND;
	else if (*line == '=')
	    token = AND_EQUALS;
	else {
	    --line;
	    token = AND;
	}
	++line;
	break;
    case '>':
	if (*line == '>') {
	    ++line;
	    token = follow(line, '=', RSHIFT_EQUALS, RSHIFT);
	}
	else if (*line == '=') {
	    ++line;
	    token = GTEQ;
	}
	else
	    token = GREATERTHAN;
	break;
    case '<':
	if (*line == '<') {
	    ++line;
	    token = follow(line, '=', LSHIFT_EQUALS, LSHIFT);
	}
	else if (*line == '=') {
	    ++line;
	    token = LESSEQ;
	}
	else
	    token = LESSTHAN;
	break;

    default:
	le->le_lptr = line; /* because we are about to call diagf */
	diagf(ET_ERROR, (lexinfo_t *)NULL,
		"Illegal character '%c' (0x%02x)", line[-1], line[-1]);
	token = BADTOK;
	break;
    }
    le->le_lptr = line;

#ifdef DEBUG
    if (Want_debugging_output) {
	const char *name;

	if (pos > 70) {
	    putchar('\n');
	    pos = 0;
	}
	name = tokname(token);
	printf("%s ", name);
	pos += strlen(name) + 1;
	fflush(stdout);
    }
#endif
    /* Note when we are out of a declaration specifier. */
    if (in_decl_specifier && !is_decl_specifier(token)) {
	in_decl_specifier = FALSE;
	seen_type_specifier = FALSE;
    }

    prev_token = token;
    return token;
}

static int
get_string(le, line, sc)
lex_env_t *le;
const char *line;
string_const_t *sc;
{
    static const char badalloc[] =
		"Unable to allocate memory for string constant";
    static char *buf;
    static size_t bufsize = 0;
    size_t opos;
    bool ok;

    if (bufsize == 0) {
	bufsize = 50;
	if ((buf = malloc(bufsize + 1)) == NULL) {
	    diagf(ET_ERROR, (lexinfo_t *)NULL, "%s", badalloc);
	    return BADTOK;
	}
    }

    opos = 0;
    ok = FALSE;	/* set to TRUE on success */

    for (; *line != '\0'; ++line) {
	int ch;

	if (*line == '"') {
	    const char *new_line;

	    new_line = skip_whitespace(le, line + 1);
	    if (new_line == NULL || *new_line != '"') {
		ok = TRUE;
		le->le_lptr = new_line;
		break;
	    }

	    line = new_line;
	    continue;
	}

	if (*line != '\\')
	    ch = *line;
	else if (*++line == '\n') {
	    line = get_line(le);
	    ch = (line != NULL) ? *line : '\0';
	}
	else
	    line = ci_translate_escape(line, &ch);

	if (line == NULL || *line == '\n' || *line == '\0') {
	    le->le_lptr = line;
	    diagf(ET_ERROR, (lexinfo_t *)NULL,
			"Unterminated string constant");
	    break;
	}

	if (opos == bufsize) {
	    bufsize *= 2;
	    if ((buf = realloc(buf, bufsize + 1)) == NULL) {
		le->le_lptr = line;
		diagf(ET_ERROR, (lexinfo_t *)NULL,
			    "%s", badalloc);
		break;
	    }
	}
	buf[opos++] = ch;
    }
    buf[opos++] = '\0';

    if (!ok)
	return BADTOK;

    sc->sc_val = memcpy(allocstr(Parse_apool, opos), buf, opos);
    sc->sc_size = opos;
    return STRING_CONSTANT;
}

static int
get_float_constant(le, line, p_end, p_co)
lex_env_t *le;
const char *line, **p_end;
constant_t **p_co;
{
    double val;
    constant_t *co;
    char *end;
    bool is_float, is_long_double;

    val = strtod(line, &end);

    if (end == line) {
	le->le_lptr = line;
	diagf(ET_ERROR, (lexinfo_t *)NULL,
		"Badly formed floating constant \"%s\"", line);
	return BADTOK;
    }
    /* IDE/Dibyendu (19/12/98) : recognise 'lLfF' suffixes. */
    /* FIX : 'float' and 'long double' constants not supported. */
    is_float = is_long_double = FALSE;
    if (*end == 'f' || *end == 'F') {
	is_float = TRUE;
	end++;
    }
    else if (*end == 'l' || *end == 'L') {
	is_long_double = TRUE;
	end++;
    }
    co = NEW(constant_t);
#if WANT_UINT_CONST
    co->co_is_unsigned = FALSE;
#endif
    co->co_lexinfo = make_lexinfo(le, line);
    co->co_floating_val = val;

    if (is_float || is_long_double)
	diagf(ET_WARNING, co->co_lexinfo,
		"Floating point suffix ignored, \"%s\" read as %.12g",
		line, val);

    *p_co = co;
    *p_end = end;

    return FLOATING_CONSTANT;
}

#ifdef DEBUG
static const char *
tokname(token)
token_t token;
{
    static struct {
	    const char *name;
	    token_t token;
    } tab[] = {
	    { "IF",                     IF			},
	    { "ELSE",                   ELSE		},
	    { "WHILE",                  WHILE		},
	    { "FOR",                    FOR			},
	    { "DO",                     DO			},
	    { "GOTO",                   GOTO		},
	    { "BREAK",                  BREAK		},
	    { "CONTINUE",               CONTINUE		},
	    { "RETURN",                 RETURN		},
	    { "SWITCH",                 SWITCH		},
	    { "CASE",                   CASE		},
	    { "DEFAULT",                DEFAULT		},
	    { "SIZEOF",                 SIZEOF		},
	    { "AUTO",                   AUTO		},
	    { "REGISTER",               REGISTER		},
	    { "STATIC",                 STATIC		},
	    { "EXTERN",                 EXTERN		},
	    { "TYPEDEF",                TYPEDEF		},
	    { "VOID",                   VOID		},
	    { "CHAR",                   CHAR		},
	    { "SHORT",                  SHORT		},
	    { "INT",                    INT			},
	    { "LONG",                   LONG		},
	    { "FLOAT",                  FLOAT		},
	    { "DOUBLE",                 DOUBLE		},
	    { "SIGNED",                 SIGNED		},
	    { "UNSIGNED",               UNSIGNED		},
	    { "CONST",                  CONST		},
	    { "VOLATILE",               VOLATILE		},
	    { "STRUCT",                 STRUCT		},
	    { "UNION",                  UNION		},
	    { "ENUM",                   ENUM		},
	    { "AND",                    AND			},
	    { "TILDE",                  TILDE		},
	    { "NOT",                    NOT			},
	    { "LESSTHAN",               LESSTHAN		},
	    { "GREATERTHAN",            GREATERTHAN		},
	    { "XOR",                    XOR			},
	    { "OR",                     OR			},
	    { "PLUS",                   PLUS		},
	    { "MINUS",                  MINUS		},
	    { "SLASH",                  SLASH		},
	    { "PERCENT",                PERCENT		},
	    { "STAR",                   STAR		},
	    { "DOT",                    DOT			},
	    { "COLON",                  COLON		},
	    { "QUERY",                  QUERY		},
	    { "SEMI",                   SEMI		},
	    { "COMMA",                  COMMA		},
	    { "LPAREN",                 LPAREN		},
	    { "RPAREN",                 RPAREN		},
	    { "LBRACE",                 LBRACE		},
	    { "RBRACE",                 RBRACE		},
	    { "LBRAC",                  LBRAC		},
	    { "RBRAC",                  RBRAC		},
	    { "EQUALS",                 EQUALS		},
	    { "STAR_EQUALS",            STAR_EQUALS		},
	    { "SLASH_EQUALS",           SLASH_EQUALS	},
	    { "PERCENT_EQUALS",         PERCENT_EQUALS	},
	    { "PLUS_EQUALS",            PLUS_EQUALS		},
	    { "MINUS_EQUALS",           MINUS_EQUALS	},
	    { "LSHIFT_EQUALS",          LSHIFT_EQUALS	},
	    { "RSHIFT_EQUALS",          RSHIFT_EQUALS	},
	    { "AND_EQUALS",             AND_EQUALS		},
	    { "XOR_EQUALS",             XOR_EQUALS		},
	    { "OR_EQUALS",              OR_EQUALS		},
	    { "ANDAND",                 ANDAND		},
	    { "OROR",                   OROR		},
	    { "EQEQ",                   EQEQ		},
	    { "NOTEQ",                  NOTEQ		},
	    { "GTEQ",                   GTEQ		},
	    { "LESSEQ",                 LESSEQ		},
	    { "LSHIFT",                 LSHIFT		},
	    { "RSHIFT",                 RSHIFT		},
	    { "PLUSPLUS",               PLUSPLUS		},
	    { "MINUSMINUS",             MINUSMINUS		},
	    { "ARROW",                  ARROW		},
	    { "ELLIPSIS",               ELLIPSIS		},
	    { "STRING_CONSTANT",        STRING_CONSTANT	},
	    { "INTEGER_CONSTANT",       INTEGER_CONSTANT	},
	    { "CHARACTER_CONSTANT",     CHARACTER_CONSTANT	},
	    { "FLOATING_CONSTANT",      FLOATING_CONSTANT	},
	    { "IDENTIFIER",             IDENTIFIER		},
	    { "TYPEDEF_NAME",           TYPEDEF_NAME	},
	    { "BADTOK",                 BADTOK		},
	    { "EOF",                    0			},
    };
    static char buf[100];
    int i;

    for (i = 0; i < sizeof tab / sizeof *tab; ++i)
	    if (tab[i].token == token)
		    return tab[i].name;

    (void) sprintf(buf, "<unknown token %d>", token);
    return buf;
}
#endif /* DEBUG */
