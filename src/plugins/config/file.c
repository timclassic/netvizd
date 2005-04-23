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

#define config_init		file_LTX_config_init

int config_init(struct nv_config_p *p);
static int file_reload(struct nv_config_p *p);
static int file_free(struct nv_config_p *p);
static int create_conf();

int yylex();
extern FILE *yyin;

static nv_list(file_plug_list);
static nv_list(file_stor_list);
static nv_list(file_sens_list);
static nv_list(file_dset_list);
static nv_list(file_sys_list);

int config_init(struct nv_config_p *p) {
	FILE *cfile = NULL;
	int ret = 0;
	int stat = 0;
	
	/* fill in config_p structure */
	p->reload = file_reload;
	p->free = file_free;

	/* read in our config file */
	nv_log(LOG_INFO, "reading config file from " SYSCONFDIR "/netvizd.conf");
	cfile = fopen(SYSCONFDIR "/netvizd.conf", "r");
	yyin = cfile;
	ret = begin_parse();
	if (ret != 0) {
		nv_log(LOG_ERROR, "configuration file parse failed");
		stat = -1;
		goto cleanup;
	}

	/* Now we need to go through our lists and try to create netvizd's
	 * internal configuration.  This involves using nvconfig.h's
	 * structs and resolving the links between various plugins, instances,
	 * and data sets. */
	ret = create_conf();
	if (ret != 0) {
		nv_log(LOG_ERROR, "configuration failed");
		stat = -1;
		goto cleanup;
	}

cleanup:
	return stat;
}

int file_reload(struct nv_config_p *p) {
	return 0;
}

