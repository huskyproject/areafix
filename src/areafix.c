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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

#include <huskylib/compiler.h>
#include <huskylib/huskylib.h>

#ifdef HAS_IO_H
#include <io.h>
#endif

#ifdef HAS_UNISTD_H
#include <unistd.h>
#endif

#include <fidoconf/fidoconf.h>
#include <fidoconf/common.h>
#include <huskylib/xstr.h>
#include <fidoconf/areatree.h>
#include <fidoconf/afixcmd.h>
#include <fidoconf/arealist.h>
#include <huskylib/recode.h>

/*
#include <fcommon.h>
#include <global.h>
#include <pkt.h>
#include <version.h>
#include <toss.h>
#include <ctype.h>
#include <seenby.h>
#include <scan.h>
#include <scanarea.h>
#include <hpt.h>
#include <dupe.h>
*/

#define DLLEXPORT
#include <huskylib/huskyext.h>
#include <areafix.h>
#include <query.h>
#include <afglobal.h>
#include <callback.h>

#define _AF (af_app->module == M_HTICK ? "file" : "area")

unsigned char RetFix;
static int rescanMode = 0;
static int rulesCount = 0;
static char **rulesList = NULL;

char *print_ch(int len, char ch)
{
    static char tmp[256];

    if (len <= 0 || len > 255) return "";

    memset(tmp, ch, len);
    tmp[len]=0;
    return tmp;
}

char *getPatternFromLine(char *line, int *reversed)
{

    *reversed = 0;
    if (!line) return NULL;
    /* original string is like "%list ! *.citycat.*" or withut '!' sign*/
    if (line[0] == '%') line++; /* exclude '%' sign */
    while((strlen(line) > 0) && isspace(line[0])) line++; /* exclude spaces between '%' and command */
    while((strlen(line) > 0) && !isspace(line[0])) line++; /* exclude command */
    while((strlen(line) > 0) && isspace(line[0])) line++; /* exclude spaces between command and pattern */

    if ((strlen(line) > 2) && (line[0] == '!') && (isspace(line[1])))
    {
        *reversed = 1;
        line++;     /* exclude '!' sign */
        while(isspace(line[0])) line++; /* exclude spaces between '!' and pattern */
    }

    if (strlen(line) > 0)
        return line;
    else
        return NULL;
}

char *list(s_listype type, s_link *link, char *cmdline) {
    unsigned int cnt, i, j, export, import, mandatory, active, avail, rc = 0;
    char *report = NULL;
    char *list = NULL;
    char *pattern = NULL;
    int reversed;
    ps_arealist al;
    ps_area area;
    int grps = (af_config->listEcho == lemGroup) || (af_config->listEcho == lemGroupName);
    int pause = af_app->module == M_HTICK ? FILEAREA : ECHOAREA;

    if (cmdline) pattern = getPatternFromLine(cmdline, &reversed);
    if (af_app->module != M_HTICK) {
      if ((pattern) && (strlen(pattern)>60 || !isValidConference(pattern))) {
          w_log(LL_FUNC, "areafix::list() FAILED (error request line)");
          return errorRQ(cmdline);
      }
    }

    switch (type) {
      case lt_all:
        xscatprintf(&report, "Available %sareas for %s\r\r", af_app->module == M_HTICK ? "file" : "", aka2str(link->hisAka));
        break;
      case lt_linked:
        xscatprintf(&report, "%s %sareas for %s\r\r", 
                    (link->Pause & pause) ? "Passive" : "Active", 
                    af_app->module == M_HTICK ? "file" : "", 
                    aka2str(link->hisAka));
        break;
      case lt_unlinked:
        xscatprintf(&report, "Unlinked %sareas for %s\r\r", af_app->module == M_HTICK ? "file" : "", aka2str(link->hisAka));
        break;
    }

    al = newAreaList(af_config);
    cnt = af_app->module == M_HTICK ? af_config->fileAreaCount : af_config->echoAreaCount;
    for (i=active=avail=0; i<cnt; i++) {
        area = af_app->module == M_HTICK ? &(af_config->fileAreas[i]) : &(af_config->echoAreas[i]);
	rc = subscribeCheck(area, link);

        if ( (type == lt_all && rc < 2)
             || (type == lt_linked && rc == 0)
             || (type == lt_unlinked && rc == 1)
           ) { /*  add line */

            import = export = 1; mandatory = 0;
            for (j = 0; j < area->downlinkCount; j++) {
               if (addrComp(link->hisAka, area->downlinks[j]->link->hisAka) == 0) {
                  import = area->downlinks[j]->import;
                  export = area->downlinks[j]->export;
                  mandatory = area->downlinks[j]->mandatory;
                  break;
               }
            }

            if (pattern)
            {
                /* if matches pattern and not reversed (or vise versa) */
                if (patimat(area->areaName, pattern)!=reversed)
                {
                    addAreaListItem(al,rc==0, area->msgbType!=MSGTYPE_PASSTHROUGH, import, export, mandatory, area->areaName,area->description,area->group);
                    if (rc==0) active++; avail++;
                }
            } else
            {
                addAreaListItem(al,rc==0, area->msgbType!=MSGTYPE_PASSTHROUGH, import, export, mandatory, area->areaName,area->description,area->group);
                if (rc==0) active++; avail++;
            }
	} /* end add line */

    } /* end for */
  if ( (hook_echolist == NULL) ||
       !(*hook_echolist)(&report, type, al, aka2str(link->hisAka)) 
  ) {
    sortAreaList(al);
    switch (type) {
      case lt_linked:
      case lt_all:      list = formatAreaList(al,78," *SRW M", grps); break;
      case lt_unlinked: list = formatAreaList(al,78,"  S    ", grps); break;
    }
    if (list) xstrcat(&report,list);
    nfree(list);
    freeAreaList(al);

    if (type != lt_unlinked) 
        xstrcat(&report, "\r'*' = area is active");
        xstrcat(&report, "\r'R' = area is readonly for you");
        xstrcat(&report, "\r'W' = area is writeonly for you");
        xstrcat(&report, "\r'M' = area is mandatory for you");
    xstrcat(&report, "\r'S' = area is rescanable");

    if (type == lt_linked) {
    }
    switch (type) {
      case lt_all:
        xscatprintf(&report, "\r\r %i area(s) available, %i area(s) linked\r", avail, active);
        break;
      case lt_linked:
        xscatprintf(&report, "\r\r %i area(s) linked\r", active);
        break;
      case lt_unlinked:
        xscatprintf(&report, "\r\r %i area(s) available\r", avail);
        break;
    }
/*    xscatprintf(&report,  "\r for link %s\r", aka2str(link->hisAka));*/

    if (link->afixEchoLimit) xscatprintf(&report, "\rYour limit is %u areas for subscribe\r", link->afixEchoLimit);
  } /* hook_echolist */
    switch (type) {
      case lt_all:
        w_log(LL_AREAFIX, "%sfix: List sent to %s", _AF, aka2str(link->hisAka));
        break;
      case lt_linked:
        w_log(LL_AREAFIX, "%sfix: Linked areas list sent to %s", _AF, aka2str(link->hisAka));
        break;
      case lt_unlinked:
        w_log(LL_AREAFIX, "%sfix: Unlinked areas list sent to %s", _AF, aka2str(link->hisAka));
        break;
    }

    return report;
}

char *help(s_link *link) {
    FILE *f;
    int i=1;
    char *help;
    long endpos;
    char *hf = af_app->module == M_HTICK ? af_config->filefixhelp : af_config->areafixhelp;

    if (hf != NULL) {
        if ((f=fopen(hf, "r")) == NULL) {
            w_log (LL_ERR, "%sfix: Cannot open help file \"%s\": %s", _AF,
                   hf, strerror(errno));
	    if (!af_quiet)
                fprintf(stderr,"%sfix: Cannot open help file \"%s\": %s\n", _AF,
                        hf, strerror(errno));
	    return NULL;
	}
		
	fseek(f,0L,SEEK_END);
	endpos=ftell(f);

	help=(char*) (*call_smalloc)((size_t) endpos+1);

	fseek(f,0L,SEEK_SET);
	endpos = fread(help,1,(size_t) endpos,f);

	for (i=0; i<endpos; i++) if (help[i]=='\n') help[i]='\r';
	help[endpos]='\0';

	fclose(f);

        w_log(LL_AREAFIX, "%sfix: HELP sent to %s", _AF, link->name);

	return help;
    }
    return NULL;
}

int tag_mask(char *tag, char **mask, unsigned num) {
    unsigned int i;

    for (i = 0; i < num; i++) {
	if (patimat(tag,mask[i])) return 1;
    }

    return 0;
}

/* Process %avail command.
 *
 */
char *available(s_link *link, char *cmdline)
{
    FILE *f;
    unsigned int j=0, found;
    unsigned int k, rc;
    char *report = NULL, *line, *token, *running, linkAka[SIZE_aka2str];
    char *pattern;
    int reversed;
    s_link *uplink=NULL;
    ps_arealist al=NULL, *hal=NULL;
    unsigned int halcnt=0, isuplink;
    s_linkdata uplinkData;

    pattern = getPatternFromLine(cmdline, &reversed);

    if (af_app->module != M_HTICK) {
      if ((pattern) && (strlen(pattern)>60 || !isValidConference(pattern))) {
          w_log(LL_FUNC, "areafix::avail() FAILED (error request line)");
          return errorRQ(cmdline);
      }
    }

    for (j = 0; j < af_config->linkCount; j++)
    {
	uplink = af_config->links[j];
        (*call_getLinkData)(uplink, &uplinkData);

	found = 0;
	isuplink = 0;
	for (k = 0; k < link->numAccessGrp && uplink->LinkGrp; k++)
	    if (strcmp(link->AccessGrp[k], uplink->LinkGrp) == 0)
		found = 1;

        if ((uplinkData.fwd && uplinkData.fwdFile) && ((uplink->LinkGrp == NULL) || (found != 0)))
	{
            if ((f=fopen(uplinkData.fwdFile,"r")) == NULL) {
                w_log(LL_ERR, "%sfix: Cannot open forwardRequestFile \"%s\": %s", _AF,
                      uplinkData.fwdFile, strerror(errno));
 		continue;
	    }

	    isuplink = 1;

            if ((!hal)&&(link->availlist == AVAILLIST_UNIQUEONE))
                xscatprintf(&report, "Available Area List from all uplinks:\r");

            if ((!halcnt)||(link->availlist != AVAILLIST_UNIQUEONE))
            {
              halcnt++;
              hal = realloc(hal, sizeof(ps_arealist)*halcnt);
              hal[halcnt-1] = newAreaList(af_config);
              al = hal[halcnt-1];
              w_log(LL_DEBUGW,  __FILE__ ":%u: New item added to hal, halcnt = %u", __LINE__, halcnt);
            }

            while ((line = readLine(f)) != NULL)
            {
                line = trimLine(line);
                if (line[0] != '\0')
                {
                    running = line;
                    token = strseparate(&running, " \t\r\n");
                    rc = 0;

                    if (uplinkData.numDfMask)
                      rc |= tag_mask(token, uplinkData.dfMask, uplinkData.numDfMask);

                    if (uplinkData.denyFwdFile)
                      rc |= IsAreaAvailable(token, uplinkData.denyFwdFile, NULL, 0);

                    if (pattern)
                    {
                        /* if matches pattern and not reversed (or vise versa) */
                        if ((rc==0) &&(patimat(token, pattern)!=reversed))
                            addAreaListItem(al,0,0,1,1,0,token,running,uplink->LinkGrp);
                    } else
                    {
                        if (rc==0) addAreaListItem(al,0,0,1,1,0,token,running,uplink->LinkGrp);
                    }

    	        }
    	        nfree(line);
            }
            fclose(f);


            /*  warning! do not ever use aka2str twice at once! */
            sprintf(linkAka, "%s", aka2str(link->hisAka));
            w_log(LL_AREAFIX, "%sfix: Available Area List from %s %s to %s", _AF,
                  aka2str(uplink->hisAka),
                  (link->availlist == AVAILLIST_UNIQUEONE) ? "prepared": "sent",
                  linkAka );
        }


 	if ((link->availlist != AVAILLIST_UNIQUEONE)||(j==(af_config->linkCount-1)))
 	{
 		if((hal)&&((hal[halcnt-1])->count))
 		    if ((link->availlist != AVAILLIST_UNIQUE)||(isuplink))
 		    {
 		        sortAreaListNoDupes(halcnt, hal, link->availlist != AVAILLIST_FULL);
 		        if ((hal[halcnt-1])->count)
 		        {
 			    line = formatAreaList(hal[halcnt-1],78,NULL,(af_config->listEcho==lemGroup)||(af_config->listEcho==lemGroupName));
 			    if (link->availlist != AVAILLIST_UNIQUEONE)
 			        xscatprintf(&report, "\rAvailable Area List from %s:\r", aka2str(uplink->hisAka));
			    if (line)
 			        xstrscat(&report, "\r", line,print_ch(77,'-'), "\r", NULL);
 			    nfree(line);
	 	        }
 		    }

            if ((link->availlist != AVAILLIST_UNIQUE)||(j==(af_config->linkCount-1)))
              if (hal)
              {
  	        w_log(LL_DEBUGW,  __FILE__ ":%u: hal freed, (%u items)", __LINE__, halcnt);
                for(;halcnt>0;halcnt--)
                  freeAreaList(hal[halcnt-1]);
                nfree(hal);
              }
 	}
    }	

    if (report==NULL) {
	xstrcat(&report, "\r  No links for creating Available Area List\r");
        w_log(LL_AREAFIX, "%sfix: No links for creating Available Area List", _AF);
    }
    return report;
}


