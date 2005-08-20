/***************************************************************************
 *   Copyright (C) 2004 by Tim Stewart                                     *
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

/***************************************************************************
 * Portions of this code are taken from W. Richard Stevens' UNIX Network   *
 * Programming, Vol. 1.                                                    *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <netvizd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

static pthread_key_t rl_key;
static pthread_once_t rl_once = PTHREAD_ONCE_INIT;

typedef struct {
	int   rl_cnt;           /* initialize to 0 */
	char *rl_bufptr;        /* initialize to rl_buf */
	char  rl_buf[BUF_LEN];
} rline_t;

static void readline_destructor(void *ptr);
static void readline_once(void);
static ssize_t my_read(rline_t *tsd, int fd, char *ptr);

/* read n bytes from a descriptor */
ssize_t readn(int fd, void *vptr, size_t n) {
	size_t nleft;
	size_t nread;
	char *ptr;
	
	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ((nread = read(fd, ptr, nleft)) < 0) {
			if (errno == EINTR)
				nread = 0;        /* and call read() again */
			else
				return -1;
		} else if (nread == 0)
			break;                /* EOF */
		
		nleft -= nread;
		ptr +=nread;
	}
	return (n - nleft);
}

/* write n bytes to a descriptor */
ssize_t writen(int fd, const void *vptr, size_t n) {
	size_t nleft;
	ssize_t nwritten;
	const char *ptr;
	
	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ((nwritten = write(fd, ptr, nleft)) <= 0) {
			if (errno == EINTR)
				nwritten = 0;    /* and call write() again */
			else
				return -1;       /* error */
		}
		nleft -= nwritten;
		ptr += nwritten;
	}
	return n;
}

/* read a text line from a descriptor, 1 byte at a time */
ssize_t readline(int fd, void *vptr, size_t maxlen) {
	ssize_t  n, rc;
	char     c, *ptr;
	rline_t *tsd;
	int      s;
	
	s = pthread_once(&rl_once, readline_once);
	if (s != 0) {
		nv_log(NVLOG_ERROR, "pthread_once: %s", strerror(s));
		n = s;
		goto cleanup;
	}
	if ((tsd = pthread_getspecific(rl_key)) == NULL) {
		tsd = nv_calloc(rline_t, 1);
		s = pthread_setspecific(rl_key, tsd);
		if (s != 0) {
			nv_log(NVLOG_ERROR, "pthread_setspecific: %s", strerror(s));
			n = s;
			goto cleanup;
		}
	}
	ptr = vptr;
	for (n = 1; n < maxlen; n++) {
again:
		if ((rc = my_read(tsd, fd, &c)) == 1) {
			*ptr++ = c;
			if (c == '\n')
				break;          /* newline is stored, like fgets() */
		} else if (rc == 0) {
			if (n == 1)
				return 0;       /* EOF, no data read */
			else
				break;          /* EOF, some data read */
		} else {
			if (errno == EINTR)
				goto again;
			return -1;          /* error, errno set by read() */
		}
	}
	
cleanup:
	*ptr = 0;
	return n;
}

static void readline_destructor(void *ptr) {
	free(ptr);
}

static void readline_once(void) {
	int s = 0;
	s = pthread_key_create(&rl_key, readline_destructor);
	if (s != 0) {
		nv_log(NVLOG_ERROR, "pthread_key_create: %s", strerror(s));
	}
}

static ssize_t my_read(rline_t *tsd, int fd, char *ptr) {
	if (tsd->rl_cnt <= 0) {
again:
		if ((tsd->rl_cnt = read(fd, tsd->rl_buf, BUF_LEN)) < 0) {
			if (errno == EINTR)
				goto again;
			return -1;
		} else if (tsd->rl_cnt == 0)
			return 0;
		tsd->rl_bufptr = tsd->rl_buf;
	}
	tsd->rl_cnt--;
	*ptr = *tsd->rl_bufptr++;
	return 1;
}
