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
extern struct storage_p *storage_ps;

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


//static void nv_enum_plugins(char *path, void *(*func)(char *, char *));
//static void *nv_config_init(char *path, char *name);
//static void *nv_storage_init(char *path, char *name);
//static void *nv_sensor_init(char *path, char *name);
//static void *nv_proto_init(char *path, char *name);
//
//void nv_plugins_init() {
//	int ret = 0;
//	
//	/* init the plugin system */
//	ret = lt_dlinit();
//	if (ret) {
//		nv_log(LOG_ERROR, "lt_dlinit(): %s", lt_dlerror());
//		goto done;
//	}
//
//	/* load the plugins */
//	nv_log(LOG_DEBUG, "loading plugins from %s", PLUGIN_PATH);
//	nv_log(LOG_INFO, "loading config plugins");
//	nv_enum_plugins(CONFIG_PATH, nv_config_init);
//	nv_log(LOG_INFO, "loading storage plugins");
//	nv_enum_plugins(STORAGE_PATH, nv_storage_init);
//	nv_log(LOG_INFO, "loading sensor plugins");
//	nv_enum_plugins(SENSOR_PATH, nv_sensor_init);
//	nv_log(LOG_INFO, "loading proto plugins");
//	nv_enum_plugins(PROTO_PATH, nv_proto_init);
//
//done:
//	return;
//}
//
//void nv_plugins_exit() {
//	int ret = 0;
//
//	/* close down the plugin system */
//	ret = lt_dlexit();
//	if (ret) {
//		nv_log(LOG_ERROR, "lt_dlexit(): %s", lt_dlerror());
//	}
//}
//
//void nv_enum_plugins(char *path, void *(*func)(char *, char *)) {
//	int ret = 0;
//	DIR *dir = NULL;;
//	struct dirent *entry;
//	char *object = NULL;
//	struct stat statbuf;
//	
//	/* enumerate plugins in given directory and call given func */
//	dir = opendir(path);
//	if (dir == NULL) {
//		nv_perror(LOG_ERROR, path, errno);
//	} else {
//		for (;;) {
//			/* zero errno because readdir() because we need it to detect
//			   errors */
//			errno = 0;
//			entry = readdir(dir);
//			if (entry == NULL) {
//				if (errno) {
//					nv_perror(LOG_ERROR, "readdir()", errno);
//				}
//				break;
//			} else {
//				/* do some sanity checks */
//				int len = strlen(entry->d_name);
//				if (len < 4) continue;
//				if (strncmp(".la", entry->d_name+len-3, 3)) continue;
//
//				/* are we a real file or a symlink? */
//				object = nv_calloc(char, strlen(path)+len+2);
//				snprintf(object, strlen(path)+len+2, "%s/%s", path,
//						 entry->d_name);
//				ret = lstat(object, &statbuf);
//				nv_free(object);
//				if (ret) {
//					nv_perror(LOG_ERROR, "lstat()", errno);
//					continue;
//				}
//				if (!S_ISREG(statbuf.st_mode) && !S_ISLNK(statbuf.st_mode))
//					continue;
//
//				/* register the plugin */
//				func(path, entry->d_name);
//			}
//		}
//	}
//}
//
//void *nv_config_init(char *path, char *name) {
//	int len = 0;
//	char *object = NULL;
//	
//	nv_log(LOG_INFO, "new config plugin: %s", name);
//	len = strlen(name);
//	object = nv_calloc(char, strlen(path)+len+2);
//	snprintf(object, strlen(path)+len+2, "%s/%s", path, name);
//
//	/* register the plugin */
//	
//
//	nv_free(object);
//}
//
//void *nv_storage_init(char *path, char *name) {
//	int len = 0;
//	char *object = NULL;
//	
//	nv_log(LOG_INFO, "new storage plugin: %s", name);
//	len = strlen(name);
//	object = nv_calloc(char, strlen(path)+len+2);
//	snprintf(object, strlen(path)+len+2, "%s/%s", path, name);
//
//	/* register the plugin */
//	
//
//	nv_free(object);
//}
//
//void *nv_sensor_init(char *path, char *name) {
//	int len = 0;
//	char *object = NULL;
//	
//	nv_log(LOG_INFO, "new sensor plugin: %s", name);
//	len = strlen(name);
//	object = nv_calloc(char, strlen(path)+len+2);
//	snprintf(object, strlen(path)+len+2, "%s/%s", path, name);
//
//	/* register the plugin */
//	
//
//	nv_free(object);
//}
//
//void *nv_proto_init(char *path, char *name) {
//	int len = 0;
//	char *object = NULL;
//	
//	nv_log(LOG_INFO, "new protocol plugin: %s", name);
//	len = strlen(name);
//	object = nv_calloc(char, strlen(path)+len+2);
//	snprintf(object, strlen(path)+len+2, "%s/%s", path, name);
//
//	/* register the plugin */
//	
//
//	nv_free(object);
//}

/* vim: set ts=4 sw=4: */