/*  subscribe if (act==0),  unsubscribe if (act==1), delete if (act==2) */
int forwardRequestToLink (char *areatag, s_link *uplink, s_link *dwlink, int act) {
    s_message *msg;
    char *base, pass[]="passthrough";
    s_linkdata uplinkData;

    if (!uplink) return -1;

    (*call_getLinkData)(uplink, &uplinkData);

    if (uplink->msg == NULL) {
	msg = makeMessage(uplink->ourAka, &(uplink->hisAka), af_config->sysop,
        uplinkData.robot, uplinkData.pwd, 1, uplinkData.attrs);
	msg->text = createKludges(af_config, NULL, uplink->ourAka, &(uplink->hisAka),
                              af_versionStr);
        if (uplinkData.flags)
            xstrscat(&(msg->text), "\001FLAGS ", uplinkData.flags, "\r",NULL);
	uplink->msg = msg;
    } else msg = uplink->msg;
	
    if (act==0) {
      if (af_app->module == M_HTICK) {
        if (getFileArea(areatag) == NULL) {
            if (af_config->filefixQueueFile) {
                af_CheckAreaInQuery(areatag, &(uplink->hisAka), &(dwlink->hisAka), ADDFREQ);
            }
            else {
                base = uplink->fileBaseDir;
                if (af_config->createFwdNonPass==0) uplink->fileBaseDir = pass;
                /*  create from own address */
                if (isOurAka(af_config,dwlink->hisAka)) uplink->fileBaseDir = base;
                strUpper(areatag);
                autoCreate(areatag, NULL, uplink->hisAka, &(dwlink->hisAka));
                uplink->fileBaseDir = base;
            }
        }
      } else {
        if (getArea(af_config, areatag) == &(af_config->badArea)) {
            if(af_config->areafixQueueFile) {
                af_CheckAreaInQuery(areatag, &(uplink->hisAka), &(dwlink->hisAka), ADDFREQ);
            }
            else {
                base = uplink->msgBaseDir;
                if (af_config->createFwdNonPass==0) uplink->msgBaseDir = pass;
                /*  create from own address */
                if (isOurAka(af_config,dwlink->hisAka)) uplink->msgBaseDir = base;
                strUpper(areatag);
                autoCreate(areatag, NULL, uplink->hisAka, &(dwlink->hisAka));
                uplink->msgBaseDir = base;
            }
        }
      }
    xstrscat(&msg->text, "+", areatag, "\r", NULL);
    } else if (act==1) {
        xscatprintf(&(msg->text), "-%s\r", areatag);
    } else {
        /*  delete area */
        if (uplinkData.advAfix)
            xscatprintf(&(msg->text), "~%s\r", areatag);
        else
            xscatprintf(&(msg->text), "-%s\r", areatag);
    }
    return 0;
}

char* GetWordByPos(char* str, UINT pos)
{
    UINT i = 0;
    while( i < pos)
    {
        while( *str &&  isspace(*str)) str++; /* skip spaces */
        i++;
        if( i == pos ) break;
        while( *str && !isspace(*str)) str++; /* skip word   */
        if( *str == '\0' ) return NULL;
    }
    return str; 
}

int changeconfig(char *fileName, s_area *area, s_link *link, int action) {
    char *cfgline=NULL, *token=NULL, *tmpPtr=NULL, *line=NULL, *buff=NULL;
    char *strbegfileName = fileName;
    long strbeg = 0, strend = -1;
    int rc=0;
    char *qf = af_app->module == M_HTICK ? af_config->filefixQueueFile : af_config->areafixQueueFile;

    e_changeConfigRet nRet = I_ERR;
    char *areaName = area->areaName;

    w_log(LL_FUNC, __FILE__ "::changeconfig(%s,...)", fileName);

    if (init_conf(fileName))
		return -1;

    w_log(LL_SRCLINE, __FILE__ ":%u:changeconfig() action=%i",__LINE__,action);

    while ((cfgline = configline()) != NULL) {
        line = sstrdup(cfgline);
        line = trimLine(line);
        line = stripComment(line);
        if (line[0] != 0) {
            line = shell_expand(line);
            line = tmpPtr = vars_expand(line);
            token = strseparate(&tmpPtr, " \t");
            if (stricmp(token, af_app->module == M_HTICK ? "filearea" : "echoarea")==0) {
                token = strseparate(&tmpPtr, " \t");
                if (*token=='\"' && token[strlen(token)-1]=='\"' && token[1]) {
                    token++;
                    token[strlen(token)-1]='\0';
                }
                if (stricmp(token, areaName)==0) {
                    fileName = (*call_sstrdup)(getCurConfName());
                    strend = get_hcfgPos();
                    if (strcmp(strbegfileName, fileName) != 0) strbeg = 0;
                    break;
                }
            }
        }
        strbeg = get_hcfgPos();
        strbegfileName = (*call_sstrdup)(getCurConfName());
        w_log(LL_DEBUGF, __FILE__ ":%u:changeconfig() strbeg=%ld", __LINE__, strbeg);
        nfree(line);
        nfree(cfgline);
    }
    close_conf();
    nfree(line);
    if (strend == -1) { /*  impossible   error occurred */
        nfree(cfgline);
        nfree(fileName);
        return -1;
    }

    switch (action) {
    case 0: /*  forward Request To Link */
        if ((area->msgbType==MSGTYPE_PASSTHROUGH) &&
            !qf &&
            (area->downlinkCount==1) &&
            (area->downlinks[0]->link->hisAka.point == 0))
        {
            forwardRequestToLink(areaName, area->downlinks[0]->link, NULL, 0);
        }
    case 3: /*  add link to existing area */
        xscatprintf(&cfgline, " %s", aka2str(link->hisAka));
        nRet = ADD_OK;
        break;
    case 10: /*  add link as defLink to existing area */
        xscatprintf(&cfgline, " %s -def", aka2str(link->hisAka));
        nRet = ADD_OK;
        break;
    case 1: /*  remove link from area */
        if ((area->msgbType==MSGTYPE_PASSTHROUGH)
            && (area->downlinkCount==1) &&
            (area->downlinks[0]->link->hisAka.point == 0)) {
            forwardRequestToLink(areaName, area->downlinks[0]->link, NULL, 1);
        }
    case 7:
        if ((rc = DelLinkFromString(cfgline, link->hisAka)) == 1) {
            w_log(LL_ERR,"%sfix: Unlink is not possible for %s from %s area \'%s\'", _AF,
                aka2str(link->hisAka), af_app->module == M_HTICK ? "file" : "echo", areaName);
            nRet = O_ERR;
        } else {
            nRet = DEL_OK;
        }
        break;
    case 2:
        /* makepass(f, fileName, areaName); */
    case 4: /*  delete area */
        nfree(cfgline);
        nRet = DEL_OK;
        break;
    case 5: /*  subscribe us to  passthrough */
        w_log(LL_SRCLINE, __FILE__ "::changeconfig():%u",__LINE__);

        tmpPtr = fc_stristr(cfgline,"passthrough");
        if ( tmpPtr )  {
            char* msgbFileName = makeMsgbFileName(af_config, area->areaName);
            char* bDir = af_app->module != M_HTICK ? af_config->msgBaseDir : NULL;
            char* areaname = (*call_sstrdup)(area->areaName);

            /*  translating name of the area to lowercase/uppercase */
            if (af_config->createAreasCase == eUpper) strUpper(areaname);
            else strLower(areaname);

            /*  translating filename of the area to lowercase/uppercase */
            if (af_config->areasFileNameCase == eUpper) strUpper(msgbFileName);
            else strLower(msgbFileName);

            if (af_app->module == M_HTICK) {
              char *tmp = NULL;
              bDir = link->fileBaseDir ? link->fileBaseDir : af_config->fileAreaBaseDir;

              if (link->autoFileCreateSubdirs) {
                  char *cp;
                  for (cp = msgbFileName; *cp; cp++)
                      if (*cp == '.') *cp = PATH_DELIM;
              }

              xscatprintf(&tmp, "%s%s", bDir, msgbFileName);
              if (_createDirectoryTree(tmp))
              {
                  w_log(LL_ERROR, "cannot make all subdirectories for %s\n", msgbFileName);
                  nfree(tmp);
                  w_log(LL_FUNC, "%s::changeconfig() rc=1", __FILE__);
                  return 1;
              }
#if defined (__UNIX__)
              if (af_config->fileAreaCreatePerms && chmod(tmp, af_config->fileAreaCreatePerms))
                  w_log(LL_ERR, "Cannot chmod() for newly created filearea directory '%s': %s",
                        sstr(tmp), strerror(errno));
#endif
              nfree(tmp);
            }

            *(--tmpPtr) = '\0';
            xstrscat(&buff, cfgline, " ", bDir, msgbFileName, tmpPtr+12, NULL);
            nfree(cfgline);
            cfgline = buff;

            nfree(msgbFileName);
        } else  {
            tmpPtr = fc_stristr(cfgline,"-pass");
            *(--tmpPtr) = '\0';
            xstrscat(&buff, cfgline, tmpPtr+6 , NULL);
            nfree(cfgline);
            cfgline = buff;
        }
        nRet = ADD_OK;
        break;
    case 6: /*  make area pass. */
        tmpPtr = GetWordByPos(cfgline,4);
        if ( tmpPtr ) {
            *(--tmpPtr) = '\0';
            xstrscat(&buff, cfgline, " -pass ", ++tmpPtr , NULL);
            nfree(cfgline);
            cfgline = buff;
        } else {
            xstrcat(&cfgline, " -pass");
        }
        nRet = DEL_OK;
        break;
    case 8: /*  make area paused. */
        tmpPtr = GetWordByPos(cfgline,4);
        if ( tmpPtr ) {
            *(--tmpPtr) = '\0';
            xstrscat(&buff, cfgline, " -paused ", ++tmpPtr , NULL);
            nfree(cfgline);
            cfgline = buff;
        } else {
            xstrcat(&cfgline, " -paused");
        }
        nRet = ADD_OK;
        break;
    case 9: /*  make area not paused */
        tmpPtr = fc_stristr(cfgline,"-paused");
        *(--tmpPtr) = '\0';
        xstrscat(&buff, cfgline, tmpPtr+8 , NULL);
        nfree(cfgline);
        cfgline = buff;
        nRet = ADD_OK;
        break;
    default: break;
    } /*  switch (action) */

    w_log(LL_DEBUGF, __FILE__ ":%u:changeconfig() call InsertCfgLine(\"%s\",<cfgline>,%ld,%ld)", __LINE__, fileName, strbeg, strend);
    InsertCfgLine(fileName, cfgline, strbeg, strend);
    nfree(cfgline);
    nfree(fileName);

    w_log(LL_FUNC, __FILE__ "::changeconfig() rc=%i", nRet);
    return nRet;
}

static int compare_links_priority(const void *a, const void *b) {
    int ia = *((int*)a);
    int ib = *((int*)b);
    unsigned int pa, pb;
    if (af_app->module == M_HTICK) {
      pa = af_config->links[ia]->forwardFilePriority;
      pb = af_config->links[ib]->forwardFilePriority;
    } else {
      pa = af_config->links[ia]->forwardAreaPriority;
      pb = af_config->links[ib]->forwardAreaPriority; 
    }
    if (pa < pb) return -1;
    else if(pa > pb) return 1;
    else return 0;
}

int forwardRequest(char *areatag, s_link *dwlink, s_link **lastRlink) {
    unsigned int i=0, rc = 1;
    s_link *uplink=NULL;
    int *Indexes=NULL;
    unsigned int Requestable = 0;
    s_linkdata uplinkData;

    /* From Lev Serebryakov -- sort Links by priority */
    Indexes = (*call_smalloc)(sizeof(int)*af_config->linkCount);
    for (i = 0; i < af_config->linkCount; i++) {
	if ( (af_app->module != M_HTICK && af_config->links[i]->forwardRequests) ||
             (af_app->module == M_HTICK && af_config->links[i]->forwardFileRequests)
           ) Indexes[Requestable++] = i;
    }
    qsort(Indexes,Requestable,sizeof(Indexes[0]),compare_links_priority);
    i = 0;
    if(lastRlink) { /*  try to find next requestable uplink */
        for (; i < Requestable; i++) {
            uplink = af_config->links[Indexes[i]];
            if( addrComp(uplink->hisAka, (*lastRlink)->hisAka) == 0)
            {   /*  we found lastRequestedlink */
                i++;   /*  let's try next link */
                break;
            }
        }
    }

    for (; i < Requestable; i++) {
        uplink = af_config->links[Indexes[i]];
        (*call_getLinkData)(uplink, &uplinkData);

        if(lastRlink) *lastRlink = uplink;

        if (uplinkData.fwd && (uplink->LinkGrp) ?
            grpInArray(uplink->LinkGrp,dwlink->AccessGrp,dwlink->numAccessGrp) : 1)
        {
            /* skip downlink from list of uplinks */
            if(addrComp(uplink->hisAka, dwlink->hisAka) == 0)
            {
                rc = 2;
                continue;
            }
            if ( (uplinkData.numDfMask) &&
                 (tag_mask(areatag, uplinkData.dfMask, uplinkData.numDfMask)))
            {
                rc = 2;
                continue;
            }
            if ( (uplinkData.denyFwdFile!=NULL) &&
                 (IsAreaAvailable(areatag,uplinkData.denyFwdFile,NULL,0)))
            {
                rc = 2;
                continue;
            }
            rc = 0;
            if (uplinkData.fwdFile != NULL) {
                /*  first try to find the areatag in forwardRequestFile */
                if (tag_mask(areatag, uplinkData.frMask, uplinkData.numFrMask) ||
                    IsAreaAvailable(areatag, uplinkData.fwdFile, NULL, 0))
                {
                    break;
                }
                else
                { rc = 2; }/*  found link with freqfile, but there is no areatag */
            } else {
                if (uplinkData.numFrMask) /*  found mask */
                {
                    if (tag_mask(areatag, uplinkData.frMask, uplinkData.numFrMask))
                        break;
                    else rc = 2;
                } else { /*  unconditional forward request */
                    if (dwlink->denyUFRA==0)
                        break;
                    else rc = 2;
                }
            }/* (uplinkData.fwdFile != NULL) */

        }/*  if (uplinkData.fwd && (uplink->LinkGrp) ? */
    }/*  for (i = 0; i < Requestable; i++) { */

    if(rc == 0)
        forwardRequestToLink(areatag, uplink, dwlink, 0);

    nfree(Indexes);
    return rc;
}

