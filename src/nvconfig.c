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

#define NV_GLOBAL_CONFIG

#include <netvizd.h>
#include <nvconfig.h>
#include <nvlist.h>

/* Global linked lists */
nv_list(nv_stor_p_list);
nv_list(nv_sens_p_list);
nv_list(nv_proto_p_list);
nv_list(nv_auth_p_list);
nv_list(nv_stor_list);
nv_list(nv_sens_list);
nv_list(nv_sys_list);

int nv_config_init(char *plugin) {
	int ret = 0;

	/* load the configuration plugin */
	ret = config_p_init(plugin);
	if (ret < 0) {
		return ret;
	}





}


/* vim: set ts=4 sw=4: */
