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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <pthread.h>
#include <ctype.h>
#include <stdio.h>
#include <storage.h>

static int net_listen();
static void *client_thread(void *);

#define proto_init		net_LTX_proto_init

int proto_init(struct nv_proto_p *p) {
	int stat = 0;

	p->listen = net_listen;

	return stat;
}

#define NET_PORT	12346
int net_listen() {
	int stat = 0;
	int ret = 0;
	int opt = 0;
	int sockfd = 0;
	struct sockaddr_in addr;
	int c_sockfd = 0;
	socklen_t c_len = 0;
	struct sockaddr_in c_addr;
	pthread_t *t = NULL;
	pthread_attr_t attr;
	int *fd = NULL;

	/* init pthreads */
	pthread_attr_init(&attr);

    /* get a socket */
    sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (0 > sockfd) {
        nv_perror(LOG_ERROR, "socket", errno);
        stat = -1;
		goto cleanup;
    }
    /* reuse addresses */
    opt = 1;
    ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (0 > ret) {
        nv_perror(LOG_ERROR, "setsockopt", errno);
        stat = -1;
		goto cleanup;
    }
    /* set up address structure */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(NET_PORT);
    /* bind to local address */
    ret = bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    if (0 > ret) { 
        nv_perror(LOG_ERROR, "bind", errno);
        stat = -1;
		goto cleanup;
    }   
    /* set socket to listen */
    ret = listen(sockfd, 5);
    if (0 > ret) {
        nv_perror(LOG_ERROR, "listen", errno);
        stat = -1;
		goto cleanup;
    }

    /* accept incoming connections */
    for (;;) {
        c_len = sizeof(c_addr);
        c_sockfd = accept(sockfd, (struct sockaddr *)&c_addr, &c_len);

		t = nv_calloc(pthread_t, 1);
		fd = nv_calloc(int, 1);
		*fd = c_sockfd;
		ret = pthread_create(t, &attr, client_thread, (void *)fd);
		if (ret != 0) {
			nv_perror(LOG_ERROR, "pthread_create", ret);
			stat = -1;
			nv_free(t);
		}
	}

cleanup:
	return stat;
}

#define MSG_100		"100 netvizd v0.1 Copyright (c) Robert Timothy Stewart\r\n"
#define MSG_101		"101 proto_1.0\r\n"
#define MSG_102		"102 Goodbye.\r\n"
#define MSG_103		"103 %i %f %f %f\r\n"
#define MSG_104		"104 FETCH command complete.\r\n"

#define MSG_200		"200 Invalid request.\r\n"

#define WORD_FETCH		"fetch"
#define WORD_QUIT		"quit"
#define WORD_EXIT		"exit"

#define invalid_query(fd)	writen((fd), MSG_200, strlen(MSG_200))

