dnl Copyright (C) 2021 thatlittlegit
dnl -
dnl This program is free software: you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation, either version 3 of the License, or
dnl (at your option) any later version.
dnl -
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl -
dnl You should have received a copy of the GNU General Public License
dnl along with this program.  If not, see <http://www.gnu.org/licenses/>.
dnl -
AC_INIT([mfi], [1], [https://github.com/thatlittlegit/mfi])
AM_INIT_AUTOMAKE([foreign])

AC_PROG_CC

AC_ARG_ENABLE([init-messages],
	[AS_HELP_STRING([--enable-init-messages],
			[Enable messages as if we're PID1])])

AS_IF([test x"$enable_init_messages" != x"yes"],
	[AC_DEFINE([MFI_INIT_MESSAGE_STYLE], 0)],
	[AC_DEFINE([MFI_INIT_MESSAGE_STYLE], 1)])
AH_TEMPLATE([MFI_INIT_MESSAGE_STYLE],
	[Defines to 1 or 0 depending on if output and behaviour of the program
	 should appear more PID1-like or utility-like; e.g. should we mention
	 that as PID1 the kernel will panic in five seconds? It sure won't if
	 the user's running it as a utility!])

AC_ARG_ENABLE([custom-commands],
	[AS_HELP_STRING([--disable-custom-commands],
			[Disable setting commands with arguments])])

AS_IF([test x"$enable_custom_commands" != x"no"],
	[AC_DEFINE([MFI_CUSTOM_COMMANDS], 1)],
	[AC_DEFINE([MFI_CUSTOM_COMMANDS], 0)])
AH_TEMPLATE([MFI_CUSTOM_COMMANDS],
	[Defines to 1 or 0 depending on if custom commands are permitted.])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
