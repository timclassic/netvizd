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
#include "file.h"

#define PLUGIN_NAME		"file"

#define config_init		file_LTX_config_init

int config_init(struct config_p *p);
static int file_reload(struct config_p *p);
static int file_free(struct config_p *p);

int yylex();
extern FILE *yyin;

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
	cfile = fopen("/net/hu13/tims/cs3901/netvizd/netvizd.conf", "r");
	yyin = cfile;
	ret = yyparse();
	if (ret != 0) {
		nv_log(LOG_INFO, "configuration file parse failed, aborting");
		stat = -1;
	}

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
int add_plugin(struct global_plugin *p) {
	union {
		struct nv_stor_p *	stor;
		struct nv_sens_p *	sens;
		struct nv_proto_p *	proto;
		struct nv_auth_p *	auth;
	} plug;
	union {
		nv_node		stor;
		nv_node		sens;
		nv_node		proto;
		nv_node		auth;
	} node;

	nv_log(LOG_DEBUG, "Adding plugin:");
	nv_log(LOG_DEBUG, "    name: %s", p->name);
	nv_log(LOG_DEBUG, "    type: %i", p->type);
	nv_log(LOG_DEBUG, "    file: %s", p->file);

	switch(p->type) {
		case p_type_storage:
			plug.stor = nv_calloc(struct nv_stor_p, 1);
			strncpy(plug.stor->name, p->name, NAME_LEN);
			plug.stor->name[NAME_LEN-1] = '\0';
			strncpy(plug.stor->file, p->file, NAME_LEN);
			plug.stor->file[NAME_LEN-1] = '\0';
			nv_node_new(node.stor);
			set_node_data(node.stor, plug.stor);
			list_append(&nv_stor_p_list, node.stor);
			break;

		case p_type_sensor:
			plug.sens = nv_calloc(struct nv_sens_p, 1);
			strncpy(plug.sens->name, p->name, NAME_LEN);
			plug.sens->name[NAME_LEN-1] = '\0';
			strncpy(plug.sens->file, p->file, NAME_LEN);
			plug.sens->file[NAME_LEN-1] = '\0';
			nv_node_new(node.sens);
			set_node_data(node.sens, plug.sens);
			list_append(&nv_sens_p_list, node.sens);
			break;

		case p_type_proto:
			plug.proto = nv_calloc(struct nv_proto_p, 1);
			strncpy(plug.proto->name, p->name, NAME_LEN);
			plug.proto->name[NAME_LEN-1] = '\0';
			strncpy(plug.proto->file, p->file, NAME_LEN);
			plug.proto->file[NAME_LEN-1] = '\0';
			nv_node_new(node.proto);
			set_node_data(node.proto, plug.proto);
			list_append(&nv_proto_p_list, node.proto);
			break;

		case p_type_auth:
			plug.auth = nv_calloc(struct nv_auth_p, 1);
			strncpy(plug.auth->name, p->name, NAME_LEN);
			plug.auth->name[NAME_LEN-1] = '\0';
			strncpy(plug.auth->file, p->file, NAME_LEN);
			plug.auth->file[NAME_LEN-1] = '\0';
			nv_node_new(node.auth);
			set_node_data(node.auth, plug.auth);
			list_append(&nv_auth_p_list, node.auth);
			break;
	}

	return 0;
}

/*
 * Called after each storage definition in the global section of the config
 * file.
 */
int add_storage(struct global_storage *s, struct values *v) {
	struct nv_stor *stor;
	int stat = 0;
	nv_node n;

	nv_log(LOG_DEBUG, "Adding storage definition:");
	nv_log(LOG_DEBUG, "    name: %s", s->name);
	nv_log(LOG_DEBUG, "    plugin name: %s", s->p_name);
	nv_log(LOG_DEBUG, "    values:");
	while (v != NULL) {
		nv_log(LOG_DEBUG, "        %s, %s", v->word, v->value);
		v = v->next;
	}

	/* save the instance name */
	stor = nv_calloc(struct nv_stor, 1);
	strncpy(stor->name, s->name, NAME_LEN);
	stor->name[NAME_LEN-1] = '\0';

	/* find the plugin for this instance */
	stat = -1;
	list_for_each(n, &nv_stor_list) {
		struct nv_stor_p *p;
		p = node_data(struct nv_stor_p, n);
		if (strncmp(p->name, s->p_name, NAME_LEN) == 0) {
			stor->plug = p;
			stat = 0;
			nv_log(LOG_DEBUG, "Found the correct storage plugin");
		}
	}

	/* save the configuration of this instance */
	
	return stat;
}

/*
 * Called after ach sensor definition in the global section of the config
 * file.
 */
int add_sensor(struct global_sensor *s, struct values *v) {
	nv_log(LOG_DEBUG, "Adding sensor definition:");
	nv_log(LOG_DEBUG, "    name: %s", s->name);
	nv_log(LOG_DEBUG, "    plugin name: %s", s->p_name);
	nv_log(LOG_DEBUG, "    values:");
	while (v != NULL) {
		nv_log(LOG_DEBUG, "        %s, %s", v->word, v->value);
		v = v->next;
	}
	return 0;
}

/*
 * Called after each data set definition within a system section in the
 * config file.  Because of the way the parser is structured, we're only
 * sure to have the name of the system so far and not the complete
 * system structure.  We resolve this situation when the system definition
 * is complete inside of add_system().
 */
int add_data_set(struct data_set *d, char *s_name) {
	nv_log(LOG_DEBUG, "Adding data set:");
	nv_log(LOG_DEBUG, "    name: %s", d->name);
	nv_log(LOG_DEBUG, "    system: %s", s_name);
	nv_log(LOG_DEBUG, "    description: %s", d->desc);
	nv_log(LOG_DEBUG, "    type: %i", d->type);
	nv_log(LOG_DEBUG, "    sensor: %s", d->sensor);
	nv_log(LOG_DEBUG, "    storage: %s", d->storage);

	return 0;
}

/*
 * Called after an entire system block has been defined in the config file.
 * We have all the data set definitions for this system, so make sure
 * they're all connected properly.
 */
int add_system(struct system *s) {
	nv_log(LOG_DEBUG, "Adding system:");
	nv_log(LOG_DEBUG, "    name: %s", s->name);
	nv_log(LOG_DEBUG, "    description: %s", s->desc);

	return 0;
}

/* vim: set ts=4 sw=4: */
