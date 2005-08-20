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
#include <plugin.h>
#include <ltdl.h>
#include <sys/stat.h>
#include <dirent.h>


#define PLUGIN_PATH			PKGLIBDIR "/plugins"
#define CONFIG_PATH			PLUGIN_PATH "/config"
#define STORAGE_PATH		PLUGIN_PATH "/storage"
#define SENSOR_PATH			PLUGIN_PATH "/sensor"
#define PROTO_PATH			PLUGIN_PATH "/proto"
#define AUTH_PATH			PLUGIN_PATH "/auth"

#define CONFIG_INIT			"config_init"
#define STORAGE_INIT		"storage_init"
#define SENSOR_INIT			"sensor_init"
#define PROTO_INIT			"proto_init"
#define AUTH_INIT			"auth_init"

int nv_plugins_init() {
	int ret = 0;

	/* init the plugin system */
	LTDL_SET_PRELOADED_SYMBOLS();
	ret = lt_dlinit();
	if (ret) {
		nv_log(NVLOG_ERROR, "lt_dlinit(): %s", lt_dlerror());
		return -1;
	}
	return ret;
}

int nv_plugins_free() {
	int ret = 0;

	/* close down the plugin system */
	ret = lt_dlexit();
	if (ret) {
		nv_log(NVLOG_ERROR, "lt_dlexit(): %s", lt_dlerror());
		return -1;
	}
	return ret;
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
	int (*config_init)(struct nv_config_p *);
	int stat = 0;
	int ret = 0;

	/* get reference to config plugin file */
	nv_log(NVLOG_INFO, "loading config plugin from %s", file);
	len = strlen(CONFIG_PATH)+strlen(file)+2;
	buf = nv_calloc(char, len);
	snprintf(buf, len, "%s/%s", CONFIG_PATH, file);
	module = lt_dlopen(buf);
	nv_free(buf);
	if (module == NULL) {
		nv_log(NVLOG_ERROR, "lt_dlopen(): %s", lt_dlerror());
		stat = -1;
		goto cleanup;
	}

	/* allocate memory for config plugin struct if necessary */
	if (nv_config_p == NULL) {
		nv_config_p = nv_calloc(struct nv_config_p, 1);
	}

	/* get reference to init function and run it */
	*(void **)(&config_init) = lt_dlsym(module, CONFIG_INIT);
	if (config_init == NULL) {
		nv_log(NVLOG_ERROR, "lt_dlsym(): %s", lt_dlerror());
		stat = -1;
		goto error1;
	}
	ret = config_init(nv_config_p);
	if (ret != 0) {
		stat = ret;
	}

	/*
	 * Cleanup Code
	 */
	goto cleanup;

error1:
	nv_free(nv_config_p);

cleanup:
	return stat;
}

/*
 * Register our storage plugins
 */
int stor_p_init() {
	nv_node i;
	int len = 0;
	int stat = 0;
	int ret = 0;
	char *buf = NULL;
	lt_dlhandle module = NULL;
	int (*storage_init)(struct nv_stor_p *);

	list_for_each(i, &nv_stor_p_list) {
		struct nv_stor_p *p = node_data(struct nv_stor_p, i);

		/* find our shared object */
		nv_log(NVLOG_INFO, "loading storage plugin from %s", p->file);
		len = strlen(STORAGE_PATH)+strlen(p->file)+2;
		buf = nv_calloc(char, len);
		snprintf(buf, len, "%s/%s", STORAGE_PATH, p->file);
		module = lt_dlopen(buf);
		nv_free(buf);
		if (module == NULL) {
			nv_log(NVLOG_ERROR, "lt_dlopen(): %s", lt_dlerror());
			stat = -1;
			continue;
		}

		/* get reference to init function and run it */
		*(void **)(&storage_init) = lt_dlsym(module, STORAGE_INIT);
		if (storage_init == NULL) {
			nv_log(NVLOG_ERROR, "lt_dlsym(): %s", lt_dlerror());
			stat = -1;
			continue;
		}
		ret = storage_init(p);
		if (ret != 0) {
			stat = ret;
			continue;
		}
	}

	return stat;
}