int isPatternLine(char *s) {
    if (strchr(s,'*') || strchr(s,'?')) return 1;
    return 0;
}

void fixRules (s_link *link, char *area) {
    char *fileName = NULL;

    if (!af_config->rulesDir) return;
    if (link->noRules) return;

    xscatprintf(&fileName, "%s%s.rul", af_config->rulesDir, strLower(makeMsgbFileName(af_config, area)));

    if (fexist(fileName)) {
        rulesCount++;
        rulesList = (*call_srealloc) (rulesList, rulesCount * sizeof (char*));
        rulesList[rulesCount-1] = (*call_sstrdup) (area);
        /*  don't simply copy pointer because area may be */
        /*  removed while processing other commands */
    }
    nfree (fileName);
}

char *subscribe(s_link *link, char *cmd) {
    unsigned int i, rc=4, found=0, matched=0, cnt;
    char *line, *an=NULL, *report = NULL;
    s_area *area=NULL;
    int pause = af_app->module == M_HTICK ? FILEAREA : ECHOAREA;
    char *qf = af_app->module == M_HTICK ? af_config->filefixQueueFile : af_config->areafixQueueFile;

    w_log(LL_FUNC, "%s::subscribe(...,%s)", __FILE__, cmd);

    line = cmd;
	
    if (line[0]=='+') line++;
    while (*line==' ') line++;

    if (*line=='+') line++; while (*line==' ') line++;
	
    if (af_app->module != M_HTICK) {
      if (strlen(line)>60 || !isValidConference(line)) {
        report = errorRQ(line);
        w_log(LL_FUNC, "%s::subscribe() FAILED (error request line) rc=%s", __FILE__, report);
        return report;
      }
    }

    cnt = af_app->module == M_HTICK ? af_config->fileAreaCount : af_config->echoAreaCount;
    for (i=0; !found && rc!=6 && i<cnt; i++) {
	area = af_app->module == M_HTICK ? &(af_config->fileAreas[i]) : &(af_config->echoAreas[i]);
	an = area->areaName;

	rc=subscribeAreaCheck(area, line, link);
	if (rc==4) continue;        /* not match areatag, try next */
	if (rc==1 && manualCheck(area, link)) rc = 5; /* manual area/group/link */

	if (rc!=0 && limitCheck(link)) rc = 6; /* areas limit exceed for link */

        switch (rc) {
	case 0:         /* already linked */
	    if (isPatternLine(line)) {
		matched = 1;
	    } else {
		xscatprintf(&report, " %s %s  already linked\r",
			    an, print_ch(49-strlen(an), '.'));
		w_log(LL_AREAFIX, "%sfix: %s already linked to area \'%s\'", _AF,
		      aka2str(link->hisAka), an);
		i = cnt;
	    }
	    break;
	case 1:         /* not linked */
        if( isOurAka(af_config,link->hisAka)) {
           if(area->msgbType==MSGTYPE_PASSTHROUGH) {
              int state =
                  changeconfig(af_cfgFile?af_cfgFile:getConfigFileName(),area,link,5);
              if( state == ADD_OK) {
                  af_CheckAreaInQuery(an, NULL, NULL, DELIDLE);
                  xscatprintf(&report," %s %s  added\r",an,print_ch(49-strlen(an),'.'));
                  w_log(LL_AREAFIX, "%sfix: %s subscribed to area \'%s\'", _AF, aka2str(link->hisAka), an);
                  if ((af_config->autoAreaPause & pause) && area->paused)
                      pauseAreas(1, NULL, area);
              } else {
                  xscatprintf(&report, " %s %s  not subscribed\r", an, print_ch(49-strlen(an), '.'));
                  w_log(LL_AREAFIX, "%sfix: %s not subscribed to area \'%s\', cause uplink", _AF, aka2str(link->hisAka), an);
                  w_log(LL_AREAFIX, "%sfix: %s has \"passthrough\" in \"autoAreaCreateDefaults\" for area \'%s\'", _AF,
                                    an, aka2str(area->downlinks[0]->link->hisAka));
              }
           } else {  /* ??? (not passthrou echo) */
                     /*   non-passthrough area for our aka means */
                     /*   that we already linked to this area */
               xscatprintf(&report, " %s %s  already linked\r", an, print_ch(49-strlen(an), '.'));
               w_log(LL_AREAFIX, "%sfix: %s already linked to area \'%s\'", _AF, aka2str(link->hisAka), an);
           }
        } else {
            if (changeconfig(af_cfgFile?af_cfgFile:getConfigFileName(),area,link,0)==ADD_OK) {
                Addlink(af_config, link, area);
                if (af_app->module != M_HTICK) fixRules(link, area->areaName);
                xscatprintf(&report," %s %s  added\r", an, print_ch(49-strlen(an),'.'));
                w_log(LL_AREAFIX, "%sfix: %s subscribed to area \'%s\'", _AF, aka2str(link->hisAka), an);
                if ((af_config->autoAreaPause & pause) && area->paused && (link->Pause & pause))
                    pauseAreas(1, link, area);
                af_CheckAreaInQuery(an, NULL, NULL, DELIDLE);
                if (af_send_notify)
                    forwardRequestToLink(area->areaName, link, NULL, 0);
            } else {
                xscatprintf(&report," %s %s  error. report to sysop!\r", an, print_ch(49-strlen(an),'.'));
                w_log(LL_AREAFIX, "%sfix: %s not subscribed to area \'%s\'", _AF, aka2str(link->hisAka), an);
                w_log(LL_ERR, "%sfix: Can't write to af_config file: %s!", _AF, strerror(errno));
            }/* if (changeconfig(af_cfgFile?af_cfgFile:getConfigFileName(),area,link,3)==0) */
        }
	    if (!isPatternLine(line)) i = cnt;
	    break;
	case 6:         /* areas limit exceed for link */
            break;
	default : /*  rc = 2  not access */
	    if (!area->hide && !isPatternLine(line)) {
		w_log(LL_AREAFIX, "%sfix: %s no access for area \'%s\'", _AF,
		      an, aka2str(link->hisAka));
		xscatprintf(&report," %s %s  no access\r", an,
			    print_ch(49-strlen(an), '.'));
		found=1;
	    }
	    if (area->hide && !isPatternLine(line)) found=1;
	    break;
	}
    }

    if (rc!=0 && limitCheck(link)) rc = 6; /*double!*/ /* areas limit exceed for link */

    if (rc==4 && !isPatternLine(line) && !found) { /* rc not equal 4 there! */
        if (checkRefuse(line))
        {
            xscatprintf(&report, " %s %s  forwarding refused\r",
			    line, print_ch(49-strlen(line), '.'));
            w_log(LL_WARN, "Can't forward request for %sarea \'%s\' : refused by New%sAreaRefuseFile\n", 
                  af_app->module == M_HTICK ? "file" : "", line, 
                  af_app->module == M_HTICK ? "File" : "");
        } else
        if (link->denyFRA==0) {
            s_query_areas *node = NULL;
            /* check if area is already requested */
            if (qf && (node = af_CheckAreaInQuery(line,NULL,NULL,FINDFREQ)) != NULL) {
                af_CheckAreaInQuery(line, &(node->downlinks[0]), &(link->hisAka), ADDFREQ);
		xscatprintf(&report, " %s %s  request forwarded\r",
			    line, print_ch(49-strlen(line), '.'));
                w_log(LL_AREAFIX, "%sfix: Area \'%s\' is already requested at %s", _AF, line, aka2str(node->downlinks[0]));
	    }
	    /*  try to forward request */
            else if ((rc=forwardRequest(line, link, NULL))==2) {
		xscatprintf(&report, " %s %s  no uplinks to forward\r",
			    line, print_ch(49-strlen(line), '.'));
		w_log(LL_AREAFIX, "%sfix: No uplinks to forward area \'%s\'", _AF, line);
	    }
	    else if (rc==0) {
		xscatprintf(&report, " %s %s  request forwarded\r",
			    line, print_ch(49-strlen(line), '.'));
		w_log(LL_AREAFIX, "%sfix: Request forwarded to area \'%s\'", _AF, line);
        if (!qf && isOurAka(af_config,link->hisAka)==0)
        {
            area = af_app->module == M_HTICK ? getFileArea(line) : getArea(af_config, line);
            if ( !isLinkOfArea(link, area) ) {
                if(changeconfig(af_cfgFile?af_cfgFile:getConfigFileName(),area,link,3)==ADD_OK) {
                    Addlink(af_config, link, area);
                    if (af_app->module != M_HTICK) fixRules(link, area->areaName);
                    if ((af_config->autoAreaPause & pause) && (link->Pause & pause))
                        pauseAreas(0, link, area);
                    w_log(LL_AREAFIX, "%sfix: %s subscribed to area \'%s\'", _AF,
                        aka2str(link->hisAka),line);
                } else {
                    xscatprintf( &report," %s %s  error. report to sysop!\r",
                        an, print_ch(49-strlen(an),'.') );
                    w_log(LL_AREAFIX, "%sfix: %s not subscribed to area \'%s\'", _AF,
                        aka2str(link->hisAka),an);
                    w_log(LL_ERR, "%sfix: Can't change af_config file: %s!", _AF, strerror(errno));
                }
            } else w_log(LL_AREAFIX, "%sfix: %s already subscribed to area \'%s\'", _AF,
                aka2str(link->hisAka), line );

        } else {
            if (af_app->module != M_HTICK) fixRules (link, line);
        }
        }
	}
    }

    if (rc == 6) {   /* areas limit exceed for link */
	w_log(LL_AREAFIX, "%sfix: %sarea \'%s\' no access (full limit) for %s", _AF,
              af_app->module == M_HTICK ? "file" : "",
	      line, aka2str(link->hisAka));
	xscatprintf(&report," %s %s  no access (full limit)\r",
		    line, print_ch(49-strlen(line), '.'));
    }

    if (matched) {
	if (report == NULL)
	    w_log(LL_AREAFIX, "%sfix: All %sareas matching %s are already linked", _AF, 
                  af_app->module == M_HTICK ? "file" : "", line);
	xscatprintf(&report, "All %sareas matching %s are already linked\r", report ? "other " : "", line);
    }
    else if ((report == NULL && found==0) || (found && area->hide)) {
	xscatprintf(&report," %s %s  not found\r",line,print_ch(49-strlen(line),'.'));
	w_log(LL_AREAFIX, "%sfix: Not found area \'%s\'", _AF, line);
    }
    w_log(LL_FUNC, "%sfix::subscribe() OK", _AF);

    /* update perl vars */
    /* al: todo: rewrite function and place this call to the write place */
    if (hook_onConfigChange) (*hook_onConfigChange)(PERL_CONF_AREAS);

    return report;
}

char *errorRQ(char *line) {
    char *report = NULL;

    if (strlen(line)>48) {
	xstrscat(&report, " ", line, " .......... error line\r", NULL);
    }
    else xscatprintf(&report, " %s %s  error line\r",
		    line, print_ch(49-strlen(line),'.'));
    return report;
}

