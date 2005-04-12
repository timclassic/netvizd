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

/* Loaded configuration plugin */
struct nv_config_p *nv_config_p = NULL;
/* Global linked lists */
nv_list(nv_stor_p_list);
nv_list(nv_sens_p_list);
nv_list(nv_proto_p_list);
nv_list(nv_auth_p_list);
nv_list(nv_stor_list);
nv_list(nv_sens_list);
nv_list(nv_sys_list);
nv_list(nv_dsts_list);

static int nv_validate_conf();

/*
 * Initialize our configuration, loading the plugin specified on the
 * command line.
 */
int nv_config_init(char *plugin) {
	int stat = 0;

	/* load the configuration plugin */
	stat = config_p_init(plugin);
	if (stat != 0) {
		goto cleanup;
	}

	/* validate the configration */
	stat = nv_validate_conf();
	if (stat != 0) {
		goto cleanup;
	}

cleanup:
	return;
}

/*
 * At this stage the configuration plugin has attempted to build our
 * internal data structures.  Referential integrity is preserved up until
 * now (e.g. our plugin instances referring to a real plugin, etc.), but we
 * need to make sure we've got enough defined to run properly.
 *
 * We need:
 *     1. a config plugin - already verified
 *     2. at least one protocol plugin
 *     3. at least one auth plugin
 *     4. at least one storage instance (this also verifies that we have
 *        a storage plugin defined)
 *     5. at least one data set (this also verifies that we have a system
 *        defined)
 */
int nv_validate_conf() {
	int stat = 0;

	/* check for a protocol plugin */
	if (nv_proto_p_list.next == NULL) {
		nv_log(LOG_ERROR, "need at least one protocol plugin defined");
		stat = -1;
	}

	/* check for an auth plugin */
	if (nv_auth_p_list.next == NULL) {
		nv_log(LOG_ERROR, "need at least one authentication plugin defined");
		stat = -1;
	}

	/* check for a storage instance */
	if (nv_stor_list.next == NULL) {
		nv_log(LOG_ERROR, "need at least one storage instance defined");
		stat = -1;
	}

	/* check for a data set */
	if (nv_dsts_list.next == NULL) {
		nv_log(LOG_ERROR, "need at least one data set defined");
		stat = -1;
	}

	return stat;
}

/* vim: set ts=4 sw=4: */
