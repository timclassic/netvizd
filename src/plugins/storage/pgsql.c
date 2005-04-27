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
#include <nvconfig.h>
#include <libpq-fe.h>
#include <time.h>
#include <storage.h>

/* various PostgreSQL OIDs, used for parameter types */
#define VARCHAROID			1043
#define INT4OID				23

enum pgsql_stat {
	PGSQL_DISCONNECTED,
	PGSQL_CONNECTING,
	PGSQL_CONNECTED
};

struct pgsql_data {
	char *			host;
	int				port;
	char *			user;
	char *			pass;
	char *			db;
	int				ssl;
	enum pgsql_stat	stat; /* currently not used */
};


#define storage_init	pgsql_LTX_storage_init
static int pgsql_free(struct nv_stor_p *p);
static int pgsql_inst_init(struct nv_stor *s);
static int pgsql_inst_free(struct nv_stor *s);
static int pgsql_stor_ts_data(struct nv_stor *s, char *dset, char *sys,
							  time_t time, double value);
static int pgsql_stor_ts_utime(struct nv_stor *s, char *dset, char *sys,
							   time_t time);
static time_t pgsql_get_ts_utime(struct nv_stor *s, char *dset, char *sys);
static PGconn *pgsql_connect(struct nv_stor *s);
static void pgsql_disconnect(PGconn *conn);
static int pgsql_init_table(PGconn *c, char *table, char *sql);
static int pgsql_add_row(PGconn *c, char *table, time_t time, double value);
static nv_list *pgsql_get_ts_data(struct nv_stor *s, char *dset, char *sys,
								  time_t start, time_t end, int res);

int storage_init(struct nv_stor_p *p) {
	int stat = 0;

	/* fill in config structure */
	p->free = pgsql_free;
	p->inst_init = pgsql_inst_init;
	p->inst_free = pgsql_inst_free;
	p->stor_ts_data = pgsql_stor_ts_data;
	p->get_ts_data = pgsql_get_ts_data;
	p->stor_ts_utime = pgsql_stor_ts_utime;
	p->get_ts_utime = pgsql_get_ts_utime;

	return stat;
}

static int pgsql_free(struct nv_stor_p *p) {
	return 0;
}

#define SQL_CREATE_META		"CREATE TABLE %s ( " \
							"    name VARCHAR(256) PRIMARY KEY, " \
							"    utime TIMESTAMP WITH TIME ZONE NOT NULL" \
							");"
static int pgsql_inst_init(struct nv_stor *s) {
	nv_node i;
	int stat = 0;
	int ret = 0;
	PGconn *c = NULL;
	struct pgsql_data *me = NULL;

	/* process configuration */
	me = nv_calloc(struct pgsql_data, 1);
	me->stat = PGSQL_DISCONNECTED;
	s->data = (void *)me;
	s->beat = 0;
	s->beatfunc = NULL;
	list_for_each(i, s->conf) {
		struct nv_conf *c = node_data(struct nv_conf, i);

		if (strncmp(c->key, "host", NAME_LEN) == 0) {
			me->host = c->value;
		} else if (strncmp(c->key, "port", NAME_LEN) == 0) {
			me->port = atoi(c->value);
		} else if (strncmp(c->key, "user", NAME_LEN) == 0) {
			me->user = c->value;
		} else if (strncmp(c->key, "pass", NAME_LEN) == 0) {
			me->pass = c->value;
		} else if (strncmp(c->key, "db", NAME_LEN) == 0) {
			me->db = c->value;
		} else if (strncmp(c->key, "ssl", NAME_LEN) == 0) {
			if (strncmp(c->value, "yes", 3) == 0) {
				me->ssl = 1;
			} else {
				me->ssl = 0;
			}
		} else {
			nv_log(LOG_ERROR, "unknown key \"%s\" with value \"%s\"",
				   c->key, c->value);
			stat = -1;
			goto cleanup;
		}
	}

	/* check for missing information */
	if (me->host == NULL) {
		stat = -1;
		nv_log(LOG_ERROR, "host not specified for pgsql plugin instance %s",
			   s->name);
		goto cleanup;
	}
	if (me->user == NULL) {
		stat = -1;
		nv_log(LOG_ERROR, "user not specified for pgsql plugin instance %s",
			   s->name);
		goto cleanup;
	}
	if (me->pass == NULL) {
		stat = -1;
		nv_log(LOG_ERROR, "pass not specified for pgsql plugin instance %s",
			   s->name);
		goto cleanup;
	}
	if (me->port == 0) {
		me->port = 5432;
	}

	/* init the nv_dsts table */
	c = pgsql_connect(s);
	if (NULL == c) {
		stat = -1;
		goto cleanup;
	}
	ret = pgsql_init_table(c, "nv_dsts", SQL_CREATE_META);
	if (0 > ret) {
		nv_log(LOG_ERROR, "error initializing table nv_dsts, aborting");
		stat = -1;
		goto cleanup;
	}
	pgsql_disconnect(c);

cleanup:
	return stat;
}

