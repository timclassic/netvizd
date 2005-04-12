AC_PREREQ(2.59)
AC_INIT(netvizd, 0.1)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_AWK
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_YACC
AM_PROG_LEX
AC_PATH_PROG([SED], [sed])

# Checks for libraries.
# FIXME: Replace `main' with a function in `-ldl':
#AC_CHECK_LIB([dl], [main])

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_LSTAT
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRERROR_R
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([bzero strerror])

# Check for pthreads
ACX_PTHREAD([],[
	echo "*** I cannot figure out how to use POSIX threads on your system."
	echo "*** Please make sure the correct libraries are installed."
	exit 1
])
CC="$PTHREAD_CC"
CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
LIBS="$LIBS $PTHREAD_LIBS"

# Figure out how to use sockets
ETR_SOCKET_NSL
LIBS="$LIBS $ETR_SOCKET_LIBS"

# Set up ltdl and libtool
AC_LIBLTDL_CONVENIENCE
AC_SUBST([LTDLINCL])
AC_SUBST([LIBLTDL])
AC_LIBTOOL_DLOPEN
AM_PROG_LIBTOOL
AC_CONFIG_SUBDIRS([libltdl])

AC_CONFIG_FILES([Makefile
				 src/Makefile
				 src/plugins/Makefile
				 src/plugins/config/Makefile
				 src/plugins/storage/Makefile
				 src/plugins/sensor/Makefile
				 src/plugins/proto/Makefile
				 src/plugins/auth/Makefile])

AC_OUTPUT
