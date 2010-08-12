#!/bin/bash
###################################################################
## isotty - a utility to run a command in a different encoding
##
## Copyright (C) 2008 Thomas Eriksson <arne@users.sourceforge.net>
## 
## This program is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License
## as published by the Free Software Foundation; either version 2
## of the License, or (at your option) any later version.
## 
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
## 
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
##
####################################################################
## ver 1.5
####################################################################

##FIXME: exotic systems as IRIX do not support UTF-8 in iconv, there UTF-8 support must be had through screen

## this hack is needed since vim and joe (and probably others)
## hangs upon startup from ttyconv, and I can't seem to find the problem
## perhaps only happens on amd64?
case "$1" in
--UGLYTTYCONVHACK)
sleep 0.1 2>/dev/null || sleep 1 ## comment this line to remove the workaround
shift
exec "$@"
esac


STDERR ()
{
	echo "${0##*/}: $@" 1>&2 | echo - 1>/dev/null
	echo "${0##*/}: exit"  1>&2 | echo - 1>/dev/null
	exit 1
}

## default to iso8859-1(5)
LATIN1=1
locale -a|grep iso885915 &>/dev/null && LATIN1=15
if test "x$LATIN1" = x15
then
	HOSTENC=ISO8859-15
else
	HOSTENC=ISO8859-1
fi

## load fallback encoding
test -f "${HOME}/.isottyenc" && . "${HOME}/.isottyenc"


## is ttyconv required
TTYCONV=0

## special case for obsolete 8-bit encodings not available in any locale
OBSOLETE_ENCODING=0

## select encodings based on filename
case "${0##*/}" in
## encodings supported only by ttyconv
### http://www.bedroomlan.org/~alexios/coding_ttyconv.html
### http://www.gnu.org/software/libiconv/
oemtty|cp437tty) ## US (DOS legacy), Popular ASCII-art encoding
OBSOLETE_ENCODING=1; TARGETENC=CP437; TTYCONV=1;;
## encodings supported by GNU screen and ttyconv
eucjptty) TARGETENC=eucJP;;
sjistty) TARGETENC=SJIS;;
euckrtty) TARGETENC=eucKR;;
euccntty) TARGETENC=eucCN;;
big5tty) TARGETENC=Big5;;
gbktty) TARGETENC=GBK;;
koi8rtty) TARGETENC=KOI8-R;;
cp1251tty) TARGETENC=CP1251;;
utf8tty) TARGETENC=UTF-8;;
iso2tty) TARGETENC=ISO8859-2;;
iso3tty) TARGETENC=ISO8859-3;;
iso4tty) TARGETENC=ISO8859-4;;
iso5tty) TARGETENC=ISO8859-5;;
iso6tty) TARGETENC=ISO8859-6;;
iso7tty) TARGETENC=ISO8859-7;;
iso8tty) TARGETENC=ISO8859-8;;
iso9tty) TARGETENC=ISO8859-9;;
iso10tty) TARGETENC=ISO8859-10;;
jistty) TARGETENC=jis;;
*) 
	if test "x$LATIN1" = x15
		then
			TARGETENC=ISO8859-15
		else
			TARGETENC=ISO8859-1
		fi
esac

## fallback host encoding
case "$1" in
--sjis|-s) HOSTENC=SJIS;;
--gbk|-g) HOSTENC=GBK;;
--koi8r|-r) HOSTENC=KOI8-R;;
--cp1251|-w) HOSTENC=CP1251;;
--iso1|-1) HOSTENC=ISO8859-1;;
--iso2|-2) HOSTENC=ISO8859-2;;
--iso3|-3) HOSTENC=ISO8859-3;;
--iso4|-4) HOSTENC=ISO8859-4;;
--iso5|-5) HOSTENC=ISO8859-5;;
--iso6|-6) HOSTENC=ISO8859-6;;
--iso7|-7) HOSTENC=ISO8859-7;;
--iso8|-8) HOSTENC=ISO8859-8;;
--iso9|-9) HOSTENC=ISO8859-9;;
--iso10|-10) HOSTENC=ISO8859-10;;
--iso15|-15) HOSTENC=ISO8859-15;;
--jis) HOSTENC=jis;;
-d|--detect) DETECTENC=1;shift;;
--) shift;;
--help|-h) ## use stdout so people can grep through the numerious options ;)
printf "\
Usage: ${0##*/} [<OPTION>] [CMD] [ARGS]...

Run a command or shell in $TARGETENC encoding.

Debugging:
  -d,  --detect\tPrint detected encoding and exit

Options to set the host fallback encoding:

Japanese encodings:
  -s,  --sjis\tSJIS Encoding
       --jis\tJIS Encoding

Traditional chinese encodings:
  -g,  --gbk\tGBK Encoding

Cyrillic encodings:
  -r,  --koi8r\tKOI8-R Encoding
  -w,  --cp1251\tCP1251 Encoding