static int pgsql_inst_free(struct nv_stor *s) {
	return 0;
}


#define SQL_CREATE_DS	"CREATE TABLE %s ( " \
						"    time TIMESTAMP WITH TIME ZONE PRIMARY KEY, " \
						"    value DOUBLE PRECISION" \
						");"
static int pgsql_stor_ts_data(struct nv_stor *s, char *dset, char *sys,
							  time_t time, double value) {
	struct pgsql_data *me = NULL;
	int stat = 0;
	PGconn *c = NULL;
	int res = 0;
	char buf[NAME_LEN];

	me = (struct pgsql_data *)s->data;
	
	/* get a connection and init the table */
	c = pgsql_connect(s);
	if (NULL == c) {
		stat = -1;
		goto cleanup;
	}
	snprintf(buf, NAME_LEN, "dsts_%s_%s", sys, dset);
	buf[NAME_LEN-1] = '\0';
	res = pgsql_init_table(c, buf, SQL_CREATE_DS);
	if (0 > res) {
		nv_log(LOG_ERROR, "error initializing table %s, aborting", buf);
		stat = -1;
		goto cleanup;
	}

	/* store the data element in the table */
	res = pgsql_add_row(c, buf, time, value);
	if (0 > res) {
		nv_log(LOG_ERROR, "error writing values to table %s, aborting", buf);
		stat = -1;
		goto cleanup;
	}

cleanup:
	pgsql_disconnect(c);
	return stat;
}


#define SQL_GET_TS		"SELECT EXTRACT(epoch FROM time), value " \
						"FROM %s " \
						"WHERE time >= '%s' AND time <= '%s';"
static nv_list *pgsql_get_ts_data(struct nv_stor *s, char *dset, char *sys,
								  time_t start, time_t end, int res) {
	struct pgsql_data *me = NULL;
	nv_list *list = NULL;
	int stat = 0;
	int ret = 0;
	PGconn *c = NULL;
	char buf[NAME_LEN];
	char table[NAME_LEN];
	char startbuf[26];
	char endbuf[26];
	PGresult *result = NULL;
	int row = 0;
	int rownum = 0;
	int status = 0;
	
	nv_list_new(list);
	me = (struct pgsql_data *)s->data;

	/* get a connection and init the table */
	c = pgsql_connect(s);
	if (NULL == c) {
		stat = -1;
		goto cleanup;
	}
	snprintf(table, NAME_LEN, "dsts_%s_%s", sys, dset);
	table[NAME_LEN-1] = '\0';
	ret = pgsql_init_table(c, table, SQL_CREATE_DS);
	if (0 > ret) {
		nv_log(LOG_ERROR, "error initializing table %s, aborting", buf);
		stat = -1;
		goto cleanup;
	}

	/* pull data from the table */
	ctime_r(&start, startbuf);
	ctime_r(&end, endbuf);
	snprintf(buf, NAME_LEN, SQL_GET_TS, table, startbuf, endbuf);
	buf[NAME_LEN-1] = '\0';
	PQclear(result);
	result = PQexec(c, buf);
	status = PQresultStatus(result);
	switch(status) {
		case PGRES_TUPLES_OK:
			/* do nothing, we're OK */
			break;

		default:
			nv_log(LOG_ERROR, "libpq: %s", PQresultErrorMessage(result));
			stat = -1;
			goto cleanup;
			break;
	}

	/* add data to list */
	rownum = PQntuples(result);
	for (row = 0; row < rownum; row++) {
		nv_node n;
		struct nv_ts_data *d = NULL;

		d = nv_calloc(struct nv_ts_data, 1);
		d->time = atoi(PQgetvalue(result, row, 0));
		d->value = atof(PQgetvalue(result, row, 1));
		
		nv_node_new(n);
		set_node_data(n, d);
		list_append(list, n);
	}
	
cleanup:
	PQclear(result);
	pgsql_disconnect(c);
	return list;
}


