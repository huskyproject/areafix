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
        xscatprintf(&report, "%s %sareas for %s\r\r", af_app->module == M_HTICK ? "file" : "",
                    ((link->Pause & ECHOAREA) == ECHOAREA) ? "Passive" : "Active", aka2str(link->hisAka));
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
        xscatprintf(&report, "\r\r %i area(s) available, %i area(s) active\r", avail, active);
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
        w_log(LL_AREAFIX, "%sfix: list sent to %s", _AF, aka2str(link->hisAka));
        break;
      case lt_linked:
        w_log(LL_AREAFIX, "%sfix: linked areas list sent to %s", _AF, aka2str(link->hisAka));
        break;
      case lt_unlinked:
        w_log(LL_AREAFIX, "%sfix: unlinked areas list sent to %s", _AF, aka2str(link->hisAka));
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
            w_log (LL_ERR, "%sfix: cannot open help file \"%s\": %s", _AF,
                   hf, strerror(errno));
	    if (!af_quiet)
                fprintf(stderr,"%sfix: cannot open help file \"%s\": %s\n", _AF,
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

        w_log(LL_AREAFIX, "%sfix: help sent to %s", _AF, link->name);

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

    pattern = getPatternFromLine(cmdline, &reversed);

    if (af_app->module != M_HTICK) {
      if ((pattern) && (strlen(pattern)>60 || !isValidConference(pattern))) {
          w_log(LL_FUNC, "areafix::avail() FAILED (error request line)");
          return errorRQ(cmdline);
      }
    }

    for (j = 0; j < af_config->linkCount; j++)
    {
        int fr_on = 0;
        char *frf = NULL;

	uplink = af_config->links[j];
        fr_on = af_app->module == M_HTICK ? uplink->forwardFileRequests : uplink->forwardRequests;
        frf = af_app->module == M_HTICK ? uplink->forwardFileRequestFile : uplink->forwardRequestFile;

	found = 0;
	isuplink = 0;
	for (k = 0; k < link->numAccessGrp && uplink->LinkGrp; k++)
	    if (strcmp(link->AccessGrp[k], uplink->LinkGrp) == 0)
		found = 1;

        if ((fr_on && frf) && ((uplink->LinkGrp == NULL) || (found != 0)))
	{
            if ((f=fopen(frf,"r")) == NULL) {
                w_log(LL_ERR, "%sfix: cannot open forwardRequestFile \"%s\": %s", _AF,
                      frf, strerror(errno));
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
                    int numDfMask = af_app->module == M_HTICK ? uplink->numDfMask : uplink->numDfMask;
                    char **dfMask = af_app->module == M_HTICK ? uplink->dfMask : uplink->dfMask;
                    char *denyFwdFile = af_app->module == M_HTICK ? uplink->denyFwdFile : uplink->denyFwdFile;
                    running = line;
                    token = strseparate(&running, " \t\r\n");
                    rc = 0;

                    if (numDfMask)
                      rc |= tag_mask(token, dfMask, numDfMask);

                    if (denyFwdFile)
                      rc |= IsAreaAvailable(token, denyFwdFile, NULL, 0);

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
            w_log( LL_AREAFIX, "%sfix: Available Area List from %s %s to %s", _AF,
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
	xstrcat(&report, "\r  no links for creating Available Area List\r");
        w_log(LL_AREAFIX, "%sfix: no links for creating Available Area List", _AF);
    }
    return report;
}


/*  subscribe if (act==0),  unsubscribe if (act==1), delete if (act==2) */
int forwardRequestToLink (char *areatag, s_link *uplink, s_link *dwlink, int act) {
    s_message *msg;
    char *base, pass[]="passthrough";
    char *robot, *pwd, *flags;
    int attrs;

    if (!uplink) return -1;

    if (af_app->module == M_HTICK) {
      robot = uplink->RemoteFileRobotName ? uplink->RemoteFileRobotName : "filefix";
      pwd = uplink->fileFixPwd;
      attrs = uplink->filefixReportsAttr ? uplink->filefixReportsAttr : af_config->filefixReportsAttr;
      flags = uplink->filefixReportsFlags;
      if (!flags) flags = af_config->filefixReportsFlags;
    } else {
      robot = uplink->RemoteRobotName ? uplink->RemoteRobotName : "areafix";
      pwd = uplink->areaFixPwd;
      attrs = uplink->areafixReportsAttr ? uplink->areafixReportsAttr : af_config->areafixReportsAttr;
      flags = uplink->areafixReportsFlags;
      if (!flags) flags = af_config->areafixReportsFlags;
    }

    if (uplink->msg == NULL) {
	msg = makeMessage(uplink->ourAka, &(uplink->hisAka), af_config->sysop,
        robot, pwd ? pwd : "\x00", 1, attrs);
	msg->text = createKludges(af_config, NULL, uplink->ourAka, &(uplink->hisAka),
                              af_versionStr);
        if (flags)
            xstrscat(&(msg->text), "\001FLAGS ", flags, "\r",NULL);
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
        if (af_app->module == M_HTICK ? 0/*uplink->advancedFilefix*/ : uplink->advancedAreafix)
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
    case 1: /*  remove link from area */
        if ((area->msgbType==MSGTYPE_PASSTHROUGH)
            && (area->downlinkCount==1) &&
            (area->downlinks[0]->link->hisAka.point == 0)) {
            forwardRequestToLink(areaName, area->downlinks[0]->link, NULL, 1);
        }
    case 7:
        if ((rc = DelLinkFromString(cfgline, link->hisAka)) == 1) {
            w_log(LL_ERR,"%sfix: Unlink is not possible for %s from %s area %s", _AF,
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

    /* for example, update perl vars */
    if (hook_onConfigChange) (*hook_onConfigChange)();

    w_log(LL_FUNC, __FILE__ "::changeconfig() rc=%i", nRet);
    return nRet;
}

static int compare_links_priority(const void *a, const void *b) {
    int ia = *((int*)a);
    int ib = *((int*)b);
    unsigned int pa, pb;
    if (af_app->module == M_HTICK) {
      pa = af_config->links[ia]->forwardAreaPriority;
      pb = af_config->links[ib]->forwardAreaPriority; 
    } else {
      pa = af_config->links[ia]->forwardFilePriority;
      pb = af_config->links[ib]->forwardFilePriority;
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
        int fr_ok;
        char *frf;
        uplink = af_config->links[Indexes[i]];
        fr_ok = af_app->module == M_HTICK ? uplink->forwardFileRequests : uplink->forwardRequests;
        frf   = af_app->module == M_HTICK ? uplink->forwardFileRequestFile : uplink->forwardRequestFile;

        if(lastRlink) *lastRlink = uplink;

        if (fr_ok && (uplink->LinkGrp) ?
            grpInArray(uplink->LinkGrp,dwlink->AccessGrp,dwlink->numAccessGrp) : 1)
        {
            /* skip downlink from list of uplinks */
            if(addrComp(uplink->hisAka, dwlink->hisAka) == 0)
            {
                rc = 2;
                continue;
            }
            if ( (uplink->numDfMask) &&
                 (tag_mask(areatag, uplink->dfMask, uplink->numDfMask)))
            {
                rc = 2;
                continue;
            }
            if ( (uplink->denyFwdFile!=NULL) &&
                 (IsAreaAvailable(areatag,uplink->denyFwdFile,NULL,0)))
            {
                rc = 2;
                continue;
            }
            rc = 0;
            if (frf != NULL) {
                /*  first try to find the areatag in forwardRequestFile */
                if (tag_mask(areatag, uplink->frMask, uplink->numFrMask) ||
                    IsAreaAvailable(areatag, frf, NULL, 0))
                {
                    break;
                }
                else
                { rc = 2; }/*  found link with freqfile, but there is no areatag */
            } else {
                if (uplink->numFrMask) /*  found mask */
                {
                    if (tag_mask(areatag, uplink->frMask, uplink->numFrMask))
                        break;
                    else rc = 2;
                } else { /*  unconditional forward request */
                    if (dwlink->denyUFRA==0)
                        break;
                    else rc = 2;
                }
            }/* (uplink->forwardRequestFile!=NULL) */

        }/*  if (uplink->forwardRequests && (uplink->LinkGrp) ? */
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
		w_log(LL_AREAFIX, "%sfix: %s already linked to %s", _AF,
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
                  w_log(LL_AREAFIX, "%sfix: %s subscribed to %s", _AF, aka2str(link->hisAka), an);
                  if ((af_config->autoAreaPause & pause) && area->paused)
                      pauseAreas(1, NULL, area);
              } else {
                  xscatprintf(&report, " %s %s  not subscribed\r", an, print_ch(49-strlen(an), '.'));
                  w_log(LL_AREAFIX, "%sfix: %s not subscribed to %s , cause uplink", _AF, aka2str(link->hisAka), an);
                  w_log(LL_AREAFIX, "%sfix: %s has \"passthrough\" in \"autoAreaCreateDefaults\" for %s", _AF,
                                    an, aka2str(area->downlinks[0]->link->hisAka));
              }
           } else {  /* ??? (not passthrou echo) */
                     /*   non-passthrough area for our aka means */
                     /*   that we already linked to this area */
               xscatprintf(&report, " %s %s  already linked\r", an, print_ch(49-strlen(an), '.'));
               w_log(LL_AREAFIX, "%sfix: %s already linked to %s", _AF, aka2str(link->hisAka), an);
           }
        } else {
            if (changeconfig(af_cfgFile?af_cfgFile:getConfigFileName(),area,link,0)==ADD_OK) {
                Addlink(af_config, link, area);
                if (af_app->module != M_HTICK) fixRules(link, area->areaName);
                xscatprintf(&report," %s %s  added\r", an, print_ch(49-strlen(an),'.'));
                w_log(LL_AREAFIX, "%sfix: %s subscribed to %s", _AF, aka2str(link->hisAka), an);
                if ((af_config->autoAreaPause & pause) && area->paused && (link->Pause & pause))
                    pauseAreas(1, link, area);
                af_CheckAreaInQuery(an, NULL, NULL, DELIDLE);
                if (af_send_notify)
                    forwardRequestToLink(area->areaName, link, NULL, 0);
            } else {
                xscatprintf(&report," %s %s  error. report to sysop!\r", an, print_ch(49-strlen(an),'.'));
                w_log(LL_AREAFIX, "%sfix: %s not subscribed to %s", _AF, aka2str(link->hisAka), an);
                w_log(LL_ERR, "%sfix: can't write to af_config file: %s!", _AF, strerror(errno));
            }/* if (changeconfig(af_cfgFile?af_cfgFile:getConfigFileName(),area,link,3)==0) */
        }
	    if (!isPatternLine(line)) i = cnt;
	    break;
	case 6:         /* areas limit exceed for link */
            break;
	default : /*  rc = 2  not access */
	    if (!area->hide && !isPatternLine(line)) {
		w_log(LL_AREAFIX, "%sfix: area %s -- no access for %s", _AF,
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
            w_log(LL_WARN, "Can't forward request for %sarea %s : refused by New%sAreaRefuseFile\n", 
                  af_app->module == M_HTICK ? "file" : "", line, 
                  af_app->module == M_HTICK ? "File" : "");
        } else
        if (link->denyFRA==0) {
	    /*  try to forward request */
	    if ((rc=forwardRequest(line, link, NULL))==2) {
		xscatprintf(&report, " %s %s  no uplinks to forward\r",
			    line, print_ch(49-strlen(line), '.'));
		w_log( LL_AREAFIX, "%sfix: %s - no uplinks to forward", _AF, line);
	    }
	    else if (rc==0) {
		xscatprintf(&report, " %s %s  request forwarded\r",
			    line, print_ch(49-strlen(line), '.'));
		w_log( LL_AREAFIX, "%sfix: %s - request forwarded", _AF, line);
        if (!qf && isOurAka(af_config,link->hisAka)==0)
        {
            area = af_app->module == M_HTICK ? getFileArea(line) : getArea(af_config, line);
            if ( !isLinkOfArea(link, area) ) {
                if(changeconfig(af_cfgFile?af_cfgFile:getConfigFileName(),area,link,3)==ADD_OK) {
                    Addlink(af_config, link, area);
                    if (af_app->module != M_HTICK) fixRules(link, area->areaName);
                    w_log(LL_AREAFIX, "%sfix: %s subscribed to area %s", _AF,
                        aka2str(link->hisAka),line);
                } else {
                    xscatprintf( &report," %s %s  error. report to sysop!\r",
                        an, print_ch(49-strlen(an),'.') );
                    w_log(LL_AREAFIX, "%sfix: %s not subscribed to %s", _AF,
                        aka2str(link->hisAka),an);
                    w_log(LL_ERR, "%sfix: can't change af_config file: %s!", _AF, strerror(errno));
                }
            } else w_log(LL_AREAFIX, "%sfix: %s already subscribed to area %s", _AF,
                aka2str(link->hisAka), line );

        } else {
            if (af_app->module != M_HTICK) fixRules (link, line);
        }
        }
	}
    }

    if (rc == 6) {   /* areas limit exceed for link */
	w_log(LL_AREAFIX, "%sfix: %sarea %s -- no access (full limit) for %s", _AF,
              af_app->module == M_HTICK ? "file" : "",
	      line, aka2str(link->hisAka));
	xscatprintf(&report," %s %s  no access (full limit)\r",
		    line, print_ch(49-strlen(line), '.'));
    }

    if (matched) {
	if (report == NULL)
	    w_log(LL_AREAFIX, "%sfix: all %sareas matching %s are already linked", _AF, 
                  af_app->module == M_HTICK ? "file" : "", line);
	xscatprintf(&report, "All %sareas matching %s are already linked\r", report ? "other " : "", line);
    }
    else if ((report == NULL && found==0) || (found && area->hide)) {
	xscatprintf(&report," %s %s  not found\r",line,print_ch(49-strlen(line),'.'));
	w_log(LL_AREAFIX, "%sfix: area %s is not found", _AF, line);
    }
    w_log(LL_FUNC, "%sfix::subscribe() OK", _AF);
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
	if (addrComp(area->downlinks[i]->link->hisAka, link->hisAka))
	    forwardRequestToLink(an, area->downlinks[i]->link, NULL, 2);
/*   todo: advanced features
	if (addrComp(area->downlinks[i]->link->hisAka, link->hisAka)) {
            s_link *dwlink;
            dwlink = area->downlinks[i]->link;

	    if (dwlink->unsubscribeOnAreaDelete)
                forwardRequestToLink(an, dwlink, NULL, 2);
            if (dwlink->sendNotifyMessages) {
                s_message *tmpmsg = NULL;

                tmpmsg = makeMessage(dwlink->ourAka, &(dwlink->hisAka),
                    config->filefixFromName ? config->filefixFromName : versionStr,
                    dwlink->name, "Notification message", 1,
                    dwlink->filefixReportsAttr ? dwlink->filefixReportsAttr : config->filefixReportsAttr);
                tmpmsg->text = createKludges(config, NULL, dwlink->ourAka,
                    &(dwlink->hisAka), versionStr);
                if (dwlink->filefixReportsFlags)
                    xstrscat(&(tmpmsg->text), "\001FLAGS ", dwlink->filefixReportsFlags, "\r",NULL);
                else if (config->filefixReportsFlags)
                    xstrscat(&(tmpmsg->text), "\001FLAGS ", config->filefixReportsFlags, "\r",NULL);

                xscatprintf(&tmpmsg->text, "\r Area %s is deleted.\r", area->areaName);
                    xstrcat(&tmpmsg->text, "\r Do not forget to remove it from your configs.\r");
                xscatprintf(&tmpmsg->text, "\r\r--- %s filefix\r", versionStr);

                tmpmsg->textLength = strlen(tmpmsg->text);
                writeNetmail(tmpmsg, getRobotsArea(config)->areaName);
                freeMsgBuffers(tmpmsg);
                w_log( LL_AREAFIX, "filefix: write notification msg for %s",aka2str(dwlink->hisAka));
                nfree(tmpmsg);
            }
        }
*/
    }
    /* remove area from config-file */
    if( changeconfig ((af_cfgFile) ? af_cfgFile : getConfigFileName(),  area, link, 4) != DEL_OK) {
       w_log( LL_AREAFIX, "%sfix: can't remove area from af_config: %s", _AF, strerror(errno));
    }

    /* delete msgbase and dupebase for the area */

    /*
    if (area->msgbType!=MSGTYPE_PASSTHROUGH)
	MsgDeleteBase(area->fileName, (word) area->msgbType);
    */

    if (hook_onDeleteArea) hook_onDeleteArea(link, area); // new!!!

    w_log( LL_AREAFIX, "%sfix: area %s deleted by %s", _AF,
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
	w_log(LL_AREAFIX, "%sfix: area %s is not found", _AF, line);
	return report;
    }
    rc = subscribeCheck(area, link);
    an = area->areaName;

    switch (rc) {
    case 0:
	break;
    case 1:
	xscatprintf(&report, " %s %s  not linked\r", an, print_ch(49-strlen(an), '.'));
	w_log(LL_AREAFIX, "%sfix: area %s is not linked to %s", _AF,
	      an, aka2str(link->hisAka));
	return report;
    case 2:
	xscatprintf(&report, " %s %s  no access\r", an, print_ch(49-strlen(an), '.'));
	w_log(LL_AREAFIX, "%sfix: area %s -- no access for %s", _AF, an, aka2str(link->hisAka));
	return report;
    }
    if (link->LinkGrp == NULL || (area->group && strcmp(link->LinkGrp, area->group))) {
	xscatprintf(&report, " %s %s  delete not allowed\r",
		    an, print_ch(49-strlen(an), '.'));
	w_log(LL_AREAFIX, "%sfix: area %s delete not allowed for %s", _AF,
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
                        w_log(LL_AREAFIX, "%sfix: %s doesn't unlinked from %s", _AF,
                            aka2str(link->hisAka), an);
                    } else {
                        w_log(LL_AREAFIX,"%sfix: %s unlinked from %s", _AF, aka2str(link->hisAka),an);
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
            w_log(LL_AREAFIX, "%sfix: area %s is not linked to %s", _AF,
                area->areaName, aka2str(link->hisAka));
            break;
        case 5:
            xscatprintf(&report, " %s %s  unlink is not possible\r",
                an, print_ch(49-strlen(an), '.'));
            w_log(LL_AREAFIX, "%sfix: area %s -- unlink is not possible for %s", _AF,
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
            w_log(LL_AREAFIX, "%sfix: no areas to unlink", _AF);
        } else {
            xscatprintf(&report, " %s %s  not found\r",
                line, print_ch(49-strlen(line), '.'));
            w_log(LL_AREAFIX, "%sfix: area %s is not found", _AF, line);
        }
    }
    w_log(LL_FUNC,__FILE__ ":%u:unsubscribe() end", __LINE__);
    return report;
}

/* if act==0 pause area, if act==1 unpause area */
/* returns 0 if no messages to links were created */
int pauseAreas(int act, s_link *searchLink, s_area *searchArea) {
  unsigned int i, j, k, linkCount, cnt;
  unsigned int rc = 0;
  int pause = af_app->module == M_HTICK ? FILEAREA : ECHOAREA;

  if (!searchLink && !searchArea) return rc;

  cnt = af_app->module == M_HTICK ? af_config->fileAreaCount : af_config->echoAreaCount;
  for (i = 0; i < cnt; i++) {
    s_link *uplink;
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
        w_log(LL_AREAFIX, "%sfix: area %s is paused (uplink: %s)", _AF,
                 area->areaName, aka2str(uplink->hisAka));
      } else {
        w_log(LL_AREAFIX, "%sfix: error pausing area %s", _AF, area->areaName);
        continue;
      }
    } else if (act==1) { /* make area non paused */
      if (changeconfig(af_cfgFile?af_cfgFile:getConfigFileName(),area,NULL,9)==ADD_OK) {
        w_log(LL_AREAFIX, "%sfix: area %s is not paused any more (uplink: %s)", _AF,
                 area->areaName, aka2str(uplink->hisAka));
      } else {
        w_log(LL_AREAFIX, "%sfix: error unpausing area %s", _AF, area->areaName);
        continue;
      }
    }

    /* write messages */
    if (uplink->msg == NULL) {
      char *robotName, *robotPwd, *reportsFlags;
      int reportsAttr;

      if (af_app->module == M_HTICK) {
        robotName = uplink->RemoteFileRobotName ? uplink->RemoteFileRobotName : "filefix";
        robotPwd = uplink->fileFixPwd ? uplink->fileFixPwd : "\x00";
        reportsAttr = uplink->filefixReportsAttr ? uplink->filefixReportsAttr : af_config->filefixReportsAttr;
        reportsFlags = uplink->filefixReportsFlags;
        if (!reportsFlags) reportsFlags = af_config->filefixReportsFlags;
      } else {
        robotName = uplink->RemoteRobotName ? uplink->RemoteRobotName : "areafix";
        robotPwd = uplink->areaFixPwd ? uplink->areaFixPwd : "\x00";
        reportsAttr = uplink->areafixReportsAttr ? uplink->areafixReportsAttr : af_config->areafixReportsAttr;
        reportsFlags = uplink->areafixReportsFlags;
        if (!reportsFlags) reportsFlags = af_config->areafixReportsFlags;
      }

      msg = makeMessage(uplink->ourAka, &(uplink->hisAka), af_config->sysop,
                        robotName, robotPwd, 1, reportsAttr);
      msg->text = createKludges(af_config, NULL, uplink->ourAka, &(uplink->hisAka),
                      af_versionStr);
      if (reportsFlags)
        xstrscat(&(msg->text), "\001FLAGS ", reportsFlags, "\r",NULL);
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

      if (theApp.module == M_HPT)
      {
          areaCount = theApp.config->echoAreaCount;
          areas     = theApp.config->echoAreas;
      }
      else if (theApp.module == M_HTICK)
      {
          areaCount = theApp.config->fileAreaCount;
          areas     = theApp.config->fileAreas;
      }

      for (i = 0; i < areaCount; i++)
          for (j = 0; j < areas[i].downlinkCount; j++)
              if (link == areas[i].downlinks[j]->link)
                  setLinkAccess(af_config, &(areas[i]), areas[i].downlinks[j]);
      /* update perl vars */
      if (hook_onConfigChange) (*hook_onConfigChange)();
   }
   xstrscat(&report, " System switched to ", mode ? "active" : "passive", "\r", NULL);
   tmp = list(lt_linked, link, NULL);/*linked (link);*/
   xstrcat(&report, tmp);
   nfree(tmp);

   /* check for areas with one link alive and others paused */
   if (af_config->autoAreaPause & pause)
       pauseAreas(0, link, NULL);

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
    w_log(LL_AREAFIX, "areafix: link information sent to %s", aka2str(link->hisAka));
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
         	w_log(LL_AREAFIX, "areafix: %s rescan is not supported", line);
            }
	    if (!isPatternLine(line)) i = af_config->echoAreaCount;
	    break;
	case 1: if (isPatternLine(line)) continue;
	    w_log(LL_AREAFIX, "areafix: %s area not linked for rescan to %s",
		  area->areaName, aka2str(link->hisAka));
	    xscatprintf(&report, " %s %s  not linked for rescan\r",
			an, print_ch(49-strlen(an), '.'));
	    break;
	default: w_log(LL_AREAFIX, "areafix: %s area not access for %s",
		       area->areaName, aka2str(link->hisAka));
	    break;
	}
    }
    if (report == NULL) {
	xscatprintf(&report," %s %s  not linked for rescan\r",
		    line, print_ch(49-strlen(line), '.'));
	w_log(LL_AREAFIX, "areafix: %s area not linked for rescan", line);
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

char *pktsize (s_link *link, char *cmdline) {

    char *report = NULL;
    char *pattern = NULL;
    int reversed;
    char *error = NULL;
    unsigned long num = 0;

    pattern = (*call_sstrdup)(getPatternFromLine(cmdline, &reversed));

    if (pattern == NULL) {
        xscatprintf(&report, "Invalid request :%s\rPlease, read help!\r\r", cmdline);
        return report;
    }

    pattern = trimLine(pattern);

    num = strtoul(pattern, &error, 10);
    if ( ((error != NULL) && (*error != '\0')) || num == unsigned_long_max ) {
        xscatprintf(&report, "'%s' is not a valid number!\r", pattern);
        nfree(error);
        return report;
    } else {

        char *confName = NULL;
        char *pktSizeString = NULL;
        long strbeg = 0;
        long strend = 0;

        if (link->pktSize == num) {
            xscatprintf(&report, "Pkt size is already set to %u kbytes. No changes were made.\r", num);
            return report;
        }

        xstrcat(&confName,(af_cfgFile) ? af_cfgFile : getConfigFileName());
        FindTokenPos4Link(&confName, "pktSize", link, &strbeg, &strend);
        xscatprintf(&pktSizeString,"pktSize %u",num);
        if( InsertCfgLine(confName, pktSizeString, strbeg, strend) ) {
            link->pktSize = num;
            xscatprintf(&report, "Pkt size is set to %u kbytes.\r", num);
        }

        nfree(confName);
        nfree(pktSizeString);
        return report;
    }
}

char *arcmailsize (s_link *link, char *cmdline) {

    char *report = NULL;
    char *pattern = NULL;
    int reversed;
    char *error = NULL;
    unsigned long num = 0;

    pattern = (*call_sstrdup)(getPatternFromLine(cmdline, &reversed));

    if (pattern == NULL) {
        xscatprintf(&report, "Invalid request :%s\rPlease, read help!\r\r", cmdline);
        return report;
    }

    pattern = trimLine(pattern);

    num = strtoul(pattern, &error, 10);
    if ( ((error != NULL) && (*error != '\0')) || num == unsigned_long_max ) {
        xscatprintf(&report, "'%s' is not a valid number!\r", pattern);
        nfree(error);
        return report;
    } else {

        char *confName = NULL;
        char *arcmailSizeString = NULL;
        long strbeg = 0;
        long strend = 0;

        if (link->arcmailSize == num) {
            xscatprintf(&report, "Arcmail size is already set to %u kbytes. No changes were made.\r", num);
            return report;
        }

        xstrcat(&confName,(af_cfgFile) ? af_cfgFile : getConfigFileName());
        FindTokenPos4Link(&confName, "arcmailSize", link, &strbeg, &strend);
        xscatprintf(&arcmailSizeString,"arcmailSize %u",num);
        if( InsertCfgLine(confName, arcmailSizeString, strbeg, strend) ) {
            link->arcmailSize = num;
            xscatprintf(&report, "Arcmail size is set to %u kbytes.\r", num);
        }

        nfree(confName);
        nfree(arcmailSizeString);
        return report;
    }
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
        FindTokenPos4Link(&confName, "Packer", link, &strbeg, &strend);
        xscatprintf(&packerString,"Packer %s",pattern);
        if( InsertCfgLine(confName, packerString, strbeg, strend) )
        {
           link->packerDef = packerDef;
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

char *rsb(s_link *link, char *cmdline)
{
    int mode; /*  1 = RSB on, 0 - RSB off. */
    char *param=NULL; /*  RSB value. */
    char *report=NULL;
    char *confName = NULL;
    long  strbeg=0;
    long  strend=0;

    param = getPatternFromLine(cmdline, &mode); /*  extract rsb value (on or off) */
    if (param == NULL)
    {
        xscatprintf(&report, "Invalid request: %s\rPlease read help.\r\r", cmdline);
        return report;
    }

    param = trimLine(param);

    if ((!strcmp(param, "0")) || (!strcasecmp(param, "off")))
        mode = 0;
    else
    {
        if ((!strcmp(param, "1")) || (!strcasecmp(param, "on")))
            mode = 1;
        else
        {
            xscatprintf(&report, "Unknown parameter for areafix %rsb command: %s\r. Please read help.\r\r",
                        param);
            nfree(param);
            return report;
        }
    }
    nfree(param);
    if (link->reducedSeenBy == (UINT)mode)
    {
        xscatprintf(&report, "Redused SEEN-BYs had not been changed.\rCurrent value is '%s'\r\r",
                    mode?"on":"off");
        return report;
    }
    xstrcat(&confName,(af_cfgFile) ? af_cfgFile : getConfigFileName());
    FindTokenPos4Link(&confName, "reducedSeenBy", link, &strbeg, &strend);
    xscatprintf(&param, "reducedSeenBy %s", mode?"on":"off");
    if( InsertCfgLine(confName, param, strbeg, strend) )
    {
        xscatprintf(&report, "Redused SEEN-BYs is turned %s now\r\r", mode?"on":"off");
        link->reducedSeenBy = mode;
    }
    nfree(param);
    nfree(confName);
    return report;
}

char *rules(s_link *link, char *cmdline)
{
    int mode; /*  1 = RULES on (noRules off), 0 - RULES off (noRules on). */
              /*  !!! Use inversed values for noRules keyword. !!! */

    char *param=NULL; /*  RULES value. */
    char *report=NULL;
    char *confName = NULL;
    long  strbeg=0;
    long  strend=0;

    param = (*call_sstrdup)(getPatternFromLine(cmdline, &mode)); /*  extract rules value (on or off) */
    param = trimLine(param);
    if (*param == '\0')
    {
        xscatprintf(&report, "Invalid request: %s\rPlease read help.\r\r", cmdline);
        return report;
    }

    if ((!strcmp(param, "0")) || (!strcasecmp(param, "off")))
        mode = 0;
    else
    {
        if ((!strcmp(param, "1")) || (!strcasecmp(param, "on")))
            mode = 1;
        else
        {
            xscatprintf(&report, "Unknown parameter for areafix %rules command: %s\r. Please read help.\r\r",
                        param);
            nfree(param);
            return report;
        }
    }
    nfree(param);
    if (link->noRules != (UINT)mode)
    {
        xscatprintf(&report, "Send rules mode had not been changed.\rCurrent value is '%s'\r\r",
                    mode?"on":"off");
        return report;
    }
    xstrcat(&confName,(af_cfgFile) ? af_cfgFile : getConfigFileName());
    FindTokenPos4Link(&confName, "noRules", link, &strbeg, &strend);
    xscatprintf(&param, "noRules %s", mode?"off":"on");
    if( InsertCfgLine(confName, param, strbeg, strend) )
    {
        xscatprintf(&report, "Send rules mode is turned %s now\r\r", mode?"on":"off");
        link->noRules = (mode ? 0 : 1);
    }
    nfree(param);
    nfree(confName);
    return report;
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
    case RSB: report = rsb (link, line);
        RetFix=RSB;
        break;
    case RULES: report = rules (link, line);
        RetFix=RULES;
        break;
    case PKTSIZE: report = pktsize (link, line);
        RetFix=PKTSIZE;
        break;
    case ARCMAILSIZE: report = arcmailsize (link, line);
        RetFix=ARCMAILSIZE;
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
    reply = GetCtrlToken(msg->ctl, "MSGID");

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
	    w_log(LL_AREAFIX, "areafix: send '%s' as rules for area '%s'",
		  fileName, areaName);
	} else {
	    xscatprintf(&subj, "Echo related text #%d of %s", nrul, areaName);
	    w_log(LL_AREAFIX, "areafix: send '%s' as text %d for area '%s'",
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
	w_log(LL_ERR, "areafix: can't open file '%s' for reading: %s", fileName, strerror(errno));
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

        w_log(LL_AREAFIX, "areafix: write netmail msg for %s", aka2str(link->hisAka));

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
    s_link *link = NULL;
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
	textBuff = (*call_sstrdup)(msg->text);
        tmp = textBuff;
	token = strseparate (&tmp, "\n\r");
	while(token != NULL) {
	    while ((*token == ' ') || (*token == '\t')) token++;
	    while(isspace(token[strlen(token)-1])) token[strlen(token)-1]='\0';
            w_log(LL_AREAFIX, "Process command: %s", token);
	    preport = processcmd( link, token, tellcmd (token) );
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
	w_log(LL_AREAFIX, "areafix: security violation from %s", aka2str(link->hisAka));
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

    w_log(LL_AREAFIX, "Areafix: successfully done for %s",aka2str(link->hisAka));

    /*  send msg to the links (forward requests to areafix) */
    sendAreafixMessages();

    /* for example, update perl vars */
    if (hook_onConfigChange) (*hook_onConfigChange)();

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
    MsgReadMsg(SQmsg, NULL, 0, msg->textLength, (unsigned char *) msg->text, msg->ctlLength, msg->ctl);
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
    w_log(LL_INFO, "Start AreaFix...");

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
	  } else w_log(LL_WARN, "areafix: empty areafix command from %s", aka2str(addr));
        } else w_log(LL_ERR, "areafix: no such link in af_config: %s!", aka2str(addr));
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
    w_log(LL_FUNC, __FILE__ "::afix() end");
}

/* ensure all callbacks are inited */
int init_areafix(void) {
  if (!af_config || !af_versionStr || !af_app) return 0;
  if (call_sstrdup == NULL) call_sstrdup = &sstrdup;
  if (call_smalloc == NULL) call_smalloc = &smalloc;
  if (call_srealloc == NULL) call_srealloc = &srealloc;
  if (!call_sendMsg || !call_writeMsgToSysop) return 0;
  return 1;
}
