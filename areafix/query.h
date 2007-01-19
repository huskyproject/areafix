/*****************************************************************************
 * AreaFix library for Husky (FTN Software Package) supplemental file
 *****************************************************************************
 * Copyright (C) 1998-2003
 *
 * val khokhlov (Fido: 2:550/180), Kiev, Ukraine
 *
 * Based on original HPT code by Max Chernogor 2:464/108
 *
 * This file is part of libareafix.
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

#ifndef _HQUERY_H
#define _HQUERY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <huskylib/compiler.h>
#include <huskylib/huskylib.h>

/* Badmail reason (area write access) numbers (from hpt/h/fcommon.h) */
typedef enum {
 BM_MAIL_OK=0,
 BM_DENY_CREATE=1,
 BM_NOT_IN_GROUP=2,
 BM_LOW_WRITE_LEVEL=3,
 BM_DENY_IMPORT=4,
 BM_NOT_LINKED=5,
 BM_DENY_BY_FILTER=6,
 BM_MSGAPI_ERROR=7,
 BM_ILLEGAL_CHARS=8,
 BM_SENDER_NOT_FOUND=9,
 BM_CANT_OPEN_CONFIG=10,
 BM_NO_LINKS=11,
 BM_AREATAG_TOO_LONG=12,
 BM_AREA_KILLED=13,
 BM_DENY_NEWAREAREFUSEFILE=14,
 BM_WRONG_LINK_TO_AUTOCREATE=15,
 BM_AREA_IS_PAUSED=16,
 BM_NO_AREATAG=17,
 BM_CANT_CREATE_PATH=18,
 BM_MAXERROR=18       /* Set to max value, equivalence of last element */
} e_BadmailReasons;

HUSKYEXT e_BadmailReasons autoCreate(char *c_area, char *descr, hs_addr pktOrigAddr, ps_addr forwardAddr);

typedef struct query_areas
{
    char *name;
    char  type[5];
    char *report;

    time_t bTime;
    time_t eTime;

    int nFlag;
    ps_addr downlinks;
    size_t linksCount;
    struct query_areas *next;
/*     struct query_areas *prev; */
} s_query_areas;

enum  query_action{
    FIND,     /* Find area in query list */
    FINDFREQ, /* Find area with active query in query list */
    ADDFREQ,  /* Add node into existing query in list */
    ADDIDLE,  /* Create idle forward request into query list */
    DELIDLE   /* Remove idle forward request from query list */
};
typedef enum query_action e_query_action;

enum  changeConfigRet{ I_ERR=-2, /*  read config error */
                       O_ERR=-1, /*  write config error */
                       IO_OK,    /*  config file rewrited */
                       ADD_OK,   /*  link successfully added */
                       DEL_OK    /*  link removed */
};
typedef enum changeConfigRet e_changeConfigRet;

HUSKYEXT int isValidConference(const char *s);
HUSKYEXT s_query_areas* af_CheckAreaInQuery(char *areatag, ps_addr uplink, ps_addr dwlink, e_query_action act);
HUSKYEXT char* af_Req2Idle(char *areatag, char* report, hs_addr linkAddr);
HUSKYEXT int   af_OpenQuery();
HUSKYEXT int   af_CloseQuery();
HUSKYEXT void  af_QueueUpdate();
HUSKYEXT void  af_QueueReport();
HUSKYEXT int checkRefuse(char *areaName);


/* originally from hpt/h/toss.h */
HUSKYEXT s_arealink *getAreaLink(s_area *area, hs_addr aka);

/* originally from hpt/h/toss.h */
HUSKYEXT int  checkAreaLink(s_area *area, hs_addr aka, int type);

#ifdef __cplusplus
}
#endif

#endif
