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
#include <sensor.h>
#include <storage.h>

/*
 * The entry point for a sensor heartbeat thread.  This will only be started
 * if the sensor instance has indicated that it periodically needs to be
 * called.  The sensor instance can do anything it wishes here, including
 * blocking.
 */
void *sens_thread(void *arg) {
	struct nv_sens *s = (struct nv_sens *)arg;
	time_t last;
	time_t now;
	int *stat = NULL;

	nv_log(LOG_INFO, "%s: sensor heartbeat thread starting", s->name);

	stat = nv_calloc(int, 1);

//	last = time(NULL);
	last = 0;
	for (;;) {
		/* call beatfunc if necessary */
		if (s->beat > 0 && s->beatfunc != NULL) {
			now = time(NULL);
			if (now >= last+s->beat) {
				last = now;
				*stat = s->beatfunc(s);
			}
			if (*stat != 0) break;
		}

		/* give up the processor for a bit */
		usleep(THREAD_SLEEP);
	}

	nv_log(LOG_INFO, "%s: sensor heartbeat thread stopping", s->name);
	return (void *)stat;
}

/*
 * Called by a sensor plugin when it has new time series data to submit.
 * Here we hand the data off to the storage plugins associated with data
 * sets concerned with this sensor.
 */
int sens_submit_ts_data(struct nv_sens *s, int time, int value) {
	nv_node n;

	list_for_each(n, s->dsets) {
		struct nv_dsts *d = node_data(struct nv_dsts, n);
		stor_submit_ts_data(d, time, value);
	}

	return 0;
}



/* vim: set ts=4 sw=4: */
