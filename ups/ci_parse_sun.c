

/* @(#)ci_parse.y	1.15 16 Apr 1994 (UKC) */
char ups_ci_parse_y_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#include <setjmp.h>
#include <stdio.h>

#include <local/ukcprog.h>

#include "ups.h"
#include "symtab.h"
#include "ci.h"
#include "ci_parse.h"
#include "ci_types.h"
#include "ci_util.h"
#include "ci_decl.h"
#include "ci_func.h"
#include "ci_expr.h"
#include "ci_stm.h"
#include "ci_lex.h"

int yyparse PROTO((void));
int ci_labels_allowed PROTO((void));

static int Switch_level = 0;
static int Loop_level = 0;
/* Dibyendu : 28/12/98
 * Typedef_allowed - for resolving typedef/identifier ambiguity
 * Labels_allowed  - for resolving label/typedef ambiguity
 *                   in bitfield declarations
 */
static bool Typedef_allowed = TRUE;
static bool Labels_allowed = TRUE;
bool Expecting_declarator  = FALSE;	/* Typedef fix : 18/1/99 */

#define YYMAXDEPTH	400
#define YYDEBUG		1


typedef union
#ifdef __cplusplus
	YYSTYPE
#endif
 {
	int undef;
	constant_t *constant;
	statement_t *statement;
	expr_list_t *expr_list;
	expr_t *expr;
	type_t *type;
	class_t class;
	typecode_t typecode;
	qualifiers_t qualifier;
	func_t *function;
	declaration_t *declaration;
	declarator_t *declarator;
	var_t *varlist;
	enum_member_t *enum_member;
	identifier_t *identifier;
	identifier_list_t *identifier_list;
	optype_t optype;
	initexpr_t *initialiser;
	lexinfo_t *lexinfo;
} YYSTYPE;
# define FOR 257
# define BREAK 258
# define CONTINUE 259
# define RETURN 260
# define IF 261
# define ELSE 262
# define WHILE 263
# define DO 264
# define SWITCH 265
# define CASE 266
# define DEFAULT 267
# define GOTO 268
# define SIZEOF 269
# define AUTO 270
# define REGISTER 271
# define STATIC 272
# define EXTERN 273
# define TYPEDEF 274
# define VOID 275
# define CHAR 276
# define SHORT 277
# define INT 278
# define LONG 279
# define FLOAT 280
# define DOUBLE 281
# define SIGNED 282
# define UNSIGNED 283
# define CONST 284
# define VOLATILE 285
# define STRUCT 286
# define UNION 287
# define ENUM 288
# define AND 289
# define TILDE 290
# define NOT 291
# define LESSTHAN 292
# define GREATERTHAN 293
# define XOR 294
# define OR 295
# define PLUS 296
# define MINUS 297
# define SLASH 298
# define PERCENT 299
# define STAR 300
# define DOT 301
# define COLON 302
# define QUERY 303
# define SEMI 304
# define COMMA 305
# define LPAREN 306
# define RPAREN 307
# define LBRACE 308
# define RBRACE 309
# define LBRAC 310
# define RBRAC 311
# define EQUALS 312
# define STAR_EQUALS 313
# define SLASH_EQUALS 314
# define PERCENT_EQUALS 315
# define PLUS_EQUALS 316
# define MINUS_EQUALS 317
# define LSHIFT_EQUALS 318
# define RSHIFT_EQUALS 319
# define AND_EQUALS 320
# define XOR_EQUALS 321
# define OR_EQUALS 322
# define ANDAND 323
# define OROR 324
# define EQEQ 325
# define NOTEQ 326
# define GTEQ 327
# define LESSEQ 328
# define LSHIFT 329
# define RSHIFT 330
# define PLUSPLUS 331
# define MINUSMINUS 332
# define ARROW 333
# define ELLIPSIS 334
# define INTEGER_CONSTANT 335
# define CHARACTER_CONSTANT 336
# define FLOATING_CONSTANT 337
# define STRING_CONSTANT 338
# define IDENTIFIER 339
# define TYPEDEF_NAME 340
# define BADTOK 341

#ifdef __STDC__
#include <stdlib.h>
#include <string.h>
#else
#include <memory.h>
#endif

#include <values.h>

#ifdef __cplusplus

#ifndef yyerror
	void yyerror(const char *);
#endif

#ifndef yylex
#ifdef __EXTERN_C__
	extern "C" { int yylex(void); }
#else
	int yylex(void);
#endif
#endif
	int yyparse(void);

#endif
#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
extern int yychar;
extern int yyerrflag;
YYSTYPE yylval;
YYSTYPE yyval;
typedef int yytabelem;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif
#if YYMAXDEPTH > 0
int yy_yys[YYMAXDEPTH], *yys = yy_yys;
YYSTYPE yy_yyv[YYMAXDEPTH], *yyv = yy_yyv;
#else	/* user does initial allocation */
int *yys;
YYSTYPE *yyv;
#endif
static int yymaxdepth = YYMAXDEPTH;
# define YYERRCODE 256



/*  Resolve the C typedef ambiguity - return IDENTIFIER or TYPEDEF_NAME
 *  as the type of the string name.
 */

int
ci_labels_allowed()
{
	/* Labels are not allowed when parsing struct/union/enum types */
	return Labels_allowed;
}

token_t
name_type(name)
const char *name;
{
	/* Typedefs are not allowed when parsing enum constants */
	if (!Typedef_allowed)
		return IDENTIFIER;

	return (ci_lookup_typedef(name) != NULL) ? TYPEDEF_NAME : IDENTIFIER;
}

parse_id_t
ci_parse_file(filename, block, flags, report_error_func,
				resolve_name_func, getline_func, getline_arg)
const char *filename;
block_t *block;
unsigned long flags;
ci_report_error_func_t report_error_func;
ci_resolve_name_func_t resolve_name_func;
const char *(*getline_func)PROTO((char *arg));
char *getline_arg;
{
	lex_env_t lebuf;
	int res;

	lebuf.le_report_func = report_error_func;
	lebuf.le_getline = getline_func;
	lebuf.le_getline_arg = getline_arg;
	lebuf.le_filename = filename;
	lebuf.le_had_error = FALSE;
	lebuf.le_lnum = -1;
	lebuf.le_lptr = "";
	lebuf.le_abort_parse = FALSE;

	Lex_env = &lebuf;
	ci_set_diag_handler(ci_lex_error, (char *)&lebuf);
	ci_start_parse_tree(resolve_name_func, block, flags);
	Loop_level = Switch_level = 0;

	if ((flags & CI_CP_DONT_PANIC) == 0)
		res = yyparse();
	else {
		extern bool ci_Catching_panics;
		extern jmp_buf ci_Catch_panic_env;

		if (ci_Catching_panics)
			panic("catch_panics botch");
		ci_Catching_panics = TRUE;

		if (setjmp(ci_Catch_panic_env) != 0)
			res = -1;
		else
			res = yyparse();

		ci_Catching_panics = FALSE;
	}

	ci_set_diag_handler((diag_handler_func_t)NULL, (char *)NULL);
	Lex_env = NULL;

	/*  We call yylex with Lex_env NULL, to make it put out a newline
	 *  when debugging.
	 */
	(void) yylex();

	return (parse_id_t)ci_end_parse_tree(res == 0 && !lebuf.le_had_error);
}
yytabelem yyexca[] ={
-1, 1,
	0, -1,
	-2, 0,
-1, 44,
	304, 52,
	305, 52,
	-2, 10,
-1, 59,
	308, 42,
	-2, 44,
	};