char *do_delete(s_link *link, s_area *area) {
    char *report = NULL, *an = area->areaName;
    unsigned int i = 0, *cnt;
    ps_area areas;

    if(!link)
    {
        link = getLinkFromAddr(af_config, *area->useAka);
        while( !link && i < af_config->addrCount )
        {
            link = getLinkFromAddr( af_config, af_config->addr[i] );
            i++;
        }
        if(!link) return NULL;
    }
    /* unsubscribe from downlinks */
    xscatprintf(&report, " %s %s  deleted\r", an, print_ch(49-strlen(an), '.'));
    for (i=0; i<area->downlinkCount; i++) {
	if (addrComp(area->downlinks[i]->link->hisAka, link->hisAka)) {

            s_link *dwlink = area->downlinks[i]->link;

	    if (dwlink->unsubscribeOnAreaDelete)
                forwardRequestToLink(an, dwlink, NULL, 2);

            if (dwlink->sendNotifyMessages) {
                s_message *tmpmsg = NULL;
                s_linkdata linkData;
                char *from;

                (*call_getLinkData)(dwlink, &linkData);

                if (af_app->module == M_HTICK)
                    from = af_config->filefixFromName ? af_config->filefixFromName : af_versionStr;
                else
                    from = af_config->areafixFromName ? af_config->areafixFromName : af_versionStr;

                tmpmsg = makeMessage(dwlink->ourAka, &(dwlink->hisAka),
                    from, dwlink->name, "Notification message", 1, linkData.attrs);
                tmpmsg->text = createKludges(af_config, NULL, dwlink->ourAka,
                    &(dwlink->hisAka), af_versionStr);
                if (linkData.flags)
                    xstrscat(&(tmpmsg->text), "\001FLAGS ", linkData.flags, "\r", NULL);

                xscatprintf(&tmpmsg->text, "\r Area \'%s\' is deleted.\r", area->areaName);
                    xstrcat(&tmpmsg->text, "\r Do not forget to remove it from your configs.\r");
                xscatprintf(&tmpmsg->text, "\r\r--- %s %sfix\r", af_versionStr, _AF);

                tmpmsg->textLength = strlen(tmpmsg->text);
                (*call_sendMsg)(tmpmsg);
                nfree(tmpmsg);
                w_log( LL_AREAFIX, "%sfix: Write notification msg for %s", _AF, aka2str(dwlink->hisAka));
            }
        }
    }
    /* remove area from config-file */
    if( changeconfig ((af_cfgFile) ? af_cfgFile : getConfigFileName(),  area, link, 4) != DEL_OK) {
       w_log(LL_AREAFIX, "%sfix: Can't remove area from af_config: %s", _AF, strerror(errno));
    }

    /* delete msgbase and dupebase for the area */

    /*
    if (area->msgbType!=MSGTYPE_PASSTHROUGH)
	MsgDeleteBase(area->fileName, (word) area->msgbType);
    */

    if (hook_onDeleteArea) hook_onDeleteArea(link, area); // new!!!

    w_log(LL_AREAFIX, "%sfix: %s deleted by %s", _AF,
                  an, aka2str(link->hisAka));

    /* delete the area from in-core af_config */
    cnt   = af_app->module == M_HTICK ? &af_config->fileAreaCount : &af_config->echoAreaCount;
    areas = af_app->module == M_HTICK ? af_config->fileAreas : af_config->echoAreas;
    for (i = 0; i < (*cnt); i++) {
        if (stricmp(areas[i].areaName, an) == 0) break;
    }
    if (i < (*cnt) && area == &(areas[i])) {
        fc_freeEchoArea(area);
        for (; i < (*cnt)-1; i++)
            memcpy(&(areas[i]), &(areas[i+1]), sizeof(s_area));
        (*cnt)--;
        RebuildEchoAreaTree(af_config);
        /* update perl vars */
        if (hook_onConfigChange) (*hook_onConfigChange)(PERL_CONF_AREAS);
    }
    return report;
}

char *delete(s_link *link, char *cmd) {
    int rc;
    char *line, *report = NULL, *an;
    s_area *area;

    w_log(LL_FUNC, __FILE__ "::delete(...,%s)", cmd);

    for (line = cmd + 1; *line == ' ' || *line == '\t'; line++);

    if (*line == 0) return errorRQ(cmd);

    area = af_app->module == M_HTICK ? getFileArea(line) : getArea(af_config, line);
    if ((af_app->module == M_HTICK && !area) || area == &(af_config->badArea)) {
	xscatprintf(&report, " %s %s  not found\r", line, print_ch(49-strlen(line), '.'));
	w_log(LL_AREAFIX, "%sfix: Not found area \'%s\'", _AF, line);
	return report;
    }
    rc = subscribeCheck(area, link);
    an = area->areaName;

    switch (rc) {
    case 0:
	break;
    case 1:
	xscatprintf(&report, " %s %s  not linked\r", an, print_ch(49-strlen(an), '.'));
	w_log(LL_AREAFIX, "%sfix: Area \'%s\' is not linked to %s", _AF,
	      an, aka2str(link->hisAka));
	return report;
    case 2:
	xscatprintf(&report, " %s %s  no access\r", an, print_ch(49-strlen(an), '.'));
	w_log(LL_AREAFIX, "%sfix: Area \'%s\' no access for %s", _AF, an, aka2str(link->hisAka));
	return report;
    }
    if (link->LinkGrp == NULL || (area->group && strcmp(link->LinkGrp, area->group))) {
	xscatprintf(&report, " %s %s  delete not allowed\r",
		    an, print_ch(49-strlen(an), '.'));
	w_log(LL_AREAFIX, "%sfix: Area \'%s\' delete not allowed for %s", _AF,
	      an, aka2str(link->hisAka));
	return report;
    }
    return do_delete(link, area);
}

char *unsubscribe(s_link *link, char *cmd) {
    unsigned int i, rc = 2, j=(unsigned int)I_ERR, from_us=0, matched = 0, cnt;
    char *line, *an, *report = NULL;
    s_area *area;
    char *qf = af_app->module == M_HTICK ? af_config->filefixQueueFile : af_config->areafixQueueFile;
    int pause = af_app->module == M_HTICK ? FILEAREA : ECHOAREA;

    w_log(LL_FUNC,__FILE__ ":%u:unsubscribe() begin", __LINE__);
    line = cmd;
	
    if (line[1]=='-') return NULL;
    line++;
    while (*line==' ') line++;

    cnt = af_app->module == M_HTICK ? af_config->fileAreaCount : af_config->echoAreaCount;
    for (i = 0; i < cnt; i++) {
        area = af_app->module == M_HTICK ? &(af_config->fileAreas[i]) : &(af_config->echoAreas[i]);
        an = area->areaName;

        rc = subscribeAreaCheck(area, line, link);
        if (rc==4) continue;
        if (rc==0 && mandatoryCheck(area,link)) rc = 5;

        if (isOurAka(af_config,link->hisAka))
        {
            from_us = 1;
            rc = area->msgbType == MSGTYPE_PASSTHROUGH ? 1 : 0 ;
        }

        switch (rc) {
        case 0:
            if (from_us == 0) {
                unsigned int k;
                for (k=0; k<area->downlinkCount; k++)
                    if (addrComp(link->hisAka, area->downlinks[k]->link->hisAka)==0 &&
                        area->downlinks[k]->defLink)
                        return do_delete(link, area);
                    RemoveLink(link, area);
                    if ((area->msgbType == MSGTYPE_PASSTHROUGH) &&
                        (area->downlinkCount == 1) &&
                        (area->downlinks[0]->link->hisAka.point == 0))
                    {
                        if (qf)
                        {
                            af_CheckAreaInQuery(an, &(area->downlinks[0]->link->hisAka), NULL, ADDIDLE);
                            j = changeconfig(af_cfgFile?af_cfgFile:getConfigFileName(),area,link,7);
                        }
                        else
                        {
                            j = changeconfig(af_cfgFile?af_cfgFile:getConfigFileName(),area,link,1);
                        }
                    }
                    else
                    {
                        j = changeconfig(af_cfgFile?af_cfgFile:getConfigFileName(),area,link,7);
                        if (j == DEL_OK && (af_config->autoAreaPause & pause) && !area->paused && !(link->Pause & pause))
                            pauseAreas(0,NULL,area);
                    }
                    if (j != DEL_OK) {
                        w_log(LL_AREAFIX, "%sfix: %s doesn't unlinked from area \'%s\'", _AF,
                            aka2str(link->hisAka), an);
                    } else {
                        w_log(LL_AREAFIX,"%sfix: %s unlinked from area \'%s\'", _AF, aka2str(link->hisAka),an);
                        if (af_send_notify)
                            forwardRequestToLink(area->areaName,link, NULL, 1);
                    }
            } else { /*  unsubscribing from own address - set area passtrough */
                if (area->downlinkCount==0)
                {
                    return do_delete(getLinkFromAddr(af_config,*(area->useAka)), area);
                }
                else if ( (area->downlinkCount==1) &&
                          (area->downlinks[0]->link->hisAka.point == 0 ||
                           area->downlinks[0]->defLink) ) {
                    if (qf) {
                        af_CheckAreaInQuery(an, &(area->downlinks[0]->link->hisAka), NULL, ADDIDLE);
                    } else {
                        forwardRequestToLink(area->areaName,
                            area->downlinks[0]->link, NULL, 1);
                    }
                } else if (af_config->autoAreaPause && !area->paused) {
                       area->msgbType = MSGTYPE_PASSTHROUGH;
                       pauseAreas(0,NULL,area);
                }
                j = changeconfig(af_cfgFile?af_cfgFile:getConfigFileName(),area,link,6);
/*                if ( (j == DEL_OK) && area->msgbType!=MSGTYPE_PASSTHROUGH ) */
                if (j == DEL_OK) switch (af_app->module) {
                  case M_HTICK:
                    if (af_config->autoAreaPause & FILEAREA)
                      pauseAreas(0, NULL, area);
                    break;
                  default:
                    if (area->fileName && area->killMsgBase)
                       MsgDeleteBase(area->fileName, (word) area->msgbType);
                }
            }
            if (j == DEL_OK){
                xscatprintf(&report," %s %s  unlinked\r",an,print_ch(49-strlen(an),'.'));
            }else
                xscatprintf(&report," %s %s  error. report to sysop!\r",
                                         an, print_ch(49-strlen(an),'.') );
            break;
        case 1:
            if (isPatternLine(line)) {
                matched = 1;
                continue;
            }
            if (area->hide) {
                i = cnt;
                break;
            }
            xscatprintf(&report, " %s %s  not linked\r",
                an, print_ch(49-strlen(an), '.'));
            w_log(LL_AREAFIX, "%sfix: Area \'%s\' is not linked to %s", _AF,
                area->areaName, aka2str(link->hisAka));
            break;
        case 5:
            xscatprintf(&report, " %s %s  unlink is not possible\r",
                an, print_ch(49-strlen(an), '.'));
            w_log(LL_AREAFIX, "%sfix: Area \'%s\' unlink is not possible for %s", _AF,
                area->areaName, aka2str(link->hisAka));
            break;
        default:
            break;
        }
    }
    if (qf)
        report = af_Req2Idle(line, report, link->hisAka);
    if (report == NULL) {
        if (matched) {
            xscatprintf(&report, " %s %s  no %sareas to unlink\r",
                line, print_ch(49-strlen(line), '.'),
                af_app->module == M_HTICK ? "file" : "");
            w_log(LL_AREAFIX, "%sfix: No areas to unlink", _AF);
        } else {
            xscatprintf(&report, " %s %s  not found\r",
                line, print_ch(49-strlen(line), '.'));
            w_log(LL_AREAFIX, "%sfix: Area \'%s\' is not found", _AF, line);
        }
    }
    w_log(LL_FUNC,__FILE__ ":%u:unsubscribe() end", __LINE__);

    /* update perl vars */
    /* al: todo: rewrite function and place this call to the write place */
    if (hook_onConfigChange) (*hook_onConfigChange)(PERL_CONF_AREAS);

    return report;
}

/* if act==0 pause area, if act==1 unpause area */
/* returns 0 if no messages to links were created */
int pauseAreas(int act, s_link *searchLink, s_area *searchArea) {
  unsigned int i, j, linkCount, cnt;
  unsigned int rc = 0;
  int pause = af_app->module == M_HTICK ? FILEAREA : ECHOAREA;

  if (!searchLink && !searchArea) return rc;

  cnt = af_app->module == M_HTICK ? af_config->fileAreaCount : af_config->echoAreaCount;
  for (i = 0; i < cnt; i++) {
    s_link *uplink = NULL;
    s_area *area;
    s_message *msg;

    area = af_app->module == M_HTICK ? &(af_config->fileAreas[i]) : &(af_config->echoAreas[i]);
    if (act==0 && (area->paused || area->noautoareapause || area->msgbType!=MSGTYPE_PASSTHROUGH)) continue;
    if (act==1 && (!area->paused)) continue;
    if (searchArea && searchArea != area) continue;
    if (searchLink && isLinkOfArea(searchLink, area)!=1) continue;

    linkCount = 0;
    for (j=0; j < area->downlinkCount; j++) { /* try to find uplink */
      if ( !(area->downlinks[j]->link->Pause & pause) &&
           (!searchLink || addrComp(searchLink->hisAka, area->downlinks[j]->link->hisAka)!=0) ) {
        linkCount++;
        if (area->downlinks[j]->defLink) uplink = area->downlinks[j]->link;
      }
    }

    if (linkCount!=1 || !uplink) continue; /* uplink not found */

    /* change af_config */
    if (act==0) { /* make area paused */
      if (changeconfig(af_cfgFile?af_cfgFile:getConfigFileName(),area,NULL,8)==ADD_OK) {
        w_log(LL_AREAFIX, "%sfix: Area \'%s\' is paused (uplink: %s)", _AF,
                 area->areaName, aka2str(uplink->hisAka));
      } else {
        w_log(LL_AREAFIX, "%sfix: Error pausing area \'%s\'", _AF, area->areaName);
        continue;
      }
    } else if (act==1) { /* make area non paused */
      if (changeconfig(af_cfgFile?af_cfgFile:getConfigFileName(),area,NULL,9)==ADD_OK) {
        w_log(LL_AREAFIX, "%sfix: Area \'%s\' is not paused any more (uplink: %s)", _AF,
                 area->areaName, aka2str(uplink->hisAka));
      } else {
        w_log(LL_AREAFIX, "%sfix: Error unpausing area \'%s\'", _AF, area->areaName);
        continue;
      }
    }

    /* write messages */
    if (uplink->msg == NULL) {
      s_linkdata uplinkData;

      (*call_getLinkData)(uplink, &uplinkData);

      msg = makeMessage(uplink->ourAka, &(uplink->hisAka), af_config->sysop,
                        uplinkData.robot, uplinkData.pwd, 1, uplinkData.attrs);
      msg->text = createKludges(af_config, NULL, uplink->ourAka, &(uplink->hisAka),
                      af_versionStr);
      if (uplinkData.flags)
        xstrscat(&(msg->text), "\001FLAGS ", uplinkData.flags, "\r",NULL);
      uplink->msg = msg;
    } else msg = uplink->msg;

    if (act==0)
      xscatprintf(&(msg->text), "-%s\r", area->areaName);
    else if (act==1)
      xscatprintf(&(msg->text), "+%s\r", area->areaName);

    rc = 1;
  }

  return rc;
}

