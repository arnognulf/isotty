/* 
   isotty.c - Convert TTY connections from one encoding to another.

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

$Id: ttyconv.c,v 1.4 2003/09/27 12:45:35 alexios Exp $

$Log: ttyconv.c,v $
Revision 1.4  2003/09/27 12:45:35  alexios
Better signal handling to deal with unusual end-of-file conditions.

Revision 1.3  2003/09/13 12:17:33  alexios
Improved (read: fixed) support for changing terminal window sizes.

Revision 1.2  2003/08/29 13:20:36  alexios
Removed superfluous debugging information.

Revision 1.1.1.1  2003/08/29 12:33:18  alexios
Initial revision.


*/

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <stropts.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
#include <pty.h>
#include <iconv.h>
#include "cmdline.h"
#include "isotty.h"
#include <stdlib.h>

#define max(a,b) ((a) > (b)? (a): (b))

static char const rcsid[] = "$Id: isotty.c,v 1.4 2003/09/27 12:45:35 alexios Exp $";

char *progname;

typedef struct {
	iconv_t iconv;
	char    inb  [1024];
	char    outb [8192];
	char    *readptr;
	char    *inbuf;
	char    *outbuf;
	size_t     nin;
	size_t     nout;
} translate_t;

static translate_t r2l, l2r;

static pid_t          child_pid = 0;
static struct termios tty_old_state;
static int            tty_state_set = 0;
static struct winsize winsize;
static int            tty_winsize_set = 0;
static int            master;


void stty_raw (int master)
{
	struct termios tty_state;
	
	if (tcgetattr (fileno (stdin), &tty_state) < 0) {
		fprintf (stderr,
			 "%s: warning: unable to get TTY state for standard input. "
			 "The session may start acting strange.\n", progname);
		return;
	} else {
		if (tcsetattr (master, TCSAFLUSH, &tty_state) < 0) {
			fprintf (stderr,
				 "%s: warning: unable to set TTY state for pseudoterminal. "
				 "The session may start acting strange.\n", progname);
		}

		memcpy (&tty_old_state, &tty_state, sizeof (tty_old_state));
	}

	if (ioctl (fileno (stdin), TIOCGWINSZ, &winsize)) {
		fprintf (stderr,
			 "%s: warning: unable to get window size for standard input. "
			 "The session may start acting strange.\n", progname);
		return;
	} else 	if (ioctl (master, TIOCSWINSZ, &winsize)) {
		fprintf (stderr,
			 "%s: warning: unable to get window size for standard input. "
			 "The session may start acting strange.\n", progname);
		tty_winsize_set = 1;
	}

	tty_state.c_lflag &= ~(ICANON | IEXTEN | ISIG | ECHO);
	tty_state.c_iflag &= ~(ICRNL | INPCK | ISTRIP | IXON | BRKINT);
	tty_state.c_oflag &= ~OPOST;
	tty_state.c_cflag |= CS8;
	
	tty_state.c_cc [VMIN]  = 1;
	tty_state.c_cc [VTIME] = 0;
	
	if (tcsetattr (fileno (stdin), TCSAFLUSH, &tty_state) < 0) {
		fprintf (stderr,
			 "%s: warning: unable to set TTY state for standard input. "
			 "The session may start acting strange.\n", progname);
	} else tty_state_set = 1;
}


void stty_orig (void)
{
	if (tty_state_set)
		tcsetattr (fileno (stdin), TCSAFLUSH, &tty_old_state);
}



int spawn_command ()
{
	int master;

	child_pid = forkpty (&master, NULL, NULL, NULL);
	
	if (child_pid < 0) {
		perror ("forkpty");
		exit (1);
	}

	/* This is the child process */
	
	if (child_pid == 0) {
		execvp (cmdline [0], cmdline);
		perror ("execvp()");
		exit(1);
	}

	return master;
}

int open_pty_pair (int *amaster, int *aslave)
{
	int master, slave;
	char *name;
		
	master = getpt ();
	if (master < 0) return 0;
	
	if (grantpt (master) < 0 || unlockpt (master) < 0)
		goto close_master;

	name = ptsname (master);
	fprintf(stderr, "MASTER=%s\n",name);
	if (name == NULL)
		goto close_master;
	
	slave = open (name, O_RDWR);
	if (slave == -1)
		goto close_master;
	
	if (isastream (slave)) {
		if (ioctl (slave, I_PUSH, "ptem") < 0
		    || ioctl (slave, I_PUSH, "ldterm") < 0)
			goto close_slave;
	}
	
	*amaster = master;
	*aslave = slave;
	return 1;
	
 close_slave:
	close (slave);
	
 close_master:
	close (master);
	return 0;
}


