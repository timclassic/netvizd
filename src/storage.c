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
#include <storage.h>

/*
 * The entry point for a storage heartbeat thread.  This will only be started
 * if the storage instance has indicated that it periodically needs to be
 * called.  The storage instance can do anything it wishes here, including
 * blocking.
 */
void *stor_thread(void *arg) {
	struct nv_stor *s = (struct nv_stor *)arg;
	time_t last;
	time_t now;
	int *stat = NULL;

	nv_log(LOG_DEBUG, "work thread for storage instance %s starting", s->name);

	stat = nv_calloc(int, 1);

	last = time(NULL);
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

	nv_log(LOG_DEBUG, "work thread for storage instance %s terminating",
		   s->name);
	return (void *)stat;
}

/*
 * Here we submit a time-series data element to be stored in the given
 * storage plugin.
 */
int stor_submit_ts_data(struct nv_dsts *d, time_t time, int value) {
	return d->stor->plug->stor_ts_data(d->stor, d->name, d->sys->name, time,
									   value);
}

/*
 * Let a sensor plugin store the last-updated time in a storage plugin.
 */
int stor_submit_ts_utime(struct nv_dsts *d, time_t time) {
	return d->stor->plug->stor_ts_utime(d->stor, d->name, d->sys->name, time);
}

/*
 * Let a sensor plugin retreive the last-updated time.
 */
time_t stor_get_ts_utime(struct nv_dsts *d) {
	return d->stor->plug->get_ts_utime(d->stor, d->name, d->sys->name);
}

/* vim: set ts=4 sw=4: */
