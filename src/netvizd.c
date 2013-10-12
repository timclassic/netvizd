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

#define NV_GLOBAL_MAIN

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <netvizd.h>
#include <plugin.h>
#include <nvconfig.h>
#include <nvlist.h>
#include <sensor.h>
#include <storage.h>
#include <proto.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <syslog.h>

static void print_usage(FILE *fp, char *cmd);

/*
 * Global debug_mode
 */
int debug_mode = 0;

static int log_stdout = 0;
static int forked = 0;

void print_usage(FILE *fp, char *cmd) {
	fprintf(fp, "Usage: %s [options]\n"
				"Options:\n"
				"  -c PLUGIN, --config=PLUGIN   Load a specific config plugin, default is 'file'.\n"
				"  -d, --debug                  Output debug information.\n"
				"  -f, --foreground             Run in foreground, do not fork.\n"
				"  -h, --help                   Display usage information.\n"
				"  -s, --stdout                 Log to stdout/stderr instead of using syslog().\n",
			cmd);
}

int main(int argc, char *argv[]) {
	int c = 0;
	int digit_optind = 0;
	char config_name[NAME_LEN];
	int stat = EXIT_SUCCESS;
	nv_node i;
	pthread_attr_t attr;
	sigset_t newmask, oldmask;
	int foreground = 0;
	pid_t pid = 0;

	/* set option defaults */
	strncpy(config_name, "file.la", NAME_LEN);
	config_name[NAME_LEN-1] = '\0';
	
	/* do syslog initialization */
	openlog("netvizd", 0, LOG_LOCAL1);

	/* do cmd line processing */
	for (;;) {
		int this_option_optind = optind ? optind : 1;
		int option_index = 0;
		static struct option long_options[] = {
			{"config", 1, 0, 'c'},
			{"debug", 0, 0, 'd'},
			{"foreground", 0, 0, 'f'},
			{"stdout", 0, 0, 's'},
			{"help", 0, 0, 'h'},
			{0, 0, 0, 0}
		};

		c = getopt_long(argc, argv, "c:dfsh", long_options, &option_index);
		if (c == -1) break;

		switch (c) {
			case 'c':
				strncpy(config_name, optarg, NAME_LEN);
				config_name[NAME_LEN-1] = '\0';
				break;
			
			case 'd':
				debug_mode = 1;
				break;
				
			case 'f':
				foreground = 1;
				break;
				
			case 's':
				log_stdout = 1;
				break;
				
			case 'h':
				print_usage(stdout, argv[0]);
				exit(EXIT_SUCCESS);

			case '?':
				break;

			default:
				print_usage(stderr, argv[0]);
				exit(EXIT_FAILURE);
				break;
		};
	};
	
	nv_log(NVLOG_INFO, "netvizd " VERSION " coming up");
	
    /* Block SIGPIPE for the daemon - if unblocked, some sockets will cause
	 * a SIGPIPE to be delivered, screwing things up for us. */
    sigemptyset(&newmask);
    sigaddset(&newmask, SIGPIPE);
    if (sigprocmask(SIG_BLOCK, &newmask, &oldmask) < 0) {
		nv_perror(NVLOG_ERROR, "sigaddset()", errno);
		stat = EXIT_FAILURE;
		goto cleanup;
    }
	
	/* set up plugin system */
	if (nv_plugins_init() != 0) {
		nv_log(NVLOG_ERROR, "plugin initialization failed, aborting");
		stat = EXIT_FAILURE;
		goto cleanup;
	}

	/* load our configuration from supplied plugin */
	if (nv_config_init(config_name) != 0) {
		nv_log(NVLOG_ERROR, "configuration plugin initialization failed, "
			   "aborting");
		stat = EXIT_FAILURE;
		goto cleanup;
	}

	/* load the rest of the plugins as per our configuration */
	if (stor_p_init() != 0) {
		nv_log(NVLOG_ERROR, "storage plugin initialization failed, aborting");
		stat = EXIT_FAILURE;
		goto cleanup;
	}
	if (sens_p_init() != 0) {
		nv_log(NVLOG_ERROR, "sensor plugin initialization failed, aborting");
		stat = EXIT_FAILURE;
		goto cleanup;
	}
	if (proto_p_init() != 0) {
		nv_log(NVLOG_ERROR, "protocol plugin initialization failed, aborting");
		stat = EXIT_FAILURE;
		goto cleanup;
	}
	if (auth_p_init() != 0) {
		nv_log(NVLOG_ERROR, "authentication plugin initialization failed, "
			   "aborting");
		stat = EXIT_FAILURE;
		goto cleanup;
	}

	/* init storage instances */
	list_for_each(i, &nv_stor_list) {
		struct nv_stor *s = node_data(struct nv_stor, i);
		if (s->plug->inst_init == NULL) continue;
		if (s->plug->inst_init(s) != 0) {
			nv_log(NVLOG_ERROR, "storage instance initialization failed, "
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
			nv_log(NVLOG_ERROR, "sensor instance initialization failed, "
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
			nv_perror(NVLOG_ERROR, "pthread_create()", ret);
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
			nv_perror(NVLOG_ERROR, "pthread_create()", ret);
			stat = EXIT_FAILURE;
			goto cleanup;
		}
	}

	/* start up our protocol threads
	 * TODO This needs to be changed to support instances of a protocol. */
	list_for_each(i, &nv_proto_p_list) {
		int ret = 0;
		struct nv_proto_p *p = node_data(struct nv_proto_p, i);

		if (p->listen == NULL) continue;
		p->thread = nv_calloc(pthread_t, 1);
		ret = pthread_create(p->thread, &attr, proto_thread, p);
		if (ret != 0) {
			nv_perror(NVLOG_ERROR, "pthread_create()", ret);
			stat = EXIT_FAILURE;
			goto cleanup;
		}
	}

	/* do some thread startup cleanup */
	pthread_attr_destroy(&attr);
	
	/* fork a new process if not in foreground mode */
	if (!foreground) {
		pid = fork();
		if (pid < 0) {
			nv_perror(NVLOG_ERROR, "fork()", errno);
			exit(EXIT_FAILURE);
		} else if (pid > 0)	{
			exit(EXIT_SUCCESS);
		}
		forked = 1;
	}

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
	nv_log(NVLOG_INFO, "netvizd " VERSION " shutdown complete");
	closelog();
	return stat;
}

/*
 * Wrapper for malloc()
 */
void *_nv_malloc(size_t num) {
	void *new = (void *)malloc(num);
	if (!new) {
		nv_perror(NVLOG_ERROR, "malloc()", errno);
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
		nv_perror(NVLOG_ERROR, "realloc()", errno);
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
	va_list argv;
	char *buf = NULL;
	int len = 0;
	int pri = LOG_INFO;

	/* format message string */
	len = BUF_LEN + strlen(message);
	buf = nv_calloc(char, len+1);
	va_start(argv, message);
	switch (type) {
		case NVLOG_DEBUG:
			if (debug_mode) snprintf(buf, len, NVLOGD_DEBUG_MSG, message);
			pri = LOG_DEBUG;
			break;
			
		case NVLOG_INFO:
			if (debug_mode) snprintf(buf, len, NVLOGD_INFO_MSG, message);
			else snprintf(buf, len, NVLOG_INFO_MSG, message);
			pri = LOG_INFO;
			break;

		case NVLOG_WARN:
			if (debug_mode) snprintf(buf, len, NVLOGD_WARN_MSG, message);
			else snprintf(buf, len, NVLOG_WARN_MSG, message);			
			pri = LOG_WARNING;
			break;
			
		case NVLOG_ERROR:
			if (debug_mode) snprintf(buf, len, NVLOGD_ERROR_MSG, message);
			else snprintf(buf, len, NVLOG_ERROR_MSG, message);
			pri = LOG_ERR;
			break;
	}
	
	/* output message */
	if (log_stdout) {
		buf[strlen(buf)] = '\n';
		buf[strlen(buf)+1] = '\0';
		/* log to standard out */
		if (type == NVLOG_ERROR) {
			vfprintf(stderr, buf, argv);
			fflush(stderr);
		} else if (debug_mode) {
			vprintf(buf, argv);
			fflush(stdout);
		} else if (type != NVLOG_DEBUG) {
			vprintf(buf, argv);
			fflush(stdout);
		}
	} else {
		/* log to syslog - note that we also log NVLOG_ERROR messages to the
		 * console if we haven't forked yet */
		if (debug_mode) vsyslog(pri, buf, argv);
		else if (type != NVLOG_DEBUG) vsyslog(pri, buf, argv);
		if (type == NVLOG_ERROR && !forked) {
			buf[strlen(buf)] = '\n';
			buf[strlen(buf)+1] = '\0';
			vfprintf(stderr, buf, argv);
			fflush(stderr);
		}
	}
	
	va_end(argv);
	nv_free(buf);
}

void _nv_perror_r(log_type_t type, char *message, int error, char *file, int line) {
	char buf[BUF_LEN];
	
	strerror_r(error, buf, BUF_LEN);
	_nv_log(type, "%s: %s", file, line, message, buf);
}

/* vim: set ts=4 sw=4: */
