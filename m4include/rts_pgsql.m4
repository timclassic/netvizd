dnl @synopsis RTS_PGSQL
dnl
dnl This macro tries to find the headers and librarys for the
dnl PostgreSQL database to build client applications.
dnl
dnl If includes are found, the variable PQINCPATH will be set. If
dnl librarys are found, the variable PQLIBPATH will be set. if no check
dnl was successful, the script exits with a error message.
dnl
dnl @category InstalledPackages
dnl @author Tim Stewart <tim@stoo.org>
dnl @version 2005-07-27
dnl @license AllPermissive

AC_DEFUN([RTS_PGSQL], [

AC_ARG_WITH(pgsql,
	[  --with-pgsql=PREFIX     Prefix of your PostgreSQL installation],
	[pg_prefix=$withval], [pg_prefix=])
AC_ARG_WITH(pgsql-inc,
	[  --with-pgsql-inc=PATH   Path to the include directory of PostgreSQL],
	[pg_inc=$withval], [pg_inc=])
AC_ARG_WITH(pgsql-lib,
	[  --with-pgsql-lib=PATH   Path to the librarys of PostgreSQL],
	[pg_lib=$withval], [pg_lib=])



dnl Save current flags
oCPPFLAGS="$CPPFLAGS"
oLDFLAGS="$LDFLAGS"

dnl Set paths if prefix is defined
if test "$pg_prefix" != ""; then
	tmp_PQINCPATH="-I$pg_prefix/include"
	tmp_PQLIBPATH="-L$pg_prefix/lib"
fi

dnl Set include and lib paths if defined individually
if test "$pg_inc" != ""; then
	tmp_PQINCPATH="-I$pg_inc"
fi
if test "$pg_lib" != ""; then
	tmp_PQLIBPATH="-L$pg_lib"
fi

dnl Update flags for testing
CPPFLAGS="$tmp_PQINCPATH $CPPFLAGS"
LDFLAGS="$tmp_PQLIBPATH $LDFLAGS"

dnl Do checks
AC_CHECK_HEADER([libpq-fe.h], [
	PQINCPATH="$tmp_PQINCPATH"
], [
	echo "*** I cannot find the PostgreSQL headers on your system."
	echo "*** Please check config.log for errors and try the"
	echo "*** --with-pgsql* configure options."
	exit 1
])
AC_CHECK_LIB(pq, PQconnectdb, [
	PQLIBPATH="$tmp_PQLIBPATH"
], [
	echo "*** I cannot find the PostgreSQL libraries on your system."
	echo "*** Please check config.log for errors and try the"
	echo "*** --with-pgsql* configure options."
	exit 1
])
	
dnl Restore original flags
CPPFLAGS="$oCPPFLAGS"
LDFLAGS="$oLDFLAGS"

AC_SUBST(PQINCPATH)
AC_SUBST(PQLIBPATH)

])