# define YYNPROD 238
# define YYLAST 1147
yytabelem yyact[]={

   201,   218,    84,   247,    60,   365,   229,   289,    18,    19,
    20,    21,    22,    23,    24,    25,    26,    30,    31,    37,
    38,    36,    79,    82,    83,   197,    33,   189,    34,    80,
    81,   190,   248,    78,   189,    61,   309,   275,   190,    91,
   246,    34,   234,   228,   227,   226,   220,    33,   232,   233,
   231,   216,   217,   225,    84,    59,    34,    34,   291,    32,
    32,    45,    33,    33,    76,    77,   158,    32,    93,    94,
    95,    90,    88,    29,    79,    82,    83,   174,   175,   295,
    32,    80,    81,   142,    72,    78,   168,   169,   141,   121,
    66,    91,   140,   230,   297,    32,    32,   138,   161,   234,
   228,   227,   226,   220,   354,   232,   233,   231,   216,   217,
   225,    84,   329,   144,   145,   143,    76,    77,   139,   253,
    93,    94,    95,    90,   215,   353,   280,   125,   253,   194,
   279,    79,    82,    83,   316,   358,   335,   120,    80,    81,
   170,   171,    78,    34,   253,   240,   373,   242,    91,   257,
   230,   199,   257,   190,   161,   253,   190,   362,   378,   234,
   228,   227,   226,   220,   161,   232,   233,   231,   216,   217,
   225,    84,   116,    76,    77,   173,   172,    93,    94,    95,
    90,   215,    52,   314,   294,   113,    51,   313,   253,    62,
   361,    79,    82,    83,   253,   253,   360,   254,    80,    81,
   355,   298,    78,   183,   328,   182,   327,   320,    91,     7,
   230,   317,   252,   237,   181,   110,    44,   363,   330,   331,
   315,   308,   306,   253,   307,   305,   304,   318,   310,    67,
    68,   253,   184,    76,    77,   376,   374,    93,    94,    95,
    90,   215,   364,    54,   348,   347,   312,   311,   302,   340,
   332,   301,   299,    65,   319,    13,    14,    15,    16,    17,
    18,    19,    20,    21,    22,    23,    24,    25,    26,    30,
    31,    37,    38,    36,   127,   128,   129,   130,   131,   132,
   133,   134,   135,   136,   137,    34,   179,   180,   178,    30,
    31,   189,   282,   176,   177,   190,   159,   196,   166,   167,
   339,   192,   341,   345,   372,    34,   188,   108,    85,    98,
   346,   342,   343,   344,   283,   223,    47,   195,   222,   185,
    97,    87,   191,   119,    32,    29,     2,    92,    39,    41,
   117,   160,   221,   118,    65,    42,     4,     4,   219,   338,
   198,   359,    13,    14,    15,    16,    17,    18,    19,    20,
    21,    22,    23,    24,    25,    26,    30,    31,    37,    38,
    36,   367,   368,   369,   366,   293,   371,   104,   115,   356,
   370,   124,    34,   375,   101,   114,   377,    64,    33,   379,
    13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
    23,    24,    25,    26,    30,    31,    37,    38,    36,    54,
   351,    99,   102,   290,   350,    11,    58,    63,    40,   100,
    34,    32,    29,   126,   236,   281,   257,   282,    53,     8,
   190,    13,    14,    15,    16,    17,    18,    19,    20,    21,
    22,    23,    24,    25,    26,    30,    31,    37,    38,    36,
    96,   238,    27,    28,    35,   288,    46,   106,   241,     5,
    29,   235,   224,   244,   243,    64,   105,    70,   103,    13,
    14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
    24,    25,    26,    30,    31,    37,    38,    36,   262,   263,
   250,   149,   151,   152,   153,   154,   155,   251,   261,   249,
   107,    29,   277,   287,   278,     9,   325,    13,    14,    15,
    16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
    26,    30,    31,    37,    38,    36,   296,   292,    89,    86,
    75,    74,   210,   209,   208,   186,   206,   203,   207,    29,
    13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
    23,    24,    25,    26,    30,    31,    37,    38,    36,    84,
   303,   270,   271,   211,   202,   212,   214,   213,   204,   205,
   200,   276,    55,   278,     3,     1,   290,    29,     0,    79,
    82,    83,   264,   265,   266,   267,    80,    81,   255,     0,
    78,   272,   273,   274,   268,   269,    91,   164,   123,   349,
     0,     0,   336,    53,   287,   326,     0,     0,     0,     0,
    29,     0,     0,     0,     0,     0,     0,   260,     0,   164,
     0,    76,    77,     0,   193,    93,    94,    95,    90,    88,
    18,    19,    20,    21,    22,    23,    24,    25,    26,    30,
    31,    37,    38,    36,    18,    19,    20,    21,    22,    23,
    24,    25,    26,    30,    31,    37,    38,    36,   285,    84,
     0,     0,     0,   164,   334,   321,    43,     6,     6,     0,
   164,   164,     0,     0,     0,    48,    49,    50,   286,    79,
    82,    83,     0,    84,   300,     0,    80,    81,     0,     0,
    78,     0,     0,     0,     0,    29,    91,   164,     0,     0,
   164,   323,     0,    79,    82,    83,     0,     0,     0,    29,
    80,    81,     0,     0,    78,     0,   162,     0,     0,   109,
    91,    76,    77,     0,     0,    93,    94,    95,    90,    88,
    18,    19,    20,    21,    22,    23,    24,    25,    26,    30,
    31,    37,    38,    36,    84,    76,    77,   324,     0,    93,
    94,    95,    90,    88,     0,     0,     0,    84,     0,   333,
     0,     0,     0,   337,    79,    82,    83,     0,     0,     0,
     0,    80,    81,     0,     0,    78,     0,    79,    82,    83,
   163,    91,   162,     0,    80,    81,   284,     0,    78,   258,
   259,     0,     0,     0,    91,    29,   123,     0,   164,     0,
   357,     0,     0,     0,     0,     0,    76,    77,    84,     0,
    93,    94,    95,    90,    88,     0,     0,     0,     0,    76,
    77,    84,     0,    93,    94,    95,    90,    88,    79,    82,
    83,     0,    71,    73,     0,    80,    81,     0,     0,    78,
     0,    79,    82,    83,     0,    91,     0,   122,    80,    81,
    69,   109,    78,     0,     0,     0,   109,     0,   157,     0,
     0,     0,     0,    10,     0,     0,     0,     0,     0,     0,
    76,    77,    84,    12,    93,    94,    95,    90,    88,     0,
     0,     0,     0,    76,    77,     0,     0,    93,    94,    95,
    90,    88,    79,    82,    83,     0,     0,     0,    57,    80,
    81,     0,     0,    78,   122,     0,     0,   239,    56,   147,
   146,   148,   150,   150,   150,   150,   150,   150,   156,   112,
     0,     0,   245,     0,   109,     0,     0,     0,     0,   111,
     0,     0,     0,     0,    76,    77,     0,     0,    93,    94,
    95,    90,    88,     0,     0,     0,     0,   109,     0,     0,
     0,     0,     0,     0,     0,   165,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,    71,     0,   150,     0,     0,     0,   165,     0,     0,
     0,     0,     0,   187,     0,     0,     0,     0,     0,     0,
     0,     0,   150,   150,     0,     0,     0,    71,     0,     0,
   150,   150,   150,   150,   150,   150,   150,   150,   150,   150,
   150,   150,   150,   150,   150,     0,     0,     0,     0,     0,
     0,   165,     0,     0,     0,     0,     0,     0,   165,   165,
     0,     0,     0,     0,   322,     0,   256,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,   165,     0,     0,   165,     0,
    71,     0,     0,   187,     0,     0,     0,     0,     0,     0,
     0,     0,    71,     0,     0,     0,    71,     0,     0,     0,
     0,     0,     0,     0,     0,     0,   150,     0,     0,     0,
     0,     0,     0,     0,     0,   122,     0,     0,     0,   352,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,    71,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,   256,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,   150,
     0,     0,     0,     0,     0,     0,   165 };
yytabelem yypact[]={

    72,    72,-10000000,-10000000,-10000000,   260,  -243,-10000000,   260,   260,
   260,  -124,  -280,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,
-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,
-10000000,-10000000,-10000000,  -259,     5,  -284,  -304,-10000000,-10000000,-10000000,
  -119,   260,-10000000,  -243,  -222,-10000000,   -75,-10000000,-10000000,-10000000,
-10000000,   529,   151,  -124,   -92,     5,-10000000,-10000000,  -123,-10000000,
-10000000,  -136,   260,  -171,-10000000,  -222,   478,-10000000,-10000000,-10000000,
  -184,-10000000,-10000000,   -38,  -206,  -218,   593,   593,   404,   404,
   404,   404,   404,   404,   542,  -257,-10000000,     1,-10000000,-10000000,
-10000000,  -267,     4,-10000000,-10000000,-10000000,    10,  -239,  -152,  -252,
    -3,   -12,-10000000,-10000000,   -93,  -102,   -73,-10000000,-10000000,  -272,
-10000000,-10000000,-10000000,   445,  -179,  -314,-10000000,-10000000,  -158,   260,
   260,-10000000,-10000000,   478,  -259,-10000000,   404,-10000000,-10000000,-10000000,
-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,   404,   404,
   404,   404,  -299,  -307,-10000000,-10000000,-10000000,   404,-10000000,-10000000,
-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,  -267,   404,   404,
   -95,  -110,  -157,-10000000,   445,   445,   404,   404,   404,   404,
   404,   404,   404,   404,   404,   404,   404,   404,   404,   404,
   404,-10000000,-10000000,  -302,   227,-10000000,-10000000,  -279,  -180,   -15,
   465,   359,-10000000,  -244,   445,  -121,-10000000,  -233,  -314,-10000000,
  -215,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,
-10000000,-10000000,-10000000,-10000000,-10000000,   -50,   404,   -51,   -56,   260,
   -80,   -81,   -84,   -98,   -85,  -303,   404,   -57,   -58,   -74,
-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,  -122,-10000000,-10000000,-10000000,
   -82,  -257,  -177,   -96,   -78,-10000000,-10000000,   404,-10000000,  -100,
     1,     4,   404,   404,-10000000,-10000000,  -154,   110,-10000000,-10000000,
    10,  -239,  -152,  -152,  -252,  -252,  -252,  -252,    -3,    -3,
   -12,   -12,-10000000,-10000000,-10000000,-10000000,-10000000,-10000000,  -180,   380,
   189,  -101,-10000000,  -103,-10000000,  -199,-10000000,-10000000,   -86,-10000000,
   -52,   404,   345,  -173,  -314,   404,  -121,-10000000,-10000000,   -98,
   -53,   -98,-10000000,-10000000,   404,   404,   404,    40,   404,   -59,
   -60,-10000000,-10000000,-10000000,   280,   404,-10000000,-10000000,   404,  -186,
-10000000,-10000000,-10000000,-10000000,  -207,-10000000,  -107,-10000000,-10000000,-10000000,
-10000000,-10000000,   404,-10000000,-10000000,-10000000,-10000000,-10000000,  -174,-10000000,
   -98,-10000000,  -111,  -117,  -150,   -89,   -62,-10000000,-10000000,-10000000,
-10000000,-10000000,-10000000,  -334,-10000000,-10000000,  -244,-10000000,-10000000,-10000000,
   -98,   -98,   -98,   404,   404,-10000000,-10000000,    42,-10000000,-10000000,
  -161,   -68,   -98,   -69,   404,-10000000,-10000000,  -149,   -98,-10000000 };
yytabelem yypgo[]={

     0,   565,   326,   564,   853,   562,     0,   560,   559,   330,
   558,   557,   556,   555,   554,   553,   528,   527,   526,   524,
   523,   522,     6,     1,   770,    84,   457,   521,   308,   321,
   327,   440,   320,   309,   401,   409,   374,   402,   823,   520,
   519,   518,   456,   454,   453,   452,   449,   656,   614,   301,
   314,   447,   307,   322,   316,   446,     7,   445,   209,   415,
   444,   297,   317,   405,   306,   863,   331,   495,   443,   442,
   419,    89,   414,   413,   335,   408,   323,   407,   406,   375,
   371,   369,   368,   365,   340,   339,   338,   333,   332,   318,
   315 };
yytabelem yyr1[]={

     0,     1,     1,     2,     2,    75,     3,    77,     3,    46,
    46,    74,    74,    76,    76,    47,    47,    47,    47,    47,
    47,    70,    70,    70,    70,    70,    67,    67,    67,    67,
    67,    67,    67,    67,    67,    67,    67,    67,     4,     4,
    78,    69,    79,    69,    69,    60,    60,    53,    53,    55,
    80,    55,    54,    54,    49,    48,    48,    48,    48,    57,
    81,    57,    56,    56,    56,    82,    83,    68,    84,    85,
    68,    68,    62,    62,    61,    61,    58,    58,    63,    63,
    63,    63,    63,    63,    63,    65,    65,    65,    65,     5,
     5,    50,    50,    51,    51,    52,    52,    52,    42,    42,
    71,    71,    71,    72,    72,    66,    66,    59,    59,    59,
    64,    64,    64,    64,    64,    64,    64,    64,    64,     6,
     6,     6,     6,     6,     6,     6,     6,     6,     6,     6,
     6,     6,    14,    17,    17,    10,     8,     9,     9,    87,
    87,    86,     7,     7,    18,    18,    88,    16,    89,    19,
    90,    20,    45,    21,    23,    23,    15,    11,    12,    13,
    22,    22,    24,    24,    73,    73,    73,    73,    73,    73,
    73,    73,    73,    73,    73,    25,    25,    26,    27,    27,
    28,    28,    29,    29,    30,    30,    31,    31,    32,    32,
    32,    33,    33,    33,    33,    33,    34,    34,    34,    35,
    35,    35,    36,    36,    36,    36,    37,    37,    38,    38,
    38,    38,    38,    38,    38,    38,    38,    38,    38,    39,
    39,    39,    39,    39,    39,    39,    39,    40,    40,    40,
    40,    43,    43,    44,    44,    41,    41,    41 };