int file_free(struct nv_config_p *p) {
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
	nv_log(LOG_DEBUG, "    description: %s", d->desc);
	nv_log(LOG_DEBUG, "    type: %i", d->type);
	nv_log(LOG_DEBUG, "    sensor: %s", d->sensor);
	nv_log(LOG_DEBUG, "    storage: %s", d->storage);
	nv_log(LOG_DEBUG, "    system: %s", d->system);
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
 * everything looks OK here, nv_validate_conf() in the main app will verify
 * that we have enough things defined to actually run.  We also have to
 * free the memory allocated by the parser.
 *
 * 'f' is the struct from the file
 * 'c' is the struct from the internal config
 */
int create_conf() {
	nv_node i = NULL;
	nv_node t = NULL;
	int stat = 0;

	/*
	 * loop over the plugins
	 */
	/* create the real config structures */
	list_for_each(i, &file_plug_list) {
		nv_node n;
		struct nv_stor_p *stor;
		struct nv_sens_p *sens;
		struct nv_proto_p *proto;
		struct nv_auth_p *auth;
		struct global_plugin *f;

		f = node_data(struct global_plugin, i);
		
		if (f->file == NULL) {
			nv_log(LOG_ERROR, "file not specified for plugin %s", f->name);
			stat = -1;
		}
		switch (f->type) {
			case p_type_none:
				nv_log(LOG_ERROR, "type not specified for plugin %s",
					   f->name);
				stat = -1;
				break;

			case p_type_storage:
				stor = nv_calloc(struct nv_stor_p, 1);
				name_copy(stor->name, f->name);
				if (f->file) name_copy(stor->file, f->file);
				nv_node_new(n);
				set_node_data(n, stor);
				list_append(&nv_stor_p_list, n);
				break;

			case p_type_sensor:
				sens = nv_calloc(struct nv_sens_p, 1);
				name_copy(sens->name, f->name);
				if (f->file) name_copy(sens->file, f->file);
				nv_node_new(n);
				set_node_data(n, sens);
				list_append(&nv_sens_p_list, n);
				break;

			case p_type_proto:
				proto = nv_calloc(struct nv_proto_p, 1);
				name_copy(proto->name, f->name);
				if (f->file) name_copy(proto->file, f->file);
				nv_node_new(n);
				set_node_data(n, proto);
				list_append(&nv_proto_p_list, n);
				break;

			case p_type_auth:
				auth = nv_calloc(struct nv_auth_p, 1);
				name_copy(auth->name, f->name);
				if (f->file) name_copy(auth->file, f->file);
				nv_node_new(n);
				set_node_data(n, auth);
				list_append(&nv_auth_p_list, n);
				break;
		}
	}

	/*
	 * loop over storage instances
	 */
	/* match each instance with its storage plugin */
	list_for_each(i, &file_stor_list) {
		int err = 0;
		nv_node j;
		nv_node n;
		struct nv_stor *stor = NULL;
		nv_list *conf = NULL;
		struct global_storage *f = node_data(struct global_storage, i);

		stor = nv_calloc(struct nv_stor, 1);
		nv_list_new(stor->dsets);
		name_copy(stor->name, f->name);

		/* copy the config values over */
		nv_list_new(conf);
		list_for_each(j, f->values) {
			struct values *fv;
			nv_node nv;
			struct nv_conf *cv;

			/* copy the values to the new struct */
			fv = node_data(struct values, j);
			cv = nv_calloc(struct nv_conf, 1);
			name_copy(cv->key, fv->word);
			name_copy(cv->value, fv->value);
			
			/* add to list */
			nv_node_new(nv);
			set_node_data(nv, cv);
			list_append(conf, nv);
		}
		stor->conf = conf;
		
		/* try to find the name used for the plugin */
		err = -1;
		list_for_each(j, &nv_stor_p_list) {
			struct nv_stor_p *c = node_data(struct nv_stor_p, j);
			
			if (strncmp(f->p_name, c->name, NAME_LEN) == 0) {
				stor->plug = c;
				err = 0;
			}
		}
		if (err != 0) {
			nv_log(LOG_ERROR, "no storage plugin definition found with "
				   "name %s", f->p_name);
			stat = -1;
		}

		/* add storage to list */
		nv_node_new(n);
		set_node_data(n, stor);
		list_append(&nv_stor_list, n);
	}

	/*
	 * loop over sensor instances
	 */
	list_for_each(i, &file_sens_list) {
		int err = 0;
		nv_node j;
		nv_node n;
		struct nv_sens *sens = NULL;
		nv_list *conf = NULL;
		struct global_sensor *f = node_data(struct global_sensor, i);

		sens = nv_calloc(struct nv_sens, 1);
		nv_list_new(sens->dsets);
		name_copy(sens->name, f->name);

		/* copy the config values over */
		nv_list_new(conf);
		list_for_each(j, f->values) {
			struct values *fv;
			nv_node nv;
			struct nv_conf *cv;

			/* copy the values to the new struct */
			fv = node_data(struct values, j);
			cv = nv_calloc(struct nv_conf, 1);
			name_copy(cv->key, fv->word);
			name_copy(cv->value, fv->value);
			
			/* add to list */
			nv_node_new(nv);
			set_node_data(nv, cv);
			list_append(conf, nv);
		}
		sens->conf = conf;
		
		/* try to find the name used for the plugin */
		err = -1;
		list_for_each(j, &nv_sens_p_list) {
			struct nv_sens_p *c = node_data(struct nv_sens_p, j);
			
			if (strncmp(f->p_name, c->name, NAME_LEN) == 0) {
				sens->plug = c;
				err = 0;
			}
		}
		if (err != 0) {
			nv_log(LOG_ERROR, "no sensor plugin definition found with "
				   "name %s", f->p_name);
			stat = -1;
		}

		/* add sensor to list */
		nv_node_new(n);
		set_node_data(n, sens);
		list_append(&nv_sens_list, n);
	}

	/*
	 * loop over systems
	 */
	list_for_each(i, &file_sys_list) {
		nv_node n;
		struct system *f = node_data(struct system, i);
		struct nv_sys *sys = NULL;

		sys = nv_calloc(struct nv_sys, 1);
		name_copy(sys->name, f->name);
		name_copy(sys->desc, f->desc);
		
		/* add system to list */
		nv_node_new(n);
		set_node_data(n, sys);
		list_append(&nv_sys_list, n);
	}

	/*
	 * loop over data sets
	 */
	list_for_each(i, &file_dset_list) {
		int err = 0;
		nv_node n;
		nv_node o;
		nv_node j;
		struct nv_dsts *ds = NULL;
		struct data_set *f = node_data(struct data_set, i);

		ds = nv_calloc(struct nv_dsts, 1);
		name_copy(ds->name, f->name);
		name_copy(ds->desc, f->desc);
		ds->type = f->type;
		if (ds->type == ds_type_none) {
			nv_log(LOG_ERROR, "type not specified for data set %s",
				   ds->name);
			stat = -1;
		}

		/* try to find the name used for the sensor instance */
		if (f->sensor != NULL) {
			err = -1;
			list_for_each(j, &nv_sens_list) {
				struct nv_sens *c = node_data(struct nv_sens, j);
				
				if (strncmp(f->sensor, c->name, NAME_LEN) == 0) {
					ds->sens = c;
					nv_node_new(o);
					set_node_data(o, ds);
					list_append(c->dsets, o);
					err = 0;
				}
			}
			if (err != 0) {
				nv_log(LOG_ERROR, "no sensor instance definition found with "
					   "name %s", f->sensor);
				stat = -1;
			}
		}

		/* try to find the name used for the storage instance */
		if (f->storage != NULL) {
			err = -1;
			list_for_each(j, &nv_stor_list) {
				struct nv_stor *c = node_data(struct nv_stor, j);
				
				if (strncmp(f->storage, c->name, NAME_LEN) == 0) {
					ds->stor = c;
					nv_node_new(o);
					set_node_data(o, ds);
					list_append(c->dsets, o);
					err = 0;
				}
			}
			if (err != 0) {
				nv_log(LOG_ERROR, "no storage instance definition found with "
					   "name %s", f->storage);
				stat = -1;
			}
		} else {
			nv_log(LOG_ERROR, "no storage instance specified for data set %s",
				   ds->name);
			stat = -1;
		}

		/* try to find the name used for the system */
		err = -1;
		list_for_each(j, &nv_sys_list) {
			struct nv_sys *c = node_data(struct nv_sys, j);
			
			if (strncmp(f->system, c->name, NAME_LEN) == 0) {
				ds->sys = c;
				err = 0;
			}
		}
		if (err != 0) {
			nv_log(LOG_ERROR, "internal parsing error, no system found with "
				   "name %s", f->system);
			stat = -1;
		}
		
		/* add data set to list */
		nv_node_new(n);
		set_node_data(n, ds);
		list_append(&nv_dsts_list, n);
	}

cleanup:
	/* free up the plugin definitions */
	t = NULL;
	list_for_each(i, &file_plug_list) {
		struct global_plugin *f;

		/* free this file config */
		f = node_data(struct global_plugin, i);
		nv_free(f->name);
		nv_free(f->file);
		nv_free(f);

		if (t != NULL) list_del(i->prev);
		t = i;
	}
	list_del(t);

	/* free up storage instance definitions */
	t = NULL;
	list_for_each(i, &file_stor_list) {
		nv_node j = NULL;
		nv_node t2 = NULL;
		struct global_storage *f;

		/* free this storage config */
		f = node_data(struct global_storage, i);
		nv_free(f->name);
		nv_free(f->p_name);

		t2 = NULL;
		list_for_each(j, f->values) {
			if (t2 != NULL) list_del(j->prev);
			t2 = j;
		}
		list_del(t2);

		nv_free(f->values);
		nv_free(f);

		if (t != NULL) list_del(i->prev);
		t = i;
	}
	list_del(t);

	/* free up sensor instance definitions */
	t = NULL;
	list_for_each(i, &file_sens_list) {
		nv_node j = NULL;
		nv_node t2 = NULL;
		struct global_sensor *f;

		/* free this sensor config */
		f = node_data(struct global_sensor, i);
		nv_free(f->name);
		nv_free(f->p_name);

		t2 = NULL;
		list_for_each(j, f->values) {
			if (t2 != NULL) list_del(j->prev);
			t2 = j;
		}
		list_del(t2);

		nv_free(f->values);
		nv_free(f);

		if (t != NULL) list_del(i->prev);
		t = i;
	}
	list_del(t);

	/* free up the system definitions */
	t = NULL;
	list_for_each(i, &file_sys_list) {
		struct system *f;

		/* free this file config */
		f = node_data(struct system, i);
		nv_free(f->name);
		nv_free(f->desc);
		nv_free(f);

		if (t != NULL) list_del(i->prev);
		t = i;
	}
	list_del(t);

	/* free up the data set definitions */
	t = NULL;
	list_for_each(i, &file_dset_list) {
		struct data_set *f;

		/* free this file config */
		f = node_data(struct data_set, i);
		nv_free(f->name);
		nv_free(f->desc);
		nv_free(f->sensor);
		nv_free(f->storage);
		nv_free(f->system);
		nv_free(f);

		if (t != NULL) list_del(i->prev);
		t = i;
	}
	list_del(t);

	return stat;
}

/* vim: set ts=4 sw=4: */