/* mode = 0 to make passive, 1 to make active */
char *pause_resume_link(s_link *link, int mode)
{
   char *tmp, *report = NULL;
   int state, pause;

   pause = af_app->module == M_HTICK ? FILEAREA : ECHOAREA;
   state = (link->Pause & pause) ? 0 : 1; /* 0 = passive, 1 = active */

   if (state != mode) {
      unsigned int i, j, areaCount = 0;
      ps_area areas = NULL;

      if (Changepause(af_cfgFile ? af_cfgFile : getConfigFileName(), link, 0, pause) == 0)
         return NULL;

      if (af_app->module == M_HPT)
      {
          areaCount = af_config->echoAreaCount;
          areas     = af_config->echoAreas;
      }
      else if (af_app->module == M_HTICK)
      {
          areaCount = af_config->fileAreaCount;
          areas     = af_config->fileAreas;
      }

      for (i = 0; i < areaCount; i++)
          for (j = 0; j < areas[i].downlinkCount; j++)
              if (link == areas[i].downlinks[j]->link) {
                  setLinkAccess(af_config, &(areas[i]), areas[i].downlinks[j]);
                  break;
              }
      /* update perl vars */
      if (hook_onConfigChange) (*hook_onConfigChange)(PERL_CONF_LINKS|PERL_CONF_AREAS);
   }
   xstrscat(&report, " System switched to ", mode ? "active" : "passive", "\r\r", NULL);
   tmp = list(lt_linked, link, NULL);/*linked (link);*/
   xstrcat(&report, tmp);
   nfree(tmp);

   /* check for areas with one link alive and others paused */
   if (af_config->autoAreaPause & pause)
       pauseAreas(mode, link, NULL);

   return report;
}
char *pause_link(s_link *link)  { return pause_resume_link(link, 0); }
char *resume_link(s_link *link) { return pause_resume_link(link, 1); }

char *info_link(s_link *link)
{
    char *report=NULL, *ptr, linkAka[SIZE_aka2str];
    unsigned int i;

    sprintf(linkAka,aka2str(link->hisAka));
    xscatprintf(&report, "Here is some information about our link:\r\r");
    xscatprintf(&report, "            Your address: %s\r", linkAka);
    xscatprintf(&report, "           AKA used here: %s\r", aka2str(*link->ourAka));
    xscatprintf(&report, "         Reduced SEEN-BY: %s\r", link->reducedSeenBy ? "on" : "off");
    xscatprintf(&report, " Send rules on subscribe: %s\r", link->noRules ? "off" : "on");
    if (link->pktSize)
    xscatprintf(&report, "             Packet size: %u kbytes\r", link->pktSize);
    else
    xscatprintf(&report, "             Packet size: unlimited\r");
    xscatprintf(&report, "     Arcmail bundle size: %u kbytes\r", link->arcmailSize!=0 ? link->arcmailSize :
                      (af_config->defarcmailSize ? af_config->defarcmailSize : 500));
    xscatprintf(&report, " Forward requests access: %s\r", link->denyFRA ? "off" : "on");
    xscatprintf(&report, "Compression: ");

    if (link->packerDef==NULL)
	xscatprintf(&report, "No packer (");
    else
	xscatprintf(&report, "%s (", link->packerDef->packer);

    for (i=0; i < af_config->packCount; i++)
	xscatprintf(&report, "%s%s", af_config->pack[i].packer,
		    (i+1 == af_config->packCount) ? "" : ", ");
    xscatprintf(&report, ")\r\r");
    xscatprintf(&report, "Your system is %s\r", ((link->Pause & ECHOAREA) == ECHOAREA)?"passive":"active");
    ptr = list (lt_linked, link, NULL);/*linked (link);*/
    xstrcat(&report, ptr);
    nfree(ptr);
    w_log(LL_AREAFIX, "areafix: Link information sent to %s", aka2str(link->hisAka));
    return report;
}

char *rescan(s_link *link, char *cmd) {
    unsigned int i, c, rc = 0;
    long rescanCount = -1;
    char *report = NULL, *line, *countstr, *an, *end;
    s_area *area;

    line = cmd;
    if (strncasecmp(cmd, "%rescan", 7)==0) line += strlen("%rescan");

    if (*line == 0) return errorRQ(cmd);

    while (*line && (*line == ' ' || *line == '\t')) line++;

    if (*line == 0) return errorRQ(cmd);

    countstr = line;
    while (*countstr && (!isspace(*countstr))) countstr++; /*  skip areatag */
    while (*countstr && (*countstr == ' ' || *countstr == '\t')) countstr++;
    if (strncasecmp(countstr, "/R",2)==0) {
	countstr += 2;
	if (*countstr == '=') countstr++;
    }
    if (strncasecmp(countstr, "R=",2)==0) {
	countstr += 2;
    }
	
    if (*countstr != '\0') {
	rescanCount = strtol(countstr, NULL, 10);
    }

    end = strpbrk(line, " \t");
    if (end) *end = 0;

    if (*line == 0) return errorRQ(cmd);

    for (i=c=0; i<af_config->echoAreaCount; i++) {
	rc=subscribeAreaCheck(&(af_config->echoAreas[i]), line, link);
	if (rc == 4) continue;

	area = &(af_config->echoAreas[i]);
	an = area->areaName;

	switch (rc) {
	case 0:
            if (hook_onRescanArea)
              (*hook_onRescanArea)(&report, link, area, rescanCount);
            else {
         	xscatprintf(&report," %s %s  rescan is not supported\r",
         		    line, print_ch(49-strlen(line), '.'));
         	w_log(LL_AREAFIX, "areafix: Area \'%s\' rescan is not supported", line);
            }
	    if (!isPatternLine(line)) i = af_config->echoAreaCount;
	    break;
	case 1: if (isPatternLine(line)) continue;
	    w_log(LL_AREAFIX, "areafix: Area \'%s\' not linked for rescan to %s",
		  area->areaName, aka2str(link->hisAka));
	    xscatprintf(&report, " %s %s  not linked for rescan\r",
			an, print_ch(49-strlen(an), '.'));
	    break;
	default: w_log(LL_AREAFIX, "areafix: Area \'%s\' not access for %s",
		       area->areaName, aka2str(link->hisAka));
	    break;
	}
    }
    if (report == NULL) {
	xscatprintf(&report," %s %s  not linked for rescan\r",
		    line, print_ch(49-strlen(line), '.'));
	w_log(LL_AREAFIX, "areafix: Area \'%s\' not linked for rescan", line);
    }
    return report;
}

char *add_rescan(s_link *link, char *line) {
    char *report=NULL, *line2=NULL, *p;

    if (*line=='+') line++; while (*line==' ') line++;

    p = strchr(line, ' '); /* select only areaname */
    if (p) *p = '\0';

    report = subscribe (link, line);
    if (p) *p = ' '; /* resume original string */

    xstrscat(&line2,"%rescan ", line, NULL);
    xstrcat(&report, rescan(link, line2));
    nfree(line2);
    if (p) *p = '\0';

    return report;
}

char *packer(s_link *link, char *cmdline) {
    char *report=NULL;
    char *was=NULL;
    char *pattern = NULL;
    int reversed;
    UINT i;
    pattern = getPatternFromLine(cmdline, &reversed);
    if(pattern)
    {
        char *packerString=NULL;
        ps_pack packerDef = NULL;
        char *confName = NULL;
        long  strbeg=0;
        long  strend=0;

        for (i=0; i < af_config->packCount; i++)
        {
            if (stricmp(af_config->pack[i].packer,pattern) == 0)
            {
                packerDef = &(af_config->pack[i]);
                break;
            }
        }
        if( (i == af_config->packCount) && (stricmp("none",pattern) != 0) )
        {
            xscatprintf(&report, "Packer '%s' was not found\r", pattern);
            return report;
        }
        if (link->packerDef==NULL)
            xstrcat(&was, "none");
        else
            xstrcat(&was, link->packerDef->packer);

        xstrcat(&confName,(af_cfgFile) ? af_cfgFile : getConfigFileName());
        FindTokenPos4Link(&confName, "Packer", NULL, link, &strbeg, &strend);
        xscatprintf(&packerString,"Packer %s",pattern);
        if( InsertCfgLine(confName, packerString, strbeg, strend) )
        {
           link->packerDef = packerDef;
           if (hook_onConfigChange) (*hook_onConfigChange)(PERL_CONF_LINKS);
        }
        nfree(confName);
        nfree(packerString);
    }

    xstrcat(  &report, "Here is some information about current & available packers:\r\r");
    xstrcat(  &report,       "Compression: ");
    if (link->packerDef==NULL)
        xscatprintf(&report, "none (");
    else
        xscatprintf(&report, "%s (", link->packerDef->packer);

    for (i=0; i < af_config->packCount; i++)
        xscatprintf(&report, "%s%s", af_config->pack[i].packer,(i+1 == af_config->packCount) ? "" : ", ");

    xscatprintf(&report, "%snone)\r", (i == 0) ? "" : ", ");
    if(was)
    {
        xscatprintf(&report, "        was: %s\r", was);
    }
    return report;
}

char *change_token(s_link *link, char *cmdline)
{
    char *param, *token, *token2 = NULL, *desc, *report = NULL;
    char *confName = NULL, *cfgline = NULL;
    long strbeg = 0, strend = 0;
    int mode;
    char *c_prev = NULL, *c_new = NULL;  /* previous and new char values */
    unsigned int *i_prev = NULL, i_new = 0;  /* previous and new int values */

    w_log(LL_FUNC, __FILE__ "::change_token()");

    switch (RetFix) {
        case AREAFIXPWD:
            mode = 1;
            c_prev = link->areaFixPwd;
            token = "areaFixPwd";
            token2 = "password";
            desc = "Areafix";
            break;
        case FILEFIXPWD:
            mode = 1;
            c_prev = link->fileFixPwd;
            token = "fileFixPwd";
            token2 = "password";
            desc = "Filefix";
            break;
        case PKTPWD:
            mode = 1;
            c_prev = link->pktPwd;
            token = "pktPwd";
            token2 = "password";
            desc = "Packet";
            break;
        case TICPWD:
            mode = 1;
            c_prev = link->ticPwd;
            token = "ticPwd";
            token2 = "password";
            desc = "Tic";
            break;
        case PKTSIZE:
            mode = 2;
            i_prev = &(link->pktSize);
            token = "pktSize";
            desc = "Packet";
            break;
        case ARCMAILSIZE:
            mode = 2;
            i_prev = &(link->arcmailSize);
            token = "arcmailSize";
            desc = "Arcmail bundle";
            break;
        case RSB:
            mode = 3;
            i_prev = &(link->reducedSeenBy);
            token = "reducedSeenBy";
            desc = "Reduced SEEN-BY";
            break;
        case RULES:
            mode = 3;
            i_prev = &(link->noRules);
            token = "noRules";
            desc = "Send rules";
            break;
        default:
            w_log(LL_AREAFIX, "%sfix: Error! Unreacheable line %s:%u", __FILE__, __LINE__);
            return NULL;
    }

    if (cmdline[0] == '%') cmdline++; /* exclude '%' sign */
    while((strlen(cmdline) > 0) && isspace(cmdline[0])) cmdline++; /* exclude spaces between '%' and command */
    while((strlen(cmdline) > 0) && !isspace(cmdline[0])) cmdline++; /* exclude command */
    while((strlen(cmdline) > 0) && isspace(cmdline[0])) cmdline++; /* exclude spaces between command and rest of line */
    
    param = strtok(cmdline, "\0");

    switch (mode) {
        case 1:  /* AREAFIXPWD, PKTPWD, FILEFIXPWD, TICPWD */
            c_new = (param != NULL) ? param : "";
            if (stricmp(c_prev, c_new) == 0) {
                w_log(LL_AREAFIX, "%sfix: New and old passwords are the same", _AF);
                xstrcat(&report, "New and old passwords are the same. No changes were made.\r\r");
                return report;
            }
            if ((RetFix == PKTPWD || RetFix == TICPWD) && strlen(c_new) > 8) {
                w_log(LL_AREAFIX, "%sfix: Password is longer then 8 characters", _AF);
                xstrcat(&report, "Password is longer then 8 characters. Only passwords no longer than 8 characters are allowed. No changes were made.\r\r");
                return report;
            }
            xscatprintf(&cfgline, "%s %s", token, c_new);
            break;
        case 2:  /* ARCMAILSIZE, PKTSIZE */
            if (param == NULL) {
                w_log(LL_AREAFIX, "%sfix: No parameter found after command", _AF);
                xstrcat(&report, "No parameter found after command. No changes were made.\r\r");
                return report;
            } else {
                char *ptr;
                for (ptr=param; *ptr; ptr++) {
                    if (!isdigit(*ptr)) {
                        w_log(LL_AREAFIX, "%sfix: '%s' is not a valid number!", _AF, param);
                        xscatprintf(&report, "'%s' is not a valid number!\r\r", param);
                        return report;
                    }
                }
                i_new = (unsigned) atoi(param);
                if (*i_prev == i_new) {
                    w_log(LL_AREAFIX, "%sfix: %s size is already set to %i kbytes", _AF, desc, *i_prev);
                    xscatprintf(&report, "%s size is already set to %i kbytes. No changes were made.\r\r", desc, *i_prev);
                    return report;
                }
            }
            xscatprintf(&cfgline, "%s %i", token, i_new);
            break;
        case 3:  /* RSB */
            if (param == NULL) {
                w_log(LL_AREAFIX, "%sfix: No parameter found after command", _AF);
                xstrcat(&report, "No parameter found after command. No changes were made.\r\r");
                return report;
            } else {
                if (!stricmp(param, "on") || !strcmp(param, "1")) {
                    i_new = 1;
                } else if (!stricmp(param, "off") || !strcmp(param, "0")) {
                    i_new = 0;
                } else {
                    w_log(LL_AREAFIX, "%sfix: Unknown parameter for %RSB command: %s", _AF, param);
                    xscatprintf(&report, "Unknown parameter for %RSB command: %s\r. Please read help.\r\r", param);
                    return report;
                }
                if (RetFix == RULES && *i_prev != i_new) {
                    w_log(LL_AREAFIX, "%sfix: %s is already set to %s", _AF, desc, *i_prev?"off":"on");
                    xscatprintf(&report, "%s is already set to %s. No changes were made.\r\r", desc, *i_prev?"off":"on");
                    return report;
                } else if (RetFix != RULES && *i_prev == i_new) {
                    w_log(LL_AREAFIX, "%sfix: %s is already set to %s", _AF, desc, *i_prev?"on":"off");
                    xscatprintf(&report, "%s is already set to %s. No changes were made.\r\r", desc, *i_prev?"on":"off");
                    return report;
                }
            }
            if (RetFix == RULES)
                xscatprintf(&cfgline, "%s %s", token, i_new?"off":"on");
            else
                xscatprintf(&cfgline, "%s %s", token, i_new?"on":"off");
            break;
    }

    xstrcat(&confName,(af_cfgFile) ? af_cfgFile : getConfigFileName());
    FindTokenPos4Link(&confName, token, token2, link, &strbeg, &strend);

    if (InsertCfgLine(confName, cfgline, strbeg, strend)) {
        switch (mode) {
            case 1:  /* AREAFIXPWD, PKTPWD, FILEFIXPWD, TICPWD */
                w_log(LL_AREAFIX, "%sfix: %s password changed to '%s'", _AF, desc, c_new);
                xscatprintf(&report, "%s password changed to '%s'\r\r", desc, c_new);
                *c_prev = *c_new;
                break;
            case 2:  /* ARCMAILSIZE, PKTSIZE */
                w_log(LL_AREAFIX, "%sfix: %s size changed to %i kbytes", _AF, desc, i_new);
                xscatprintf(&report, "%s size changed to %i kbytes\r\r", desc, i_new);
                *i_prev = i_new;
                break;
            case 3:  /* RSB */
                w_log(LL_AREAFIX, "%sfix: %s mode changed to %s", _AF, desc, i_new?"on":"off");
                xscatprintf(&report, "%s mode changed to %s\r\r", desc, i_new?"on":"off");
                *i_prev = i_new;
                break;
        }
        if (hook_onConfigChange) (*hook_onConfigChange)(PERL_CONF_LINKS);
    }

    nfree(confName);
    nfree(cfgline);

    w_log(LL_FUNC, __FILE__ "::change_token() OK");

    return report;
}

