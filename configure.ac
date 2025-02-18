#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([tn5250],[0.18.0],[https://github.com/tn5250/tn5250/issues])
AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_AUX_DIR([build-aux])

AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_HEADERS(config.h)

#
# Automake
#
AH_TEMPLATE([SOCKET_TYPE], [Define to your system's socket descriptor type.])
AH_TEMPLATE([USE_OWN_KEY_PARSING], [Define to let cursesterm handle key mapping.])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
LT_INIT

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h locale.h sys/wait.h sys/time.h syslog.h unistd.h pwd.h])

# True for anything other than Windoze.
AC_DEFINE_UNQUOTED(SOCKET_TYPE,int)

# User specified optional packages
AC_ARG_WITH([curses-old-keys],AS_HELP_STRING([--with-curses-old-keys],[curses terminal will use old key handler (default=no)]),[ac_cv_use_old_keys=$withval],[ac_cv_use_old_keys=no]) AC_CACHE_CHECK([whether to use old key handler],[ac_cv_use_old_keys], [ac_cv_use_old_keys=no])

# run AC_CHECK_LIB this way to avoid linking curses into every executable
AC_CHECK_LIB(ncurses, initscr, CURSES_LIB=-lncurses)
if test "$ac_cv_lib_ncurses_initscr" != "yes"
then
    # Check for -lcurses if -lncurses isn't found.
    AC_CHECK_LIB(curses, initscr, CURSES_LIB=-lcurses)
    if test "$ac_cv_lib_curses_initscr" != "yes"
    then
        AC_MSG_ERROR([** You need a curses-compatible library installed.])
    fi
fi

if test "$ac_cv_use_old_keys" != "yes";
then
    AC_DEFINE_UNQUOTED(USE_OWN_KEY_PARSING,1)
fi

AC_ARG_WITH([ssl],AS_HELP_STRING([--with-ssl],[OpenSSL support (default=yes)]),[ac_cv_use_ssl=$withval],[ac_cv_use_ssl=yes]) AC_CACHE_CHECK([whether to build with OpenSSL support],[ac_cv_use_ssl], [ac_cv_use_ssl=yes])

if test "$ac_cv_use_ssl" = "yes"
then
dnl **
dnl ** Check for OpenSSL include files.
dnl ** They can be in any of a number of standard places.   Most
dnl ** operating systems want them in /usr/include or /usr/local/include,
dnl ** but the default OpenSSL install puts them in
dnl ** ${PREFIX}/ssl/include/openssl
checksslinclude() {
    if test -f "$1/include/openssl/ssl.h"; then
        sslincludedir="-I$1/include"
        return 0
    fi
    return 1
}
AC_MSG_CHECKING([for OpenSSL headers])
AC_ARG_WITH([ssl-dir],AS_HELP_STRING([--with-ssl-dir],[directory for OpenSSL headers]),[
    if test "$with_ssl_dir" != "no"; then
         checksslinclude "$with_ssl_dir"
    else
        for topdir in /usr/local /usr /usr/pkg /usr/lib /var/ssl /opt; do
            for subdir in $topdir $topdir/openssl $topdir/ssl; do
                checksslinclude "$subdir" && break 2
            done
        done
     fi
     if test -z "$sslincludedir"; then
          AC_MSG_ERROR([** Unable to find OpenSSL headers! Specify --with-ssl-dir=DIR to specify a location.])
     fi
],[
      for topdir in /usr/local /usr /usr/pkg /usr/lib /var/ssl /opt; do
          for subdir in $topdir $topdir/openssl $topdir/ssl; do
              checksslinclude "$subdir" && break 2
          done
      done
])
if test -z "$sslincludedir"; then
     AC_MSG_RESULT([not found])
else
     AC_MSG_RESULT([$sslincludedir])
fi
AC_SUBST(sslincludedir)

dnl **
dnl ** Check for OpenSSL libraries.
dnl ** happily, we don't have to hunt for them thanks to ldconfig!
dnl **
if test -n "$sslincludedir"; then
     CPPFLAGS="$CPPFLAGS $sslincludedir"
     AC_CHECK_LIB(crypto,OPENSSL_init)
     if test "$ac_cv_lib_crypto_OPENSSL_init" != "yes"
     then
             AC_MSG_ERROR([** Unable to find OpenSSL libraries!])
     fi
     AC_CHECK_LIB(ssl,OPENSSL_init_ssl)
     if test "$ac_cv_lib_ssl_OPENSSL_init_ssl" != "yes"
     then
             AC_MSG_ERROR([** Unable to find OpenSSL libraries!])
     fi
fi

dnl **
dnl ** Is OpenSSL going to require Kerberos?
dnl **
dnl ** In RedHat 9 (and who knows where else?) OpenSSL is compiled with
dnl ** kerberos support, and so it has to be able to find the krb5.h
dnl ** file... which naturally isn't in the standard /usr/include dir
dnl ** since that would be much too easy.
dnl **
checkkrb5config() {
   if test -x "$1/krb5-config"; then
       krb5includedir="`$1/krb5-config --cflags`"
       return 0
   fi
   if test -x "$1/bin/krb5-config"; then
       krb5includedir="`$1/bin/krb5-config --cflags`"
       return 0
   fi
   return 1
}
if test -n "$sslincludedir"; then
   AC_MSG_CHECKING([if OpenSSL needs krb5.h])
   needkrb5=yes
   AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
]], [[SSL_CTX *ssl_context=NULL;]])],[needkrb5=no],[])
   if test $needkrb5 = "yes"; then
       krb5config="`which krb5config 2> /dev/null`"
       if test -z "$krb5config"; then
           krb5config="nonexistent"
       fi
       if test -x "$krb5config"; then
           krb5includedir="`$krb5config --cflags`"
       else
           for topdir in /usr/local /usr /opt /usr/lib /usr/pkg; do
               for subdir in $topdir $topdir/kerberos $topdir/krb5; do
                    checkkrb5config "$subdir" && break 2
               done
           done
       fi
       if test -n $krb5includedir; then
           CPPFLAGS="$CPPFLAGS $krb5includedir"
           needkrb5=no
	      AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
]], [[SSL_CTX *ssl_context=NULL;]])],[needkrb5=yes],[])
       fi
   fi
   AC_SUBST(krb5includedir)
   AC_MSG_RESULT([$needkrb5])
fi
fi

AC_SUBST([CURSES_LIB])

AC_CONFIG_FILES([Makefile
		 curses/Makefile
		 doc/Makefile
		 lib5250/Makefile
		 lp5250d/Makefile
		 termcaps/freebsd/Makefile
		 termcaps/linux/Makefile
		 termcaps/sun/Makefile
		 win32/Makefile
		 xt5250])
AC_OUTPUT