/*
 * Register our sensor plugins
 */
int sens_p_init() {
	nv_node i;
	int len = 0;
	int stat = 0;
	int ret = 0;
	char *buf = NULL;
	lt_dlhandle module = NULL;
	int (*sensor_init)(struct nv_sens_p *);

	list_for_each(i, &nv_sens_p_list) {
		struct nv_sens_p *p = node_data(struct nv_sens_p, i);

		/* find our shared object */
		nv_log(NVLOG_INFO, "loading sensor plugin from %s", p->file);
		len = strlen(SENSOR_PATH)+strlen(p->file)+2;
		buf = nv_calloc(char, len);
		snprintf(buf, len, "%s/%s", SENSOR_PATH, p->file);
		module = lt_dlopen(buf);
		nv_free(buf);
		if (module == NULL) {
			nv_log(NVLOG_ERROR, "lt_dlopen(): %s", lt_dlerror());
			stat = -1;
			continue;
		}

		/* get reference to init function and run it */
		*(void **)(&sensor_init) = lt_dlsym(module, SENSOR_INIT);
		if (sensor_init == NULL) {
			nv_log(NVLOG_ERROR, "lt_dlsym(): %s", lt_dlerror());
			stat = -1;
			continue;
		}
		ret = sensor_init(p);
		if (ret != 0) {
			stat = ret;
			continue;
		}
	}

	return stat;
}

/*
 * Register our protocol plugins
 */
int proto_p_init() {
	nv_node i;
	int len = 0;
	int stat = 0;
	int ret = 0;
	char *buf = NULL;
	lt_dlhandle module = NULL;
	int (*proto_init)(struct nv_proto_p *);

	list_for_each(i, &nv_proto_p_list) {
		struct nv_proto_p *p = node_data(struct nv_proto_p, i);

		/* find our shared object */
		nv_log(NVLOG_INFO, "loading proto plugin from %s", p->file);
		len = strlen(PROTO_PATH)+strlen(p->file)+2;
		buf = nv_calloc(char, len);
		snprintf(buf, len, "%s/%s", PROTO_PATH, p->file);
		module = lt_dlopen(buf);
		nv_free(buf);
		if (module == NULL) {
			nv_log(NVLOG_ERROR, "lt_dlopen(): %s", lt_dlerror());
			stat = -1;
			continue;
		}

		/* get reference to init function and run it */
		*(void **)(&proto_init) = lt_dlsym(module, PROTO_INIT);
		if (proto_init == NULL) {
			nv_log(NVLOG_ERROR, "lt_dlsym(): %s", lt_dlerror());
			stat = -1;
			continue;
		}
		ret = proto_init(p);
		if (ret != 0) {
			stat = ret;
			continue;
		}
	}

	return stat;
}

/*
 * Regsiter our authentication plugins
 */
int auth_p_init() {
	nv_node i;
	int len = 0;
	int stat = 0;
	int ret = 0;
	char *buf = NULL;
	lt_dlhandle module = NULL;
	int (*auth_init)(struct nv_auth_p *);

	list_for_each(i, &nv_auth_p_list) {
		struct nv_auth_p *p = node_data(struct nv_auth_p, i);

		/* find our shared object */
		nv_log(NVLOG_INFO, "loading auth plugin from %s", p->file);
		len = strlen(AUTH_PATH)+strlen(p->file)+2;
		buf = nv_calloc(char, len);
		snprintf(buf, len, "%s/%s", AUTH_PATH, p->file);
		module = lt_dlopen(buf);
		nv_free(buf);
		if (module == NULL) {
			nv_log(NVLOG_ERROR, "lt_dlopen(): %s", lt_dlerror());
			stat = -1;
			continue;
		}

		/* get reference to init function and run it */
		*(void **)(&auth_init) = lt_dlsym(module, AUTH_INIT);
		if (auth_init == NULL) {
			nv_log(NVLOG_ERROR, "lt_dlsym(): %s", lt_dlerror());
			stat = -1;
			continue;
		}
		ret = auth_init(p);
		if (ret != 0) {
			stat = ret;
			continue;
		}
	}

	return stat;
}

/* vim: set ts=4 sw=4: */
