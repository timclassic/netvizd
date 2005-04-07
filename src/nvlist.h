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

#ifndef _HEADER_H_
#define _HEADER_H_

#include <netvizd.h>

/*
 * Generic list data structure.
 * The data element of the conventional first element (the one that
 * represents the linked list) is NULL
 */
struct _nv_list {
	struct _nv_list *	next;
	struct _nv_list *	prev;
	void *				data;
};

/* Statically initialize a new linked list */
#define nv_list(var) struct _nv_list (var) = { NULL, NULL, NULL }

/* Dynamically initialize a new linked list */
#define nv_list_new(var)	struct _nv_list *(var); \
							(var) = nv_calloc(struct _nv_list, 1)

/* Clean a linked list.  This frees all nodes except for the conventional
 * first element.  It if is dynamically allocated, call nv_list_free() to
 * free it.  The programmer is responsible for freeing all data associated
 * with the linked list before calling this.
 */
static inline nv_list_clean(struct _nv_list *l) {
	/* TODO write nv_list_clean() */
}

#endif
