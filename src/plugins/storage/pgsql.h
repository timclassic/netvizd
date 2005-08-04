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

#ifndef _PLUGINS_PGSQL_H_
#define _PLUGINS_PGSQL_H_

#include <netvizd.h>
#include <pthread.h>
#include "pgsql_pool.h"

struct pgsql_data {
	char *				host;       /* database server hostname */
	int					port;       /* port number */
	char *				user;       /* user name */
	char *				pass;       /* password */
	char *				db;         /* database name */
	int					ctimeout;   /* connection timeout */
	int					rtimeout;   /* retry timeout */
	int					ssl;        /* use ssl? */
	struct pgsql_pool	pool;       /* our connection pool */
	
	pthread_t *			thread;     /* maintenance thread */
	pthread_mutex_t *	lock;       /* data lock */
	pthread_cond_t *	readyc;     /* ready condition variables */
	int					ready;      /* are we ready to work? */
	int					quit;       /* are we ready to exit? */
};

#endif