/* if any text is returned, it should be treated as a failure report
   and message processing should be stopped */
char *change_link(s_link **link, s_link *origlink, char *cmdline)
{
    char *report = NULL, *addr = NULL;
    s_link *newlink = NULL;

    w_log(LL_FUNC, __FILE__ "::change_link()");

    if (!origlink->allowRemoteControl) {
        w_log(LL_AREAFIX, "%sfix: Remote control is not allowed for link %s", _AF, aka2str(origlink->hisAka));
        xstrcat(&report, "Remote control is not allowed to you, the rest of message is skipped.\r\r");
        return report;
    }

    if (cmdline[0] == '%') cmdline++; /* exclude '%' sign */
    while((strlen(cmdline) > 0) && isspace(cmdline[0])) cmdline++; /* exclude spaces between '%' and command */
    while((strlen(cmdline) > 0) && !isspace(cmdline[0])) cmdline++; /* exclude command */
    while((strlen(cmdline) > 0) && isspace(cmdline[0])) cmdline++; /* exclude spaces between command and rest of line */

    addr = strtok(cmdline, "\0");
    if (addr == NULL) {
        w_log(LL_AREAFIX, "%sfix: Address is missing in FROM command", _AF);
        xstrcat(&report, "Invalid request. Address is required. Please read help.\r\r");
        return report;
    }
    newlink = getLink(af_config, addr);
    if (newlink == NULL) {
        w_log(LL_AREAFIX, "%sfix: Link %s not found in config", _AF, addr);
        xscatprintf(&report, "Link %s not found, the rest of message is skipped.\r\r", addr);
        return report;
    }

    *link = newlink;
    w_log(LL_AREAFIX, "%sfix: Link changed to %s", _AF, aka2str((*link)->hisAka));
    w_log(LL_FUNC, __FILE__ "::change_link() OK");
    return NULL;
}

int tellcmd(char *cmd) {
    char *line;

    if (strncmp(cmd, "* Origin:", 9) == 0) return NOTHING;

    line = cmd;
    if (line && *line && (line[1]==' ' || line[1]=='\t')) return AFERROR;

    switch (line[0]) {
    case '%':
        line++;
        if (*line == '\000') return AFERROR;
        if (strncasecmp(line,"list",4)==0) return LIST;
        if (strncasecmp(line,"help",4)==0) return HELP;
        if (strncasecmp(line,"avail",5)==0) return AVAIL;
        if (strncasecmp(line,"all",3)==0) return AVAIL;
        if (strncasecmp(line,"unlinked",8)==0) return UNLINKED;
        if (strncasecmp(line,"linked",6)==0) return QUERY;
        if (strncasecmp(line,"query",5)==0) return QUERY;
        if (strncasecmp(line,"pause",5)==0) return PAUSE;
        if (strncasecmp(line,"resume",6)==0) return RESUME;
        if (strncasecmp(line,"info",4)==0) return INFO;
        if (strncasecmp(line,"packer",6)==0) return PACKER;
        if (strncasecmp(line,"compress",8)==0) return PACKER;
        if (strncasecmp(line,"rsb",3)==0) return RSB;
        if (strncasecmp(line,"rules",5)==0) return RULES;
        if (strncasecmp(line,"pktsize",7)==0) return PKTSIZE;
        if (strncasecmp(line,"arcmailsize",11)==0) return ARCMAILSIZE;
        if (strncasecmp(line,"rescan", 6)==0) {
            if (line[6] == '\0') {
                rescanMode=1;
                return NOTHING;
            } else {
                return RESCAN;
            }
        }
        if (strncasecmp(line,"areafixpwd",10)==0) return AREAFIXPWD;
        if (strncasecmp(line,"filefixpwd",10)==0) return FILEFIXPWD;
        if (strncasecmp(line,"pktpwd",6)==0) return PKTPWD;
        if (strncasecmp(line,"ticpwd",6)==0) return TICPWD;
        if (strncasecmp(line,"from",4)==0) return FROM;
        return AFERROR;
    case '\001': return NOTHING;
    case '\000': return NOTHING;
    case '-'  :
        if (line[1]=='-' && line[2]=='-') return DONE;
        if (line[1]=='\000') return AFERROR;
        if (strchr(line,' ') || strchr(line,'\t')) return AFERROR;
        return DEL;
    case '~'  : return REMOVE;
    case '+':
        if (line[1]=='\000') return AFERROR;
    default:
        if (fc_stristr(line, " /R")!=NULL) return ADD_RSC; /*  add & rescan */
        if (fc_stristr(line, " R=")!=NULL) return ADD_RSC; /*  add & rescan */
        return ADD;
    }
    return 0;/*  - Unreachable */
}

char *processcmd(s_link *link, char *line, int cmd) {

    char *report = NULL;

    w_log(LL_FUNC, __FILE__ "::processcmd()");

    if (hook_afixcmd) {
      if (cmd != NOTHING && cmd != DONE && cmd != AFERROR) {
        int rc = (*hook_afixcmd)(&report, cmd, aka2str(link->hisAka), line);
        if (cmd == DEL || cmd == REMOVE || cmd == RESCAN || cmd == ADD_RSC)
          RetFix = STAT;
          else RetFix = cmd;
        if (rc) return report;
      }
    }

    switch (cmd) {

    case NOTHING: return NULL;

    case DONE: RetFix=DONE;
        return NULL;

    case LIST: report = list (lt_all, link, line);
        RetFix=LIST;
        break;
    case HELP: report = help (link);
        RetFix=HELP;
        break;
    case ADD: report = subscribe (link, line);
        RetFix=ADD;
        break;
    case DEL: report = unsubscribe (link, line);
        RetFix=STAT;
        break;
    case REMOVE: report = delete (link, line);
        RetFix=STAT;
        break;
    case AVAIL: report = available (link, line);
        RetFix=AVAIL;
        break;
    case UNLINKED: report = list (lt_unlinked, link, line);/*report = unlinked (link);*/
        RetFix=UNLINKED;
        break;
    case QUERY: report = list (lt_linked, link, line);/*report = linked (link);*/
        RetFix=QUERY;
        break;
    case PAUSE: report = pause_link (link);
        RetFix=PAUSE;
        break;
    case RESUME: report = resume_link (link);
        RetFix=RESUME;
        break;
    case PACKER: report = packer (link, line);
        RetFix=PACKER;
        break;
    case INFO: report = info_link(link);
        RetFix=INFO;
        break;
    case RESCAN: report = rescan(link, line);
        RetFix=STAT;
        break;
    case ADD_RSC: report = add_rescan(link, line);
        RetFix=STAT;
        break;
    case AREAFIXPWD:
    case FILEFIXPWD:
    case PKTPWD:
    case TICPWD:
    case PKTSIZE:
    case ARCMAILSIZE:
    case RSB:
    case RULES:
        RetFix=cmd;
        report = change_token(link, line);
        break;
    case FROM:         /* command is processed in processAreafix() */
        RetFix=FROM;
        break;
    case AFERROR: report = errorRQ(line);
        RetFix=STAT;
        break;
    default: return NULL;
    }
    w_log(LL_FUNC, __FILE__ "::processcmd() OK");
    return report;
}

void preprocText(char *split, s_message *msg, char *reply, s_link *link)
{
    char *orig = af_config->areafixOrigin;

    msg->text = createKludges(af_config, NULL, &msg->origAddr,
        &msg->destAddr, af_versionStr);
    if (reply) {
        reply = strchr(reply, ' ');
        if (reply) reply++;
        if (reply[0])
            xscatprintf(&(msg->text), "\001REPLY: %s\r", reply);
    }
    /* xstrcat(&(msg->text), "\001FLAGS NPD DIR\r"); */
    if (link->areafixReportsFlags)
        xstrscat(&(msg->text), "\001FLAGS ", link->areafixReportsFlags, "\r",NULL);
    else if (af_config->areafixReportsFlags)
        xstrscat(&(msg->text), "\001FLAGS ", af_config->areafixReportsFlags, "\r",NULL);
    xscatprintf(&split, "\r--- %s areafix\r", af_versionStr);
    if (orig && orig[0]) {
        xscatprintf(&split, " * Origin: %s (%s)\r", orig, aka2str(msg->origAddr));
    }
    xstrcat(&(msg->text), split);
    msg->textLength=(int)strlen(msg->text);
    nfree(split);
}

char *textHead(void)
{
    char *text_head = NULL;

    xscatprintf(&text_head, " Area%sStatus\r",	print_ch(48,' '));
    xscatprintf(&text_head, " %s  -------------------------\r",print_ch(50, '-'));
    return text_head;
}

char *areaStatus(char *report, char *preport)
{
    if (report == NULL) report = textHead();
    xstrcat(&report, preport);
    nfree(preport);
    return report;
}

/* report already nfree() after this function */
void RetMsg(s_message *msg, s_link *link, char *report, char *subj)
{
    char *text, *split, *p, *newsubj = NULL;
    char splitted[]=" > message splitted...";
    char *splitStr = af_config->areafixSplitStr;
    int len, msgsize = af_config->areafixMsgSize * 1024, partnum=0;
    s_message *tmpmsg;
    char *reply = NULL;

    /* val: silent mode - don't write messages */
    if (af_silent_mode) return;

    text = report;
    reply = (char *) GetCtrlToken((byte *) msg->ctl, (byte *) "MSGID");

    while (text) {

        len = strlen(text);
        if (msgsize == 0 || len <= msgsize) {
            split = text;
            text = NULL;
            if (partnum) { /* last part of splitted msg */
                partnum++;
                xstrcat(&text,split);
                split = text;
                text = NULL;
                nfree(report);
            }
            if (msg->text)
                xstrscat(&split,"\rFollowing is the original message text\r--------------------------------------\r",msg->text,"\r--------------------------------------\r",NULL);
            else
                xstrscat(&split,"\r",NULL);
        } else {
            p = text + msgsize;
            while (*p != '\r') p--;
            *p = '\000';
            len = p - text;
            split = (char*)(*call_smalloc)(len+strlen(splitStr ? splitStr : splitted)+3+1);
            memcpy(split,text,len);
            strcpy(split+len,"\r\r");
            strcat(split, (splitStr) ? splitStr : splitted);
            strcat(split,"\r");
            text = p+1;
            partnum++;
        }

        if (partnum) xscatprintf(&newsubj, "%s (%d)", subj, partnum);
        else newsubj = subj;

        tmpmsg = makeMessage(link->ourAka, &(link->hisAka),
            af_config->areafixFromName ? af_config->areafixFromName : msg->toUserName,
            msg->fromUserName, newsubj, 1,
            link->areafixReportsAttr ? link->areafixReportsAttr : af_config->areafixReportsAttr);
  
        preprocText(split, tmpmsg, reply, link);

        if (af_config->outtab != NULL) {
            recodeToTransportCharset((CHAR*)tmpmsg->subjectLine);
            recodeToTransportCharset((CHAR*)tmpmsg->fromUserName);
            recodeToTransportCharset((CHAR*)tmpmsg->toUserName);
            recodeToTransportCharset((CHAR*)tmpmsg->text);
            tmpmsg->recode &= ~(REC_HDR|REC_TXT);
        }

        nfree(reply);
/*
        processNMMsg(tmpmsg, NULL, getRobotsArea(af_config),
            0, MSGLOCAL);
        writeEchoTossLogEntry(getRobotsArea(af_config)->areaName);
        closeOpenedPkt();
        freeMsgBuffers(tmpmsg);
*/
        (*call_sendMsg)(tmpmsg);
        nfree(tmpmsg);
        if (partnum) nfree(newsubj);
    }

/*    af_config->intab = tab; */
}

