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
	cfile = fopen("/home/tim/cs3901/netvizd/netvizd.conf", "r");
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

int add_plugin(struct global_plugin *p) {
	printf("Adding plugin:\n");
	printf("    name: %s\n", p->name);
	printf("    type: %i\n", p->type);
	printf("    file: %s\n", p->file);

	return 0;
}

int add_storage(struct global_storage *s, struct values *v) {
	printf("Adding storage definition:\n");
	printf("    name: %s\n", s->name);
	printf("    plugin name: %s\n", s->p_name);
	printf("    values:\n");
	while (v != NULL) {
		printf("        %s, %s\n", v->word, v->value);
		v = v->next;
	}
	return 0;
}

int add_sensor(struct global_sensor *s, struct values *v) {
	printf("Adding sensor definition:\n");
	printf("    name: %s\n", s->name);
	printf("    plugin name: %s\n", s->p_name);
	printf("    values:\n");
	while (v != NULL) {
		printf("        %s, %s\n", v->word, v->value);
		v = v->next;
	}
	return 0;
}

int add_data_set(struct data_set *d) {
	printf("Adding data set:\n");
	printf("    name: %s\n", d->name);
	printf("    type: %i\n", d->type);
	printf("    sensor: %s\n", d->sensor);
	printf("    storage: %s\n", d->storage);
}

/* vim: set ts=4 sw=4: */
