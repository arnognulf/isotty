#!/bin/sh
#
# $Id: autogen.sh,v 1.1.1.1 2003/08/29 12:33:18 alexios Exp $
#
# $Log: autogen.sh,v $
# Revision 1.1.1.1  2003/08/29 12:33:18  alexios
# Initial revision.
#

# Partially (and shamelessly) stolen from XMMS

(automake --version) < /dev/null > /dev/null 2>&1 || {
	echo;
	echo "You must have automake installed";
	echo;
	exit;
}

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
	echo;
	echo "You must have autoconf installed";
	echo;
	exit;
}

echo "Generating configuration files, please wait...."
echo;

aclocal $ACLOCAL_FLAGS;
autoheader;
automake --add-missing;
autoconf;

./configure $@
