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

//#include <fcommon.h>

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
#define DONE        100
#define STAT        101
#define AFERROR     255

typedef enum { lt_all, lt_linked, lt_unlinked } s_listype;

int init_areafix(void);

char *print_ch(int len, char ch);
int processAreaFix(s_message *msg, s_pktHeader *pktHeader, unsigned force_pwd);
void afix(hs_addr addr, char *cmd);
int pauseAreas(int act, s_link *searchLink, s_area *searchArea);
char *rescan(s_link *link, char *cmd);
char *errorRQ(char *line);
int isPatternLine(char *s);
int forwardRequest(char *areatag, s_link *dwlink, s_link **lastRlink);
int forwardRequestToLink (char *areatag, s_link *uplink, s_link *dwlink, int act);
void sendAreafixMessages();
char *do_delete(s_link *link, s_area *area);

int changeconfig(char *fileName, s_area *area, s_link *link, int action);

#endif