void RetRules (s_message *msg, s_link *link, char *areaName)
{
    FILE *f=NULL;
    char *fileName = NULL;
    char *text=NULL, *subj=NULL;
    char *msg_text;
    long len=0;
    int nrul=0;

    xscatprintf(&fileName, "%s%s.rul", af_config->rulesDir, strLower(makeMsgbFileName(af_config, areaName)));

    for (nrul=0; nrul<=9 && (f = fopen (fileName, "rb")); nrul++) {

	len = fsize (fileName);
	text = (*call_smalloc) (len+1);
	fread (text, len, 1, f);
	fclose (f);

	text[len] = '\0';

	if (nrul==0) {
	    xscatprintf(&subj, "Rules of %s", areaName);
	    w_log(LL_AREAFIX, "areafix: Send '%s' as rules for area '%s'",
		  fileName, areaName);
	} else {
	    xscatprintf(&subj, "Echo related text #%d of %s", nrul, areaName);
	    w_log(LL_AREAFIX, "areafix: Send '%s' as text %d for area '%s'",
		  fileName, nrul, areaName);
	}

        /* prevent "Following original message text" in rules msgs */
        msg_text = msg->text;
        msg->text= NULL;
        RetMsg(msg, link, text, subj);
        /* preserve original message text */
        msg->text= msg_text;

	nfree (subj);
	/* nfree (text); don't free text because RetMsg() free it */

	fileName[strlen(fileName)-1] = nrul+'1';
    }

    if (nrul==0) { /*  couldn't open any rules file while first one exists! */
	w_log(LL_ERR, "areafix: Can't open file '%s' for reading: %s", fileName, strerror(errno));
    }
    nfree (fileName);

}

