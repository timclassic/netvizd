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
static void *pgsql_pool_logthread(void *arg);
static PGconn *pgsql_connect(struct nv_stor *s);
static void pgsql_disconnect(PGconn *c);
static void pgsql_pool_status(struct nv_stor *s, int level);

int pgsql_pool_init(struct nv_stor *s, int num) {
	struct pgsql_data *me = NULL;
	int ret = 0;
	pthread_attr_t attr;
	
	me = (struct pgsql_data *)s->data;
	
	/* initialize the locks and condition variables */
	me->pool.bad_lock = nv_calloc(pthread_mutex_t, 1);
	me->pool.free_lock = nv_calloc(pthread_mutex_t, 1);
	me->pool.inuse_lock = nv_calloc(pthread_mutex_t, 1);
	me->pool.free_avail = nv_calloc(pthread_cond_t, 1);
	me->pool.bad_avail = nv_calloc(pthread_cond_t, 1);
	pthread_mutex_init(me->pool.bad_lock, NULL);
	pthread_mutex_init(me->pool.free_lock, NULL);
	pthread_mutex_init(me->pool.inuse_lock, NULL);
	pthread_cond_init(me->pool.free_avail, NULL);
	pthread_cond_init(me->pool.bad_avail, NULL);
	
	/* init pool lists */
	nv_list_new(me->pool.bad);
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
	
	/* start up logging thread */
	pthread_attr_init(&attr);
	me->pool.logthread = nv_calloc(pthread_t, 1);
	ret = pthread_create(me->pool.logthread, &attr, pgsql_pool_logthread, s);
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
	
	/* indicate that we are to shut down the pool */
	me->pool.quit = 1;
}