yytabelem yyr2[]={

     0,     3,     5,     3,     3,     1,     9,     1,    11,     3,
     5,     5,     7,     2,     4,     3,     5,     3,     5,     3,
     5,     3,     3,     3,     3,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
     1,    11,     1,    13,     5,     3,     3,     3,     5,     3,
     1,     9,     3,     7,     7,     3,     5,     3,     5,     3,
     1,     9,     3,     5,     7,     1,     1,    13,     1,     1,
    15,     5,     3,     7,     3,     7,     3,     5,     3,     7,
     7,     9,     7,     9,     9,     3,     5,     5,     7,     3,
     5,     3,     7,     3,     7,     5,     3,     5,     3,     7,
     3,     7,     9,     3,     7,     3,     5,     3,     3,     5,
     7,     5,     7,     7,     9,     5,     7,     7,     9,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     7,     9,     7,     5,     5,     5,     7,     0,
     2,     3,     3,     5,    11,    15,     3,    11,     3,    11,
     3,    15,     3,    19,     1,     3,     7,     5,     5,     7,
     3,     7,     2,     7,     3,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     3,     2,    11,     3,     2,     7,
     2,     7,     2,     7,     2,     7,     2,     7,     2,     7,
     7,     2,     7,     7,     7,     7,     2,     7,     7,     2,
     7,     7,     2,     7,     7,     7,     2,     9,     3,     5,
     5,     5,     5,     5,     5,     5,     5,     5,     9,     2,
     9,     9,     7,    13,     7,     5,     5,     3,     3,     3,
     7,     1,     3,     3,     7,     3,     3,     3 };
yytabelem yychk[]={

-10000000,    -1,    -2,    -3,   -74,   -46,   -47,   -58,   -70,   -67,
    -4,   -63,   -65,   270,   271,   272,   273,   274,   275,   276,
   277,   278,   279,   280,   281,   282,   283,   -69,   -68,   340,
   284,   285,   339,   306,   300,   -60,   288,   286,   287,    -2,
   -75,   -76,   -74,   -47,   -58,   304,   -55,   -54,   -47,   -47,
   -47,   310,   306,   -63,   -58,    -5,   -65,    -4,   -78,   339,
   308,   339,   308,   -77,   -74,   -58,   312,   304,   305,   311,
   -26,   -24,   -25,   -38,   -27,   -39,   331,   332,   300,   289,
   296,   297,   290,   291,   269,   -28,   -40,   -29,   339,   -41,
   338,   306,   -30,   335,   336,   337,   -31,   -32,   -33,   -34,
   -35,   -36,   -37,   307,   -50,   -42,   -51,   339,   -52,   -47,
   307,   -65,    -4,   308,   -79,   -82,   308,    -9,   -87,   -76,
   308,   -71,   -24,   308,   -80,   311,   -73,   312,   313,   314,
   315,   316,   317,   318,   319,   320,   321,   322,   303,   324,
   310,   306,   301,   333,   331,   332,   -38,   306,   -38,   -37,
   -38,   -37,   -37,   -37,   -37,   -37,   -38,   306,   323,   295,
   -66,   -22,   -48,   -24,   -67,    -4,   294,   289,   325,   326,
   292,   293,   328,   327,   329,   330,   296,   297,   300,   298,
   299,   307,   307,   305,   305,   -58,   -59,   -65,   -64,   306,
   310,   -53,   -49,   -48,   308,   -62,   -61,   339,   -84,   309,
    -7,    -6,   -14,   -17,   -10,    -8,   -18,   -16,   -19,   -20,
   -21,   -15,   -13,   -11,   -12,   339,   266,   267,   -23,   -86,
   261,   -88,   -89,   -90,   -45,   268,   260,   259,   258,   -22,
   308,   265,   263,   264,   257,    -9,   -72,   -71,   -54,   -24,
   -22,   -28,   -22,   -43,   -44,   -24,   339,   310,   339,   -66,
   -29,   -30,   307,   305,   307,   -59,   -65,   306,   -48,   -48,
   -31,   -32,   -33,   -33,   -34,   -34,   -34,   -34,   -35,   -35,
   -36,   -36,   -37,   -37,   -37,   339,   334,   -52,   -64,   310,
   306,   -59,   307,   -50,   311,   -26,   309,   -49,   -57,   -56,
   -58,   302,   -53,   -83,   305,   312,   -62,   309,    -6,   302,
   -26,   302,   304,    -9,   306,   306,   306,    -6,   306,   339,
   -23,   304,   304,   309,   305,   302,   311,   307,   305,   -22,
   307,   -37,   -24,   311,   -26,   307,   -50,   307,   307,   311,
   304,   305,   302,   -26,   309,   309,   -61,   -26,   -85,    -6,
   302,    -6,   -22,   -22,   -22,   263,   -23,   304,   304,   309,
   -71,   -25,   -24,   311,   311,   307,   -81,   -26,   309,    -6,
   307,   307,   307,   306,   304,   339,   -56,    -6,    -6,    -6,
   -22,   -23,   262,   307,   304,    -6,   304,   -23,   307,    -6 };
yytabelem yydef[]={

     0,    -2,     1,     3,     4,     5,     0,     9,    15,    17,
    19,    76,     0,    21,    22,    23,    24,    25,    26,    27,
    28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
    38,    39,    78,     0,    85,    40,     0,    45,    46,     2,
     0,     7,    13,     0,    -2,    11,     0,    49,    16,    18,
    20,     0,     0,    77,     0,    86,    87,    89,     0,    -2,
    65,    71,   139,     0,    14,    52,     0,    12,    50,    80,
     0,   177,   162,   206,   175,   208,     0,     0,     0,     0,
     0,     0,     0,     0,     0,   178,   219,   180,   227,   228,
   229,     0,   182,   235,   236,   237,   184,   186,   188,   191,
   196,   199,   202,    82,     0,     0,    91,    98,    93,    96,
    79,    88,    90,     0,     0,     0,    68,     6,   154,   140,
   139,    53,   100,     0,     0,    81,     0,   164,   165,   166,
   167,   168,   169,   170,   171,   172,   173,   174,     0,     0,
     0,   231,     0,     0,   225,   226,   209,     0,   210,   211,
   206,   212,   213,   214,   215,   216,   217,     0,     0,     0,
     0,     0,   105,   160,    55,    57,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,    83,    84,     0,     0,    95,    97,   107,   108,     0,
     0,     0,    47,     0,     0,    66,    72,    74,     0,   137,
   154,   142,   119,   120,   121,   122,   123,   124,   125,   126,
   127,   128,   129,   130,   131,   227,     0,     0,     0,   139,
     0,     0,     0,   154,     0,     0,   154,     0,     0,   155,
   141,   146,   148,   150,   152,     8,     0,   103,    51,   163,
     0,   179,     0,     0,   232,   233,   222,     0,   224,     0,
   181,   183,     0,     0,   230,   106,   107,     0,    56,    58,
   185,   187,   189,   190,   192,   193,   194,   195,   197,   198,
   200,   201,   203,   204,   205,    99,    92,    94,   109,     0,
     0,     0,   115,     0,   111,     0,    41,    48,     0,    59,
    62,     0,     0,     0,     0,     0,    69,   138,   143,   154,
     0,   154,   135,   136,     0,     0,     0,     0,   154,     0,
     0,   157,   158,   101,     0,     0,   220,   221,     0,     0,
   218,   207,   161,   113,     0,   117,     0,   110,   116,   112,
    54,    60,     0,    63,    43,    67,    73,    75,     0,   132,
   154,   134,     0,     0,     0,     0,     0,   156,   159,   102,
   104,   176,   234,     0,   114,   118,     0,    64,    70,   133,
   154,   154,   154,     0,   154,   223,    61,   144,   147,   149,
     0,     0,   154,     0,   154,   145,   151,     0,   154,   153 };
typedef struct
#ifdef __cplusplus
	yytoktype
#endif
{ char *t_name; int t_val; } yytoktype;
#ifndef YYDEBUG
#	define YYDEBUG	0	/* don't allow debugging */
#endif

#if YYDEBUG

yytoktype yytoks[] =
{
	"FOR",	257,
	"BREAK",	258,
	"CONTINUE",	259,
	"RETURN",	260,
	"IF",	261,
	"ELSE",	262,
	"WHILE",	263,
	"DO",	264,
	"SWITCH",	265,
	"CASE",	266,
	"DEFAULT",	267,
	"GOTO",	268,
	"SIZEOF",	269,
	"AUTO",	270,
	"REGISTER",	271,
	"STATIC",	272,
	"EXTERN",	273,
	"TYPEDEF",	274,
	"VOID",	275,
	"CHAR",	276,
	"SHORT",	277,
	"INT",	278,
	"LONG",	279,
	"FLOAT",	280,
	"DOUBLE",	281,
	"SIGNED",	282,
	"UNSIGNED",	283,
	"CONST",	284,
	"VOLATILE",	285,
	"STRUCT",	286,
	"UNION",	287,
	"ENUM",	288,
	"AND",	289,
	"TILDE",	290,
	"NOT",	291,
	"LESSTHAN",	292,
	"GREATERTHAN",	293,
	"XOR",	294,
	"OR",	295,
	"PLUS",	296,
	"MINUS",	297,
	"SLASH",	298,
	"PERCENT",	299,
	"STAR",	300,
	"DOT",	301,
	"COLON",	302,
	"QUERY",	303,
	"SEMI",	304,
	"COMMA",	305,
	"LPAREN",	306,
	"RPAREN",	307,
	"LBRACE",	308,
	"RBRACE",	309,
	"LBRAC",	310,
	"RBRAC",	311,
	"EQUALS",	312,
	"STAR_EQUALS",	313,
	"SLASH_EQUALS",	314,
	"PERCENT_EQUALS",	315,
	"PLUS_EQUALS",	316,
	"MINUS_EQUALS",	317,
	"LSHIFT_EQUALS",	318,
	"RSHIFT_EQUALS",	319,
	"AND_EQUALS",	320,
	"XOR_EQUALS",	321,
	"OR_EQUALS",	322,
	"ANDAND",	323,
	"OROR",	324,
	"EQEQ",	325,
	"NOTEQ",	326,
	"GTEQ",	327,
	"LESSEQ",	328,
	"LSHIFT",	329,
	"RSHIFT",	330,
	"PLUSPLUS",	331,
	"MINUSMINUS",	332,
	"ARROW",	333,
	"ELLIPSIS",	334,
	"INTEGER_CONSTANT",	335,
	"CHARACTER_CONSTANT",	336,
	"FLOATING_CONSTANT",	337,
	"STRING_CONSTANT",	338,
	"IDENTIFIER",	339,
	"TYPEDEF_NAME",	340,
	"BADTOK",	341,
	"-unknown-",	-1	/* ends search */
};

