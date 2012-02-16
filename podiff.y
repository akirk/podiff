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
#include <string.h>
#include "podiff.h"
#include "y.tab.h"	

int skip_entry;	
%}

%union {
	char *string;
	int number;
	struct txtacc *acc;
	struct list *lst;
	struct msgtrans *msgtrans;
	struct { unsigned line; int num; char *str; } imsgstr;
};

%token <string> STRING FLAG COMMENT MSGCTXT 
%token <number> NUMBER
%token MSGID MSGID_PLURAL MSGSTR FUZZY OLDTRANS 

%type <string> msgid msgid_plural msgstr msgctxt
%type <acc> strings
%type <lst> msgstrlist entries ocomm comment
%type <imsgstr> imsgstr
%type <msgtrans> entry
%%

input     : entries
            {
		    msgtrans_list = $1;
		    msgtrans_list->cmp = msgtrans_sort_id;
	    }
          ;
 
entries   : entry
            {
		    $$ = list_create();
		    /*$$->free_entry = free;*/
		    if ($1)
			    list_append($$, $1);
	    }
          | entries entry
            {
		    if ($2)
			    list_append($1, $2);
		    $$ = $1;
	    }
          ;

entry     : ocomm oflags msgctxt msgid msgstr
            {
		    if (skip_entry)
			    $$ = NULL;
		    else {
			    $$ = emalloc(sizeof(*$$));
			    $$->comment = $1;
			    $$->msgctxt = $3;
			    $$->msgid = $4;
			    $$->msgid_plural = NULL;
			    $$->msgstr = list_create();
			    $$->msgstr->free_entry = free;
			    list_append($$->msgstr, $5);
		    }
		    skip_entry = 0;
	    }
          | ocomm oflags msgctxt msgid msgid_plural msgstrlist
            {
		    if (skip_entry)
			    $$ = NULL;
		    else {
			    $$ = emalloc(sizeof(*$$));
			    $$->comment = $1;
			    $$->msgctxt = $3;
			    $$->msgid = $4;
			    $$->msgid_plural = $5;
			    $$->msgstr = $6;
		    }
		    skip_entry = 0;
	    }
          | ocomm oflags msgctxt oldentry
            {
		    /* FIXME: Reclaim memory */
		    $$ = NULL;
	    }
          ;

oldentry  : OLDTRANS
          | oldentry OLDTRANS
          ;

oflags    : /* empty */
          | flags;
          ;

flags     : flag
          | flags flag
          ;

flag      : FLAG
          | FUZZY
            {
		    skip_entry = skip_fuzzy_option;
	    }
          ;

ocomm     : /* empty */
            {
		    $$ = NULL;
	    }
          | comment
	  ;

comment   : COMMENT
            {
		    $$ = list_create();
		    list_append($$, $1);
	    }
          | comment COMMENT
            {
		    list_append($1, $2);
	    }
          ;

msgctxt   : /* empty */
            {
		    $$ = NULL;
	    }
          | MSGCTXT STRING
	    {
		    $$ = $2;
	    }
          ;

msgid     : MSGID strings
            {
		    $$ = txtacc_finish($2, 1);
		    txtacc_free($2);
	    }
          ;

msgid_plural: MSGID_PLURAL strings
            {
		    $$ = txtacc_finish($2, 1);
		    txtacc_free($2);
	    }
          ;

msgstr    : MSGSTR strings
            {
		    $$ = txtacc_finish($2, 1);
		    txtacc_free($2);
	    }
          ;

imsgstr   : MSGSTR { $<number>$ = line_no; } '[' NUMBER ']' strings
            {
		    $$.line = $<number>2;
		    $$.num = $4;
		    $$.str = txtacc_finish($6, 1);
		    txtacc_free($6);
	    }
          ;

msgstrlist: imsgstr
            {
		    if ($1.num != 0)
			    parse_error_at_line($1.line,
						"msgstr number should be zero");

		    $$ = list_create();
		    $$->free_entry = free;
		    list_append($$, $1.str);
	    }
          | msgstrlist imsgstr
            {
                    if ($2.num != list_size($1))
			    parse_error_at_line($2.line,
						"msgstr number out of sequence");
		    list_append($1, $2.str);
		    $$ = $1;
	    }
          ;

strings   : STRING
            {
		    $$ = txtacc_create();
		    txtacc_grow($$, $1, strlen($1));
	    }
          | strings STRING
            {
		    txtacc_grow($1, $2, strlen($2));
		    $$ = $1;
	    }
          ;

%%

int
yyerror(const char *s)
{
	parse_error("%s", s);
}

int
msgtrans_sort_id(void const *a, void const *b)
{
	struct msgtrans const *pa = a;
	struct msgtrans const *pb = b;
	int rc;

	rc = strcmp(pa->msgid, pb->msgid);
	if (rc == 0) {
		if (pa->msgctxt) {
			if (pb->msgctxt)
				rc = strcmp(pa->msgctxt, pb->msgctxt);
			else
				rc = -1;
		} else if (pb->msgctxt)
			rc = 1;
	}
	return rc;
}

