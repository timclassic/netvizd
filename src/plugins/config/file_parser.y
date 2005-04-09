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
#include <nvconfig.h>
#include "file.h"

#define YYDEBUG 0
int yydebug = 0;

void push_generic();
void pop_generic();

static struct global_plugin *g_plugin;
static struct global_storage *g_storage;
static struct global_sensor *g_sensor;
static nv_list *g_values = NULL;
static struct data_set *d_set;
static struct system *sys;

void add_value(char *word, char *value);
void clear_values();

#define new_plugin		g_plugin = nv_calloc(struct global_plugin, 1)
#define new_storage		g_storage = nv_calloc(struct global_storage, 1)
#define new_sensor		g_sensor = nv_calloc(struct global_sensor, 1)
#define new_data_set	d_set = nv_calloc(struct data_set, 1)
#define new_system		sys = nv_calloc(struct system, 1)
%}

%union {
	int i;
	char *ch;
}

%type <i>	start global_block data_block global_list global_item data_list
			data_item plugin_block storage_block sensor_block dtype_stmt
			sensor_stmt plugin_list plugin_item ptype_stmt file_stmt
			generic_list generic_item top_list top_item storage_stmt
			global_list2 data_list2 plugin_list2 generic_list2 system_list
			system_list2 system_item d_desc_stmt s_desc_stmt system_block
%type <ch>	generic_item2
%token <i>	GLOBAL PLUGIN TYPE FILEE STORAGE SENSOR PROTO AUTH DATA_SET SEMI
			LBRACE RBRACE CRAP SYSTEM DESC COUNTER DERIVE ABSOLUTE GAUGE
%token <ch> WORD STRING INT YES NO

%start start

%%

start			: top_list
				| { }

top_list		: top_list top_item
		  		| top_item

top_item		: global_block SEMI
	  			| system_block SEMI

global_block	: GLOBAL LBRACE global_list RBRACE

system_block	: SYSTEM STRING { sys->name = $2; } LBRACE system_list RBRACE
				{	add_system(sys);
					new_system; }

data_block		: DATA_SET STRING LBRACE data_list RBRACE
				{	d_set->name = $2;
					d_set->system = strdup(sys->name);
					add_data_set(d_set);
					new_data_set; }

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
				| d_desc_stmt SEMI

system_list		: system_list2
			 	| { }

system_list2	: system_list2 system_item
			 	| system_item

system_item		: data_block SEMI
				| s_desc_stmt SEMI

plugin_block	: PLUGIN STRING LBRACE plugin_list RBRACE
				{	g_plugin->name = $2;
					add_plugin(g_plugin);
					new_plugin; }

storage_block	: STORAGE STRING TYPE STRING LBRACE { push_generic(); }
				  generic_list
				{	g_storage->name = $2;
					g_storage->p_name = $4;
					g_storage->values = g_values;
					add_storage(g_storage);
					new_storage;
					clear_values(); }

sensor_block	: SENSOR STRING TYPE STRING LBRACE { push_generic(); }
				  generic_list
				{	g_sensor->name = $2;
					g_sensor->p_name = $4;
					g_sensor->values = g_values;
					add_sensor(g_sensor);
					new_sensor;
					clear_values(); }

dtype_stmt		: TYPE COUNTER		{ d_set->type = ds_type_counter; }
				| TYPE DERIVE		{ d_set->type = ds_type_derive; }
				| TYPE ABSOLUTE		{ d_set->type = ds_type_absolute; }
				| TYPE GAUGE		{ d_set->type = ds_type_gauge; }

d_desc_stmt		: DESC STRING		{ d_set->desc = $2; }

sensor_stmt		: SENSOR STRING		{ d_set->sensor = $2; }

storage_stmt	: STORAGE STRING	{ d_set->storage = $2; }

s_desc_stmt		: DESC STRING		{ sys->desc = $2; }

plugin_list		: plugin_list2
			 	| { }

plugin_list2	: plugin_list2 plugin_item
			 	| plugin_item

plugin_item		: ptype_stmt SEMI
			 	| file_stmt SEMI

ptype_stmt		: TYPE STORAGE	{ g_plugin->type = p_type_storage; }
				| TYPE SENSOR	{ g_plugin->type = p_type_sensor; }
				| TYPE PROTO	{ g_plugin->type = p_type_proto; }
				| TYPE AUTH		{ g_plugin->type = p_type_auth; }

file_stmt		: FILEE STRING	{ g_plugin->file = $2; }

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
 * begin_parse() and end_parse() initialize and clean up structures used
 * during the parsing process. */
int begin_parse() {
	int stat = 0;

	/* allocate initial structures */
	new_plugin;
	new_storage;
	new_sensor;
	new_data_set;
	new_system;

	/* start the parser */
	stat = yyparse();

	/* clean up left-over structures and leave */
	nv_free(g_plugin);
	nv_free(g_storage);
	nv_free(g_sensor);
	nv_free(d_set);
	nv_free(sys);
	return stat;
}

/*
 * The following functions manage the dynamic value linked list.
 * It stores the word/value pairs for the custom plugin
 * configurations.
 */
void add_value(char *word, char *value) {
	nv_node n;
	struct values *v = NULL;

	if (g_values == NULL)
		nv_list_new(g_values);
	v = nv_calloc(struct values, 1);
	v->word = word;
	v->value = value;

	nv_node_new(n);
	set_node_data(n, v);
	list_append(g_values, n);
}

void clear_values() {
	g_values = NULL;
}

/* vim: set ts=4 sw=4: */