char * yyreds[] =
{
	"-no such reduction-",
	"translation_unit : external_declaration",
	"translation_unit : translation_unit external_declaration",
	"external_declaration : function_definition",
	"external_declaration : declaration",
	"function_definition : function_declaration",
	"function_definition : function_declaration LBRACE compound_statement_body",
	"function_definition : function_declaration declaration_list",
	"function_definition : function_declaration declaration_list LBRACE compound_statement_body",
	"function_declaration : declarator",
	"function_declaration : declaration_specifiers declarator",
	"declaration : declaration_specifiers SEMI",
	"declaration : declaration_specifiers init_declarator_list SEMI",
	"declaration_list : declaration",
	"declaration_list : declaration_list declaration",
	"declaration_specifiers : storage_class_specifier",
	"declaration_specifiers : storage_class_specifier declaration_specifiers",
	"declaration_specifiers : type_specifier",
	"declaration_specifiers : type_specifier declaration_specifiers",
	"declaration_specifiers : type_qualifier",
	"declaration_specifiers : type_qualifier declaration_specifiers",
	"storage_class_specifier : AUTO",
	"storage_class_specifier : REGISTER",
	"storage_class_specifier : STATIC",
	"storage_class_specifier : EXTERN",
	"storage_class_specifier : TYPEDEF",
	"type_specifier : VOID",
	"type_specifier : CHAR",
	"type_specifier : SHORT",
	"type_specifier : INT",
	"type_specifier : LONG",
	"type_specifier : FLOAT",
	"type_specifier : DOUBLE",
	"type_specifier : SIGNED",
	"type_specifier : UNSIGNED",
	"type_specifier : struct_or_union_specifier",
	"type_specifier : enum_specifier",
	"type_specifier : TYPEDEF_NAME",
	"type_qualifier : CONST",
	"type_qualifier : VOLATILE",
	"struct_or_union_specifier : struct_or_union",
	"struct_or_union_specifier : struct_or_union LBRACE struct_declaration_list RBRACE",
	"struct_or_union_specifier : struct_or_union IDENTIFIER",
	"struct_or_union_specifier : struct_or_union IDENTIFIER LBRACE struct_declaration_list RBRACE",
	"struct_or_union_specifier : struct_or_union IDENTIFIER",
	"struct_or_union : STRUCT",
	"struct_or_union : UNION",
	"struct_declaration_list : struct_declaration",
	"struct_declaration_list : struct_declaration_list struct_declaration",
	"init_declarator_list : init_declarator",
	"init_declarator_list : init_declarator_list COMMA",
	"init_declarator_list : init_declarator_list COMMA init_declarator",
	"init_declarator : declarator",
	"init_declarator : declarator EQUALS initialiser",
	"struct_declaration : specifier_qualifier_list struct_declarator_list SEMI",
	"specifier_qualifier_list : type_specifier",
	"specifier_qualifier_list : type_specifier specifier_qualifier_list",
	"specifier_qualifier_list : type_qualifier",
	"specifier_qualifier_list : type_qualifier specifier_qualifier_list",
	"struct_declarator_list : struct_declarator",
	"struct_declarator_list : struct_declarator_list COMMA",
	"struct_declarator_list : struct_declarator_list COMMA struct_declarator",
	"struct_declarator : declarator",
	"struct_declarator : COLON constant_expression",
	"struct_declarator : declarator COLON constant_expression",
	"enum_specifier : ENUM LBRACE",
	"enum_specifier : ENUM LBRACE enumerator_list",
	"enum_specifier : ENUM LBRACE enumerator_list RBRACE",
	"enum_specifier : ENUM IDENTIFIER LBRACE",
	"enum_specifier : ENUM IDENTIFIER LBRACE enumerator_list",
	"enum_specifier : ENUM IDENTIFIER LBRACE enumerator_list RBRACE",
	"enum_specifier : ENUM IDENTIFIER",
	"enumerator_list : enumerator",
	"enumerator_list : enumerator_list COMMA enumerator",
	"enumerator : IDENTIFIER",
	"enumerator : IDENTIFIER EQUALS constant_expression",
	"declarator : direct_declarator",
	"declarator : pointer direct_declarator",
	"direct_declarator : IDENTIFIER",
	"direct_declarator : LPAREN declarator RPAREN",
	"direct_declarator : direct_declarator LBRAC RBRAC",
	"direct_declarator : direct_declarator LBRAC constant_expression RBRAC",
	"direct_declarator : direct_declarator LPAREN RPAREN",
	"direct_declarator : direct_declarator LPAREN parameter_type_list RPAREN",
	"direct_declarator : direct_declarator LPAREN identifier_list RPAREN",
	"pointer : STAR",
	"pointer : STAR type_qualifier_list",
	"pointer : STAR pointer",
	"pointer : STAR type_qualifier_list pointer",
	"type_qualifier_list : type_qualifier",
	"type_qualifier_list : type_qualifier_list type_qualifier",
	"parameter_type_list : parameter_list",
	"parameter_type_list : parameter_list COMMA ELLIPSIS",
	"parameter_list : parameter_declaration",
	"parameter_list : parameter_list COMMA parameter_declaration",
	"parameter_declaration : declaration_specifiers declarator",
	"parameter_declaration : declaration_specifiers",
	"parameter_declaration : declaration_specifiers abstract_declarator",
	"identifier_list : IDENTIFIER",
	"identifier_list : identifier_list COMMA IDENTIFIER",
	"initialiser : assignment_expression",
	"initialiser : LBRACE initialiser_list RBRACE",
	"initialiser : LBRACE initialiser_list COMMA RBRACE",
	"initialiser_list : initialiser",
	"initialiser_list : initialiser_list COMMA initialiser",
	"type_name : specifier_qualifier_list",
	"type_name : specifier_qualifier_list abstract_declarator",
	"abstract_declarator : pointer",
	"abstract_declarator : direct_abstract_declarator",
	"abstract_declarator : pointer direct_abstract_declarator",
	"direct_abstract_declarator : LPAREN abstract_declarator RPAREN",
	"direct_abstract_declarator : LBRAC RBRAC",
	"direct_abstract_declarator : LBRAC constant_expression RBRAC",
	"direct_abstract_declarator : direct_abstract_declarator LBRAC RBRAC",
	"direct_abstract_declarator : direct_abstract_declarator LBRAC constant_expression RBRAC",
	"direct_abstract_declarator : LPAREN RPAREN",
	"direct_abstract_declarator : LPAREN parameter_type_list RPAREN",
	"direct_abstract_declarator : direct_abstract_declarator LPAREN RPAREN",
	"direct_abstract_declarator : direct_abstract_declarator LPAREN parameter_type_list RPAREN",
	"statement : labeled_statement",
	"statement : case_labeled_statement",
	"statement : expression_statement",
	"statement : compound_statement",
	"statement : if_statement",
	"statement : switch_statement",
	"statement : while_statement",
	"statement : do_statement",
	"statement : for_statement",
	"statement : goto_statement",
	"statement : return_statement",
	"statement : continue_statement",
	"statement : break_statement",
	"labeled_statement : IDENTIFIER COLON statement",
	"case_labeled_statement : CASE constant_expression COLON statement",
	"case_labeled_statement : DEFAULT COLON statement",
	"expression_statement : opt_expression SEMI",
	"compound_statement : start_block compound_statement_body",
	"compound_statement_body : compound_statement_declarations RBRACE",
	"compound_statement_body : compound_statement_declarations statement_list RBRACE",
	"compound_statement_declarations : /* empty */",
	"compound_statement_declarations : declaration_list",
	"start_block : LBRACE",
	"statement_list : statement",
	"statement_list : statement_list statement",
	"if_statement : IF LPAREN expression RPAREN statement",
	"if_statement : IF LPAREN expression RPAREN statement ELSE statement",
	"switch : SWITCH",
	"switch_statement : switch LPAREN expression RPAREN statement",
	"while : WHILE",
	"while_statement : while LPAREN expression RPAREN statement",
	"do : DO",
	"do_statement : do statement WHILE LPAREN expression RPAREN SEMI",
	"for : FOR",
	"for_statement : for LPAREN opt_expression SEMI opt_expression SEMI opt_expression RPAREN statement",
	"opt_expression : /* empty */",
	"opt_expression : expression",
	"goto_statement : GOTO IDENTIFIER SEMI",
	"continue_statement : CONTINUE SEMI",
	"break_statement : BREAK SEMI",
	"return_statement : RETURN opt_expression SEMI",
	"expression : assignment_expression",
	"expression : expression COMMA assignment_expression",
	"assignment_expression : conditional_expression",
	"assignment_expression : unary_expression assignment_operator assignment_expression",
	"assignment_operator : EQUALS",
	"assignment_operator : STAR_EQUALS",
	"assignment_operator : SLASH_EQUALS",
	"assignment_operator : PERCENT_EQUALS",
	"assignment_operator : PLUS_EQUALS",
	"assignment_operator : MINUS_EQUALS",
	"assignment_operator : LSHIFT_EQUALS",
	"assignment_operator : RSHIFT_EQUALS",
	"assignment_operator : AND_EQUALS",
	"assignment_operator : XOR_EQUALS",
	"assignment_operator : OR_EQUALS",
	"conditional_expression : logical_or_expression",
	"conditional_expression : logical_or_expression QUERY expression COLON conditional_expression",
	"constant_expression : assignment_expression",
	"logical_or_expression : logical_and_expression",
	"logical_or_expression : logical_or_expression OROR logical_and_expression",
	"logical_and_expression : inclusive_or_expression",
	"logical_and_expression : logical_and_expression ANDAND inclusive_or_expression",
	"inclusive_or_expression : exclusive_or_expression",
	"inclusive_or_expression : inclusive_or_expression OR exclusive_or_expression",
	"exclusive_or_expression : and_expression",
	"exclusive_or_expression : exclusive_or_expression XOR and_expression",
	"and_expression : equality_expression",
	"and_expression : and_expression AND equality_expression",
	"equality_expression : relational_expression",
	"equality_expression : equality_expression EQEQ relational_expression",
	"equality_expression : equality_expression NOTEQ relational_expression",
	"relational_expression : shift_expression",
	"relational_expression : relational_expression LESSTHAN shift_expression",
	"relational_expression : relational_expression GREATERTHAN shift_expression",
	"relational_expression : relational_expression LESSEQ shift_expression",
	"relational_expression : relational_expression GTEQ shift_expression",
	"shift_expression : additive_expression",
	"shift_expression : shift_expression LSHIFT additive_expression",
	"shift_expression : shift_expression RSHIFT additive_expression",
	"additive_expression : multiplicative_expression",
	"additive_expression : additive_expression PLUS multiplicative_expression",
	"additive_expression : additive_expression MINUS multiplicative_expression",
	"multiplicative_expression : cast_expression",
	"multiplicative_expression : multiplicative_expression STAR cast_expression",
	"multiplicative_expression : multiplicative_expression SLASH cast_expression",
	"multiplicative_expression : multiplicative_expression PERCENT cast_expression",
	"cast_expression : unary_expression",
	"cast_expression : LPAREN type_name RPAREN cast_expression",
	"unary_expression : postfix_expression",
	"unary_expression : PLUSPLUS unary_expression",
	"unary_expression : MINUSMINUS unary_expression",
	"unary_expression : STAR cast_expression",
	"unary_expression : AND cast_expression",
	"unary_expression : PLUS cast_expression",
	"unary_expression : MINUS cast_expression",
	"unary_expression : TILDE cast_expression",
	"unary_expression : NOT cast_expression",
	"unary_expression : SIZEOF unary_expression",
	"unary_expression : SIZEOF LPAREN type_name RPAREN",
	"postfix_expression : primary_expression",
	"postfix_expression : postfix_expression LBRAC expression RBRAC",
	"postfix_expression : postfix_expression LPAREN opt_argument_expression_list RPAREN",
	"postfix_expression : postfix_expression DOT IDENTIFIER",
	"postfix_expression : postfix_expression ARROW LBRAC expression RBRAC IDENTIFIER",
	"postfix_expression : postfix_expression ARROW IDENTIFIER",
	"postfix_expression : postfix_expression PLUSPLUS",
	"postfix_expression : postfix_expression MINUSMINUS",
	"primary_expression : IDENTIFIER",
	"primary_expression : constant",
	"primary_expression : STRING_CONSTANT",
	"primary_expression : LPAREN expression RPAREN",
	"opt_argument_expression_list : /* empty */",
	"opt_argument_expression_list : argument_expression_list",
	"argument_expression_list : assignment_expression",
	"argument_expression_list : argument_expression_list COMMA assignment_expression",
	"constant : INTEGER_CONSTANT",
	"constant : CHARACTER_CONSTANT",
	"constant : FLOATING_CONSTANT",
};
#endif /* YYDEBUG */
/*
 * Copyright (c) 1993 by Sun Microsystems, Inc.
 */

