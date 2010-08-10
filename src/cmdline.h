/* 
   cmdline.h - Command-line parser interface.

   Copyright (C) 2002 Alexios Chouchoulas

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  

$Id: cmdline.h,v 1.1.1.1 2003/08/29 12:33:18 alexios Exp $

$Log: cmdline.h,v $
Revision 1.1.1.1  2003/08/29 12:33:18  alexios
Initial revision.


*/


#ifndef __CMDLINE_H
#define __CMDLINE_H

#include <argp.h>

extern struct argp_option options[];

/* argp option keys */

enum {DUMMY_KEY=129
      ,BRIEF_KEY
      ,KEY_DEFAULT
      ,KEY_TEST
      ,KEY_ID
      ,KEY_OFF
      ,KEY_INFO
      ,KEY_GETALMANAC
      ,KEY_SENDALMANAC
      ,KEY_GETTRACKS
      ,KEY_SENDTRACKS
      ,KEY_GETWAYPOINTS
      ,KEY_SENDWAYPOINTS
      ,KEY_GETROUTES
      ,KEY_SENDROUTES
      ,KEY_TIME
      ,KEY_BACKUP
      ,KEY_RESTORE
};

/* Option flags and variables.  These are initialized in parse_opt.  */

extern char *local, *remote;
extern char ** cmdline;
extern struct argp argp;

void cmdline_parse (int argc, char ** argv);

#endif /* __CMDLINE_H */
