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

#ifdef __cplusplus
extern "C" {
#endif

#include <huskylib/compiler.h>
#include <huskylib/huskylib.h>
#include <fidoconf/fidoconf.h>
#include <fidoconf/arealist.h>
#include "areafix.h"

/* set to &safe_strdup for hpt */
HUSKYEXT char* (*call_sstrdup)(const char *);
/* set to &safe_malloc for hpt */
HUSKYEXT void* (*call_smalloc)(size_t);
/* set to &safe_realloc for hpt */
HUSKYEXT void* (*call_srealloc)(void *, size_t);

/* get area by name */
HUSKYEXT s_area* (*call_getArea)(char *);
/* validate a conference name */
HUSKYEXT int (*call_isValid)(const char *);
/* send a netmail message */
HUSKYEXT int (*call_sendMsg)(s_message *);
/* send a message to sysop */
HUSKYEXT int (*call_writeMsgToSysop)(s_message *);
/* return a pointer to robot's data in the link structure */
HUSKYEXT s_link_robot* (*call_getLinkRobot)(s_link *link);

/* do program-dependent stuff when creating area (e.g. announce) */
/* possible usage: afReportAutoCreate */
HUSKYEXT void (*hook_onAutoCreate)(char *c_area, char *descr, hs_addr pktOrigAddr, ps_addr forwardAddr);

/* do program-dependent stuff when deleting area (e.g. delete dupe base) */
/* possible usage: afDeleteArea */
HUSKYEXT int (*hook_onDeleteArea)(s_link *link, s_area *area);

/* do program-dependent stuff when rescanning area */
/* possible usage: afRescanArea */
HUSKYEXT int (*hook_onRescanArea)(char **report, s_link *link, s_area *area, long rescanCount, long rescanAfter);

/* call after config has been changed */
/* possible usage: perl_invalidate */
HUSKYEXT void (*hook_onConfigChange)(e_perlconftype configType);

/* return 1 to use **report as %list result; 0 to use built-in */
/* possible usage: perl_echolist */
HUSKYEXT int (*hook_echolist)(char **report, s_listype type, ps_arealist al, char *aka);

/* return 1 to use **report as cmd result; 0 to use built-in */
/* possible usage: perl_afixcmd */
HUSKYEXT int (*hook_afixcmd)(char **report, int cmd, char *aka, char *line);

/* return 1 to update request originating address */
/* possible usage: perl_afixreq */
HUSKYEXT int (*hook_afixreq)(s_message *msg, hs_addr pktOrigAddr);

/* return 1 to update robot's msg text and/or headers */
/* possible usage: perl_robotmsg */
HUSKYEXT int (*hook_robotmsg)(s_message *msg, char *type);

#ifdef __cplusplus
}
#endif

#endif
