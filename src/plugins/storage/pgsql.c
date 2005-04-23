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

struct pgsql_data {
	char *			host;
	int				port;
	char *			user;
	char *			pass;
	int				ssl;
};

#define storage_init	pgsql_LTX_storage_init
static int pgsql_free(struct nv_stor_p *p);
static int pgsql_inst_init(struct nv_stor *s);
static int pgsql_inst_free(struct nv_stor *s);
static int pgsql_stor_ts_data(char *dset, char *sys, int time, int value);

int storage_init(struct nv_stor_p *p) {
	int stat = 0;

	/* fill in config structure */
	p->free = pgsql_free;
	p->inst_init = pgsql_inst_init;
	p->inst_free = pgsql_inst_free;
	p->stor_ts_data = pgsql_stor_ts_data;

	return stat;
}

static int pgsql_free(struct nv_stor_p *p) {
	return 0;
}

static int pgsql_inst_init(struct nv_stor *s) {
	nv_node i;
	int stat = 0;
	struct pgsql_data *me = NULL;

	/* process configuration */
	me = nv_calloc(struct pgsql_data, 1);
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

cleanup:
	return stat;
}

static int pgsql_inst_free(struct nv_stor *s) {
	return 0;
}

static int pgsql_stor_ts_data(char *dset, char *sys, int time, int value) {
	return 0;
}

/* vim: set ts=4 sw=4: */
