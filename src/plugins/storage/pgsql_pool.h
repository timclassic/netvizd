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
	pthread_mutex_t *	lock;         /* pool read/write lock */
	pthread_cond_t *	conn_avail;   /* "connection(s) available" condition */
	pthread_cond_t *	quit;         /* "quit thread" condition */
	int					num_free;     /* number of free connections */
	int					num_inuse;    /* number of inuse connections */
	nv_list *			free;         /* list of (struct pgsql_conn *) */
	nv_list *			inuse;        /* list of (struct pgsql_conn *) */
};
					
struct pgsql_conn {
	PGconn *		conn;
	nv_node			node;
};

/* public connection pool interface */
int pgsql_pool_init(struct nv_stor *s, int num);
int pgsql_pool_free(struct nv_stor *s);
struct pgsql_conn *pgsql_pool_get(struct nv_stor *s);
void pgsql_pool_release(struct nv_stor *s, struct pgsql_conn *conn);

#endif
