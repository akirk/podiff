/* This file is part of PODIFF.
   Copyright (C) 2011 Sergey Poznyakoff

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

#include <stdlib.h>
#include <stdio.h>

#ifndef VERSION
# define VERSION "0.0 (not defined)"
# warning "VERSION not defined"
#endif

void *emalloc(size_t);
void *ecalloc(size_t, size_t);


struct list_entry {
        struct list_entry *next, *prev;
        void *data;
};

struct list {
        struct list_entry *head, *tail;
        size_t count;
        int (*cmp)(const void *, const void *);
        void (*free_entry)(void *);
};

struct list *list_create(void);
size_t list_size(struct list *lp);
void list_append(struct list *lp, void *val);
void list_push(struct list *lp, void *val);
void *list_pop(struct list *lp);
void *list_locate(struct list *lp, void *data);
void *list_index(struct list *lp, size_t idx);
void *list_remove_tail(struct list *lp);
void list_remove_entry(struct list *lp, struct list_entry *ent);
void list_clear(struct list *lp);
void list_free(struct list *lp);
void list_add(struct list *dst, struct list *src);
void list_join(struct list *a, struct list *b);
void list_qsort(struct list *list);

struct txtacc *txtacc_create();
void txtacc_free(struct txtacc *acc);
void txtacc_grow(struct txtacc *acc, const char *buf, size_t size);
char *txtacc_finish(struct txtacc *acc, int steal);
void txtacc_free_string(struct txtacc *acc, char *str);
void txtacc_clear(struct txtacc *acc);

void parse_error(const char *fmt, ...);
void parse_error_at_line(unsigned line, const char *fmt, ...);

int msgtrans_sort_id(void const *a, void const *b);
struct list *parse_po(const char *file);


struct msgtrans {
	struct list *comment;
	char *msgctxt;
	char *msgid;
	char *msgid_plural;
	struct list *msgstr;
	int flags;
};


extern unsigned line_no;
extern struct list *msgtrans_list;

extern int skip_entry;
extern int skip_fuzzy_option;
extern int keep_comments_option;