static void *pgsql_pool_thread(void *arg) {
	struct nv_stor *s = NULL;
	struct pgsql_data *me = NULL;
	nv_node n;
	nv_node j;
	int i = 0;
	int ret = 0;
	struct pgsql_conn *c = NULL;
	struct timespec timeout;
	int fail_report = 0;
	int init_report = 0;
	char *buf = NULL;
	
	/* get typed pointers to our data */
	s = (struct nv_stor *)arg;
	me = (struct pgsql_data *)s->data;

	/* grab the pool lock and set default counters */
	nv_lock(me->pool.free_lock);
	me->pool.free_num = 0;
	nv_unlock(me->pool.free_lock);
	nv_lock(me->pool.inuse_lock);
	me->pool.inuse_num = 0;
	nv_unlock(me->pool.inuse_lock);
	nv_lock(me->pool.bad_lock);
	me->pool.bad_num = 0;
	nv_unlock(me->pool.bad_lock);
	
	/* initialize the pool via the bad list */
	nv_lock(me->pool.bad_lock);
	for (i = 0; i < me->pool.num; i++) {
		nv_node_new(n);
		c = nv_calloc(struct pgsql_conn, 1);
		c->node = n;
		c->id = i;
		set_node_data(n, c);
		list_append(me->pool.bad, n);
		me->pool.bad_num++;
	}
	nv_broadcast(me->pool.bad_avail);
	nv_unlock(me->pool.bad_lock);
	
	/* begin connection repair */
	nv_log(LOG_INFO, "%s: pool repair thread starting", s->name);
	for (;;) {
		/* wait for there to be bad connections */
		nv_lock(me->pool.bad_lock);
		while (me->pool.bad_num == 0) {
			timeout.tv_sec = time(NULL) + 1;
			timeout.tv_nsec = 0;
			ret = pthread_cond_timedwait(me->pool.bad_avail, me->pool.bad_lock,
								   &timeout);
			if (ret == ETIMEDOUT) {
				if (me->pool.quit) {
					nv_unlock(me->pool.bad_lock);
					goto cleanup;
				}
			}
		}

		/* pull first node out and release bad lock */
		n = me->pool.bad->next;
		c = node_data(struct pgsql_conn, n);
		list_del(n);
		me->pool.bad_num--;
		nv_unlock(me->pool.bad_lock);

		/* try to reconnect to the database */
retry:
		if (me->pool.quit) goto cleanup;
		if (NULL == c->conn) {
			/* new connection, never before initialized */
			if (!init_report && !fail_report) {
				nv_log(LOG_WARN, "%s: starting pool initialization", s->name);
				init_report = 1;
			}
			nv_log(LOG_DEBUG, "%s: attempting a new connection", s->name);
			c->conn = pgsql_connect(s);
		} else {
			/* this connection needs repair */
			if (!fail_report && !init_report) {
				nv_log(LOG_WARN, "%s: failed connections exist in pool, "
					   "running repair attempts every %i seconds", s->name,
					   me->rtimeout);
				fail_report = 1;
			}
			nv_log(LOG_DEBUG, "%s: attempting a connection reset", s->name);
			PQreset(c->conn);
		}
		if (me->pool.quit) goto cleanup;
		
		/* did the connection succeed?  If not, retry */
		switch (PQstatus(c->conn)) {
			case CONNECTION_OK:
				nv_log(LOG_DEBUG, "%s: connection id %i successful", s->name,
					   c->id);
				if (fail_report && me->pool.bad_num == 0) {
					nv_log(LOG_INFO, "%s: all connections in pool are up, "
						   "finished with repair", s->name);
					fail_report = 0;
				} else if (init_report && me->pool.bad_num == 0) {
					nv_log(LOG_INFO, "%s: all connections in pool are up, "
						   "finished with pool initialization", s->name);
					init_report = 0;
				}
				/* do nothing */
				break;
				
			case CONNECTION_BAD:
				buf = strdup(PQerrorMessage(c->conn));
				buf[strlen(buf)-1] = '\0';
				nv_log(LOG_DEBUG, "%s: connection failed: %s", s->name, buf);
				free(buf);
				nv_log(LOG_DEBUG, "%s: database connection id %i failed, "
					   "resuming repair in %i seconds", s->name, c->id,
					   me->rtimeout);
				sleep(me->rtimeout);
				goto retry;
				break;
		}
		
		/* we should now have a good connection, add to free list */
		n = nv_node_new(n);
		c->node = n;
		set_node_data(n, c);
		nv_lock(me->pool.free_lock);
		me->pool.free_num++;
		list_append(me->pool.free, n);
		nv_signal(me->pool.free_avail);
		nv_unlock(me->pool.free_lock);
	}

cleanup:
	nv_log(LOG_INFO, "%s: pool repair thread stopping", s->name);

	/* release our connections */
	nv_lock(me->pool.free_lock);
	list_for_each(j, me->pool.free) {
		c = node_data(struct pgsql_conn, j);
		if (c->conn) pgsql_disconnect(c->conn);
		me->pool.free_num--;
	}
	nv_unlock(me->pool.free_lock);
	nv_lock(me->pool.inuse_lock);
	list_for_each(j, me->pool.inuse) {
		c = node_data(struct pgsql_conn, j);
		if (c->conn) pgsql_disconnect(c->conn);
		me->pool.inuse_num--;
	}
	nv_unlock(me->pool.inuse_lock);
	nv_lock(me->pool.bad_lock);
	list_for_each(j, me->pool.bad) {
		c = node_data(struct pgsql_conn, j);
		if (c->conn) pgsql_disconnect(c->conn);
		me->pool.bad_num--;
	}
	nv_unlock(me->pool.bad_lock);
	
	/* free the locks and condition variables */
	pthread_mutex_destroy(me->pool.bad_lock);
	pthread_mutex_destroy(me->pool.free_lock);
	pthread_mutex_destroy(me->pool.inuse_lock);
	pthread_cond_destroy(me->pool.free_avail);
	pthread_cond_destroy(me->pool.bad_avail);
	nv_free(me->pool.bad_lock);
	nv_free(me->pool.free_lock);
	nv_free(me->pool.inuse_lock);
	nv_free(me->pool.free_avail);
	nv_free(me->pool.bad_avail);
	
	/* free lists */
	nv_free(me->pool.bad);
	nv_free(me->pool.free);
	nv_free(me->pool.inuse);
	
	return NULL;
}

struct pgsql_conn *pgsql_pool_get(struct nv_stor *s) {
	struct pgsql_data *me = NULL;
	struct pgsql_conn *c = NULL;
	nv_node n;
	
	me = (struct pgsql_data *)s->data;
	
	/* wait for a connection to become available and grab one */
retry:
	nv_lock(me->pool.free_lock);
	while (me->pool.free_num == 0) {
		nv_log(LOG_DEBUG, "%s: no free connections in pool, waiting...",
			   s->name);
		nv_wait(me->pool.free_avail, me->pool.free_lock);
	}
	n = me->pool.free->next;
	c = node_data(struct pgsql_conn, n);
	list_del(n);
	me->pool.free_num--;
	nv_unlock(me->pool.free_lock);
	