void *client_thread(void *arg) {
	int *fd = NULL;
	int c = 0;
	char buf[BUF_LEN];
	char sep[] = " \t\r\n";
	char *word = NULL;
	char *brk = NULL;
	int skip = 0;
	int skipt = 0;
	int skip_cnt = 0;
	double tally = 0;
	time_t midtime = 0;

	/* get our file desciptor */
	fd = (int *)arg;

	/* send intro msg and protocol version */
	writen(*fd, MSG_100, strlen(MSG_100));
	writen(*fd, MSG_101, strlen(MSG_101));

	/* main loop */
	for (;;) {
		int i = 0;

		/* read a line of input and check for disconnect */
		c = readline(*fd, buf, BUF_LEN-1);
		buf[BUF_LEN-1] = '\0';
		if (c == 0) break;

		/* convert everything to lowercase */
		for (i = 0; i < c; i++) {
			buf[i] = tolower(buf[i]);
		}

		/* parse incoming request */
		word = strtok_r(buf, sep, &brk);
		if (word == NULL) continue;

		if (strncmp(word, WORD_QUIT, strlen(WORD_QUIT)) == 0 ||
			strncmp(word, WORD_EXIT, strlen(WORD_EXIT)) == 0) {
			/* they want to leave :( */
			writen(*fd, MSG_102, strlen(MSG_102));
			break;
		} else if (strncmp(word, WORD_FETCH, strlen(WORD_FETCH)) == 0) {
			char *system = NULL;
			char *dsname = NULL;
			time_t start;
			time_t end;
			int res;
			nv_list *result;
			nv_node i;
			nv_node t;
			struct nv_dsts *dset = NULL;
			
			/* we have a fetch request... format:
			 *     fetch <system> <dataset> <start> <end> <resolution> */
			system = strtok_r(NULL, sep, &brk);
			if (system == NULL) {
				invalid_query(*fd);
				continue;
			}
			dsname = strtok_r(NULL, sep, &brk);
			if (dsname == NULL) {
				invalid_query(*fd);
				continue;
			}
			word = strtok_r(NULL, sep, &brk);
			if (word == NULL) {
				invalid_query(*fd);
				continue;
			}
			start = atoi(word);
			word = strtok_r(NULL, sep, &brk);
			if (word == NULL) {
				invalid_query(*fd);
				continue;
			}
			end = atoi(word);
			word = strtok_r(NULL, sep, &brk);
			if (word == NULL) {
				invalid_query(*fd);
				continue;
			}
			res = atoi(word);
			word = strtok_r(NULL, sep, &brk);
			if (word != NULL) {
				invalid_query(*fd);
				continue;
			}

			/* find the dataset */
			list_for_each(i, &nv_dsts_list) {
				struct nv_dsts *d = NULL;
				d = node_data(struct nv_dsts, i);
				if (strcmp(d->name, dsname) == 0 &&
					strcmp(d->sys->name, system) == 0) {
					dset = d;
					break;
				}
			}
			if (dset == NULL) {
				invalid_query(*fd);
				continue;
			}

			/* pull the data and figure out how to achieve given
			 * resolution */
			result = stor_get_ts_data(dset, start, end, res);
			if (result->next != NULL && result->next->next != result) {
				/* OK, we have at least 2 elements */
				int diff = 0;
				struct nv_ts_data *d1 = NULL;
				struct nv_ts_data *d2 = NULL;
				
				/* find time offset between elements in seconds */
				d1 = node_data(struct nv_ts_data, result->next);
				d2 = node_data(struct nv_ts_data, result->next->next);
				diff = d2->time-d1->time;
				
				/* find the frequency of rows to accept (skip):
				 *     1 = take every row
				 *     2 = take every other row
				 *     3 = take every third row
				 * etc.  Note that this is an estimate - when inexact, you
				 * will get the next highest resolution possible, given
				 * the colleted data.  Finally, get skipt, which is half of
				 * skip - this is the time value we pick for submission */
				if (res*60 <= diff) skip = 1;
				else skip = res*60/diff;
				nv_log(LOG_DEBUG, "Calculated skip of %i", skip);
				if (skip == 1) skipt = 1;
				else if (skip == 2) skipt = 1;
				else skipt = skip/2+1;
			}

			/* send data to the client */
			t = NULL;
			skip_cnt = 1;
			list_for_each(i, result) {
				struct nv_ts_data *d = NULL;
				d = node_data(struct nv_ts_data, i);

				/* add this value to tally */
				tally += d->value;

				/* do we report this time? */
				if (skip_cnt == skipt) {
					midtime = d->time;
				}
				
				/* is it time to report? */
				if (skip_cnt == skip) {
					snprintf(buf, BUF_LEN, MSG_103, midtime, tally,
							 d->min, d->max);
					writen(*fd, buf, strlen(buf));
					skip_cnt = 1;
					tally = 0.0L;
				} else {
					skip_cnt++;
				}

				/* clean this entry and previous node */
				nv_free(d);
				if (t != NULL) list_del(i->prev);
				t = i;
			}
			list_del(t);
			nv_free(result);
			writen(*fd, MSG_104, strlen(MSG_104));
		} else {
			invalid_query(*fd);
		}
	}

	close(*fd);
	nv_free(arg);

	return (void *)NULL;
}

