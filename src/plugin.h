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

#ifndef _PLUGIN_H_
#define _PLUGIN_H_

#include <netvizd.h>

BEGIN_C_DECLS;

/*
 * Configuration plugin definitions
 * Note there is no next pointer - we only ever load a single configuration
 * plugin at a time.
 */
struct config_p {
	char name[NAME_LEN];					/* plugin name */
	
	int (*reload)(struct config_p *p);		/* reload() function */
	int (*free)(struct config_p *p);		/* free() function */
};

/*
 * Storage plugin definitions
 * Defines the list of storage plugins that have been loaded.
 */
struct storage_p {
	char name[NAME_LEN];					/* plugin name */
	char instance[NAME_LEN];				/* instance name */

	int (*reload)(struct storage_p *p);		/* reload() function */
	int (*free)(struct storage_p *p);		/* free() function */

	struct storage_p *next;					/* the next plugin */
};

/*
 * Public interface
 */
int nv_plugins_init();
int nv_plugins_free();
int config_p_init(char *name);

END_C_DECLS;

#endif

/* vim: set ts=4 sw=4: */
