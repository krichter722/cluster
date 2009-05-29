
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63b])
AC_INIT([cluster], [master], [linux-cluster@redhat.com])
AM_INIT_AUTOMAKE
LT_PREREQ([2.2.7])
LT_INIT

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([config/plugins/xml/config.c])
AC_CONFIG_HEADERS([make/config.h])

AC_CANONICAL_HOST

AC_LANG([C])

# Sanitize path

if test "$prefix" = "NONE"; then
	prefix="/usr"
	if test "$localstatedir" = "\${prefix}/var"; then
		localstatedir="/var"
	fi
	if test "$sysconfdir" = "\${prefix}/etc"; then
		sysconfdir="/etc"
	fi
	if test "$libdir" = "\${exec_prefix}/lib"; then
		if test -e /usr/lib64; then
			libdir="/usr/lib64"
		else
			libdir="/usr/lib"
		fi	
	fi
fi

case $exec_prefix in
  NONE)   exec_prefix=$prefix;;
  prefix) exec_prefix=$prefix;;
esac

# Checks for programs.

# check stolen from gnulib/m4/gnu-make.m4
if ! ${MAKE-make} --version /cannot/make/this >/dev/null 2>&1; then
	AC_MSG_ERROR([you don't seem to have GNU make; it is required])
fi

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LN_S
AC_PROG_INSTALL
AC_PROG_MAKE_SET

## local helper functions

# this function checks if CC support options passed as
# args. Global CFLAGS are ignored during this test.
cc_supports_flag() {
	local CFLAGS="$@"
	AC_MSG_CHECKING([whether $CC supports "$@"])
	AC_COMPILE_IFELSE([int main(){return 0;}] ,
			  [RC=0; AC_MSG_RESULT([yes])],
			  [RC=1; AC_MSG_RESULT([no])])
	return $RC
}

# this function tests if a library has a certain function
# by using AC_CHECK_LIB but restores the original LIBS global
# envvar. This is required to avoid libtool to link everything
# with everything.
check_lib_no_libs() {
	AC_CHECK_LIB([$1], [$2],,
		     [AC_MSG_ERROR([Unable to find $1 library])])
	LIBS=$ac_check_lib_save_LIBS
}

# corosync libs
PKG_CHECK_MODULES([corosync],[corosync])
PKG_CHECK_MODULES([cfg],[libcfg])
PKG_CHECK_MODULES([confdb],[libconfdb])
PKG_CHECK_MODULES([coroipcc],[libcoroipcc])
PKG_CHECK_MODULES([quorum],[libquorum])
PKG_CHECK_MODULES([votequorum],[libvotequorum])

# external libs
PKG_CHECK_MODULES([xml],[libxml-2.0])

# external libs (no pkgconfig)
check_lib_no_libs pthread pthread_mutex_lock
check_lib_no_libs ldap ldap_initialize
check_lib_no_libs rt clock_gettime
check_lib_no_libs z crc32

# Checks for header files.
AC_FUNC_ALLOCA
AC_CHECK_HEADERS([arpa/inet.h fcntl.h inttypes.h limits.h netdb.h netinet/in.h stddef.h stdint.h stdlib.h string.h sys/ioctl.h sys/param.h sys/socket.h sys/time.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_CHECK_MEMBERS([struct stat.st_rdev])
AC_TYPE_UID_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_HEADER_MAJOR
AC_FUNC_MALLOC
AC_CHECK_FUNCS([alarm atexit clock_gettime dup2 ftruncate gethostname gettimeofday memmove memset select socket strcasecmp strchr strdup strerror strncasecmp strndup strstr strtol uname])

# local options
AC_ARG_ENABLE([debug],
	[  --enable-debug          enable debug build. ],
	[ default="no" ])

AC_ARG_WITH([lcrso-dir],
	[  --with-lcrso-dir=DIR    corosync lcrso files. ],
	[ LCRSODIR="$withval" ],
	[ LCRSODIR="$libexecdir/lcrso" ])

AC_ARG_WITH([default-config-dir],
	[  --default-config-dir=DIR
                          cluster config directory. ],
	[ DEFAULT_CONFIG_DIR="$withval" ],
	[ DEFAULT_CONFIG_DIR="$sysconfdir/cluster" ])

AC_ARG_WITH([default-config-file],
	[  --default-config-file=FILE
                          cluster config file. ],
	[ DEFAULT_CONFIG_FILE="$withval" ],
	[ DEFAULT_CONFIG_FILE="cluster.conf" ])

AC_ARG_WITH([syslogfacility],
	[  --syslogfacility=FACILITY
                          cluster default syslog facility. ],
	[ SYSLOGFACILITY="$withval" ],
	[ SYSLOGFACILITY="LOG_LOCAL4" ])

AC_ARG_WITH([sysloglevel],
	[  --sysloglevel=LEVEL
                          cluster default syslog level. ],
	[ SYSLOGLEVEL="$withval" ],
	[ SYSLOGLEVEL="LOG_INFO" ])

AC_ARG_ENABLE([bindings],
	[  --enable-bindings       enable bindings build. ],
	[ default="no" ])

## random vars

NOTIFYDDIR=${DEFAULT_CONFIG_DIR}/cman-notify.d
LOGDIR=${localstatedir}/log/cluster
LOGROTATEDIR=${sysconfdir}/logrotate.d
CLUSTERVARRUN=${localstatedir}/run/cluster
CLUSTERVARLIB=${localstatedir}/lib/cluster

## do subst

AC_SUBST([LCRSODIR])
AC_DEFINE_UNQUOTED([LCRSODIR], "$(eval echo ${LCRSODIR})", [LCRSO directory])

AC_SUBST([DEFAULT_CONFIG_DIR])
AC_DEFINE_UNQUOTED([DEFAULT_CONFIG_DIR], "$(eval echo ${DEFAULT_CONFIG_DIR})",
		   [Default config directory])

AC_SUBST([DEFAULT_CONFIG_FILE])
AC_DEFINE_UNQUOTED([DEFAULT_CONFIG_FILE], "$(eval echo ${DEFAULT_CONFIG_FILE})",
		   [Default config file])

AC_SUBST([LOGDIR])
AC_DEFINE_UNQUOTED([LOGDIR], "$(eval echo ${LOGDIR})",
		   [Default logging directory])

AC_SUBST([NOTIFYDDIR])

AC_SUBST([LOGROTATEDIR])

AC_SUBST([CLUSTERVARRUN])

AC_SUBST([CLUSTERVARLIB])

AC_DEFINE_UNQUOTED([SBINDIR], "$(eval echo ${sbindir})",
		   [/sbin path])

AC_DEFINE_UNQUOTED([COROSYNCBIN], "$(eval echo ${sbindir}/corosync)",
		   [corosync executable file])

AC_DEFINE_UNQUOTED([SYSLOGFACILITY], $(eval echo ${SYSLOGFACILITY}),
		   [Default syslog facility])

AC_DEFINE_UNQUOTED([SYSLOGLEVEL], $(eval echo ${SYSLOGLEVEL}),
		   [Default syslog level])

AM_CONDITIONAL(BUILD_BINDINGS, test "x${enable_bindings}" = xyes)

## *FLAGS handling

ENV_CFLAGS="$CFLAGS"
ENV_CPPFLAGS="$CPPFLAGS"
ENV_LDFLAGS="$LDFLAGS"

# debug build stuff
if test "x${enable_debug}" = xyes; then
	AC_DEFINE_UNQUOTED([DEBUG], [1], [Compiling Debugging code])
	OPT_CFLAGS="-O0"
else
	OPT_CFLAGS="-O2"
fi

# gdb flags
if test "x${GCC}" = xyes; then
	GDB_FLAGS="-ggdb3"
else
	GDB_FLAGS="-g"
fi

# extra warnings
EXTRA_WARNINGS=""

WARNLIST="
	all
	shadow
	missing-prototypes
	missing-declarations
	strict-prototypes
	declaration-after-statement
	pointer-arith
	write-strings
	cast-align
	bad-function-cast
	missing-format-attribute
	format=2
	format-security
	format-nonliteral
	no-long-long
	unsigned-char
	gnu89-inline
	no-strict-aliasing
	"

for j in $WARNLIST; do
	if cc_supports_flag -W$j; then
		EXTRA_WARNINGS="$EXTRA_WARNINGS -W$j";
	fi
done

CFLAGS="$ENV_CFLAGS $OPT_CFLAGS $GDB_FLAGS \
	$EXTRA_WARNINGS $WERROR_CFLAGS"
CPPFLAGS="-I\$(top_builddir)/make -I\$(top_srcdir)/make -I. $ENV_CPPFLAGS"
LDFLAGS="$ENV_LDFLAGS"

#
# missing:
# cman/init.d/Makefile
# bindings/perl/ccs/Makefile

AC_CONFIG_FILES([Makefile
		 common/Makefile
		 common/liblogthread/Makefile
		 common/liblogthread/liblogthread.pc
		 config/Makefile
		 config/libs/Makefile
		 config/libs/libccsconfdb/Makefile
		 config/libs/libccsconfdb/libccs.pc
		 config/plugins/Makefile
		 config/plugins/ldap/Makefile
		 config/plugins/xml/Makefile
		 config/tools/Makefile
		 config/tools/ccs_tool/Makefile
		 config/tools/ldap/Makefile
		 config/tools/mkconf/Makefile
		 config/tools/man/Makefile
		 config/man/Makefile
		 cman/Makefile
		 cman/services/Makefile
		 cman/services/cman/Makefile
		 cman/services/cman/include/Makefile
		 cman/services/cman/services/Makefile
		 cman/services/cman/lib/Makefile
		 cman/services/cman/lib/libcman.pc
		 cman/cman_tool/Makefile
		 cman/config/Makefile
		 cman/qdisk/Makefile
		 cman/notifyd/Makefile
		 cman/man/Makefile
		 cman/tests/Makefile
		 group/Makefile
		 group/man/Makefile
		 group/tool/Makefile
		 doc/Makefile
		 bindings/Makefile
		 bindings/perl/Makefile
		 ])

AC_OUTPUT
