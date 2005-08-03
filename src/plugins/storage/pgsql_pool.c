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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <netvizd.h>
#include <nvlist.h>
#include <nvconfig.h>
#include <libpq-fe.h>
#include <pthread.h>
#include "pgsql.h"
#include "pgsql_pool.h"

static void *pgsql_pool_thread(void *arg);
static PGconn *pgsql_connect(struct nv_stor *s);
static void pgsql_disconnect(PGconn *c);
static void pgsql_pool_status(struct pgsql_data *me);

int pgsql_pool_init(struct nv_stor *s, int num) {
	struct pgsql_data *me = NULL;
	int ret = 0;
	pthread_attr_t attr;
	
	me = (struct pgsql_data *)s->data;
	
	/* initialize the pool lock and condition variables */
	me->pool.lock = nv_calloc(pthread_mutex_t, 1);
	me->pool.conn_avail = nv_calloc(pthread_cond_t, 1);
	me->pool.quit = nv_calloc(pthread_cond_t, 1);
	pthread_mutex_init(me->pool.lock, NULL);
	pthread_cond_init(me->pool.conn_avail, NULL);
	pthread_cond_init(me->pool.quit, NULL);
	
	/* init pool lists */
	nv_list_new(me->pool.free);
	nv_list_new(me->pool.inuse);
	
	/* save num */
	me->pool.num = num;
	
	/* start up our pool management thread */
	pthread_attr_init(&attr);
	me->pool.thread = nv_calloc(pthread_t, 1);
	ret = pthread_create(me->pool.thread, &attr, pgsql_pool_thread, s);
	if (ret != 0) {
		nv_perror(LOG_ERROR, "pthread_create()", ret);
		return EXIT_FAILURE;
	}
	
	return 0;	
}

int pgsql_pool_free(struct nv_stor *s) {
	struct pgsql_data *me = NULL;
	PGconn *conn = NULL;
	nv_node i;
	
	me = (struct pgsql_data *)s->data;
	
	/* signal that we are to shut down the pool */
	nv_signal(me->pool.quit);
}

static void *pgsql_pool_thread(void *arg) {
	struct nv_stor *s = NULL;
	struct pgsql_data *me = NULL;
	nv_node n;
	nv_node j;
	int i = 0;
	PGconn *conn = NULL;
	struct pgsql_conn *c = NULL;

	/* get typed pointers to our data */
	s = (struct nv_stor *)arg;
	me = (struct pgsql_data *)s->data;

	/* grab the pool lock and set default counters */
	nv_lock(me->pool.lock);
	me->pool.num_free = 0;
	me->pool.num_inuse = 0;
	
	/* begin connection acquisition */
	for (i = 0; i < me->pool.num; i++) {
		nv_node_new(n);
		c = nv_calloc(struct pgsql_conn, 1);
		conn = pgsql_connect(s);
		if (NULL == conn) {
			/* TODO fill me in */
			
			
		}
		c->conn = conn;
		c->node = n;
		set_node_data(n, c);
		list_append(me->pool.free, n);
		me->pool.num_free++;
	}
	nv_log(LOG_DEBUG, "pool initialization complete, announcing to waiting "
		   "threads");
	pgsql_pool_status(me);
	
	/* broadcast that connections are available */
	nv_broadcast(me->pool.conn_avail);	
	
	/* wait for the signal to shut down */
	pthread_cond_wait(me->pool.quit, me->pool.lock);
	
	/* disconnect our connections */
	list_for_each(j, me->pool.free) {
		c = node_data(struct pgsql_conn, j);
		pgsql_disconnect(c->conn);
	}
	list_for_each(j, me->pool.inuse) {
		c = node_data(struct pgsql_conn, j);
		pgsql_disconnect(c->conn);
	}
	
	/* release the pool lock */
	nv_unlock(me->pool.lock);
	
	/* free the lock and condition variables */
	pthread_mutex_destroy(me->pool.lock);
	pthread_cond_destroy(me->pool.conn_avail);
	pthread_cond_destroy(me->pool.quit);
	nv_free(me->pool.lock);
	nv_free(me->pool.conn_avail);
	nv_free(me->pool.quit);
	
	return NULL;
}

struct pgsql_conn *pgsql_pool_get(struct nv_stor *s) {
	struct pgsql_data *me = NULL;
	struct pgsql_conn *c = NULL;
	nv_node n;
	
	me = (struct pgsql_data *)s->data;
	
	/* grab the pool lock */
	nv_lock(me->pool.lock);
	
	/* wait for a connection to become available and grab one */
	while (me->pool.num_free == 0) {
		nv_log(LOG_DEBUG, "pgsql_pool_get(); waiting for a free connection");
		nv_wait(me->pool.conn_avail, me->pool.lock);
	}
	n = me->pool.free->next;
	c = node_data(struct pgsql_conn, n);
	list_del(n);
	me->pool.num_free--;
	me->pool.num_inuse++;
	
	/* move to the inuse list */
	nv_node_new(n);
	c->node = n;
	set_node_data(n, c);
	list_append(me->pool.inuse, n);
	
	nv_log(LOG_DEBUG, "pgsql_pool_get(): grabbed a connection");
	pgsql_pool_status(me);

	/* release the pool lock */
	nv_unlock(me->pool.lock);
	
	return c;
}

void pgsql_pool_release(struct nv_stor *s, struct pgsql_conn *c) {
	struct pgsql_data *me = NULL;
	nv_node n;
	
	me = (struct pgsql_data *)s->data;
	
	/* grab the pool lock */
	nv_lock(me->pool.lock);
	
	/* return the connection to the pool */
	n = c->node;
	list_del(n);
	nv_node_new(n);
	c->node = n;
	set_node_data(n, c);
	list_append(me->pool.free, n);

	/* signal that another connection is available */
	me->pool.num_free++;
	me->pool.num_inuse--;
	nv_signal(me->pool.conn_avail);

	nv_log(LOG_DEBUG, "pgsql_pool_release(): released a connection");
	pgsql_pool_status(me);

	/* release the pool lock */
	nv_unlock(me->pool.lock);
	
}

PGconn *pgsql_connect(struct nv_stor *s) {
	struct pgsql_data *me = NULL;
	PGconn *conn = NULL;
	char buf[NAME_LEN];
	char sslmode[NAME_LEN];
	
	me = (struct pgsql_data *)s->data;
	if (me->ssl) strncpy(sslmode, "require", 8);
	else strncpy(sslmode, "disable", 8);
	snprintf(buf, NAME_LEN, "host=%s port=%i user=%s password=%s dbname=%s "
			 "sslmode=%s",
			 me->host, me->port, me->user, me->pass, me->db, sslmode);
	conn = PQconnectdb(buf);
	switch (PQstatus(conn)) {
		case CONNECTION_OK:
			/* do nothing */
			break;
			
		case CONNECTION_BAD:
			nv_log(LOG_ERROR, "%s: database connection failed", s->name);
			conn = NULL;
			break;
	}
	return conn;
}

void pgsql_disconnect(PGconn *conn) {
	PQfinish(conn);
}

static void pgsql_pool_status(struct pgsql_data *me) {
	nv_log(LOG_DEBUG, "pool status: %i in use, %i free", me->pool.num_inuse,
		   me->pool.num_free);
}
