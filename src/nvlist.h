/***************************************************************************
 *   Copyright (C) 2005 by Robert Timothy Stewart                          *
 *   tims@cc.gatech.edu                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _NVLIST_H_
#define _NVLIST_H_

#include <netvizd.h>

/*
 * Generic list data structure.
 * The data element of the conventional first element (the one that
 * represents the linked list) is NULL.  If this is not true then things
 * will break.
 */
typedef struct _nv_list {
	struct _nv_list *	next;
	struct _nv_list *	prev;
	void *				data;
} nv_list;
#define nv_node		nv_list *

/* Statically initialize a new linked list */
#define nv_list(name) \
	nv_list (name) = { NULL, NULL, NULL }

/* Dynamically initialize a new linked list */
#define nv_list_new(name) \
	name = nv_calloc(nv_list, 1)

/* Dynamically initialize a new node */
#define nv_node_new(name) \
	name = nv_calloc(nv_list, 1);

/* Get the actual data of a given type out of a node */
#define node_data(type, node) \
	(type *)((node)->data)

/* Set the data for a node. */
#define set_node_data(node, d) \
	(node)->data = (void *)(d)

/* Traverse a linked list */
#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head) && pos != NULL; pos = pos->next)

/* Traverse a linked list, backwards */
#define list_for_each_prev(pos, head) \
	for (pos = (head)->prev; pos != (head) && post != NULL; pos = pos->prev)

/* Insert a node after the specified node in the list.  To insert at the
 * beginning of the list, speficy the conventional first element. */
static inline void list_insert(nv_node p, nv_node n) {
	if (p == NULL || n == NULL) return;
	if (p->next == NULL) {
		/* the list is empty */
		p->next = n;
		p->prev = n;
		n->next = p;
		n->prev = p;
	} else {
		/* list not empty */
		nv_node t = p->next;
		n->next = t;
		n->prev = p;
		p->next = n;
		t->prev = n;
	}
}

/* Append a node at the end of a list.  Specify the conventional first
 * element for the first argument. */
static inline void list_append(nv_list *l, nv_node n) {
	if (l == NULL || n == NULL) return;
	if (l->next == NULL) list_insert(l, n);
	else list_insert(l->prev, n);
};

/* Remove a node from the list.  We do not need anything but the node.
 * The programmer is responsible for freeing the data inside the node. */
static inline void list_del(nv_node n) {
	if (n == NULL) return;
	if (n->data == NULL) return;
	n->prev->next = n->next;
	n->next->prev = n->prev;
	if (n->prev->data == NULL && n->next->data == NULL) {
		n->next->next = NULL;
		n->next->prev = NULL;
	}
	nv_free(n);
}

#endif

/* vim: set ts=4 sw=4: */