#pragma ident	"@(#)yaccpar	6.12	93/06/07 SMI"

/*
** Skeleton parser driver for yacc output
*/

/*
** yacc user known macros and defines
*/
#define YYERROR		goto yyerrlab
#define YYACCEPT	return(0)
#define YYABORT		return(1)
#define YYBACKUP( newtoken, newvalue )\
{\
	if ( yychar >= 0 || ( yyr2[ yytmp ] >> 1 ) != 1 )\
	{\
		yyerror( "syntax error - cannot backup" );\
		goto yyerrlab;\
	}\
	yychar = newtoken;\
	yystate = *yyps;\
	yylval = newvalue;\
	goto yynewstate;\
}
#define YYRECOVERING()	(!!yyerrflag)
#define YYNEW(type)	malloc((size_t)sizeof(type) * yynewmax)
#define YYCOPY(to, from, type) \
	(type *) memcpy(to, (char *) from, yynewmax * sizeof(type))
#define YYENLARGE( from, type) \
	(type *) realloc((char *) from, yynewmax * sizeof(type))
#ifndef YYDEBUG
#	define YYDEBUG	1	/* make debugging available */
#endif

/*
** user known globals
*/
int yydebug;			/* set to 1 to get debugging */

/*
** driver internal defines
*/
#define YYFLAG		(-10000000)

/*
** global variables used by the parser
*/
YYSTYPE *yypv;			/* top of value stack */
int *yyps;			/* top of state stack */

int yystate;			/* current state */
int yytmp;			/* extra var (lasts between blocks) */

int yynerrs;			/* number of errors */
int yyerrflag;			/* error recovery flag */
int yychar;			/* current input token number */



#ifdef YYNMBCHARS
#define YYLEX()		yycvtok(yylex())
/*
** yycvtok - return a token if i is a wchar_t value that exceeds 255.
**	If i<255, i itself is the token.  If i>255 but the neither 
**	of the 30th or 31st bit is on, i is already a token.
*/
#if defined(__STDC__) || defined(__cplusplus)
int yycvtok(int i)
#else
int yycvtok(i) int i;
#endif
{
	int first = 0;
	int last = YYNMBCHARS - 1;
	int mid;
	wchar_t j;

	if(i&0x60000000){/*Must convert to a token. */
		if( yymbchars[last].character < i ){
			return i;/*Giving up*/
		}
		while ((last>=first)&&(first>=0)) {/*Binary search loop*/
			mid = (first+last)/2;
			j = yymbchars[mid].character;
			if( j==i ){/*Found*/ 
				return yymbchars[mid].tvalue;
			}else if( j<i ){
				first = mid + 1;
			}else{
				last = mid -1;
			}
		}
		/*No entry in the table.*/
		return i;/* Giving up.*/
	}else{/* i is already a token. */
		return i;
	}
}
#else/*!YYNMBCHARS*/
#define YYLEX()		yylex()
#endif/*!YYNMBCHARS*/