	/* check the connection status */
	if (PQstatus(c->conn) == CONNECTION_BAD) {
		/* add to bad list */
		nv_node_new(n);
		c->node = n;
		set_node_data(n, c);
		nv_lock(me->pool.bad_lock);
		list_append(me->pool.bad, n);
		me->pool.bad_num++;
		nv_signal(me->pool.bad_avail);
		nv_unlock(me->pool.bad_lock);
		
		nv_log(LOG_DEBUG, "%s: marked connection id %i bad", s->name, c->id);
		goto retry;
	}
	
	/* move to the inuse list */
	nv_node_new(n);
	c->node = n;
	set_node_data(n, c);
	nv_lock(me->pool.inuse_lock);
	list_append(me->pool.inuse, n);
	me->pool.inuse_num++;
	nv_unlock(me->pool.inuse_lock);
	
	nv_log(LOG_DEBUG, "%s: grabbed connection id %i", s->name, c->id);

	return c;
}

void pgsql_pool_release(struct nv_stor *s, struct pgsql_conn *c) {
	struct pgsql_data *me = NULL;
	nv_node n;
	
	me = (struct pgsql_data *)s->data;
	
	/* remove from in-use list */
	n = c->node;
	nv_lock(me->pool.inuse_lock);
	list_del(n);
	me->pool.inuse_num--;
	nv_unlock(me->pool.inuse_lock);
	
	/* check status of connection */
	if (PQstatus(c->conn) == CONNECTION_BAD) {
		/* add to bad list */
		nv_node_new(n);
		c->node = n;
		set_node_data(n, c);
		nv_lock(me->pool.bad_lock);
		list_append(me->pool.bad, n);
		me->pool.bad_num++;
		nv_signal(me->pool.bad_avail);
		nv_unlock(me->pool.bad_lock);
		
		nv_log(LOG_DEBUG, "%s: marked connection id %i bad", s->name, c->id);
	} else {
		/* add to free list */
		nv_node_new(n);
		c->node = n;
		set_node_data(n, c);
		nv_lock(me->pool.free_lock);
		list_append(me->pool.free, n);
		me->pool.free_num++;
		nv_signal(me->pool.free_avail);
		nv_unlock(me->pool.free_lock);

		nv_log(LOG_DEBUG, "%s: released connection id %i", s->name, c->id);
	}
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
			 "sslmode=%s connect_timeout=%i",
			 me->host, me->port, me->user, me->pass, me->db, sslmode,
			 me->ctimeout);
	conn = PQconnectdb(buf);
	return conn;
}

void pgsql_disconnect(PGconn *conn) {
	PQfinish(conn);
}

static void pgsql_pool_status(struct nv_stor *s, int level) {
	struct pgsql_data *me = NULL;

	me = (struct pgsql_data *)s->data;
	nv_log(level, "%s: pool status: %i in use, %i free, %i bad", s->name,
		   me->pool.inuse_num, me->pool.free_num, me->pool.bad_num);
}

static void *pgsql_pool_logthread(void *arg) {
	struct nv_stor *s = NULL;
	struct pgsql_data *me = NULL;
	
	/* get typed pointers to our data */
	s = (struct nv_stor *)arg;
	me = (struct pgsql_data *)s->data;

	/* initial report */
	nv_log(LOG_INFO, "%s: pool logging thread starting", s->name);
	pgsql_pool_status(s, LOG_INFO);
	
	/* log major changes in pool status */
	me->pool.lastbad = me->pool.bad_num;
	for (;;) {
		sleep(5);

		/* report status if bad number changes */
		if (me->pool.bad_num < me->pool.lastbad) {
			pgsql_pool_status(s, LOG_INFO);
			me->pool.lastbad = me->pool.bad_num;
		} else if (me->pool.bad_num > me->pool.lastbad) {
			pgsql_pool_status(s, LOG_WARN);
			me->pool.lastbad = me->pool.bad_num;
		}
		
		/* check for exit */
		if (me->pool.quit) break;
	}
	
	/* final report */
	pgsql_pool_status(s, LOG_INFO);
	nv_log(LOG_INFO, "%s: pool logging thread stopping", s->name);
}

