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

#ifndef _PLUGINS_PGSQL_POOL_H_
#define _PLUGINS_PGSQL_POOL_H_

#include <netvizd.h>
#include <nvconfig.h>
#include <nvlist.h>
#include <libpq-fe.h>

struct pgsql_pool {
	int					num;          /* number of connections in the pool */
	pthread_t *			thread;       /* management thread */
	int					quit;         /* quit flag */
	
	pthread_t *			logthread;    /* logging thread */
	int					lastbad;      /* previous number of bad connections */
	
	/* bad list */
	pthread_mutex_t *	bad_lock;     /* lock on bad list */
	int					bad_num;      /* number of bad connections */
	pthread_cond_t *	bad_avail;    /* "bad conn available" condition */
	nv_list *			bad;          /* list of (struct pgsql_conn *) */
	
	/* free list */
	pthread_mutex_t *	free_lock;    /* lock on free list */
	int					free_num;     /* number of free connections */
	pthread_cond_t *	free_avail;   /* "free conn available" condition */
	nv_list *			free;         /* list of (struct pgsql_conn *) */
	
	/* in-use list */
	pthread_mutex_t *   inuse_lock;   /* lock on in-use list */
	int					inuse_num;    /* number of in-use connections */
	nv_list *			inuse;        /* list of (struct pgsql_conn *) */
};
					
struct pgsql_conn {
	int				id;
	PGconn *		conn;
	nv_node			node;
};

/* public connection pool interface */
int pgsql_pool_init(struct nv_stor *s, int num);
int pgsql_pool_free(struct nv_stor *s);
struct pgsql_conn *pgsql_pool_get(struct nv_stor *s);
void pgsql_pool_release(struct nv_stor *s, struct pgsql_conn *conn);

#define pgsql_pool_conncheck(s, c, label) \
	if (PQstatus((c)->conn) == CONNECTION_BAD) { \
		pgsql_pool_release((s), (c)); \
		goto label; \
	}

#endif