void iconv_init ()
{
	if ((l2r.iconv = iconv_open (remote,local)) == (iconv_t) -1) {
		fprintf (stderr,
			 "%s: unsupported conversion %s -> %s.\n"
			 "\nPerhaps you misspelled either of the encodings?\n"
			 "Use iconv -l to see a list of possible encodings.\n",
			 progname, local, remote);
		exit (1);
	}

	if ((r2l.iconv = iconv_open (local,remote)) == (iconv_t) -1) {
		fprintf (stderr,
			 "%s: unsupported conversion %s -> %s.\n"
			 "\nPerhaps you misspelled either of the encodings?\n"
			 "Use iconv -l to see a list of possible encodings.\n",
			 progname, remote, local);
		exit (1);
	}
}

void iconv_done ()
{
	iconv_close (l2r.iconv);
	iconv_close (r2l.iconv);
}

int translate (translate_t * t, int fdin, int fdout)
{
	int n, e;
	
	while (1) {
		n = read (fdin, t->readptr,
			  sizeof (t->inb) - (t->readptr - t->inb));
		
		/* End of file condition */
		if (n <= 0) return 1;
		t->readptr += n;
		t->nin += n;
		
		t->nout = sizeof (t->outb);
		
	try_again:
		
		n = iconv (t->iconv, &t->inbuf, &t->nin, &t->outbuf, &t->nout);
		e = errno;
		
		
		if (n < 0) {
			
			/* iconv reports an illegal sequence. Try to recover. */
			
			if (e == EILSEQ) {
				char q = '?';
				
				write (fdout, t->outb, sizeof (t->outb) - t->nout);
				write (fdout, &q, 1);
				
				t->inbuf++;
				t->nin--;
				
				t->outbuf = t->outb;
				t->nout = sizeof (t->outb);
				
				goto try_again;
				
			} else if (e == EINVAL) {
				
				/* iconv reports an incomplete sequence.  Keep
				   what we have so far and wait for the rest. */
				
				write (fdout, t->outb, sizeof(t->outb) - t->nout);
				
				memmove (t->inb, t->inbuf, t->nin);
				t->inbuf = t->inb;
				t->readptr = t->inb + t->nin;
				
				t->outbuf = t->outb;
				t->nout = sizeof(t->outb);
				
				continue;
			}
		} else {
			write (fdout, t->outb, sizeof(t->outb) - t->nout);
			
			t->outbuf = t->outb;
			t->inbuf = t->inb;
			t->readptr = t->inb;
			
			break;
		}
	}

	return 0;
}


struct winsize ws, ws0;

void child_winch (int signum, siginfo_t* foo, void* bar)
{
	if (ioctl (fileno (stdin), TIOCGWINSZ, &ws0)) return;

	if (memcmp (&ws, &ws0, sizeof (ws))) {
		if (ioctl (master, TIOCSWINSZ, &ws0) == 0) {
			kill (child_pid, SIGWINCH);
			memcpy (&ws, &ws0, sizeof (ws));
		}
	}
}


void child_graceful_exit (int signum)
{
	stty_orig();
	iconv_done();

	fprintf (stderr, "%s: session ended (by signal).\n", progname);

	exit (0);
}



void process (int master)
{
	int fdmax;
	sigset_t sigmask, orig_sigmask;
	fd_set readset, errset;
	struct sigaction act;
	struct timespec tp;

	/* Set up signal handling */


	sigemptyset( &sigmask);
	sigaddset( &sigmask, SIGWINCH);
	sigaddset( &sigmask, SIGCHLD);
	sigaddset( &sigmask, SIGPIPE);
	sigprocmask( SIG_UNBLOCK, &sigmask, &orig_sigmask);

	act.sa_flags = SA_NODEFER;
	act.sa_mask = sigmask;

	act.sa_handler = child_winch;
	sigaction (SIGWINCH, &act, NULL);

	act.sa_handler = child_graceful_exit;
	sigaction (SIGCHLD, &act, NULL);
	sigaction (SIGPIPE, &act, NULL);


	/* Calculate maximum FD for pselect(2) */

	fdmax = max (fileno (stdin), fileno (stdout));
	fdmax = max (fdmax, fileno (stderr));
	fdmax = max (fdmax, master);

	/* Initialise input and output translation buffers */

	r2l.readptr = r2l.inb;
	r2l.inbuf = r2l.inb;
	r2l.outbuf = r2l.outb;

	l2r.readptr = l2r.inb;
	l2r.inbuf = l2r.inb;
	l2r.outbuf = l2r.outb;

	/* Go. */

	while (1) {
		int sel;

		FD_ZERO (&readset);
		FD_SET (fileno (stdin), &readset);
		FD_SET (master, &readset);

		FD_ZERO (&errset);
		FD_SET (fileno (stdin), &errset);
		FD_SET (master, &errset);

		tp.tv_sec = 0L;
		tp.tv_nsec = 500000000L;

		sel = pselect (fdmax + 1, &readset, NULL, &errset, &tp, &sigmask);

		/* We timed out. */
		if ((sel == 0) && (errno == 0)) continue;

		/* Are we done yet? */
		if ((sel < 1) && (errno != EINTR) && (errno > 0)) break;

		/* Received input from the user */
		if (FD_ISSET(fileno(stdin), &readset)) {
			if (translate (&l2r, fileno (stdin), master)) break;
		}

		/* Received output from the remote end */
		if (FD_ISSET(master, &readset)) {
			if (translate (&r2l, master, fileno (stdout))) break;
		}

		/* Something nasty happened, terminate */
		if (FD_ISSET( master, &errset) ||
		    FD_ISSET( fileno(stdin), &errset)) break;
	}
}

