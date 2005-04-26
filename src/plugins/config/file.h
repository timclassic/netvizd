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

#ifndef _FILE_H_
#define _FILE_H_

#include <netvizd.h>
#include <nvconfig.h>

/*
 * The following data structures are used to hold parsed information
 * temporarily until a complete structure can be submitted to file.c
 * and added to the global configuration.
 */

enum p_type {
	p_type_none = 0,
	p_type_storage,
	p_type_sensor,
	p_type_proto,
	p_type_auth
};

struct global_plugin {
	char *			name;
	enum p_type		type;
	char *			file;
};

struct values {
	char *			word;
	char *			value;
};

struct global_storage {
	char *			name;
	char *			p_name;
	nv_list *		values;
};

struct global_sensor {
	char *			name;
	char *			p_name;
	nv_list *		values;
};

struct data_set {
	char *			name;
	char *			desc;
	enum nv_ds_type	type;
	enum nv_ds_cf	cf;
	char *			sensor;
	char *			storage;
	char *			system;
};

struct system {
	char *			name;
	char *			desc;
};

void add_plugin(struct global_plugin *p);
void add_storage(struct global_storage *s);
void add_sensor(struct global_sensor *s);
void add_data_set(struct data_set *d);
void add_system(struct system *s);
int begin_parse();

#endif