/*
** yyparse - return 0 if worked, 1 if syntax error not recovered from
*/
#if defined(__STDC__) || defined(__cplusplus)
int yyparse(void)
#else
int yyparse()
#endif
{
	register YYSTYPE *yypvt;	/* top of value stack for $vars */

#if defined(__cplusplus) || defined(lint)
/*
	hacks to please C++ and lint - goto's inside switch should never be
	executed; yypvt is set to 0 to avoid "used before set" warning.
*/
	static int __yaccpar_lint_hack__ = 0;
	switch (__yaccpar_lint_hack__)
	{
		case 1: goto yyerrlab;
		case 2: goto yynewstate;
	}
	yypvt = 0;
#endif

	/*
	** Initialize externals - yyparse may be called more than once
	*/
	yypv = &yyv[-1];
	yyps = &yys[-1];
	yystate = 0;
	yytmp = 0;
	yynerrs = 0;
	yyerrflag = 0;
	yychar = -1;

#if YYMAXDEPTH <= 0
	if (yymaxdepth <= 0)
	{
		if ((yymaxdepth = YYEXPAND(0)) <= 0)
		{
			yyerror("yacc initialization error");
			YYABORT;
		}
	}
#endif

	{
		register YYSTYPE *yy_pv;	/* top of value stack */
		register int *yy_ps;		/* top of state stack */
		register int yy_state;		/* current state */
		register int  yy_n;		/* internal state number info */
	goto yystack;	/* moved from 6 lines above to here to please C++ */

		/*
		** get globals into registers.
		** branch to here only if YYBACKUP was called.
		*/

		/*
		** get globals into registers.
		** either we just started, or we just finished a reduction
		*/
	yystack:
		yy_pv = yypv;
		yy_ps = yyps;
		yy_state = yystate;

		/*
		** top of for (;;) loop while no reductions done
		*/
	yy_stack:
		/*
		** put a state and value onto the stacks
		*/
#if YYDEBUG
		/*
		** if debugging, look up token value in list of value vs.
		** name pairs.  0 and negative (-1) are special values.
		** Note: linear search is used since time is not a real
		** consideration while debugging.
		*/
		if ( yydebug )
		{
			register int yy_i;

			printf( "State %d, token ", yy_state );
			if ( yychar == 0 )
				printf( "end-of-file\n" );
			else if ( yychar < 0 )
				printf( "-none-\n" );
			else
			{
				for ( yy_i = 0; yytoks[yy_i].t_val >= 0;
					yy_i++ )
				{
					if ( yytoks[yy_i].t_val == yychar )
						break;
				}
				printf( "%s\n", yytoks[yy_i].t_name );
			}
		}
#endif /* YYDEBUG */
		if ( ++yy_ps >= &yys[ yymaxdepth ] )	/* room on stack? */
		{
			/*
			** reallocate and recover.  Note that pointers
			** have to be reset, or bad things will happen
			*/
			int yyps_index = (yy_ps - yys);
			int yypv_index = (yy_pv - yyv);
			int yypvt_index = (yypvt - yyv);
			int yynewmax;
#ifdef YYEXPAND
			yynewmax = YYEXPAND(yymaxdepth);
#else
			yynewmax = 2 * yymaxdepth;	/* double table size */
			if (yymaxdepth == YYMAXDEPTH)	/* first time growth */
			{
				char *newyys = (char *)YYNEW(int);
				char *newyyv = (char *)YYNEW(YYSTYPE);
				if (newyys != 0 && newyyv != 0)
				{
					yys = YYCOPY(newyys, yys, int);
					yyv = YYCOPY(newyyv, yyv, YYSTYPE);
				}
				else
					yynewmax = 0;	/* failed */
			}
			else				/* not first time */
			{
				yys = YYENLARGE(yys, int);
				yyv = YYENLARGE(yyv, YYSTYPE);
				if (yys == 0 || yyv == 0)
					yynewmax = 0;	/* failed */
			}
#endif
			if (yynewmax <= yymaxdepth)	/* tables not expanded */
			{
				yyerror( "yacc stack overflow" );
				YYABORT;
			}
			yymaxdepth = yynewmax;

			yy_ps = yys + yyps_index;
			yy_pv = yyv + yypv_index;
			yypvt = yyv + yypvt_index;
		}
		*yy_ps = yy_state;
		*++yy_pv = yyval;

		/*
		** we have a new state - find out what to do
		*/
	yy_newstate:
		if ( ( yy_n = yypact[ yy_state ] ) <= YYFLAG )
			goto yydefault;		/* simple state */
#if YYDEBUG
		/*
		** if debugging, need to mark whether new token grabbed
		*/
		yytmp = yychar < 0;
#endif
		if ( ( yychar < 0 ) && ( ( yychar = YYLEX() ) < 0 ) )
			yychar = 0;		/* reached EOF */
#if YYDEBUG
		if ( yydebug && yytmp )
		{
			register int yy_i;

			printf( "Received token " );
			if ( yychar == 0 )
				printf( "end-of-file\n" );
			else if ( yychar < 0 )
				printf( "-none-\n" );
			else
			{
				for ( yy_i = 0; yytoks[yy_i].t_val >= 0;
					yy_i++ )
				{
					if ( yytoks[yy_i].t_val == yychar )
						break;
				}
				printf( "%s\n", yytoks[yy_i].t_name );
			}
		}
#endif /* YYDEBUG */
		if ( ( ( yy_n += yychar ) < 0 ) || ( yy_n >= YYLAST ) )
			goto yydefault;
		if ( yychk[ yy_n = yyact[ yy_n ] ] == yychar )	/*valid shift*/
		{
			yychar = -1;
			yyval = yylval;
			yy_state = yy_n;
			if ( yyerrflag > 0 )
				yyerrflag--;
			goto yy_stack;
		}

	yydefault:
		if ( ( yy_n = yydef[ yy_state ] ) == -2 )
		{
#if YYDEBUG
			yytmp = yychar < 0;
#endif
			if ( ( yychar < 0 ) && ( ( yychar = YYLEX() ) < 0 ) )
				yychar = 0;		/* reached EOF */
#if YYDEBUG
			if ( yydebug && yytmp )
			{
				register int yy_i;

				printf( "Received token " );
				if ( yychar == 0 )
					printf( "end-of-file\n" );
				else if ( yychar < 0 )
					printf( "-none-\n" );
				else
				{
					for ( yy_i = 0;
						yytoks[yy_i].t_val >= 0;
						yy_i++ )
					{
						if ( yytoks[yy_i].t_val
							== yychar )
						{
							break;
						}
					}
					printf( "%s\n", yytoks[yy_i].t_name );
				}
			}
#endif /* YYDEBUG */
			/*
			** look through exception table
			*/
			{
				register int *yyxi = yyexca;

				while ( ( *yyxi != -1 ) ||
					( yyxi[1] != yy_state ) )
				{
					yyxi += 2;
				}
				while ( ( *(yyxi += 2) >= 0 ) &&
					( *yyxi != yychar ) )
					;
				if ( ( yy_n = yyxi[1] ) < 0 )
					YYACCEPT;
			}
		}

		/*
		** check for syntax error
		*/
		if ( yy_n == 0 )	/* have an error */
		{
			/* no worry about speed here! */
			switch ( yyerrflag )
			{
			case 0:		/* new error */
				yyerror( "syntax error" );
				yynerrs++;
				/* FALLTHRU */
			case 1:
			case 2:		/* incompletely recovered error */
					/* try again... */
				yyerrflag = 3;
				/*
				** find state where "error" is a legal
				** shift action
				*/
				while ( yy_ps >= yys )
				{
					yy_n = yypact[ *yy_ps ] + YYERRCODE;
					if ( yy_n >= 0 && yy_n < YYLAST &&
						yychk[yyact[yy_n]] == YYERRCODE)					{
						/*
						** simulate shift of "error"
						*/
						yy_state = yyact[ yy_n ];
						goto yy_stack;
					}
					/*
					** current state has no shift on
					** "error", pop stack
					*/
#if YYDEBUG
#	define _POP_ "Error recovery pops state %d, uncovers state %d\n"
					if ( yydebug )
						printf( _POP_, *yy_ps,
							yy_ps[-1] );
#	undef _POP_
#endif
					yy_ps--;
					yy_pv--;
				}
				/*
				** there is no state on stack with "error" as
				** a valid shift.  give up.
				*/
				YYABORT;
			case 3:		/* no shift yet; eat a token */
#if YYDEBUG
				/*
				** if debugging, look up token in list of
				** pairs.  0 and negative shouldn't occur,
				** but since timing doesn't matter when
				** debugging, it doesn't hurt to leave the
				** tests here.
				*/
				if ( yydebug )
				{
					register int yy_i;

					printf( "Error recovery discards " );
					if ( yychar == 0 )
						printf( "token end-of-file\n" );
					else if ( yychar < 0 )
						printf( "token -none-\n" );
					else
					{
						for ( yy_i = 0;
							yytoks[yy_i].t_val >= 0;
							yy_i++ )
						{
							if ( yytoks[yy_i].t_val
								== yychar )
							{
								break;
							}
						}
						printf( "token %s\n",
							yytoks[yy_i].t_name );
					}
				}
#endif /* YYDEBUG */
				if ( yychar == 0 )	/* reached EOF. quit */
					YYABORT;
				yychar = -1;
				goto yy_newstate;
			}
		}/* end if ( yy_n == 0 ) */
		/*
		** reduction by production yy_n
		** put stack tops, etc. so things right after switch
		*/
#if YYDEBUG
		/*
		** if debugging, print the string that is the user's
		** specification of the reduction which is just about
		** to be done.
		*/
		if ( yydebug )
			printf( "Reduce by (%d) \"%s\"\n",
				yy_n, yyreds[ yy_n ] );
#endif
		yytmp = yy_n;			/* value to switch over */
		yypvt = yy_pv;			/* $vars top of value stack */
		/*
		** Look in goto table for next state
		** Sorry about using yy_state here as temporary
		** register variable, but why not, if it works...
		** If yyr2[ yy_n ] doesn't have the low order bit
		** set, then there is no action to be done for
		** this reduction.  So, no saving & unsaving of
		** registers done.  The only difference between the
		** code just after the if and the body of the if is
		** the goto yy_stack in the body.  This way the test
		** can be made before the choice of what to do is needed.
		*/
		{
			/* length of production doubled with extra bit */
			register int yy_len = yyr2[ yy_n ];

			if ( !( yy_len & 01 ) )
			{
				yy_len >>= 1;
				yyval = ( yy_pv -= yy_len )[1];	/* $$ = $1 */
				yy_state = yypgo[ yy_n = yyr1[ yy_n ] ] +
					*( yy_ps -= yy_len ) + 1;
				if ( yy_state >= YYLAST ||
					yychk[ yy_state =
					yyact[ yy_state ] ] != -yy_n )
				{
					yy_state = yyact[ yypgo[ yy_n ] ];
				}
				goto yy_stack;
			}
			yy_len >>= 1;
			yyval = ( yy_pv -= yy_len )[1];	/* $$ = $1 */
			yy_state = yypgo[ yy_n = yyr1[ yy_n ] ] +
				*( yy_ps -= yy_len ) + 1;
			if ( yy_state >= YYLAST ||
				yychk[ yy_state = yyact[ yy_state ] ] != -yy_n )
			{
				yy_state = yyact[ yypgo[ yy_n ] ];
			}
		}
					/* save until reenter driver code */
		yystate = yy_state;
		yyps = yy_ps;
		yypv = yy_pv;
	}
	/*
	** code supplied by user is placed in this switch
	*/
	switch( yytmp )
	{
		
case 1:
{
		yyval.undef = 0;
	} break;
case 2:
{
		yyval.undef = 0;
	} break;
case 3:
{
		if (Switch_level != 0 || Loop_level != 0)
			ci_panic("level botch in ed");
		yyval.undef = 0;
	} break;
case 4:
{
		if (Switch_level != 0 || Loop_level != 0)
			ci_panic("level botch in ed");
		yyval.undef = 0;
	} break;
case 5:
{
		ci_check_func_decls(yypvt[-0].function);
	} break;
case 6:
{
		ci_end_func(yypvt[-0].statement);
	} break;
case 7:
{
		ci_check_func_decls(yypvt[-1].function);
	} break;
case 8:
{
		ci_end_func(yypvt[-0].statement);
	} break;
case 9:
{
		declaration_t *dn;

		dn = ci_make_declaration(CL_DECL, ci_code_to_type(TY_INT), 0, yypvt[-0].declarator);
		yyval.function = ci_start_func(dn);
	} break;
case 10:
{
		declaration_t *dn;

		dn = ci_make_declaration(yypvt[-1].declaration->dn_class, ci_get_dn_basetype(yypvt[-1].declaration),
					 yypvt[-1].declaration->dn_qualifiers, yypvt[-0].declarator);
		yyval.function = ci_start_func(dn);
	} break;
case 11:
{
		ci_check_declaration(yypvt[-1].declaration);
	} break;
case 12:
{
		yypvt[-2].declaration->dn_declarators = yypvt[-1].declarator;
		ci_handle_declaration(yypvt[-2].declaration);
	} break;
case 15:
{
		yyval.declaration = ci_make_declaration(yypvt[-0].class, (type_t *)NULL, 0,
						(declarator_t *)NULL);
	} break;
case 16:
{
		ci_set_storage_class(yypvt[-0].declaration, yypvt[-1].class);
		yyval.declaration = yypvt[-0].declaration;
	} break;
case 17:
{
		yyval.declaration = ci_make_declaration(CL_NOCLASS, yypvt[-0].type, 0, (declarator_t *)NULL);
	} break;
case 18:
{
		ci_add_type_specifier(yypvt[-0].declaration, yypvt[-1].type);
		yyval.declaration = yypvt[-0].declaration;
	} break;
case 19:
{
		yyval.declaration = ci_make_declaration(CL_NOCLASS, (type_t *)NULL, yypvt[-0].qualifier,
							(declarator_t *)NULL);
	} break;
case 20:
{
		yypvt[-0].declaration->dn_qualifiers |= yypvt[-1].qualifier;
		yyval.declaration = yypvt[-0].declaration;
	} break;
case 21:
{ yyval.class = CL_AUTO;		} break;
case 22:
{ yyval.class = CL_NOCLASS;	} break;
case 23:
{ yyval.class = CL_STAT;		} break;
case 24:
{ yyval.class = CL_DECL;		} break;
case 25:
{ yyval.class = CL_TYPEDEF;	} break;
case 26:
{ yyval.type = ci_code_to_type(TY_VOID);	} break;
case 27:
{ yyval.type = ci_code_to_type(TY_CHAR);	} break;
case 28:
{ yyval.type = ci_code_to_type(TY_SHORT);	} break;
case 29:
{ yyval.type = ci_code_to_type(TY_INT);		} break;
case 30:
{ yyval.type = ci_code_to_type(TY_LONG);	} break;
case 31:
{ yyval.type = ci_code_to_type(TY_FLOAT);	} break;
case 32:
{ yyval.type = ci_code_to_type(TY_DOUBLE);	} break;
case 33:
{ yyval.type = ci_code_to_type(TY_SIGNED);	} break;
case 34:
{ yyval.type = ci_code_to_type(TY_UNSIGNED);	} break;
case 35:
{ yyval.type = yypvt[-0].type;				} break;
case 36:
{ yyval.type = yypvt[-0].type;				} break;
case 37:
{ yyval.type = ci_lookup_typedef(yypvt[-0].identifier->id_name);
} break;
case 38:
{ yyval.qualifier = QU_CONST; } break;
case 39:
{ yyval.qualifier = QU_VOLATILE; } break;
case 40:
{
		Labels_allowed = FALSE;
	} break;
case 41:
{
		Labels_allowed = TRUE;
		yyval.type = ci_build_aggr_or_enum_def(yypvt[-4].typecode, (identifier_t *)NULL,
				AE_COMPLETE, yypvt[-1].declaration, (enum_member_t *)NULL, FALSE);
	} break;
case 42:
{
		typecode_t typecode;

		/* Redeclaration Fix : Dibyendu 8/1/99 */
		/*
		 * We now define an incomplete type immediately after we see
		 * the tag (ISO C) - previously this was deferred until we saw
		 * a reference to the tag.
		 * The final argument to ci_build_aggr_or_enum_def() tells it
		 * that although this is an incomplete type the full definition
		 * is about to follow.  This means that this is a new (distinct)
		 * type. See ci_build_aggr_or_enum_def() for more details.
		 */
		typecode = (yypvt[-1].typecode == TY_STRUCT) ? TY_U_STRUCT : TY_U_UNION;
		ci_build_aggr_or_enum_def(typecode, yypvt[-0].identifier, AE_INCOMPLETE,
					      (declaration_t *)NULL,
					      (enum_member_t *)NULL, TRUE);

		Labels_allowed = FALSE;
	} break;
case 43:
{
		Labels_allowed = TRUE;
		yyval.type = ci_build_aggr_or_enum_def(yypvt[-5].typecode, yypvt[-4].identifier, AE_COMPLETE,
					      yypvt[-1].declaration, (enum_member_t *)NULL, FALSE);
	} break;
case 44:
{
		typecode_t typecode;

		typecode = (yypvt[-1].typecode == TY_STRUCT) ? TY_U_STRUCT : TY_U_UNION;
		yyval.type = ci_build_aggr_or_enum_def(typecode, yypvt[-0].identifier, AE_INCOMPLETE,
					      (declaration_t *)NULL,
					      (enum_member_t *)NULL, FALSE);
	} break;
case 45:
{ yyval.typecode = TY_STRUCT;	} break;
case 46:
{ yyval.typecode = TY_UNION;		} break;
case 47:
{
		yyval.declaration = yypvt[-0].declaration;
	} break;
case 48:
{
		yypvt[-0].declaration->dn_next = yypvt[-1].declaration;
		yyval.declaration = yypvt[-0].declaration;
	} break;
case 49:
{
		yyval.declarator = yypvt[-0].declarator;
	} break;
case 50:
{
		Expecting_declarator = TRUE;
	} break;
case 51:
{
		yypvt[-0].declarator->dr_next = yypvt[-3].declarator;
		yyval.declarator = yypvt[-0].declarator;
	} break;
case 52:
{
		yyval.declarator = yypvt[-0].declarator;
	} break;
case 53:
{
		yypvt[-2].declarator->dr_initialiser = yypvt[-0].initialiser;
		yyval.declarator = yypvt[-2].declarator;
	} break;
case 54:
{
		yyval.declaration = ci_make_declaration(yypvt[-2].declaration->dn_class, ci_get_dn_basetype(yypvt[-2].declaration),
					 yypvt[-2].declaration->dn_qualifiers, yypvt[-1].declarator);
	} break;
case 55:
{
		yyval.declaration = ci_make_declaration(CL_NOCLASS, yypvt[-0].type, 0, (declarator_t *)NULL);
	} break;
case 56:
{
		ci_add_type_specifier(yypvt[-0].declaration, yypvt[-1].type);
		yyval.declaration = yypvt[-0].declaration;
	} break;
case 57:
{
		yyval.declaration = ci_make_declaration(CL_NOCLASS, (type_t *)NULL, yypvt[-0].qualifier,
							(declarator_t *)NULL);
	} break;
case 58:
{
		yypvt[-0].declaration->dn_qualifiers |= yypvt[-1].qualifier;
		yyval.declaration = yypvt[-0].declaration;
	} break;
case 59:
{
		yyval.declarator = yypvt[-0].declarator;
	} break;
case 60:
{
		Expecting_declarator = TRUE;
	} break;
case 61:
{
		yypvt[-0].declarator->dr_next = yypvt[-3].declarator;
		yyval.declarator = yypvt[-0].declarator;
	} break;
case 62:
{
		yyval.declarator = yypvt[-0].declarator;
	} break;
case 63:
{
		yyval.declarator = ci_make_declarator(ci_make_expr_bitfield_type((type_t *)NULL,
									       yypvt[-0].expr));
	} break;
case 64:
{
		yyval.declarator = ci_make_declarator(ci_make_expr_bitfield_type(yypvt[-2].declarator->dr_type, yypvt[-0].expr));
	} break;
case 65:
{
		Typedef_allowed = FALSE;
	} break;
case 66:
{
		Typedef_allowed = TRUE;
	} break;
case 67:
{
		yyval.type = ci_build_aggr_or_enum_def(TY_ENUM, (identifier_t *)NULL,
					   AE_COMPLETE,
					   (declaration_t *)NULL, yypvt[-2].enum_member, FALSE);
	} break;
case 68:
{
		Typedef_allowed = FALSE;
	} break;
case 69:
{
		Typedef_allowed = TRUE;
	} break;
case 70:
{
		yyval.type = ci_build_aggr_or_enum_def(TY_ENUM, yypvt[-5].identifier,
					   AE_COMPLETE,
					   (declaration_t *)NULL, yypvt[-2].enum_member, FALSE);
	} break;
case 71:
{
		yyval.type = ci_build_aggr_or_enum_def(TY_U_ENUM, yypvt[-0].identifier,
					   AE_INCOMPLETE,
					   (declaration_t *)NULL,
					   (enum_member_t *)NULL, FALSE);
	} break;
case 72:
{
		if (yypvt[-0].enum_member->em_expr_id == NULL)
			yypvt[-0].enum_member->em_val = 0;
		yypvt[-0].enum_member->em_next = NULL;
		yyval.enum_member = yypvt[-0].enum_member;
	} break;
case 73:
{
		if (yypvt[-0].enum_member->em_expr_id == NULL)
			yypvt[-0].enum_member->em_val = yypvt[-2].enum_member->em_val + 1;
		yypvt[-0].enum_member->em_next = yypvt[-2].enum_member;
		yyval.enum_member = yypvt[-0].enum_member;
	} break;
case 74:
{
		yyval.enum_member = ci_build_enum_member(yypvt[-0].identifier, (expr_t *)NULL);
	} break;
case 75:
{
		yyval.enum_member = ci_build_enum_member(yypvt[-2].identifier, yypvt[-0].expr);
	} break;
case 76:
{
		yyval.declarator = ci_make_declarator(yypvt[-0].type);
	} break;
case 77:
{
		type_t *ptype;

		ptype = ci_push_types(yypvt[-0].type, yypvt[-1].type);
		yyval.declarator = ci_make_declarator(ptype);
	} break;
case 78:
{
		type_t *type;

		Expecting_declarator = FALSE;
		type = ci_make_type(Parse_apool, TY_IDENTIFIER);
		type->ty_identifier = yypvt[-0].identifier;
		yyval.type = type;
	} break;
case 79:
{
		yyval.type = yypvt[-1].declarator->dr_type;
	} break;
case 80:
{
		yyval.type = ci_make_array_type(yypvt[-2].type, (expr_t *)NULL);
	} break;
case 81:
{
		yyval.type = ci_make_array_type(yypvt[-3].type, yypvt[-1].expr);
	} break;
case 82:
{
		yyval.type = ci_make_funcret_type(yypvt[-2].type, FDT_IDLIST, (declaration_t *)NULL,
							(identifier_list_t *)NULL);
	} break;
case 83:
{
		yyval.type = ci_make_funcret_type(yypvt[-3].type, FDT_TYPELIST, yypvt[-1].declaration,
							(identifier_list_t *)NULL);
	} break;
case 84:
{
		yyval.type = ci_make_funcret_type(yypvt[-3].type, FDT_IDLIST,
			(declaration_t *)NULL, yypvt[-1].identifier_list);
	} break;
case 85:
{
		yyval.type = ci_make_pointer((type_t *)NULL, 0);
	} break;
case 86:
{
		yyval.type = ci_make_pointer((type_t *)NULL, yypvt[-0].qualifier);
	} break;
case 87:
{
		yyval.type = ci_make_pointer(yypvt[-0].type, 0);
	} break;
case 88:
{
		yyval.type = ci_make_pointer(yypvt[-0].type, yypvt[-1].qualifier);
	} break;
case 89:
{
		yyval.qualifier = yypvt[-0].qualifier;
	} break;
case 90:
{
		yyval.qualifier = yypvt[-1].qualifier | yypvt[-0].qualifier;
	} break;
case 91:
{
		yyval.declaration = yypvt[-0].declaration;
	} break;
case 92:
{
		declaration_t *dn;

		dn = ci_make_declaration(CL_ARG, ci_code_to_type(TY_ELLIPSIS), 0,
					 ci_make_declarator((type_t *)NULL));
		dn->dn_next = yypvt[-2].declaration;
		yyval.declaration = dn;
	} break;
case 93:
{
		yyval.declaration = yypvt[-0].declaration;
	} break;
case 94:
{
		yypvt[-0].declaration->dn_next = yypvt[-2].declaration;
		yyval.declaration = yypvt[-0].declaration;
	} break;
case 95:
{
		yyval.declaration = ci_make_declaration(yypvt[-1].declaration->dn_class, ci_get_dn_basetype(yypvt[-1].declaration),
					 yypvt[-1].declaration->dn_qualifiers, yypvt[-0].declarator);
	} break;
case 96:
{
		yyval.declaration = ci_make_declaration(yypvt[-0].declaration->dn_class, ci_get_dn_basetype(yypvt[-0].declaration),
					 yypvt[-0].declaration->dn_qualifiers,
					 ci_make_declarator((type_t *)NULL));
	} break;
case 97:
{
		yyval.declaration = ci_make_declaration(yypvt[-1].declaration->dn_class, ci_get_dn_basetype(yypvt[-1].declaration),
					 yypvt[-1].declaration->dn_qualifiers, yypvt[-0].declarator);
	} break;
case 98:
{
		identifier_list_t *idl = NEW(identifier_list_t);

		idl->idl_id = yypvt[-0].identifier;
		idl->idl_next = NULL;
		yyval.identifier_list = idl;
	} break;
case 99:
{
		identifier_list_t *idl = NEW(identifier_list_t);

		idl->idl_id = yypvt[-0].identifier;
		idl->idl_next = yypvt[-2].identifier_list;
		yyval.identifier_list = idl;
	} break;
case 100:
{
		yyval.initialiser = ci_make_initexpr(FALSE, yypvt[-0].expr, (initexpr_t *)NULL);
	} break;
case 101:
{
		yyval.initialiser = ci_make_initexpr(TRUE, (expr_t *)NULL, yypvt[-1].initialiser);
	} break;
case 102:
{
		yyval.initialiser = ci_make_initexpr(TRUE, (expr_t *)NULL, yypvt[-2].initialiser);
	} break;
case 103:
{
		yyval.initialiser = yypvt[-0].initialiser;
	} break;
case 104:
{
		yypvt[-0].initialiser->ie_next = yypvt[-2].initialiser;
		yyval.initialiser = yypvt[-0].initialiser;
	} break;
case 105:
{
		yyval.type = ci_get_dn_basetype(yypvt[-0].declaration);
	} break;
case 106:
{
		yyval.type = ci_push_types(ci_get_dn_basetype(yypvt[-1].declaration), yypvt[-0].declarator->dr_type);
	} break;
case 107:
{
		yyval.declarator = ci_make_declarator(yypvt[-0].type);
	} break;
case 108:
{
		yyval.declarator = ci_make_declarator(yypvt[-0].type);
	} break;
case 109:
{
		yypvt[-1].type->ty_base = yypvt[-0].type;
		yyval.declarator = ci_make_declarator(yypvt[-1].type);
	} break;
case 110:
{
		yyval.type = yypvt[-1].declarator->dr_type;
	} break;
case 111:
{
		yyval.type = ci_make_array_type((type_t *)NULL, (expr_t *)NULL);
	} break;
case 112:
{
		yyval.type = ci_make_array_type((type_t *)NULL, yypvt[-1].expr);
	} break;
case 113:
{
		yyval.type = ci_make_array_type(yypvt[-2].type, (expr_t *)NULL);
	} break;
case 114:
{
		yyval.type = ci_make_array_type(yypvt[-3].type, yypvt[-1].expr);
	} break;
case 115:
{
		yyval.type = ci_make_funcret_type((type_t *)NULL, FDT_IDLIST,
						(declaration_t *)NULL,
						(identifier_list_t *)NULL);
	} break;
case 116:
{
		yyval.type = ci_make_funcret_type((type_t *)NULL, FDT_TYPELIST, yypvt[-1].declaration,
						(identifier_list_t *)NULL);
	} break;
case 117:
{
		yyval.type = ci_make_funcret_type(yypvt[-2].type, FDT_IDLIST, (declaration_t *)NULL,
						(identifier_list_t *)NULL);
	} break;
case 118:
{
		yyval.type = ci_make_funcret_type(yypvt[-3].type, FDT_TYPELIST, yypvt[-1].declaration,
						(identifier_list_t *)NULL);
	} break;
case 132:
{
		yyval.statement = ci_make_labeled_statement(yypvt[-2].identifier, yypvt[-0].statement);
	} break;
case 133:
{
		yyval.statement = ci_make_case_labeled_statement(Switch_level != 0, yypvt[-2].expr, yypvt[-0].statement);
	} break;
case 134:
{
		yyval.statement = ci_make_case_labeled_statement(Switch_level != 0,
								(expr_t *)NULL, yypvt[-0].statement);
	} break;
case 135:
{
		yyval.statement = ci_make_expression_statement(yypvt[-1].expr);
	} break;
case 136:
{
		yyval.statement = yypvt[-0].statement;
	} break;
case 137:
{
		yyval.statement = ci_end_compound_statement((statement_t *)NULL);
	} break;
case 138:
{
		yyval.statement = ci_end_compound_statement(yypvt[-1].statement);
	} break;
case 141:
{
		ci_start_block(TRUE);
	} break;
case 142:
{
		yypvt[-0].statement->st_next = NULL;
		yyval.statement = yypvt[-0].statement;
	} break;
case 143:
{
		yypvt[-0].statement->st_next = yypvt[-1].statement;
		yyval.statement = yypvt[-0].statement;
	} break;
case 144:
{
		yyval.statement = ci_make_if_statement(yypvt[-2].expr, yypvt[-0].statement, (statement_t *)NULL);
	} break;
case 145:
{
		yyval.statement = ci_make_if_statement(yypvt[-4].expr, yypvt[-2].statement, yypvt[-0].statement);
	} break;
case 146:
{
		++Switch_level;
	} break;
case 147:
{
		--Switch_level;
		yyval.statement = ci_make_switch_statement(yypvt[-2].expr, yypvt[-0].statement);
	} break;
case 148:
{
		++Loop_level;
	} break;
case 149:
{
		--Loop_level;
		yyval.statement = ci_make_while_statement(STT_WHILE, yypvt[-2].expr, yypvt[-0].statement);
	} break;
case 150:
{
		++Loop_level;
	} break;
case 151:
{
		--Loop_level;
		yyval.statement = ci_make_while_statement(STT_DO, yypvt[-2].expr, yypvt[-5].statement);
	} break;
case 152:
{
		++Loop_level;
		yyval.lexinfo = yypvt[-0].lexinfo;
	} break;
case 153:
{
		--Loop_level;
		yyval.statement = ci_make_for_statement(yypvt[-6].expr, yypvt[-4].expr, yypvt[-2].expr, yypvt[-0].statement, yypvt[-8].lexinfo);
	} break;
case 154:
{
		yyval.expr = NULL;
	} break;
case 155:
{
		yyval.expr = yypvt[-0].expr;
	} break;
case 156:
{
		yyval.statement = ci_make_goto_statement(yypvt[-1].identifier);
	} break;
case 157:
{
		yyval.statement = ci_make_continue_statement(Loop_level != 0, yypvt[-1].lexinfo);
	} break;
case 158:
{
		yyval.statement = ci_make_break_statement(Loop_level + Switch_level != 0, yypvt[-1].lexinfo);
	} break;
case 159:
{
		yyval.statement = ci_make_return_statement(yypvt[-1].expr, yypvt[-2].lexinfo);
	} break;
case 160:
{
		yyval.expr = yypvt[-0].expr;
	} break;
case 161:
{
		yyval.expr = ci_make_comma_expr(yypvt[-2].expr, yypvt[-0].expr);
	} break;
case 163:
{
		yyval.expr = ci_make_assignment_expr(yypvt[-1].optype, yypvt[-2].expr, yypvt[-0].expr);
	} break;
case 164:
{ yyval.optype = OP_ASSIGN;		} break;
case 165:
{ yyval.optype = OP_MUL_ASSIGN;		} break;
case 166:
{ yyval.optype = OP_DIV_ASSIGN;		} break;
case 167:
{ yyval.optype = OP_MOD_ASSIGN;		} break;
case 168:
{ yyval.optype = OP_PLUS_ASSIGN;		} break;
case 169:
{ yyval.optype = OP_MINUS_ASSIGN;		} break;
case 170:
{ yyval.optype = OP_LSHIFT_ASSIGN;	} break;
case 171:
{ yyval.optype = OP_RSHIFT_ASSIGN;	} break;
case 172:
{ yyval.optype = OP_BITWISE_AND_ASSIGN;	} break;
case 173:
{ yyval.optype = OP_BITWISE_XOR_ASSIGN;	} break;
case 174:
{ yyval.optype = OP_BITWISE_OR_ASSIGN;	} break;
case 176:
{
		yyval.expr = ci_make_conditional_expression(yypvt[-4].expr, yypvt[-2].expr, yypvt[-0].expr);
	} break;
case 177:
{
		yyval.expr = yypvt[-0].expr;
	} break;
case 179:
{
		yyval.expr = ci_make_logical_expr(OP_LOGICAL_OR, "||", yypvt[-2].expr, yypvt[-0].expr);
	} break;
case 181:
{
		yyval.expr = ci_make_logical_expr(OP_LOGICAL_AND, "&&", yypvt[-2].expr, yypvt[-0].expr);
	} break;
case 183:
{
		yyval.expr = ci_make_bitwise_expr(OP_BITWISE_OR, "|", yypvt[-2].expr, yypvt[-0].expr);
	} break;
case 185:
{
		yyval.expr = ci_make_bitwise_expr(OP_BITWISE_XOR, "^", yypvt[-2].expr, yypvt[-0].expr);
	} break;
case 187:
{
		yyval.expr = ci_make_bitwise_expr(OP_BITWISE_AND, "&", yypvt[-2].expr, yypvt[-0].expr);
	} break;
case 189:
{
		yyval.expr = ci_make_comparison_expr(OP_IS_EQUAL, "==", yypvt[-2].expr, yypvt[-0].expr);
	} break;
case 190:
{
		yyval.expr = ci_make_comparison_expr(OP_NOT_EQUAL, "!=", yypvt[-2].expr, yypvt[-0].expr);
	} break;
case 192:
{
		yyval.expr = ci_make_comparison_expr(OP_LESS, "<", yypvt[-2].expr, yypvt[-0].expr);
	} break;
case 193:
{
		yyval.expr = ci_make_comparison_expr(OP_GREATER, ">", yypvt[-2].expr, yypvt[-0].expr);
	} break;
case 194:
{
		yyval.expr = ci_make_comparison_expr(OP_LESS_OR_EQUAL, "<=", yypvt[-2].expr, yypvt[-0].expr);
	} break;
case 195:
{
		yyval.expr = ci_make_comparison_expr(OP_GREATER_OR_EQUAL, ">=", yypvt[-2].expr, yypvt[-0].expr);
	} break;
case 197:
{
		yyval.expr = ci_make_shift_expr(OP_LSHIFT, "<<", yypvt[-2].expr, yypvt[-0].expr);
	} break;
case 198:
{
		yyval.expr = ci_make_shift_expr(OP_RSHIFT, ">>", yypvt[-2].expr, yypvt[-0].expr);
	} break;
case 200:
{
		yyval.expr = ci_make_add_or_subtract_expr(OP_PLUS, "+", yypvt[-2].expr, yypvt[-0].expr);
	} break;
case 201:
{
		yyval.expr = ci_make_add_or_subtract_expr(OP_MINUS, "-", yypvt[-2].expr, yypvt[-0].expr);
	} break;
case 203:
{
		yyval.expr = ci_make_mul_or_div_expr(OP_MUL, "*", yypvt[-2].expr, yypvt[-0].expr);
	} break;
case 204:
{
		yyval.expr = ci_make_mul_or_div_expr(OP_DIV, "/", yypvt[-2].expr, yypvt[-0].expr);
	} break;
case 205:
{
		yyval.expr = ci_make_mod_expr(OP_MOD, "%", yypvt[-2].expr, yypvt[-0].expr);
	} break;
case 207:
{
		yyval.expr = ci_make_cast_expr(yypvt[-2].type, yypvt[-0].expr);
	} break;
case 208:
{
		yyval.expr = yypvt[-0].expr;
	} break;
case 209:
{
		yyval.expr = ci_make_inc_or_dec_expr(OP_PREINC, yypvt[-0].expr, "prefix ++");
	} break;
case 210:
{
		yyval.expr = ci_make_inc_or_dec_expr(OP_PREDEC, yypvt[-0].expr, "prefix --");
	} break;
case 211:
{
		yyval.expr = ci_make_deref_expr(yypvt[-0].expr);
	} break;
case 212:
{
		yyval.expr = ci_make_address_of_expr(yypvt[-0].expr);
	} break;
case 213:
{
		yyval.expr = ci_make_unary_plus_expr(yypvt[-0].expr);
	} break;
case 214:
{
		yyval.expr = ci_make_unary_minus_expr(yypvt[-0].expr);
	} break;
case 215:
{
		yyval.expr = ci_make_bitwise_not_expr(yypvt[-0].expr);
	} break;
case 216:
{
		yyval.expr = ci_make_logical_not_expr(yypvt[-0].expr);
	} break;
case 217:
{
		yyval.expr = ci_make_sizeof_expr(yypvt[-0].expr, (type_t *)NULL);
	} break;
case 218:
{
		yyval.expr = ci_make_sizeof_expr((expr_t *)NULL, yypvt[-1].type);
	} break;
case 220:
{
		/*  Rewrite as *($1 + $3).
		 */
		yyval.expr = ci_make_deref_expr(ci_make_add_or_subtract_expr(OP_PLUS, "+",
									   yypvt[-3].expr, yypvt[-1].expr));
	} break;
case 221:
{
		yyval.expr = ci_make_func_call_expr(yypvt[-3].expr, yypvt[-1].expr_list);
	} break;
case 222:
{
		yyval.expr = ci_make_dot_expr(yypvt[-2].expr, yypvt[-0].identifier, "dot");
	} break;
case 223:
{
		yyval.expr = ci_make_multi_arrow_expr(yypvt[-5].expr, yypvt[-2].expr, yypvt[-0].identifier);
	} break;
case 224:
{
		/*  Rewrite as (*$1).$3
		 */
		yyval.expr = ci_make_dot_expr(ci_make_deref_expr(yypvt[-2].expr), yypvt[-0].identifier, "arrow");
	} break;
case 225:
{
		yyval.expr = ci_make_inc_or_dec_expr(OP_POSTINC, yypvt[-1].expr, "postfix ++");
	} break;
case 226:
{
		yyval.expr = ci_make_inc_or_dec_expr(OP_POSTDEC, yypvt[-1].expr, "postfix --");
	} break;
case 227:
{
		yyval.expr = ci_make_identifier_expr(yypvt[-0].identifier);
	} break;
case 228:
{
		yyval.expr = yypvt[-0].expr;
	} break;
case 229:
{
		yyval.expr = ci_make_string_constant_expr(yypvt[-0].constant->co_lexinfo,
						  &yypvt[-0].constant->co_string_val);
	} break;
case 230:
{
		yyval.expr = yypvt[-1].expr;
	} break;
case 231:
{
		yyval.expr_list = NULL;
	} break;
case 232:
{
		yyval.expr_list = yypvt[-0].expr_list;
	} break;
case 233:
{
		expr_list_t *el = NEW(expr_list_t);

		el->el_expr = yypvt[-0].expr;
		el->el_next = NULL;
		yyval.expr_list = el;
	} break;
case 234:
{
		expr_list_t *el = NEW(expr_list_t);

		el->el_expr = yypvt[-0].expr;
		el->el_next = yypvt[-2].expr_list;
		yyval.expr_list = el;
	} break;
case 235:
{
		yyval.expr = ci_make_integer_constant_expr(ET_INT_CONST,
						   yypvt[-0].constant->co_lexinfo,
						   yypvt[-0].constant->co_integer_val,
						   yypvt[-0].constant->co_is_unsigned,
#if WANT_LL
						   yypvt[-0].constant->co_is_longlong);
#else
						   FALSE);
#endif
	} break;
case 236:
{
		yyval.expr = ci_make_integer_constant_expr(ET_CHAR_CONST,
						   yypvt[-0].constant->co_lexinfo,
						   yypvt[-0].constant->co_integer_val,
						   FALSE, FALSE);
	} break;
case 237:
{
		yyval.expr = ci_make_floating_constant_expr(yypvt[-0].constant->co_lexinfo,
						    yypvt[-0].constant->co_floating_val);
	} break;
	}
	goto yystack;		/* reset registers in driver code */
}

