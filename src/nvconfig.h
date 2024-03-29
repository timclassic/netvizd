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

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <netvizd.h>
#include <nvlist.h>


/* struct for a linked list of the configuration options for a plugin
 * instance */
struct nv_conf {
	char				key[NAME_LEN];
	char				value[NAME_LEN];
};

/* an instance of a storage plugin */
struct nv_stor {
	char				name[NAME_LEN];
	struct nv_stor_p *	plug;
	nv_list *			dsets;
	nv_list *			conf;
	pthread_t *			thread;

	int					beat;
	int					(*beatfunc)(struct nv_stor *);

	void *				data;
};

/* an instance of a sensor plugin */
struct nv_sens {
	char				name[NAME_LEN];
	struct nv_sens_p *	plug;
	nv_list *			dsets;
	nv_list *			conf;
	pthread_t *			thread;

	int					beat;
	int					(*beatfunc)(struct nv_sens *);

	void *				data;
};

/* a system definition */
struct nv_sys {
	char				name[NAME_LEN];
	char				desc[NAME_LEN];
};

/* time series-based data sets */
enum nv_ds_type {
	ds_type_none = 0,
	ds_type_counter,
	ds_type_derive,
	ds_type_absolute,
	ds_type_gauge
};
enum nv_ds_cf {
	ds_cf_average = 0,
	ds_cf_min,
	ds_cf_max,
	ds_cf_last
};
struct nv_dsts {
	char				name[NAME_LEN];
	enum nv_ds_type		type;
	enum nv_ds_cf		cf;
	char				desc[NAME_LEN];
	struct nv_sens *	sens;
	struct nv_stor *	stor;
	struct nv_sys *		sys;
};

/* a loaded config plugin */
struct nv_config_p {
	char				file[NAME_LEN];

	int					(*reload)(struct nv_config_p *);
	int					(*free)(struct nv_config_p *);
};

/* a loaded storage plugin */
struct nv_stor_p {
	char 				name[NAME_LEN];
	char				file[NAME_LEN];

	int					(*free)(struct nv_stor_p *);
	int					(*inst_init)(struct nv_stor *);
	int					(*inst_free)(struct nv_stor *);

	int					(*stor_ts_data)(struct nv_stor *, char *, char *,
										time_t, double);
	nv_list *			(*get_ts_data)(struct nv_stor *, char *, char *,
									   time_t, time_t, int);
	int					(*stor_ts_utime)(struct nv_stor *, char *, char *,
										time_t);
	time_t				(*get_ts_utime)(struct nv_stor *, char *, char *);
};
	
/* a loaded sensor plugin */
struct nv_sens_p {
	char 				name[NAME_LEN];
	char				file[NAME_LEN];

	int					(*free)(struct nv_sens_p *);
	int					(*inst_init)(struct nv_sens *);
	int					(*inst_free)(struct nv_sens *);
};

/* a loaded protocol plugin */
struct nv_proto_p {
	char 				name[NAME_LEN];
	char				file[NAME_LEN];

	pthread_t *			thread;

	int					(*listen)(struct nv_proto_p *);
};

/* a loaded authentication plugin */
struct nv_auth_p {
	char 				name[NAME_LEN];
	char				file[NAME_LEN];
};


#ifndef NV_GLOBAL_CONFIG
extern struct nv_config_p *nv_config_p;
extern nv_list nv_stor_p_list;
extern nv_list nv_sens_p_list;
extern nv_list nv_proto_p_list;
extern nv_list nv_auth_p_list;
extern nv_list nv_stor_list;
extern nv_list nv_sens_list;
extern nv_list nv_sys_list;
extern nv_list nv_dsts_list;
#endif /* NV_GLOBAL_CONFIG */

#endif

/* vim: set ts=4 sw=4: */
