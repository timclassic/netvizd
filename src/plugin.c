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
#include <ltdl.h>
#include <sys/stat.h>
#include <dirent.h>


#define PLUGIN_PATH			PKGLIBDIR "/plugins"
#define CONFIG_PATH			PLUGIN_PATH "/config"
#define STORAGE_PATH		PLUGIN_PATH "/storage"
#define SENSOR_PATH			PLUGIN_PATH "/sensor"
#define PROTO_PATH			PLUGIN_PATH "/proto"

#define CONFIG_INIT			"config_init"
#define STORAGE_INIT		"storage_init"
#define SENSOR_INIT			"sensor_init"
#define PROTO_INIT			"proto_init"

extern struct config_p *config_p;

int nv_plugins_init() {
	int ret = 0;

	/* init the plugin system */
	LTDL_SET_PRELOADED_SYMBOLS();
	ret = lt_dlinit();
	if (ret) {
		nv_log(LOG_ERROR, "lt_dlinit(): %s", lt_dlerror());
		return -1;
	}
}

int nv_plugins_free() {
	int ret = 0;

	/* close down the plugin system */
	ret = lt_dlexit();
	if (ret) {
		nv_log(LOG_ERROR, "lt_dlexit(): %s", lt_dlerror());
		return -1;
	}
}

/*
 * Load the configuration plugin from the given file.
 * If the configuration plugin structure is NULL then it is allocated.
 * This should normally be the case.
 */
int config_p_init(char *file) {
	char *buf = NULL;
	int len = 0;
	lt_dlhandle module = NULL;
	int (*config_init)(struct config_p *);
	int stat = 0;
	int ret = 0;

	/* get reference to config plugin file */
	nv_log(LOG_INFO, "loading config plugin from %s", file);
	len = strlen(CONFIG_PATH)+strlen(file)+2;
	buf = nv_calloc(char, len);
	snprintf(buf, len, "%s/%s", CONFIG_PATH, file);
	module = lt_dlopen(buf);
	if (module == NULL) {
		nv_log(LOG_ERROR, "lt_dlopen(): %s", lt_dlerror());
		stat = -1;
		goto cleanup;
	}

	/* allocate memory for config plugin struct if necessary */
	if (config_p == NULL) {
		config_p = nv_calloc(struct config_p, 1);
	}

	/* get reference to init function and run it */
	*(void **)(&config_init) = lt_dlsym(module, CONFIG_INIT);
	if (config_init == NULL) {
		nv_log(LOG_ERROR, "lt_dlsym(): %s", lt_dlerror());
		stat = -1;
		goto error1;
	}
	ret = config_init(config_p);

	/*
	 * Cleanup Code
	 */
	goto cleanup;

error1:
	nv_free(config_p);

cleanup:
	nv_free(buf);
	return stat;
}

/* vim: set ts=4 sw=4: */
