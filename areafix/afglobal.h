/*****************************************************************************
 * AreaFix library for Husky (FTN Software Package) global variables
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

#ifndef AFGLOBAL_H
#define AFGLOBAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <huskylib/compiler.h>
#include <huskylib/huskyext.h>
#include <fidoconf/fidoconf.h>

HUSKYEXT s_fidoconfig* af_config;
HUSKYEXT ps_robot      af_robot;
HUSKYEXT char*         af_cfgFile;
HUSKYEXT sApp*         af_app;
HUSKYEXT char*         af_versionStr;
HUSKYEXT int           af_quiet;
HUSKYEXT int           af_silent_mode;
HUSKYEXT int           af_report_changes;
HUSKYEXT int           af_send_notify;
HUSKYEXT int           af_pause;

#ifdef __cplusplus
}
#endif

#endif
