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
#include <string.h>
#include "podiff.h"

struct list *
list_create()
{
	struct list *lp = emalloc(sizeof(*lp));
	memset(lp, 0, sizeof(*lp));
	return lp;
}

size_t
list_size(struct list *lp)
{
	return lp ? lp->count : 0;
}

void
list_insert_entry(struct list *lp,
		  struct list_entry *anchor,
		  struct list_entry *ent, int before)
{
	struct list_entry *p;
	
	if (!anchor) {
		ent->prev = NULL;
		ent->next = lp->head;
		if (lp->head)
			lp->head->prev = ent;
		else
			lp->tail = ent;
		lp->head = ent;
		lp->count++;
		return;
	}
		
	if (before) {
		list_insert_entry(lp, anchor->prev, ent, 0);
		return;
	}

	ent->prev = anchor;
	if (p = anchor->next)
		p->prev = ent;
	else
		lp->tail = ent;
	ent->next = p;
	anchor->next = ent;
	lp->count++;
}

void
list_remove_entry(struct list *lp, struct list_entry *ent)
{
	struct list_entry *p;
	if (p = ent->prev)
		p->next = ent->next;
	else
		lp->head = ent->next;
	if (p = ent->next)
		p->prev = ent->prev;
	else
		lp->tail = ent->prev;
	ent->next = ent->prev = NULL;
	lp->count--;
}

void
list_append(struct list *lp, void *val)
{
	struct list_entry *ep = emalloc(sizeof(*ep));
	ep->data = val;
	list_insert_entry(lp, lp->tail, ep, 0);
}

void
list_add(struct list *dst, struct list *src)
{
	if (!src->head)
		return;

	src->head->prev = dst->tail;
	if (dst->tail)
		dst->tail->next = src->head;
	else
		dst->head = src->head;
	dst->tail = src->tail;
	dst->count += src->count;
	src->head = src->tail = NULL;
	src->count = 0;
}

void
list_push(struct list *lp, void *val)
{
	struct list_entry *ep = emalloc(sizeof(*ep));
	ep->data = val;
	list_insert_entry(lp, NULL, ep, 0);
}

void *
list_pop(struct list *lp)
{
	void *data;
	struct list_entry *ep = lp->head;
	if (ep)	{
		data = ep->data;
		list_remove_entry(lp, ep);
		free(ep);
	} else
		data = NULL;
	return data;
}

void *
list_remove_tail(struct list *lp)
{
	void *data;
	struct list_entry *ep;
  
	if (!lp->tail)
		return NULL;
	ep = lp->tail;
	data = lp->tail->data;
	list_remove_entry(lp, ep);
	free(ep);
	return data;
}

void
list_clear(struct list *lp)
{
	struct list_entry *ep = lp->head;

	while (ep) {
		struct list_entry *next = ep->next;
		if (lp->free_entry)
			lp->free_entry(ep->data);
		free(ep);
		ep = next;
	}
	lp->head = lp->tail = NULL;
	lp->count = 0;
}

void
list_free(struct list *lp)
{
	if (lp) {
		list_clear(lp);
		free(lp);
	}
}

static int
_ptrcmp(const void *a, const void *b)
{
	return a != b;
}

void *
list_locate(struct list *lp, void *data)
{
	struct list_entry *ep;
	int (*cmp)(const void *, const void *) = lp->cmp ? lp->cmp : _ptrcmp;

	for (ep = lp->head; ep; ep = ep->next) {
		if (cmp(ep->data, data) == 0)
			return ep->data;
	}
	return NULL;
}

void *
list_index(struct list *lp, size_t idx)
{
	struct list_entry *ep;
	
	for (ep = lp->head; ep && idx; ep = ep->next, idx--)
		;
	return ep ? ep->data : NULL;
}

void
list_join(struct list *a, struct list *b)
{
        if (!b->head)
                return;
        b->head->prev = a->tail;
        if (a->tail)
                a->tail->next = b->head;
        else
                a->head = b->head;
        a->tail = b->tail;
}

void
list_qsort(struct list *list)
{
        struct list_entry *cur, *middle;
        struct list high_list, low_list;
        int rc;

        if (!list->head)
                return;
        cur = list->head;
        do {
                cur = cur->next;
                if (!cur)
                        return;
        } while ((rc = list->cmp(list->head->data, cur->data)) == 0);

        /* Select the lower of the two as the middle value */
        middle = (rc > 0) ? cur : list->head;

        /* Split into two sublists */
        low_list.head = low_list.tail = NULL;
        high_list.head = high_list.tail = NULL;
	low_list.free_entry = high_list.free_entry = NULL;
	low_list.cmp = high_list.cmp = list->cmp;
        for (cur = list->head; cur; ) {
                struct list_entry *next = cur->next;
                cur->next = NULL;
                if (list->cmp(middle->data, cur->data) < 0)
			list_insert_entry(&high_list, high_list.tail,
					  cur, 0);
                else
			list_insert_entry(&low_list, low_list.tail,
					  cur, 0);
                cur = next;
        }

        /* Sort both sublists recursively */
        list_qsort(&low_list);
        list_qsort(&high_list);

        /* Join both lists in order */
        list_join(&low_list, &high_list);

        /* Return the resulting list */
        list->head = low_list.head;
        list->tail = low_list.tail;
}
