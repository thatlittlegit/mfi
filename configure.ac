AC_INIT([mfi], [1], [https://github.com/thatlittlegit/mfi])
AM_INIT_AUTOMAKE([foreign])

AC_PROG_CC

AC_ARG_ENABLE([init-messages],
	[AS_HELP_STRING([--enable-init-messages],
			[Enable messages as if we're PID1])])

AS_IF([test x"$enable_init_messages" != x"yes"],
	[AC_DEFINE([MFI_INIT_MESSAGE_STYLE], 0)],
	[AC_DEFINE([MFI_INIT_MESSAGE_STYLE], 1)])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
