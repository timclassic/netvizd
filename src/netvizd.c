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
#include <plugin.h>
#include <nvconfig.h>
#include <nvlist.h>
#include <sensor.h>
#include <storage.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <pthread.h>

/*
 * Global debug_mode
 */
int debug_mode = 0;

int main(int argc, char *argv[]) {
	int c = 0;
	int digit_optind = 0;
	char config_name[NAME_LEN];
	int stat = EXIT_SUCCESS;
	nv_node i;
	pthread_attr_t attr;

	strncpy(config_name, "file.la", NAME_LEN);
	config_name[NAME_LEN-1] = '\0';

	/* do cmd line processing */
	for (;;) {
		int this_option_optind = optind ? optind : 1;
		int option_index = 0;
		static struct option long_options[] = {
			{"config", 1, 0, 'c'},
			{"debug", 0, 0, 'd'},
			{0, 0, 0, 0}
		};

		c = getopt_long(argc, argv, "c:d", long_options, &option_index);
		if (c == -1) break;

		switch (c) {
			case 'c':
				strncpy(config_name, optarg, NAME_LEN);
				config_name[NAME_LEN-1] = '\0';
				break;
			
			case 'd':
				debug_mode = 1;
				break;

			case '?':
				break;

			default:
				fprintf(stderr, "getopt returned unknown character code\n");
				break;
		};
	};

	nv_log(LOG_INFO, "netvizd " VERSION " coming up");
	
	/* set up plugin system */
	if (nv_plugins_init() != 0) {
		nv_log(LOG_ERROR, "plugin initialization failed, aborting");
		stat = EXIT_FAILURE;
		goto cleanup;
	}

	/* load our configuration from supplied plugin */
	if (nv_config_init(config_name) != 0) {
		nv_log(LOG_ERROR, "configuration plugin initialization failed, "
			   "aborting");
		stat = EXIT_FAILURE;
		goto cleanup;
	}

	/* load the rest of the plugins as per our configuration */
	if (stor_p_init() != 0) {
		nv_log(LOG_ERROR, "storage plugin initialization failed, aborting");
		stat = EXIT_FAILURE;
		goto cleanup;
	}
	if (sens_p_init() != 0) {
		nv_log(LOG_ERROR, "sensor plugin initialization failed, aborting");
		stat = EXIT_FAILURE;
		goto cleanup;
	}
	if (proto_p_init() != 0) {
		nv_log(LOG_ERROR, "protocol plugin initialization failed, aborting");
		stat = EXIT_FAILURE;
		goto cleanup;
	}
	if (auth_p_init() != 0) {
		nv_log(LOG_ERROR, "authentication plugin initialization failed, "
			   "aborting");
		stat = EXIT_FAILURE;
		goto cleanup;
	}

	/* init storage instances */
	list_for_each(i, &nv_stor_list) {
		struct nv_stor *s = node_data(struct nv_stor, i);
		if (s->plug->inst_init == NULL) continue;
		if (s->plug->inst_init(s) != 0) {
			nv_log(LOG_ERROR, "storage instance initialization failed, "
				   "aborting");
			stat = EXIT_FAILURE;
			goto cleanup;
		}
	}

	/* init sensor instances */
	list_for_each(i, &nv_sens_list) {
		struct nv_sens *s = node_data(struct nv_sens, i);
		if (s->plug->inst_init == NULL) continue;
		if (s->plug->inst_init(s) != 0) {
			nv_log(LOG_ERROR, "sensor instance initialization failed, "
				   "aborting");
			stat = EXIT_FAILURE;
			goto cleanup;
		}
	}

	/* setup pthreads */
	pthread_attr_init(&attr);
	
	/* start up storage instance threads */
	list_for_each(i, &nv_stor_list) {
		int ret = 0;
		struct nv_stor *s = node_data(struct nv_stor, i);

		if (s->beat == 0 && s->beatfunc == NULL) continue;
		s->thread = nv_calloc(pthread_t, 1);
		ret = pthread_create(s->thread, &attr, stor_thread, s);
		if (ret != 0) {
			nv_perror(LOG_ERROR, "pthread_create()", ret);
			stat = EXIT_FAILURE;
			goto cleanup;
		}
	}

	/* start up sensor instance threads */
	list_for_each(i, &nv_sens_list) {
		int ret = 0;
		struct nv_sens *s = node_data(struct nv_sens, i);

		if (s->beat == 0 && s->beatfunc == NULL) continue;
		s->thread = nv_calloc(pthread_t, 1);
		ret = pthread_create(s->thread, &attr, sens_thread, s);
		if (ret != 0) {
			nv_perror(LOG_ERROR, "pthread_create()", ret);
			stat = EXIT_FAILURE;
			goto cleanup;
		}
	}

	/* do some thread startup cleanup */
	pthread_attr_destroy(&attr);

	/* wait on all the threads */
	list_for_each(i, &nv_stor_list) {
		struct nv_stor *s = node_data(struct nv_stor, i);
		if (s->beat == 0 && s->beatfunc == NULL) continue;
		pthread_join(*s->thread, NULL);
	}
	list_for_each(i, &nv_sens_list) {
		struct nv_sens *s = node_data(struct nv_sens, i);
		if (s->beat == 0 && s->beatfunc == NULL) continue;
		pthread_join(*s->thread, NULL);
	}

	/* shut down */
cleanup:
	nv_plugins_free();
	nv_log(LOG_INFO, "netvizd " VERSION " shutdown complete");
	return stat;
}

/*
 * Wrapper for malloc()
 */
void *_nv_malloc(size_t num) {
	void *new = (void *)malloc(num);
	if (!new) {
		nv_perror(LOG_ERROR, "malloc()", errno);
		exit(MALLOC_EXIT);
	}
	return new;
}

/*
 * Wrapper for realloc()
 */
void *_nv_realloc(void *p, size_t num) {
	void *new;

	if (!p) return _nv_malloc(num);

	new = (void *)realloc(p, num);
	if (!new) {
		nv_perror(LOG_ERROR, "realloc()", errno);
		exit(MALLOC_EXIT);
	}
	return new;
}

/*
 * Wrapper for calloc()
 */
void *_nv_calloc(size_t num, size_t size) {
	void *new = _nv_malloc(num * size);
	bzero(new, num * size);
	return new;
}

/*
 * Logging
 */
void _nv_log(log_type_t type, char *message, ...) {
	va_list argv = NULL;
	char *buf = NULL;
	int len = 0;

	len = BUF_LEN + strlen(message);
	buf = nv_calloc(char, len);
	va_start(argv, message);
	switch (type) {
	case LOG_DEBUG:
		if (debug_mode) snprintf(buf, len, LOG_DEBUG_MSG, message);
		break;
		
	case LOG_INFO:
		snprintf(buf, len, LOG_INFO_MSG, message);
		break;

	case LOG_WARN:
		snprintf(buf, len, LOG_WARN_MSG, message);
		break;
		
	case LOG_ERROR:
		snprintf(buf, len, LOG_ERROR_MSG, message);
		break;
	}
	vprintf(buf, argv);
	fflush(stdout);
	va_end(argv);
	nv_free(buf);
}

void _nv_perror_r(log_type_t type, char *message, int error, char *file, int line) {
	char buf[BUF_LEN];
	
	strerror_r(error, buf, BUF_LEN);
	_nv_log(type, "%s: %s", file, line, message, buf);
}

/* vim: set ts=4 sw=4: */
