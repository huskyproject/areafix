/*****************************************************************************
 * AreaFix library for Husky (FTN Software Package)
 *****************************************************************************
 * Copyright (C) 1998-2003
 *
 * val khokhlov (Fido: 2:550/180), Kiev, Ukraine
 *
 * Based on original HPT code by:
 *
 * Max Levenkov
 *
 * Fido:     2:5000/117
 * Internet: sackett@mail.ru
 * Novosibirsk, West Siberia, Russia
 *
 * Big thanks to:
 *
 * Fedor Lizunkov
 *
 * Fido:     2:5020/960
 * Moscow, Russia
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

#ifndef _AREAFIX_H
#define _AREAFIX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <huskylib/compiler.h>
#include <huskylib/huskylib.h>

#define NOTHING     0
#define LIST        1
#define HELP        2
#define ADD         3
#define DEL         4
#define AVAIL       5
#define QUERY       6
#define UNLINKED    7
#define PAUSE       8
#define RESUME      9
#define INFO        10
#define RESCAN      11
#define REMOVE      12
#define ADD_RSC     13
#define PACKER      14
#define RSB         15
#define RULES       16
#define PKTSIZE     17
#define ARCMAILSIZE 18
#define AREAFIXPWD  19
#define FILEFIXPWD  20
#define PKTPWD      21
#define TICPWD      22
#define FROM        23
#define DONE        100
#define STAT        101
#define RESEND      102
#define AFERROR     255

typedef struct linkdata {
    char  *robot;
    char  *pwd;
    char  *flags;
    long   attrs;
    int    numDfMask;
    char **dfMask;
    int    numFrMask;
    char **frMask;
    char  *denyFwdFile;
    int    fwd;
    char  *fwdFile;
    int    advAfix;
} s_linkdata;

typedef enum { lt_all, lt_linked, lt_unlinked } s_listype;
typedef enum { PERL_CONF_MAIN = 1, PERL_CONF_LINKS = 2, PERL_CONF_AREAS = 4 } e_perlconftype;
typedef enum { ACT_PAUSE, ACT_UNPAUSE } e_pauseAct;

HUSKYEXT int init_areafix(char *robotName);

/* export for htick */
HUSKYEXT unsigned char RetFix;
HUSKYEXT char *list(s_listype type, s_link *link, char *cmdline);
HUSKYEXT char *help(s_link *link);
HUSKYEXT char *available(s_link *link, char *cmdline);
HUSKYEXT int changeconfig(char *fileName, s_area *area, s_link *link, int action);
HUSKYEXT int forwardRequest(char *areatag, s_link *dwlink, s_link **lastRlink);
HUSKYEXT char *subscribe(s_link *link, char *cmd);
HUSKYEXT char *errorRQ(char *line);
HUSKYEXT char *unsubscribe(s_link *link, char *cmd);
HUSKYEXT char *pause_link(s_link *link);
HUSKYEXT char *resume_link(s_link *link);
HUSKYEXT void preprocText(char *split, s_message *msg, char *reply, s_link *link);
HUSKYEXT char *textHead(void);
HUSKYEXT char *areaStatus(char *report, char *preport);
HUSKYEXT void RetMsg(s_message *msg, s_link *link, char *report, char *subj);
HUSKYEXT void sendAreafixMessages();
/* end of list */

HUSKYEXT char *print_ch(int len, char ch);
HUSKYEXT int processAreaFix(s_message *msg, s_pktHeader *pktHeader, unsigned force_pwd);
HUSKYEXT void afix(hs_addr addr, char *cmd);
HUSKYEXT int pauseArea(e_pauseAct pauseAct, s_link *searchLink, s_area *searchArea);
HUSKYEXT char *rescan(s_link *link, char *cmd);
HUSKYEXT char *errorRQ(char *line);
HUSKYEXT int isPatternLine(char *s);
HUSKYEXT int forwardRequest(char *areatag, s_link *dwlink, s_link **lastRlink);
HUSKYEXT int forwardRequestToLink (char *areatag, s_link *uplink, s_link *dwlink, int act);
HUSKYEXT void sendAreafixMessages();
HUSKYEXT char *do_delete(s_link *link, s_area *area);
HUSKYEXT int relink(int mode, char *pattern, hs_addr fromAddr, hs_addr toAddr);

HUSKYEXT int changeconfig(char *fileName, s_area *area, s_link *link, int action);

#ifdef __cplusplus
}
#endif

#endif
