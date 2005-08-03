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
#include <io.h>
#include <sys/types.h>
#include <unistd.h>

#define DEF_INTERVAL 300

struct rrd_data {
	int			interval;
	char *		file;
	char *		rrdtool;
	int			start;
	int			column;
};

#define sensor_init		rrd_LTX_sensor_init
static int rrd_free(struct nv_sens_p *p);
static int rrd_inst_init(struct nv_sens *s);
static int rrd_inst_free(struct nv_sens *s);
static int rrd_beatfunc(struct nv_sens *s);
static int rrd_get_ts_utime(struct nv_sens *s);

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
		} else if (strncmp(c->key, "rrdtool", NAME_LEN) == 0) {
			me->rrdtool = c->value;
		} else if (strncmp(c->key, "start", NAME_LEN) == 0) {
			me->start = atoi(c->value);
		} else if (strncmp(c->key, "column", NAME_LEN) == 0) {
			me->column = atoi(c->value);
		} else {
			nv_log(LOG_ERROR, "unknown key \"%s\" with value \"%s\"",
				   c->key, c->value);
			stat = -1;
			goto cleanup;
		}
	}

	/* check for missing information */
	if (me->rrdtool == NULL) {
		me->rrdtool = strdup("rrdtool");
	}
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

	return 0;
}

int rrd_beatfunc(struct nv_sens *s) {
	nv_node n;
	time_t rrd_time = 0;
	FILE *rrdout = NULL;
	int fd = 0;
	char buf[BUF_LEN];
	struct rrd_data *me = NULL;
	ssize_t c = 0;
	char *word = NULL;
	char *brk = NULL;
	int col = -1;
	int row = 0;
	time_t vtime = 0;
	time_t valid_vtime = 0;
	double value = 0.0L;
	char sep[] = " :\t\n";

	/* get our local instance data */
	me = (struct rrd_data *)s->data;

	/* loop through data sets and submit appropriate data to each storage
	 * plugin */
	rrd_time = rrd_get_ts_utime(s);
	list_for_each(n, s->dsets) {
		time_t ds_time = 0;
		struct nv_dsts *d = node_data(struct nv_dsts, n);

		/* get last updated time for data set */
		ds_time = stor_get_ts_utime(d);
		if (ds_time == 0) {
			ds_time = me->start;
		}

		if (rrd_time > ds_time) {
			/* we need to update the storage for this dataset, call
			 * rrdtool to get data since last update */
			snprintf(buf, BUF_LEN, "%s fetch %s AVERAGE -s %i -e %i",
					 me->rrdtool, me->file, ds_time, rrd_time);
			nv_log(LOG_DEBUG, "running cmd: %s", buf);
			rrdout = popen(buf, "r");
			fd = fileno(rrdout);

			/* read each line and add to storage */
			for (;;) {
nextline:
				/* read line */
				c = readline(fd, buf, BUF_LEN-1);
				buf[BUF_LEN-1] = '\0';
				if (0 == c) break;
				row++;
				/* skip first line */
				if (row < 3) goto nextline;

				/* parse line into time and find our column */
				col = -1;
				for (word = strtok_r(buf, sep, &brk); word;
					 word = strtok_r(NULL, sep, &brk)) {
					if (col == -1) {
						/* time */
						vtime = atoi(word);
					} else if (me->column == col) {
						/* data value in our column */
						if (strncmp("nan", word, 3) == 0) goto nextline;
						value = atof(word);
						nv_log(LOG_DEBUG, "adding time %i with value %f",
							   vtime, value);
						stor_submit_ts_data(d, vtime, value);
						valid_vtime = vtime;
						break;
					}
					col++;
				}
			}
			pclose(rrdout);
		}

		/* store new updated time for data set... TODO This currently will
		 * cause the above code to attempt to re-add the last value added
		 * during this run when it is run next. */
		stor_submit_ts_utime(d, valid_vtime);
	}
	
	return 0;
}

int rrd_get_ts_utime(struct nv_sens *s) {
	FILE *rrdout = NULL;
	char buf[BUF_LEN];
	time_t utime = 0;
	int c = 0;
	struct rrd_data *me = NULL;

	/* get our local instance data */
	me = (struct rrd_data *)s->data;

	snprintf(buf, BUF_LEN, "%s last %s", me->rrdtool, me->file);
	nv_log(LOG_DEBUG, "running cmd: %s", buf);
	rrdout = popen(buf, "r");
	c = fread(buf, 1, BUF_LEN-1, rrdout);
	if (c > 0) {
		buf[c] = '\0';
		if (strncmp("-1", buf, 2) == 0) {
			/* rrdtool last returns -1 if no file */
			goto cleanup;
		}
		utime = atoi(buf);
	}

cleanup:
	pclose(rrdout);
	return utime;
}