void sendAreafixMessages()
{
    s_link *link = NULL;
    s_message *linkmsg;
    unsigned int i;

    for (i = 0; i < af_config->linkCount; i++) {
        if (af_config->links[i]->msg == NULL) continue;
        link = af_config->links[i];
        linkmsg = link->msg;

        xscatprintf(&(linkmsg->text), " \r--- %s areafix\r", af_versionStr);
        linkmsg->textLength = strlen(linkmsg->text);

        w_log(LL_AREAFIX, "areafix: Write netmail msg for %s", aka2str(link->hisAka));

/*
        processNMMsg(linkmsg, NULL, getRobotsArea(af_config),
            0, MSGLOCAL);
        writeEchoTossLogEntry(getRobotsArea(af_config)->areaName);
        closeOpenedPkt();
        freeMsgBuffers(linkmsg);
*/
        (*call_sendMsg)(linkmsg);
        nfree(linkmsg);
        link->msg = NULL;
    }
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

int processAreaFix(s_message *msg, s_pktHeader *pktHeader, unsigned force_pwd)
{
    unsigned int security = 0, notforme = 0;
    s_link *curlink = NULL;  /* perform areafix changes on this link */
                             /* can be changed by %from command */
    s_link *link = NULL;  /* whom to send areafix reports */
                          /* i.e. original sender of message */
    s_link *tmplink = NULL;
    /* s_message *linkmsg; */
    /* s_pktHeader header; */
    char *token = NULL, *report = NULL, *preport = NULL;
    char *textBuff = NULL,*tmp;
    int nr;

    w_log(LL_FUNC, __FILE__ "::processAreaFix()");

    RetFix = NOTHING;

    /*  1st security check */
    if (pktHeader) security=addrComp(msg->origAddr, pktHeader->origAddr);
#if 0
    else {
	makePktHeader(NULL, &header);
	pktHeader = &header;
	pktHeader->origAddr = msg->origAddr;
	pktHeader->destAddr = msg->destAddr;
	security = 0;
    }
#endif

    if (security) security=1; /* different pkt and msg addresses */

    /*  find link */
    link=getLinkFromAddr(af_config, msg->origAddr);

    /*  if keyword allowPktAddrDiffer for this link is on, */
    /*  we allow the addresses in PKT and MSG header differ */
    if (link!=NULL)
	if (link->allowPktAddrDiffer == pdOn)
	    security = 0;  /* OK */

    /*  is this for me? */
    if (link!=NULL)	notforme=addrComp(msg->destAddr, *link->ourAka);
    else if (!security) security=4; /*  link == NULL; unknown system */
	
    if (notforme && !security) security=5; /*  message to wrong AKA */

#if 0 /*  we're process only our messages here */
    /*  ignore msg for other link (maybe this is transit...) */
    if (notforme || (link==NULL && security==1)) {
        w_log(LL_FUNC, __FILE__ "::processAreaFix() call processNMMsg() and return");
	nr = processNMMsg(msg, pktHeader, NULL, 0, 0);
	closeOpenedPkt();
	return nr;
    }
#endif

    /*  2nd security check. link, areafixing & password. */
    if (!security && !force_pwd) {
        if (link->AreaFix==1) {
            if (link->areaFixPwd!=NULL) {
                if (stricmp(link->areaFixPwd,msg->subjectLine)==0) security=0;
                else security=3; /* password error */
            }
        } else security=2; /* areafix is turned off */
    }

    /*  remove kluges */
    tmp = msg->text;
    token = strseparate (&tmp,"\n\r");

    while(token != NULL) {
        if( !strcmp(token,"---") || !strncmp(token,"--- ",4) )
            /*  stop on tearline ("---" or "--- text") */
            break;
        if( token[0] != '\001' )
            xstrscat(&textBuff,token,"\r",NULL);
        token = strseparate (&tmp,"\n\r");
    }
    nfree(msg->text);
    msg->text = textBuff;

    if ( hook_afixreq && (*hook_afixreq)(msg, pktHeader ? pktHeader->origAddr : msg->origAddr) )
        link = getLinkFromAddr(af_config, msg->origAddr);

    if (!security) {
        curlink = link;
	textBuff = (*call_sstrdup)(msg->text);
        tmp = textBuff;
	token = strseparate (&tmp, "\n\r");
	while(token != NULL) {
	    while ((*token == ' ') || (*token == '\t')) token++;
	    while(isspace(token[strlen(token)-1])) token[strlen(token)-1]='\0';
            w_log(LL_AREAFIX, "Process command: %s", token);
	    preport = processcmd( curlink, token, tellcmd (token) );
            if (RetFix == FROM && preport == NULL) {
                preport = change_link(&curlink, link, token);
            }
	    if (preport != NULL) {
		switch (RetFix) {
		case LIST:
		    RetMsg(msg, link, preport, "Areafix reply: list request");
		    break;
		case HELP:
		    RetMsg(msg, link, preport, "Areafix reply: help request");
		    break;
		case ADD:
		    report = areaStatus(report, preport);
		    if (rescanMode) {
			preport = processcmd( link, token, RESCAN );
			if (preport != NULL)
			    report = areaStatus(report, preport);
		    }
		    break;
		case AVAIL:
		    RetMsg(msg, link, preport, "Areafix reply: available areas");
		    break;
		case UNLINKED:
		    RetMsg(msg, link, preport, "Areafix reply: unlinked request");
		    break;
		case QUERY:
		    RetMsg(msg, link, preport, "Areafix reply: linked request");
		    break;
		case PAUSE:
		    RetMsg(msg, link, preport, "Areafix reply: pause request");
		    break;
		case RESUME:
		    RetMsg(msg, link, preport, "Areafix reply: resume request");
		    break;
		case INFO:
		    RetMsg(msg, link, preport, "Areafix reply: link information");
		    break;
		case PACKER:
		    RetMsg(msg, link, preport, "Areafix reply: packer change request");
		    break;
		case RSB:
		    RetMsg(msg, link, preport, "Areafix reply: reduced seen-by change request");
		    break;
		case RULES:
		    RetMsg(msg, link, preport, "Areafix reply: send rules change request");
		    break;
		case PKTSIZE:
		    RetMsg(msg, link, preport, "Areafix reply: pkt size change request");
		    break;
		case ARCMAILSIZE:
		    RetMsg(msg, link, preport, "Areafix reply: arcmail size change request");
		    break;
		case STAT:
		    report = areaStatus(report, preport);
		    break;
		case AREAFIXPWD:
		case FILEFIXPWD:
		case PKTPWD:
		case TICPWD:
		    RetMsg(msg, link, preport, "Areafix reply: password change request");
		    break;
		case FROM:
		    RetMsg(msg, link, preport, "Areafix reply: link change request");
                    RetFix = DONE; /* error changing link, rest of message should be skipped */
		    break;
		default:
		    w_log(LL_ERR,"Unknown areafix command:%s", token);
		    break;
		}
	    } /* end if (preport != NULL) */
	    token = strseparate (&tmp, "\n\r");
	    if (RetFix==DONE) token=NULL;
	} /* end while (token != NULL) */
    nfree(textBuff);
    } else {
	if (link == NULL) {
	    tmplink = (s_link*) (*call_smalloc)(sizeof(s_link));
	    memset(tmplink, '\0', sizeof(s_link));
	    tmplink->ourAka = &(msg->destAddr);
	    tmplink->hisAka.zone = msg->origAddr.zone;
	    tmplink->hisAka.net = msg->origAddr.net;
	    tmplink->hisAka.node = msg->origAddr.node;
	    tmplink->hisAka.point = msg->origAddr.point;
	    link = tmplink;
	}
	/*  security problem */
		
	switch (security) {
	case 1:
	    xscatprintf(&report, " \r different pkt and msg addresses\r");
	    break;
	case 2:
	    xscatprintf(&report, " \r areafix is turned off\r");
	    break;
	case 3:
	    xscatprintf(&report, " \r password error\r");
	    break;
	case 4:
	    xscatprintf(&report, " \r your system is unknown\r");
	    break;
	case 5:
	    xscatprintf(&report, " \r message sent to wrong AKA\r");
	    break;
	default:
	    xscatprintf(&report, " \r unknown error. mail to sysop.\r");
	    break;
	}

	RetMsg(msg, link, report, "Areafix reply: security violation");
	w_log(LL_AREAFIX, "areafix: Security violation from %s", aka2str(link->hisAka));
	nfree(tmplink);
        w_log(LL_FUNC, __FILE__ ":%u:processAreaFix() rc=1", __LINE__);
	return 1;
    }

    if ( report != NULL ) {
        if (af_config->areafixQueryReports) {
            preport = list (lt_linked, link, NULL);/*linked (link);*/
            xstrcat(&report, preport);
            nfree(preport);
        }
        RetMsg(msg, link, report, "Areafix reply: node change request");
    }

    if (rulesCount) {
        for (nr=0; nr < rulesCount; nr++) {
            if (rulesList && rulesList[nr]) {
                RetRules (msg, link, rulesList[nr]);
                nfree (rulesList[nr]);
            }
        }
        nfree (rulesList);
        rulesCount=0;
    }

    w_log(LL_AREAFIX, "areafix: Successfully done for %s",aka2str(link->hisAka));

    /*  send msg to the links (forward requests to areafix) */
    sendAreafixMessages();

    w_log(LL_FUNC, __FILE__ "::processAreaFix() end (rc=1)");
    return 1;
}

void MsgToStruct(HMSG SQmsg, XMSG xmsg, s_message *msg)
{
    /*  convert header */
    msg->attributes  = xmsg.attr;

    msg->origAddr.zone  = xmsg.orig.zone;
    msg->origAddr.net   = xmsg.orig.net;
    msg->origAddr.node  = xmsg.orig.node;
    msg->origAddr.point = xmsg.orig.point;

    msg->destAddr.zone  = xmsg.dest.zone;
    msg->destAddr.net   = xmsg.dest.net;
    msg->destAddr.node  = xmsg.dest.node;
    msg->destAddr.point = xmsg.dest.point;

    strcpy((char *)msg->datetime, (char *) xmsg.__ftsc_date);
    xstrcat(&(msg->subjectLine), (char *) xmsg.subj);
    xstrcat(&(msg->toUserName), (char *) xmsg.to);
    xstrcat(&(msg->fromUserName), (char *) xmsg.from);

    msg->textLength = MsgGetTextLen(SQmsg);
    msg->ctlLength = MsgGetCtrlLen(SQmsg);
    xstralloc(&(msg->text),msg->textLength+1);
    xstralloc(&(msg->ctl),msg->ctlLength+1);
    MsgReadMsg(SQmsg, NULL, 0, msg->textLength, (unsigned char *) msg->text, msg->ctlLength, (byte *) msg->ctl);
    msg->text[msg->textLength] = '\0';
    msg->ctl[msg->ctlLength] = '\0';

}

void afix(hs_addr addr, char *cmd)
{
    HAREA           netmail;
    HMSG            SQmsg;
    unsigned long   highmsg, i;
    XMSG            xmsg;
    hs_addr         orig, dest;
    s_message	    msg, *tmpmsg;
    int             k, startarea = 0, endarea = af_config->netMailAreaCount;
    s_area          *area;
    char            *name = af_config->robotsArea;
    s_link          *link;

    w_log(LL_FUNC, __FILE__ "::afix() begin");
    w_log(LL_INFO, "Start areafix...");

    if ((area = getNetMailArea(af_config, name)) != NULL) {
        startarea = area - af_config->netMailAreas;
        endarea = startarea + 1;
    }

    if (cmd) {
        link = getLinkFromAddr(af_config, addr);
        if (link) {
          if (cmd && strlen(cmd)) {
            tmpmsg = makeMessage(&addr, link->ourAka, link->name,
                link->RemoteRobotName ?
                link->RemoteRobotName : "Areafix",
                link->areaFixPwd ?
                link->areaFixPwd : "", 1,
                link->areafixReportsAttr ? link->areafixReportsAttr : af_config->areafixReportsAttr);
            tmpmsg->text = (*call_sstrdup)(cmd);
            processAreaFix(tmpmsg, NULL, 1);
            freeMsgBuffers(tmpmsg);
	  } else w_log(LL_WARN, "areafix: Empty areafix command from %s", aka2str(addr));
        } else w_log(LL_ERR, "areafix: No such link in af_config: %s!", aka2str(addr));
    }
    else for (k = startarea; k < endarea; k++) {

        netmail = MsgOpenArea((unsigned char *) af_config->netMailAreas[k].fileName,
            MSGAREA_NORMAL,
            /*af_config -> netMailArea.fperm,
            af_config -> netMailArea.uid,
            af_config -> netMailArea.gid,*/
            (word)af_config -> netMailAreas[k].msgbType);

        if (netmail != NULL) {

            highmsg = MsgGetHighMsg(netmail);
            w_log(LL_INFO,"Scanning %s",af_config->netMailAreas[k].areaName);

            /*  scan all Messages and test if they are already sent. */
            for (i=1; i<= highmsg; i++) {
                SQmsg = MsgOpenMsg(netmail, MOPEN_RW, i);

                /*  msg does not exist */
                if (SQmsg == NULL) continue;

                MsgReadMsg(SQmsg, &xmsg, 0, 0, NULL, 0, NULL);
                cvtAddr(xmsg.orig, &orig);
                cvtAddr(xmsg.dest, &dest);
                w_log(LL_DEBUG, "Reading msg %lu from %s -> %s", i,
                      aka2str(orig), aka2str(dest));

                /*  if not read and for us -> process AreaFix */
                striptwhite((char*)xmsg.to);
                if ((xmsg.attr & MSGREAD) == MSGREAD) {
                    w_log(LL_DEBUG, "Message is already read, skipping");
                    MsgCloseMsg(SQmsg);
                    continue;
                }
                if (!isOurAka(af_config,dest)) {
                    w_log(LL_DEBUG, "Message is not to us, skipping");
                    MsgCloseMsg(SQmsg);
                    continue;
                }
                if ((strlen((char*)xmsg.to)>0) &&
                    fc_stristr(af_config->areafixNames,(char*)xmsg.to))
                {
                    memset(&msg,'\0',sizeof(s_message));
                    MsgToStruct(SQmsg, xmsg, &msg);
                    processAreaFix(&msg, NULL, 0);
                    if (af_config->areafixKillRequests) {
                        MsgCloseMsg(SQmsg);
                        MsgKillMsg(netmail, i--);
                    } else {
                        xmsg.attr |= MSGREAD;
                        MsgWriteMsg(SQmsg, 0, &xmsg, NULL, 0, 0, 0, NULL);
                        MsgCloseMsg(SQmsg);
                    }
                    freeMsgBuffers(&msg);
                }
                else
                {
                    w_log(LL_DEBUG, "Message is not to AreaFix, skipping");
                    MsgCloseMsg(SQmsg);
                }
            }

            MsgCloseArea(netmail);
        } else {
            w_log(LL_ERR, "Could not open %s", af_config->netMailAreas[k].areaName);
        }
    }
    w_log(LL_INFO, "End areafix...");
    w_log(LL_FUNC, __FILE__ "::afix() end");
}


/* mode==0 - relink mode */
/* mode==1 - resubscribe mode (fromLink -> toLink)*/
int relink (int mode, char *pattern, hs_addr fromAddr, hs_addr toAddr) {
    ps_area       areas = NULL;
    unsigned int  i, j, k, count, addMode, areaCount = 0, reversed = 0;
    s_link        *fromLink = NULL, *toLink = NULL;
    char          *fromCmd  = NULL, *toCmd  = NULL;
    char          *fromAka  = NULL, *toAka  = NULL;
    char          *exclMask;
    s_arealink    *arealink = NULL;
    s_linkdata    fromData, toData;

    w_log(LL_START, "Start relink...");

    fromLink = getLinkFromAddr(af_config, fromAddr);
    if (fromLink == NULL) {
        w_log(LL_ERR, "Unknown link address %s", aka2str(fromAddr));
        return 1;
    }
    fromAka = (*call_sstrdup)(aka2str(fromLink->hisAka));
    (*call_getLinkData)(fromLink, &fromData);

    if (mode) {
        toLink = getLinkFromAddr(af_config, toAddr);
        if (toLink == NULL) {
            w_log(LL_ERR, "Unknown link address %s", aka2str(toAddr));
            return 1;
        }
        toAka = (*call_sstrdup)(aka2str(toLink->hisAka));
        (*call_getLinkData)(toLink, &toData);

        /* allocate memory to check for read/write access for new link */
        arealink = (s_arealink*) scalloc(1, sizeof(s_arealink));
    }

    if (pattern) {
        if ((strlen(pattern) > 2) && (pattern[0] == '!') && (isspace(pattern[1]))) {
            reversed = 1;
            pattern++;
            while (isspace(pattern[0])) pattern++;
        } else reversed = 0;
        if (strlen(pattern) == 0) pattern = NULL;
    }

    if (af_app->module == M_HTICK) {
        areas = af_config->fileAreas;
        areaCount = af_config->fileAreaCount;
    } else {
        areas = af_config->echoAreas;
        areaCount = af_config->echoAreaCount;
    }

    count = 0;
    for (i = 0; i < areaCount; i++) {

        if ((pattern) && (patimat(areas[i].areaName, pattern) == reversed))
            continue;

        for (j = 0; j < areas[i].downlinkCount; j++) {

            if (fromLink != areas[i].downlinks[j]->link)
                continue;

            addMode = areas[i].downlinks[j]->defLink ? 10 : 3;

            /* resubscribe */
            if (mode) {

                /* check if new link would have full access to area */
                /* report to log and skip relink/resubscribe if not */
                arealink->link = toLink;
                setLinkAccess(af_config, &areas[i], arealink);

                if (af_config->readOnlyCount) {
                    for (k=0; k < af_config->readOnlyCount; k++) {
                        if(af_config->readOnly[k].areaMask[0] != '!') {
                            if (patimat(areas[i].areaName, af_config->readOnly[k].areaMask) &&
                                patmat(toAka, af_config->readOnly[k].addrMask)) {
                                    arealink->import = 0;
                            }
                        } else {
                            exclMask = af_config->readOnly[k].areaMask;
                            exclMask++;
                            if (patimat(areas[i].areaName, exclMask) &&
                                patmat(toAka, af_config->readOnly[k].addrMask)) {
                                    arealink->import = 1;
                            }
                        }
                    }
                }
                
                if (af_config->writeOnlyCount) {
                    for (k=0; k < af_config->writeOnlyCount; k++) {
                        if(af_config->writeOnly[k].areaMask[0] != '!') {
                            if (patimat(areas[i].areaName, af_config->writeOnly[k].areaMask) &&
                                patmat(toAka, af_config->writeOnly[k].addrMask)) {
                                    arealink->export = 0;
                            }
                        } else {
                            exclMask = af_config->writeOnly[k].areaMask;
                            exclMask++;
                            if (patimat(areas[i].areaName, exclMask) &&
                                patmat(toAka, af_config->writeOnly[k].addrMask)) {
                                    arealink->export = 1;
                            }
                        }
                    }
                }

                if ((arealink->export == 0) || (arealink->import == 0)) {
                    w_log(LL_AREAFIX, "%sfix: Link %s will not have full access (export=%s import=%s) to %sarea %s, skipped",
                           _AF, toAka, arealink->export?"on":"off", arealink->import?"on":"off", af_app->module == M_HTICK ? "file" : "", areas[i].areaName);
                    continue;
                }

                /* unsubscribe fromLink from area */
                if (changeconfig(af_cfgFile?af_cfgFile:getConfigFileName(),
                         &areas[i],fromLink,7) != DEL_OK) {
                    w_log(LL_AREAFIX, "%sfix: Can't unlink %s from %sarea \'%s\'",
                          _AF, fromAka, af_app->module == M_HTICK ? "file" : "",
                          areas[i].areaName);
                    continue;
                }
                RemoveLink(fromLink, &areas[i]);

                if (isLinkOfArea(toLink, &areas[i])) {
                    w_log(LL_AREAFIX, "Link %s is already subscribed to %sarea \'%s\'",
                          toAka, af_app->module == M_HTICK ? "file" : "",
                          areas[i].areaName);
                    continue;
                }

                /* subscribe toLink to area */
                if (changeconfig(af_cfgFile?af_cfgFile:getConfigFileName(),
                         &areas[i],toLink,addMode) != ADD_OK) {
                    w_log(LL_AREAFIX, "%sfix: Can't subscribe %s to %sarea \'%s\'",
                          _AF, toAka, af_app->module == M_HTICK ? "file" : "",
                          areas[i].areaName);
                    continue;
                }
                Addlink(af_config, toLink, &areas[i]);

                count++;

                if (areas[i].paused) {
                    w_log(LL_AREAFIX, "%sArea \'%s\' is paused, no command will be sent",
                          af_app->module == M_HTICK ? "File" : "", areas[i].areaName);
                } else {
                    xscatprintf(&fromCmd, "-%s\r", areas[i].areaName);
                    xscatprintf(&toCmd, "+%s\r", areas[i].areaName);
                }

                w_log(LL_AREAFIX, "%sArea \'%s\' is resubscribed from link %s to link %s",
                      af_app->module == M_HTICK ? "File" : "",
                      areas[i].areaName, fromAka, toAka);

            /* relink */
            } else {
                if (areas[i].paused) {
                    w_log(LL_AREAFIX, "%sArea \'%s\' is paused, no command will be sent",
                          af_app->module == M_HTICK ? "File" : "", areas[i].areaName);
                } else {
                    count++;
                    xscatprintf(&fromCmd, "+%s\r", areas[i].areaName);
                }

                w_log(LL_AREAFIX, "%sArea \'%s\' from link %s is relinked",
                      af_app->module == M_HTICK ? "File" : "",
                      areas[i].areaName, fromAka);
            }
            break;
        }
    }

    if (fromCmd) {
        s_message *msg;

        msg = makeMessage(fromLink->ourAka, &(fromLink->hisAka), af_config->sysop,
                       fromData.robot, fromData.pwd, 1, fromData.attrs);
        msg->text = createKludges(af_config, NULL, fromLink->ourAka,
                       &(fromLink->hisAka), af_versionStr);
        if (fromData.flags)
            xstrscat(&(msg->text), "\001FLAGS ", fromData.flags, "\r",NULL);

        xstrcat(&(msg->text), fromCmd);

        xscatprintf(&(msg->text), " \r--- %s %sfix\r", af_versionStr, _AF);
        msg->textLength = strlen(msg->text);
        w_log(LL_AREAFIX, "%s message created to %s",
            mode ? "Resubscribe" : "Relink", fromAka);
/*
        processNMMsg(msg, NULL,
                     getRobotsArea(config),
                 1, MSGLOCAL|MSGKILL);
        writeEchoTossLogEntry(getRobotsArea(config)->areaName);
        closeOpenedPkt();
        freeMsgBuffers(msg);
*/
        (*call_sendMsg)(msg);
        nfree(msg);
        nfree(fromCmd);
    }

    if (toCmd) {
        s_message *msg;

        msg = makeMessage(toLink->ourAka, &(toLink->hisAka), af_config->sysop,
                       toData.robot, toData.pwd, 1, toData.attrs);
        msg->text = createKludges(af_config, NULL, toLink->ourAka,
                       &(toLink->hisAka), af_versionStr);
        if (toData.flags)
            xstrscat(&(msg->text), "\001FLAGS ", toData.flags, "\r",NULL);

        xstrcat(&(msg->text), toCmd);

        xscatprintf(&(msg->text), " \r--- %s %sfix\r", af_versionStr, _AF);
        msg->textLength = strlen(msg->text);
        w_log(LL_AREAFIX, "%s message created to %s",
            mode ? "Resubscribe" : "Relink", toAka);
/*
        processNMMsg(msg, NULL,
                     getRobotsArea(config),
                 1, MSGLOCAL|MSGKILL);
        writeEchoTossLogEntry(getRobotsArea(config)->areaName);
        closeOpenedPkt();
        freeMsgBuffers(msg);
*/
        (*call_sendMsg)(msg);
        nfree(msg);
        nfree(toCmd);
    }

    nfree(fromAka);
    nfree(toAka);
    nfree(arealink);

    w_log(LL_AREAFIX, "%s %i %sarea(s)", mode ? "Resubscribed" : "Relinked",
             count, af_app->module == M_HTICK ? "file" : "");

    return 0;
}

/* ensure all callbacks are inited */
int init_areafix(void) {
  if (!af_config || !af_versionStr || !af_app) return 0;
  if (call_sstrdup == NULL) call_sstrdup = &sstrdup;
  if (call_smalloc == NULL) call_smalloc = &smalloc;
  if (call_srealloc == NULL) call_srealloc = &srealloc;
  if (!call_sendMsg || !call_writeMsgToSysop || !call_getLinkData) return 0;
  return 1;
}