Latin encodings:
  -1,  --iso1\tISO8859-1 Encoding
  -2,  --iso2\tISO8859-2 Encoding
  -3,  --iso3\tISO8859-3 Encoding
  -4,  --iso4\tISO8859-4 Encoding
  -5,  --iso5\tISO8859-5 Encoding
  -6,  --iso6\tISO8859-6 Encoding
  -7,  --iso7\tISO8859-7 Encoding
  -8,  --iso8\tISO8859-8 Encoding
  -9,  --iso9\tISO8859-9 Encoding
  -10, --iso10\tISO8859-10 Encoding
  -15, --iso15\tISO8859-15 Encoding

The following host encodings are auto-detected:
  UTF-8, EUC-CN, EUC-JP, EUC-KR, EUC-TW, Big5, UHC

Fallback encoding if auto-detection fails is set to ${HOSTENC}

Comments and bug reports to <arne@users.sourceforge.net>
"
exit 1
;;
*) DETECTENC=0 ;;
esac
case "$1" in
-*) printf "HOSTENC=${HOSTENC}\n" > "${HOME}/.isottyenc"
STDERR "fallback encoding set to ${HOSTENC}";;
esac

## detect presence of ttyconv
case $(type -p ttyconv) in
ttyconv*|"") test "x$TTYCONV" = 1 && STDERR "error: ttyconv required for target encoding\n";;
*) TTYCONV=1;;
esac

## create screen config file
CONFIG="$HOME/.isottyrc"
if test -f "$CONFIG"
then
	rm "$CONFIG"
	case $? in
	0):;;
	*) STDERR "error: could not create config\n";;
	esac
fi

## get locale environment variables
eval $(locale)

cat >"$CONFIG" <<EOF
startup_message off
deflogin on
vbell off
defscrollback 1024
termcapinfo vt100 dl=5\\E[M
hardstatus off

termcapinfo xterm*|rxvt*|kterm*|Eterm* hs:ts=\\E]0;:fs=\\007:ds=\\E]0;\\007
hardstatus string "%h%? users: %u%?"
termcapinfo xterm*|linux*|rxvt*|Eterm* OP
termcapinfo xterm 'is=\E[r\E[m\E[2J\E[H\E[?7h\E[?1;4;6l'
sessionname ${0##*/}
setenv LC_CTYPE "${LC_CTYPE%%.*}.${TARGETENC}"
setenv LC_ALL "${LANG%%.*}.${TARGETENC}"
EOF

printf "defencoding $TARGETENC\n" >>"$CONFIG"

test "x$TERM" = xwyse60 && STDERR "error: wyse60 compatibility mode not supported"

## the magic sequence originally comes from mined: http://towo.net/mined/

## print a sequence of bytes which encodes differently in different encodings
## based on the cursor position, we may determine the terminal encoding.
printf "\033[30m\033[?25l\r\
\xc3\xb8\xa5\xc3\x84\xd9\xa7\xd8\xb8\xe0\xe0\xa9\xa9\xb8\x88\xe5\xe5\x88\xa2\xa2\
\033[D\033[6n\033[40D                                       \033[40D\r\033[?25h" >/dev/tty

i=0
until test "$i" = 9
do
	CHAR=""
	read -n1 CHAR
	case "$CHAR" in
	|[|[0-9]|\;) CURPOS="$CURPOS$CHAR";let i++;;
	R)CURPOS="$CURPOS$CHAR";break;;
	esac
done

## clear terminal response
printf "\r\[A[20D                    [20D[0m" >/dev/tty

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

## print detected (or fallback) encoding
## for debugging purposes
if test "x$DETECTENC" = x1
then
	printf "$HOSTENC\n"
	exit 0
fi

## prefer ttyconv, fallback to screen
if test "x$TTYCONV" = x1
then
	if test "x$OBSOLETE_ENCODING" = x1
	then
		export LC_CTYPE="${LC_CTYPE%%.*}.ISO8859-1"
		export LC_ALL="${LANG%%.*}.ISO8859-1"
	else
		export LC_CTYPE="${LC_CTYPE%%.*}.${TARGETENC}"
		export LC_ALL="${LANG%%.*}.${TARGETENC}"
	fi
	if test "x$1" = x
	then
		ttyconv --local="$HOSTENC" --remote="$TARGETENC"
	else
		ttyconv --local="$HOSTENC" --remote="$TARGETENC" "$0" --UGLYTTYCONVHACK "$@"
	fi
else
	## sane locale values for screen
	export LC_CTYPE="${LANG%%.*}.${HOSTENC}"
	export LC_ALL="${LANG%%.*}.${HOSTENC}"

	## do not open a new window inside a currently running screen
	## instead, create a new screen
	STY="" screen -c "$CONFIG" "$@"
fi
