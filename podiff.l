/* This file is part of PODIFF.
   Copyright (C) 2011, 2012 Sergey Poznyakoff

   PODIFF is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   PODIFF is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this PODIFF.  If not, see <http://www.gnu.org/licenses/>. */

%{
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "podiff.h"
#include "y.tab.h"

static struct txtacc *acc;
struct list *msgtrans_list;
const char *file_name;
unsigned line_no;
%}
%x STR FLG
%%
"#, "             { BEGIN(FLG); }
#"~ ".*\n         { line_no++; return OLDTRANS; }
"#"\n |
#[^~,\n].*\n       { line_no++;
	            if (!strchr(".:|", yytext[1]) && keep_comments_option) {
			    txtacc_grow(acc, yytext, yyleng-1);
			    yylval.string = txtacc_finish(acc, 0);
			    return COMMENT;
		    }
                  }
<INITIAL,FLG>[ \t\f][ \t\f]*   ;
<FLG>\n           { BEGIN(INITIAL); line_no++; }
<FLG>,            ;
<FLG>fuzzy        return FUZZY;
<FLG>[a-zA-Z_][-a-zA-Z_0-9]*  {
	            txtacc_grow(acc, yytext, yyleng);
                    yylval.string = txtacc_finish(acc, 0);
                    return FLAG;
}
<FLG>.            /* skip bad character */;
msgctxt           return MSGCTXT;
msgid             return MSGID;
msgid_plural      return MSGID_PLURAL;
msgstr            return MSGSTR;
[0-9][0-9]*       { yylval.number = atoi(yytext); return NUMBER; }
            /* Quoted strings */
\"[^\\"\n]*\"     { txtacc_grow(acc, yytext+1, yyleng-2);
                    yylval.string = txtacc_finish(acc, 0);
                    return STRING; }
\"[^\\"\n]*\\.    { BEGIN(STR);
	            if (yytext[yyleng-1] == 'n') {
			    txtacc_grow(acc, yytext+1, yyleng-3);
			    txtacc_grow(acc, "\n", 1);
		    } else
			    txtacc_grow(acc, yytext+1, yyleng-1); }
<STR>[^\\"\n]*\\. { if (yytext[yyleng-1] == 'n') {
			    txtacc_grow(acc, yytext, yyleng-1);
			    txtacc_grow(acc, "\n", 1);
		    } else
                            txtacc_grow(acc, yytext, yyleng); }
<STR>[^\\"\n]*\"  { txtacc_grow(acc, yytext, yyleng - 1);
	yylval.string = txtacc_finish(acc, 0);
                    BEGIN(INITIAL);
	            return STRING; }
\n                line_no++;
.                 return yytext[0];
%%

int
yywrap()
{
	return 1;
}

void
parse_error(const char *fmt, ...)
{
	va_list ap;
	
	fprintf(stderr, "%s:%u: ", file_name, line_no);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
}

void
parse_error_at_line(unsigned line, const char *fmt, ...)
{
	va_list ap;
	
	fprintf(stderr, "%s:%u: ", file_name, line);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
}

struct list *
parse_po(const char *file)
{
	struct po_entry *ent;
	struct msgtrans *link;

	FILE *fp = fopen(file, "r");
	if (!fp) {
		error("cannot open file %s: %s", file, strerror(errno));
		exit(1);
	}
	yyrestart(fp);

	if (!acc)
		acc = txtacc_create();
	file_name = file;
	line_no = 1;

	if (yyparse())
		exit(3);
	fclose(fp);
	return msgtrans_list;
}

