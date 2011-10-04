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


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <string.h>
#include "podiff.h"

struct txtacc_entry
{
	char *buf;              /* Text buffer */
	size_t size;            /* Buffer size */  
	size_t len;             /* Actual number of bytes in buffer */ 
};
#define TXTACC_BUFSIZE 1024
#define txtacc_entry_freesize(e) ((e)->size - (e)->len)

struct txtacc
{
	struct list *cur;  /* Current build list */
	struct list *mem;  /* List of already allocated elements */
};

static struct txtacc_entry *
txtacc_alloc_entry(struct list *list, size_t size)
{
	struct txtacc_entry *p = malloc(sizeof (*p));
	p->buf = malloc(size);
	p->size = size;
	p->len = 0;
	list_append(list, p);
	return p;
}

static struct txtacc_entry *
txtacc_cur_entry(struct txtacc *acc)
{
	struct txtacc_entry *ent;

	if (list_size(acc->cur) == 0)
		return txtacc_alloc_entry(acc->cur,
					  TXTACC_BUFSIZE);
	ent = acc->cur->tail->data;
	if (txtacc_entry_freesize(ent) == 0)
		ent = txtacc_alloc_entry(acc->cur,
					 TXTACC_BUFSIZE);
	return ent;
}

static void
txtacc_entry_append(struct txtacc_entry *ent,
			  const char *p, size_t size)
{
	memcpy(ent->buf + ent->len, p, size);
	ent->len += size;
} 

static void
txtacc_entry_tailor(struct txtacc_entry *ent)
{
	if (ent->size > ent->len) {
		char *p = realloc(ent->buf, ent->len);
		if (!p)
			return;
		ent->buf = p;
		ent->size = ent->len;
	}
}

static void
txtacc_entry_free(void *p)
{
	if (p) {
		struct txtacc_entry *ent = p;
		free(ent->buf);
		free(ent);
	}
}

struct txtacc *
txtacc_create()
{
	struct txtacc *acc = malloc(sizeof (*acc));
	acc->cur = list_create();
	acc->cur->free_entry = txtacc_entry_free;
	acc->mem = list_create();
	acc->mem->free_entry = txtacc_entry_free;
	return acc;
}

void
txtacc_free(struct txtacc *acc)
{
	if (acc) {
		list_free (acc->cur);
		list_free (acc->mem);
		free (acc);
	}
}

void
txtacc_grow(struct txtacc *acc, const char *buf, size_t size)
{
	while (size) {
		struct txtacc_entry *ent = txtacc_cur_entry(acc);
		size_t rest = txtacc_entry_freesize(ent);
		if (rest > size)
			rest = size;
		txtacc_entry_append(ent, buf, rest);
		buf += rest;
		size -= rest;
	}
}

char *
txtacc_finish(struct txtacc *acc, int steal)
{
	struct list_entry *ep;
	struct txtacc_entry *txtent;
	size_t size;
	char *p;
	
	txtacc_grow(acc, "", 1);
	switch (list_size(acc->cur)) {
	case 1:
		txtent = acc->cur->head->data;
		acc->cur->head->data = NULL;
		txtacc_entry_tailor(txtent);
		list_append(acc->mem, txtent);
		break;

	default:
		size = 0;
		for (ep = acc->cur->head; ep; ep = ep->next) {
			txtent = ep->data;
			size += txtent->len;
		}
      
		txtent = txtacc_alloc_entry(acc->mem, size);
		for (ep = acc->cur->head; ep; ep = ep->next) {
			struct txtacc_entry *tp = ep->data;
			txtacc_entry_append(txtent, tp->buf, tp->len);
		}
	}
  
	list_clear(acc->cur);
	p = txtent->buf;
	if (steal) {
		list_remove_tail(acc->mem);
		free(txtent);
	}
	return p;
}

void
txtacc_free_string(struct txtacc *acc, char *str)
{
	struct list_entry *ep;
	for (ep = acc->mem->head; ep; ep = ep->next) {
		struct txtacc_entry *tp = ep->data;
		if (tp->buf == str) {
			list_remove_entry(acc->mem, ep);
			free(tp->buf);
			return;
		}
	}
}

void
txtacc_clear(struct txtacc *acc)
{
	list_clear(acc->cur);
}
