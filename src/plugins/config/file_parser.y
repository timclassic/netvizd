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

%{
#include "file.h"

#define YYDEBUG 0
int yydebug = 0;

void push_generic();
void pop_generic();

static struct global_plugin g_plugin;
static struct global_storage g_storage;
static struct global_sensor g_sensor;
static struct values *g_values;
static struct data_set d_set;

void add_value(char *word, char *value);
void clear_values();
%}

%union {
	int i;
	char *ch;
}

%type <i>	start global_block data_block global_list global_item data_list
			data_item plugin_block storage_block sensor_block dtype_stmt
			sensor_stmt plugin_list plugin_item ptype_stmt file_stmt
			generic_list generic_item top_list top_item storage_stmt
			global_list2 data_list2 plugin_list2 generic_list2
%type <ch>	generic_item2
%token <i>	GLOBAL PLUGIN TYPE FILEE STORAGE SENSOR PROTO AUTH DATA_SET
			TIME_SERIES SEMI LBRACE RBRACE CRAP
%token <ch> WORD STRING INT YES NO

%start start

%%

start			: top_list
				| { }

top_list		: top_list top_item
		  		| top_item

top_item		: global_block SEMI
	  			| data_block SEMI

global_block	: GLOBAL LBRACE global_list RBRACE

data_block		: DATA_SET STRING LBRACE data_list RBRACE
				{	d_set.name = $2;
					add_data_set(&d_set);
					nv_free(d_set.name);
					nv_free(d_set.sensor);
					nv_free(d_set.storage); }

global_list		: global_list2
			 	| { }

global_list2	: global_list2 global_item
			 	| global_item

global_item		: plugin_block SEMI
			 	| storage_block SEMI
				| sensor_block SEMI

data_list		: data_list2
		   		| { }

data_list2		: data_list2 data_item
		   		| data_item

data_item		: dtype_stmt SEMI
		   		| sensor_stmt SEMI
				| storage_stmt SEMI

plugin_block	: PLUGIN STRING LBRACE plugin_list RBRACE
				{	g_plugin.name = $2;
					add_plugin(&g_plugin);
					nv_free(g_plugin.name);
					nv_free(g_plugin.file); }

storage_block	: STORAGE STRING TYPE STRING LBRACE { push_generic(); }
				  generic_list
				{	g_storage.name = $2;
					g_storage.p_name = $4;
					add_storage(&g_storage, g_values);
					nv_free(g_storage.name);
					nv_free(g_storage.p_name);
					clear_values(); }

sensor_block	: SENSOR STRING TYPE STRING LBRACE { push_generic(); }
				  generic_list
				{	g_sensor.name = $2;
					g_sensor.p_name = $4;
					add_sensor(&g_sensor, g_values);
					nv_free(g_sensor.name);
					nv_free(g_sensor.p_name);
					clear_values(); }

dtype_stmt		: TYPE TIME_SERIES	{ d_set.type = ds_type_time_series; }

sensor_stmt		: SENSOR STRING		{ d_set.sensor = $2; }

storage_stmt	: STORAGE STRING	{ d_set.storage = $2; }

plugin_list		: plugin_list2
			 	| { }

plugin_list2	: plugin_list2 plugin_item
			 	| plugin_item

plugin_item		: ptype_stmt SEMI
			 	| file_stmt SEMI

ptype_stmt		: TYPE STORAGE	{ g_plugin.type = p_type_storage; }
				| TYPE SENSOR	{ g_plugin.type = p_type_sensor; }
				| TYPE PROTO	{ g_plugin.type = p_type_proto; }
				| TYPE AUTH		{ g_plugin.type = p_type_auth; }

file_stmt		: FILEE STRING	{ g_plugin.file = $2; }

generic_list	: generic_list2
			 	| { }

generic_list2	: generic_list2 generic_item
			 	| generic_item

generic_item	: WORD generic_item2 SEMI { add_value($1, $2); }
			 
generic_item2 	: STRING
			 	| INT
			 	| YES
			 	| NO

%%

/*
 * The following functions manage the dynamic value linked list.
 * It stores the word/value pairs for the custom plugin
 * configurations.
 */

void add_value(char *word, char *value) {
	struct values *v = NULL;
	
	/* find last entry and allocate a new one */
	v = g_values;
	if (v == NULL) {
		g_values = nv_calloc(struct values, 1);
		v = g_values;
	} else {
		while (v->next != NULL)
			v = v->next;
		v->next = nv_calloc(struct values, 1);
		v = v->next;
	}

	/* fill in info */
	v->word = word;
	v->value = value;
	v->next = NULL;
}

void clear_values() {
	struct values *v = NULL;
	struct values *next = NULL;

	/* loop and free all memory */
	v = g_values;
	while (v != NULL) {
		next = v->next;
		nv_free(v->word);
		nv_free(v->value);
		nv_free(v);
		v = next;
	}
	g_values = NULL;
}

/* vim: set ts=4 sw=4: */
