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

// SAMPLE LOOP CODE - USE LATER
//	/* find the plugin for this instance */
//	stat = -1;
//	list_for_each(n, &nv_stor_p_list) {
//		struct nv_stor_p *p;
//		p = node_data(struct nv_stor_p, n);
//		if (strncmp(p->name, s->p_name, NAME_LEN) == 0) {
//			stor->plug = p;
//			stat = 0;
//			nv_log(LOG_DEBUG, "Found the correct storage plugin");
//		}
//	}

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <netvizd.h>
#include <plugin.h>
#include <nvconfig.h>
#include <nvlist.h>
#include "file.h"

#define PLUGIN_NAME		"file"

#define config_init		file_LTX_config_init

int config_init(struct config_p *p);
static int file_reload(struct config_p *p);
static int file_free(struct config_p *p);
static int create_conf();

int yylex();
extern FILE *yyin;

static nv_list(file_plug_list);
static nv_list(file_stor_list);
static nv_list(file_sens_list);
static nv_list(file_dset_list);
static nv_list(file_sys_list);

int config_init(struct config_p *p) {
	FILE *cfile = NULL;
	int ret = 0;
	int stat = 0;
	
	/* fill in config_p structure */
	strncpy(p->name, PLUGIN_NAME, NAME_LEN);
	p->name[NAME_LEN-1] = '\0';
	p->reload = file_reload;
	p->free = file_free;

	/* read in our config file */
	cfile = fopen("/home/tim/cs3901/netvizd/netvizd.conf", "r");
	yyin = cfile;
	ret = yyparse();
	if (ret != 0) {
		nv_log(LOG_ERROR, "configuration file parse failed, aborting");
		stat = -1;
		goto cleanup;
	}

	/* Now we need to go through our lists and try to create netvizd's
	 * internal configuration.  This involves using nvconfig.h's
	 * structs and resolving the links between various plugins, instances,
	 * and data sets. */
	ret = create_conf();
	if (ret != 0) {
		nv_log(LOG_ERROR, "configuration failed, aborting");
		stat = -1;
		goto cleanup;
	}

cleanup:
	return stat;
}

int file_reload(struct config_p *p) {
	return 0;
}

int file_free(struct config_p *p) {
	return 0;
}

/*
 * Called after each plugin defined in the global section of the config
 * file.
 */
void add_plugin(struct global_plugin *p) {
	nv_node n = NULL;

	nv_log(LOG_DEBUG, "Adding plugin to internal list:");
	nv_log(LOG_DEBUG, "    name: %s", p->name);
	nv_log(LOG_DEBUG, "    type: %i", p->type);
	nv_log(LOG_DEBUG, "    file: %s", p->file);

	nv_node_new(n);
	set_node_data(n, p);
	list_append(&file_plug_list, n);
}

/*
 * Called after each storage definition in the global section of the config
 * file.
 */
void add_storage(struct global_storage *s) {
	nv_node n;
	nv_node i;

	nv_log(LOG_DEBUG, "Adding storage definition to internal list:");
	nv_log(LOG_DEBUG, "    name: %s", s->name);
	nv_log(LOG_DEBUG, "    plugin name: %s", s->p_name);
	nv_log(LOG_DEBUG, "    values:");
	list_for_each(i, s->values) {
		struct values *v = NULL;
		v = node_data(struct values, i);
		nv_log(LOG_DEBUG, "        %s, %s", v->word, v->value);
	}

	nv_node_new(n);
	set_node_data(n, s);
	list_append(&file_stor_list, n);
}

/*
 * Called after each sensor definition in the global section of the config
 * file.
 */
void add_sensor(struct global_sensor *s) {
	nv_node n;
	nv_node i;
	
	nv_log(LOG_DEBUG, "Adding sensor definition to internal list:");
	nv_log(LOG_DEBUG, "    name: %s", s->name);
	nv_log(LOG_DEBUG, "    plugin name: %s", s->p_name);
	nv_log(LOG_DEBUG, "    values:");
	list_for_each(i, s->values) {
		struct values *v = NULL;
		v = node_data(struct values, i);
		nv_log(LOG_DEBUG, "        %s, %s", v->word, v->value);
	}

	nv_node_new(n);
	set_node_data(n, s);
	list_append(&file_sens_list, n);
}

/*
 * Called after each data set definition within a system section in the
 * config file.  Because of the way the parser is structured, we're only
 * sure to have the name of the system so far and not the complete
 * system structure.  We resolve this situation when the system definition
 * is complete inside of add_system().
 */
void add_data_set(struct data_set *d) {
	nv_node n;

	nv_log(LOG_DEBUG, "Adding data set to internal list:");
	nv_log(LOG_DEBUG, "    name: %s", d->name);
	nv_log(LOG_DEBUG, "    system: %s", d->s_name);
	nv_log(LOG_DEBUG, "    description: %s", d->desc);
	nv_log(LOG_DEBUG, "    type: %i", d->type);
	nv_log(LOG_DEBUG, "    sensor: %s", d->sensor);
	nv_log(LOG_DEBUG, "    storage: %s", d->storage);

	nv_node_new(n);
	set_node_data(n, d);
	list_append(&file_dset_list, n);
}

/*
 * Called after an entire system block has been defined in the config file.
 * We have all the data set definitions for this system, so make sure
 * they're all connected properly.
 */
void add_system(struct system *s) {
	nv_node n;

	nv_log(LOG_DEBUG, "Adding system to internal list:");
	nv_log(LOG_DEBUG, "    name: %s", s->name);
	nv_log(LOG_DEBUG, "    description: %s", s->desc);

	nv_node_new(n);
	set_node_data(n, s);
	list_append(&file_sys_list, n);
}

/*
 * Here we iterate over our linked lists and create the internal
 * configuration as defined in nvconfig.h.  We throw errors for things
 * like names that don't match within the config file or other obvious
 * things that are specific to this type of configuration plugin.  If
 * everything looks OK here, validate_conf() in the main app will verify
 * that we have enough things defined to actually run.  We also have to
 * free the memory allocated by the parser.
 */
int create_conf() {


}

/* vim: set ts=4 sw=4: */
