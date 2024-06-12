AC_PREREQ([2.72])
AC_INIT([manhandle], [0.1])
AC_CONFIG_SRCDIR([src/manhandle.c])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_AUX_DIR([build-aux])

AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])

# Checks for compiler.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([ncurses], [mvwaddstr], [],
    [AC_MSG_ERROR(["cannot compile without ncurses"])])

AC_CHECK_LIB([readline], [rl_callback_handler_install], [],
    [AC_MSG_ERROR(["cannot compile without readline"])])

# Checks for header files.
AC_CHECK_HEADER([errno.h], [],
    [AC_MSG_ERROR(["cannot compile without errno.h"])])

AC_CHECK_HEADER([fcntl.h], [],
    [AC_MSG_ERROR(["cannot compile without fcntl.h"])])

AC_CHECK_HEADER([unistd.h], [],
    [AC_MSG_ERROR(["cannot compile without unistd.h"])])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([dup2 setenv strdup])

# Generate usage
AC_DEFINE_UNQUOTED([USAGE],
    ["$(sed 's/"/\\"/g;s/$/\\n/g' doc/usage | tr -d '\n')"],
    ["Program usage generated from doc/usage"])

AC_DEFINE_UNQUOTED([GUI_HELP],
    ["$(awk '/^NOTES:/{p=0} /GUI KEYBINDINGS:/{p=1} p' doc/usage | sed 's/"/\\"/g;s/$/\\n/g' | tr -d '\n')"],
    ["GUI help generated from doc/usage"])

# Generate --version
AC_DEFINE_UNQUOTED([VERSION_FMT],
    ["$(sed 's/"/\\"/g;s/$/\\n/g' doc/version | tr -d '\n')"],
    ["Version format for --version"])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
