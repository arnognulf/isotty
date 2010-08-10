/* 
   cmdline.c - Command-line parser.

   Copyright (C) 2003 Alexios Chouchoulas

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

$Id: cmdline.c,v 1.4 2004/09/07 23:17:19 alexios Exp $

$Log: cmdline.c,v $
Revision 1.4  2004/09/07 23:17:19  alexios
Updated URL and email addresses.

Revision 1.3  2003/08/29 13:33:29  alexios
Removed credit to libjeeps author (again a leftover from initialising this
project from garmintools).

Revision 1.2  2003/08/29 13:19:25  alexios
Removed a superfluous command line argument.

Revision 1.1.1.1  2003/08/29 12:33:18  alexios
Initial revision.


*/

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include "ttyconv.h"
#include "config.h"


static char const rcsid[] = "$Id: cmdline.c,v 1.4 2004/09/07 23:17:19 alexios Exp $";


/* Option flags and variables.  These are initialized in parse_opt.  */

char *local=NULL;		/* --local=LOCAL-ENCODING */
char *remote=NULL;		/* --remote=REMOTE-ENCODING */
char **cmdline=NULL;		/* SHELL-COMMAND */


static error_t parse_opt (int key, char *arg, struct argp_state *state);
static void show_version (FILE *stream, struct argp_state *state);


/* The argp functions examine these global variables.  */
const char *argp_program_bug_address = "<thomas.eriksson@gmail.com>";
void (*argp_program_version_hook) (FILE *, struct argp_state *) = show_version;


static char args_doc[] = "-l LOCAL-ENCODING -r REMOTE-ENCODING [SHELL-COMMAND]";


struct argp argp =
{
	options, parse_opt, args_doc,
	"Convert TTY connections from one encoding to another.\n\nA"
	"pseudo-terminal is set up with encoding REMOTE-ENCODING. The system"
	"is assumed to be in the specified LOCAL-ENCODING. The optional"
	"SHELL-COMMAND is executed. In default of a SHELL-COMMAND, the user's"
        "shell is executed (as specified in the SHELL environment variable)."
	"Input from the local side to the SHELL-COMMAND is translated from"
        "LOCAL-ENCODING to REMOTE-ENCODING. Output from the SHELL-COMMAND to"
        "the local side is translated inversely.",
	NULL, NULL, NULL
};


struct argp_option options[] =
{
	{ "remote",         'r',     "ENCODING",             0,
	  "Specify remote encoding"},
	
	{ "local",      'l',           "ENCODING",      0,
	  "Specify local encoding", 0 },
	
	{ NULL, 0, NULL, 0, NULL, 0 }
};






/* Show the version number and copyright information.  */
static void show_version (FILE *stream, struct argp_state *state) 
{
	(void) state;
	/* Print in small parts whose localizations can hopefully be copied
	   from other programs.  */
	fputs(PACKAGE" "VERSION"\n\n", stream);
	fprintf(stream, 
		"isotty by Thomas Eriksson <thomas.eriksson@gmail.com>\n"
		"Based on ttyconv 0.2.3 by Alexios Chouchoulas <alexios@bedroomlan.org>.\n"
		"Copyright (C) %s %s\n"
		"Copyright (C) %s %s\n"
		"This program is free software; you may redistribute it under the terms of\n"
		"the GNU General Public License.  This program has absolutely no warranty.\n",
		"2010", "Thomas Eriksson", 
		"2003", "Alexios Chouchoulas");
}


/* Parse a single option.  */
static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
	switch (key) {
	case 'l':			/* --local */
		local = strdup (arg);
		break;
		
	case 'r':			/* --remote */
		remote = strdup (arg);
		break;
		
         case ARGP_KEY_ARG:
		 /* End processing here. */
		 cmdline = &state->argv [state->next - 1];
		 state->next = state->argc;
		 break;

	default:
		return ARGP_ERR_UNKNOWN;
	}
	
	return 0;
}

void
cmdline_parse (int argc, char ** argv)
{
	/* Parse the arguments */
	
	argp_parse (&argp, argc, argv, ARGP_IN_ORDER, NULL, NULL);

	/* Validate the arguments */
	
	if ((local == NULL) || (remote == NULL)) {
		fprintf (stderr,
			 "%s: both local and remote encodings should be specified.\n"
			 "\nUse %s --help for usage information.\n",
			 progname,progname);
		exit (1);
	}

	/* Set up the default command, if one is needed. */

	if (cmdline == NULL) {
		if (NULL == (cmdline = malloc (2 * sizeof (char *)))) {
			perror ("malloc()");
			exit (1);
		}
		
		cmdline [0] = getenv ("SHELL");
		if (NULL == cmdline [0]) {
			cmdline [0] = strdup("/bin/sh");
		}
		cmdline [1] = NULL;

	}
}
