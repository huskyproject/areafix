/*****************************************************************************
 * AreaFix library for Husky (FTN Software Package) callbacks and hooks
 *****************************************************************************
 * Copyright (C) 2003
 *
 * val khokhlov (Fido: 2:550/180), Kiev, Ukraine
 *
 * This file is part of Husky project.
 *
 * libareafix is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libareafix; see the file COPYING.  If not, write to the Free
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/
/* $Id$ */

#ifndef _CALLBACK_H
#define _CALLBACK_H

#include <fidoconf/fidoconf.h>
#include <fidoconf/arealist.h>
#include <areafix/areafix.h>

/* set to &safe_strdup for hpt */
extern char* (*call_sstrdup)(const char *);
/* set to &safe_malloc for hpt */
extern void* (*call_smalloc)(size_t);
/* set to &safe_realloc for hpt */
extern void* (*call_srealloc)(void *, size_t);

/* send a netmail message */
extern int (*call_sendMsg)(s_message *);
/* send a message to sysop */
extern int (*call_writeMsgToSysop)(s_message *);

/* do program-dependent stuff when creating area (e.g. announce) */
/* possible usage: afReportAutoCreate */
extern void (*hook_onAutoCreate)(char *c_area, char *descr, hs_addr pktOrigAddr, ps_addr forwardAddr);

/* do program-dependent stuff when deleting area (e.g. delete dupe base) */
/* possible usage: afDeleteArea */
extern int (*hook_onDeleteArea)(s_link *link, s_area *area);

/* do program-dependent stuff when rescanning area */
/* possible usage: afRescanArea */
extern int (*hook_onRescanArea)(char **report, s_link *link, s_area *area, long rescanCount);

/* call after config has been changed */
/* possible usage: perl_setvars */
extern void (*hook_onConfigChange)(void);

/* return 1 to use **report as %list result; 0 to use built-in */
/* possible usage: perl_echolist */
extern int (*hook_echolist)(char **report, s_listype type, ps_arealist al, char *aka);

/* return 1 to use **report as cmd result; 0 to use built-in */
/* possible usage: perl_afixcmd */
extern int (*hook_afixcmd)(char **report, int cmd, char *aka, char *line);

/* return 1 to update request originating address */
/* possible usage: perl_afixreq */
extern int (*hook_afixreq)(s_message *msg, hs_addr pktOrigAddr);

#endif
