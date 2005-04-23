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

#define DEF_INTERVAL 300

struct rrd_data {
	int			interval;
	char *		file;
};

#define sensor_init		rrd_LTX_sensor_init
static int rrd_free(struct nv_sens_p *p);
static int rrd_inst_init(struct nv_sens *s);
static int rrd_inst_free(struct nv_sens *s);
static int rrd_beatfunc(struct nv_sens *s);

int sensor_init(struct nv_sens_p *p) {
	int stat = 0;

	/* fill in config structure */
	p->free = rrd_free;
	p->inst_init = rrd_inst_init;
	p->inst_free = rrd_inst_free;

	return stat;
}

int rrd_free(struct nv_sens_p *p) {

}

int rrd_inst_init(struct nv_sens *s) {
	nv_node i;
	int stat = 0;
	struct rrd_data *me = NULL;

	/* process configuration */
	s->beatfunc = rrd_beatfunc;
	me = nv_calloc(struct rrd_data, 1);
	s->data = (void *)me;
	me->interval = DEF_INTERVAL;
	s->beat = DEF_INTERVAL;
	list_for_each(i, s->conf) {
		struct nv_conf *c = node_data(struct nv_conf, i);

		if (strncmp(c->key, "interval", NAME_LEN) == 0) {
			me->interval = atoi(c->value);
			s->beat = me->interval;
		} else if (strncmp(c->key, "file", NAME_LEN) == 0) {
			me->file = c->value;
		} else {
			nv_log(LOG_ERROR, "unknown key \"%s\" with value \"%s\"",
				   c->key, c->value);
			stat = -1;
			goto cleanup;
		}
	}

	/* check for missing information */
	if (me->file == NULL) {
		stat = -1;
		nv_log(LOG_ERROR, "file not specified for rrd plugin instance %s",
			   s->name);
		goto cleanup;
	}

cleanup:
	return stat;
}

int rrd_inst_free(struct nv_sens *s) {


}

int rrd_beatfunc(struct nv_sens *s) {
	nv_log(LOG_INFO, "rrd_beatfunc() called");
	
	return 0;
}