#define SQL_GET_UTIME		"SELECT name, EXTRACT(epoch FROM utime) " \
							"FROM nv_dsts " \
							"WHERE name = '%s';"
#define SQL_ADD_UTIME		"INSERT INTO nv_dsts ( name, utime ) " \
							"VALUES ( '%s', '%s' );"
#define SQL_UPDATE_UTIME	"UPDATE nv_dsts " \
							"SET utime = '%s' " \
							"WHERE name = '%s';"

static int pgsql_stor_ts_utime(struct nv_stor *s, char *dset, char *sys,
							   time_t time) {
	PGconn *c = NULL;
	int stat = 0;
	int ret = 0;
	PGresult *res = NULL;
	char tname[NAME_LEN];
	char buf[NAME_LEN];
	char tbuf[26];

	/* get a connection */
	c = pgsql_connect(s);
	if (NULL == c) {
		stat = -1;
		goto cleanup;
	}

	/* get the table name */
	snprintf(tname, NAME_LEN, "dsts_%s_%s", sys, dset);
	buf[NAME_LEN-1] = '\0';

	/* see if we already have a value for the update time */
	snprintf(buf, NAME_LEN, SQL_GET_UTIME, tname);
	buf[NAME_LEN-1] = '\0';
	res = PQexec(c, buf);
	switch(PQresultStatus(res)) {
		case PGRES_TUPLES_OK:
			/* do nothing, we're OK */
			break;

		default:
			nv_log(LOG_ERROR, "libpq: %s", PQresultErrorMessage(res));
			stat = -1;
			goto cleanup;
			break;
	}
	ctime_r(&time, tbuf);
	if (PQntuples(res) < 1) {
		/* we need to add a new row for the update time */
		snprintf(buf, NAME_LEN, SQL_ADD_UTIME, tname, tbuf);
		buf[NAME_LEN-1] = '\0';
	} else {
		/* we need to update the existing row */
		snprintf(buf, NAME_LEN, SQL_UPDATE_UTIME, tbuf, tname);
		buf[NAME_LEN-1] = '\0';
	}

	/* run the SQL */
	PQclear(res);
	res = PQexec(c, buf);
	switch(PQresultStatus(res)) {
		case PGRES_COMMAND_OK:
			/* do nothing, we're OK */
			break;

		default:
			nv_log(LOG_ERROR, "libpq: %s", PQresultErrorMessage(res));
			stat = -1;
			goto cleanup;
			break;
	}

cleanup:
	PQclear(res);
	pgsql_disconnect(c);
	return stat;
}

