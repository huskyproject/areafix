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

#include <stddef.h>

#include <fidoconf/fidoconf.h>
#include <fidoconf/arealist.h>

#define DLLEXPORT
#include <huskylib/huskyext.h>
#include "areafix.h"
#include "callback.h"

char* (*call_sstrdup)(const char *) = NULL;
void* (*call_smalloc)(size_t) = NULL;
void* (*call_srealloc)(void *, size_t) = NULL;

s_link_robot* (*call_getLinkRobot)(s_link *link) = NULL;

int (*call_isValid)(const char *) = NULL;
int (*call_sendMsg)(s_message *) = NULL;
int (*call_writeMsgToSysop)(s_message *) = NULL;

void (*hook_onAutoCreate)(char *c_area, char *descr, hs_addr pktOrigAddr, ps_addr forwardAddr) = NULL;
int (*hook_onDeleteArea)(s_link *link, s_area *area) = NULL;
int (*hook_onRescanArea)(char **report, s_link *link, s_area *area, long rescanCount) = NULL;
void (*hook_onConfigChange)(e_perlconftype confType) = NULL;
int (*hook_echolist)(char **report, s_listype type, ps_arealist al, char *aka) = NULL;
int (*hook_afixcmd)(char **report, int cmd, char *aka, char *line) = NULL;
int (*hook_afixreq)(s_message *msg, hs_addr pktOrigAddr) = NULL;
