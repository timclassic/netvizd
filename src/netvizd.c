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
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

/*
 * Global debug_mode
 */
int debug_mode = 0;

/*
 * Global plugin lists
 */
struct config_p *config_p = NULL;
struct storage_p *storage_ps = NULL;

int main(int argc, char *argv[]) {
	int c;
	int digit_optind = 0;
	char config_name[NAME_LEN];

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

		c = getopt_long(argc, argv, "d", long_options, &option_index);
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
	nv_plugins_init();

	nv_config_init(config_name);


	

	nv_plugins_free();
	nv_log(LOG_INFO, "netvizd " VERSION " shutdown complete");
	return EXIT_SUCCESS;
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
		snprintf(buf, len, LOG_DEBUG_MSG, message);
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