#define BUFSZ 256
char* detect()
{
	// hide the cursor and try to set the text to invisible since the magic below produces garbage on the screen
	const char set_invisible[] = "\033[30m\033[?25l\r";
	// magic string - not to be spoken out loud
	const char magic[] = "\xc3\xb8\xa5\xc3\x84\xd9\xa7\xd8\xb8\xe0\xe0\xa9\xa9\xb8\x88\xe5\xe5\x88\xa2\xa2";
	// set the cursor to visible - otherwise the cursor may be away to long and the user may notice
	const char set_visible[]="\033[D\033[6n\033[40D                                       \033[40D\r\033[?25h";
	// erase the magic string and restore terminal to sane defaults
	const char restore[] = "\r\[A\033[20D                    \033[20D\033[0m";
	char termreply[BUFSZ];
	int i;
	FILE* ttydev = 0;
	
	// bypass stdout and stderr
	ttydev = fopen("/dev/tty", "a");
	if ( NULL == ttydev) {
		goto exit;
	}
	printf("%s", set_invisible, magic, set_invisible);
	i = 0;
	while ( c = getchar() && i < 9) {
		*termreply++;
		switch (c) {
			case '\033':;
			case '0':;
			case '1':;
			case '2':;
			case '3':;
			case '4':;
			case '5':;
			case '6':;
			case '7':;
			case '8':;
			case '9':;
			case ';': *termreply = c;break;
			case 'R': *termreply = c; i = 9;break;
			default: break;
		}
	}

	// restore terminal
	print("%s", restore);
#if 0
case "$CURPOS" in
## UTF-8 detection
## mlterm: 7
## xterm: 14
## gnome-terminal: 13
*";7R"|*";10R"|*";11R"|*";12R"|*";13R"|*";14R") HOSTENC=UTF-8 ;;
## legacy korean encoding: CP949 / UHC / JOHAB
*";18R") HOSTENC=UHC;;
*";24R") HOSTENC=BIG5;;
## tested with screen EUC-CN to UTF-8 translation:
*";16R") HOSTENC=EUC-CN;;
## following tested with gnome-terminal
*";21R") HOSTENC=EUC-KR;;
*";23R") HOSTENC=EUC-JP;;
*";28R") HOSTENC=EUC-TW;;
## 8 bit encoding, encoding detection not possible.
## putty: 17R
## generic: 20R
*";17R"|*";20R") : ;;
*) test "x$DETECTENC" = x1 && STDERR "\
terminal responded: $CURPOS
please send this response, along with your host encoding and terminal setup to <arne@users.sourceforge.net>";;
esac
#endif // 0
exit:
	
}
#define LOCALE_NAME_LENGTH 1024
int main (int argc, char **argv)
{
	progname = argv[0];
	char local_locale[LOCALE_NAME_LENGTH];
	char remote_locale[LOCALE_NAME_LENGTH];
	// detect encoding
	detect_local( &local_locale);
	detect_remote( &remote_locale, argc, &argv);

	// set environment variables
	//getenv(3)
	//setenv(3)

	cmdline_parse (argc, argv);
	master = spawn_command ();
	stty_raw (master);
	iconv_init ();

	process (master);

	iconv_done ();
	stty_orig ();
	
	fprintf (stderr, "%s: session ended.\n", progname);
	
	return EXIT_SUCCESS;
}