time_t pgsql_get_ts_utime(struct nv_stor *s, char *dset, char *sys) {
	PGconn *c = NULL;
	time_t utime = 0;
	int ret = 0;
	PGresult *res = NULL;
	char tname[NAME_LEN];
	char buf[NAME_LEN];
	char tbuf[26];
	char *resval = NULL;

	/* get a connection and init the table */
	c = pgsql_connect(s);
	if (NULL == c) {
		utime = -1;
		goto cleanup;
	}
	ret = pgsql_init_table(c, "nv_dsts", SQL_CREATE_META);
	if (0 > ret) {
		nv_log(LOG_ERROR, "error initializing table nv_dsts, aborting");
		utime = -1;
		goto cleanup;
	}

	/* get the table name */
	snprintf(tname, NAME_LEN, "dsts_%s_%s", sys, dset);
	buf[NAME_LEN-1] = '\0';

	/* get the value for the update time */
	snprintf(buf, NAME_LEN, SQL_GET_UTIME, tname);
	buf[NAME_LEN-1] = '\0';
	res = PQexec(c, buf);
	switch(PQresultStatus(res)) {
		case PGRES_TUPLES_OK:
			/* do nothing, we're OK */
			break;

		default:
			nv_log(LOG_ERROR, "libpq: %s", PQresultErrorMessage(res));
			utime = -1;
			goto cleanup;
			break;
	}

	/* return the value, or zero if none exists */
	if (PQntuples(res) < 1) {
		/* no value returned, never been updated */
		utime = 0;
	} else {
		/* get the time value to return */
		resval = PQgetvalue(res, 0, 1);
		utime = atoi(resval);
	}

cleanup:
	PQclear(res);
	pgsql_disconnect(c);
	return utime;
}


#define SQL_TABLE_EXISTS	"SELECT table_schema, table_name " \
							"FROM information_schema.tables " \
							"WHERE table_schema = 'public' and " \
							"      table_name = $1;"
#define NUM_TABLE_EXISTS	1
#define OID_TABLE_EXISTS	{ VARCHAROID }


int pgsql_init_table(PGconn *c, char *table, char *sql) {
	PGresult *res = NULL;
	Oid types[] = OID_TABLE_EXISTS;
	const char *params[1] = { NULL };
	char buf[NAME_LEN];
	int stat = 0;
	
	params[0] = table;
	res = PQexecParams(c, SQL_TABLE_EXISTS, NUM_TABLE_EXISTS, types,
					   params, NULL, NULL, 0);
	switch (PQresultStatus(res)) {
		case PGRES_TUPLES_OK:
			/* do nothing, we're OK */
			break;

		default:	
			nv_log(LOG_ERROR, "libpq: %s", PQresultErrorMessage(res));
			stat = -1;
			goto cleanup;
			break;
	}
	if (PQntuples(res) < 1) {
		PQclear(res);

		/* table does not exist, create it */
		snprintf(buf, NAME_LEN, sql, table);
		buf[NAME_LEN-1] = '\0';
		res = PQexec(c, buf);
		switch(PQresultStatus(res)) {
			case PGRES_COMMAND_OK:
				/* do nothing, we're OK */
				break;

			default:
				nv_log(LOG_ERROR, "libpq: %s", PQresultErrorMessage(res));
				stat = -1;
				goto cleanup;
				break;
		}
	}

cleanup:
	PQclear(res);
	return stat;
}


#define SQL_ADD_ROW		"INSERT INTO %s ( time, value ) " \
						"VALUES ( '%s', %f );"
int pgsql_add_row(PGconn *c, char *table, time_t time, double value) {
	char buf[NAME_LEN];
	char tbuf[26];
	PGresult *res = NULL;
	int stat = 0;

	ctime_r(&time, tbuf);
	snprintf(buf, NAME_LEN, SQL_ADD_ROW, table, tbuf, value);
	buf[NAME_LEN-1] = '\0';
	res = PQexec(c, buf);
	switch(PQresultStatus(res)) {
		case PGRES_COMMAND_OK:
			/* do nothing, we're OK */
			break;

		default:
			nv_log(LOG_ERROR, "libpq: %s", PQresultErrorMessage(res));
			stat = -1;
			goto cleanup;
			break;
	}

cleanup:
	PQclear(res);
	return stat;
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

/* vim: set ts=4 sw=4: */
