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

#ifndef _NETVIZD_H_
#define _NETVIZD_H_

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

/*
 * Version information
 */
#define VERSION "0.1"

/*
 * Exit code.
 */
#define PTHREAD_EXIT 98
#define MALLOC_EXIT 99

/*
 * Make headers compatible with C++
 */
#ifdef __cplusplus
#  define BEGIN_C_DECLS         extern "C" {
#  define END_C_DECLS           }
#else
#  define BEGIN_C_DECLS
#  define END_C_DECLS
#endif

/*
 * Prototypes for our overridden malloc() calls
 */
#define nv_calloc(type, num) \
((type *)_nv_calloc((num), sizeof(type)))
#define nv_malloc(type, num) \
((type *)_nv_malloc((num) * sizeof(type)))
#define nv_realloc(type, p, num) \
((type *)_nv_realloc((p), (num) * sizeof(type)))
#define nv_free(stale) do { \
    if (stale) { free(stale); (stale) = 0; } \
} while (0)

/*
 * Make mutexes and condition variables easy
 */
#define nv_lock(m) \
_nv_pth_status = pthread_mutex_lock(m); \
if (_nv_pth_status != 0) { \
	nv_log(LOG_ERROR, "pthread_mutex_lock()", _nv_pth_status); \
	exit(PTHREAD_EXIT); \
}
#define nv_unlock(m) \
_nv_pth_status = pthread_mutex_unlock(m); \
if (_nv_pth_status != 0) { \
	nv_log(LOG_ERROR, "pthread_mutex_unlock()", _nv_pth_status); \
	exit(PTHREAD_EXIT); \
}
#define nv_wait(c, m) \
_nv_pth_status = pthread_cond_wait(c, m); \
if (_nv_pth_status != 0) { \
	nv_log(LOG_ERROR, "pthread_cond_wait()", _nv_pth_status); \
	exit(PTHREAD_EXIT); \
}
#define nv_signal(c) \
_nv_pth_status = pthread_cond_signal(c); \
if (_nv_pth_status != 0) { \
	nv_log(LOG_ERROR, "pthread_cond_signal()", _nv_pth_status); \
	exit(PTHREAD_EXIT); \
}
#define nv_broadcast(c) \
_nv_pth_status = pthread_cond_broadcast(c); \
if (_nv_pth_status != 0) { \
	nv_log(LOG_ERROR, "pthread_cond_broadcast()", _nv_pth_status); \
		exit(PTHREAD_EXIT); \
}
static int _nv_pth_status;

/*
 * Temporary buffer length.
 */
#define BUF_LEN 1024

/*
 * Internal name length.
 */
#define NAME_LEN 256

/*
 * Time for various thread sleeps
 */
#define THREAD_SLEEP 100000

/*
 * Copy a string of length NAME_LEN safely.
 */
#define name_copy(d, s) \
	strncpy((d), (s), NAME_LEN); \
	d[NAME_LEN-1] = '\0'

/*
 * Logging templates
 */
#define LOG_DEBUG_MSG		"[debug|%%s:%%d]: %s\n"
#define LOG_INFO_MSG		" [info|%%s:%%d]: %s\n"
#define LOG_WARN_MSG		" [warn|%%s:%%d]: %s\n"
#define LOG_ERROR_MSG		"[error|%%s:%%d]: %s\n"
#define LOG_POSITION		__FILE__, __LINE__

BEGIN_C_DECLS;

/*
 * Logging interface
 */
typedef enum {
	LOG_DEBUG,
	LOG_INFO,
	LOG_WARN,
	LOG_ERROR
} log_type_t;

void _nv_log(log_type_t type, char *message, ...);
void _nv_perror(log_type_t type, char *message, int error, char *file,
				int line);
#define nv_log(type, message, ...) \
	_nv_log((type), (message), LOG_POSITION, ## __VA_ARGS__)
#define nv_perror(type, message, error) \
	_nv_log((type), "%s: %s", LOG_POSITION, (message), strerror(error));
#define nv_perror_r(type, message, error) \
	_nv_perror_r((type), (message), (error), LOG_POSITION)

/*
 * Public interface for malloc() calls
 */
extern void *_nv_calloc(size_t num, size_t size);
extern void *_nv_malloc(size_t num);
extern void *_nv_realloc(void *p, size_t num);

END_C_DECLS;

#endif
/* vim: set ts=4 sw=4: */
