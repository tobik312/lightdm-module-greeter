# Global Options
AUTOMAKE_OPTIONS = \
				foreign # Disable requirement for README, AUTHORS, COPYING

AM_CFLAGS = -g -Wall -O3 --std=c11 -Wextra -Werror --std=c11 -pedantic \
			-Wno-unused-parameter -Wfloat-equal -Wundef -Wshadow      \
			-Wpointer-arith -Wcast-align -Wstrict-prototypes          \
			-Wstrict-overflow=5 -Wwrite-strings -Waggregate-return    \
			-Wswitch-enum -Wconversion -Wunreachable-code -Wformat=2  \
			-Winit-self                                               \
			-ftrapv -fverbose-asm \
			-DCONFIG_FILE=\""$(sysconfdir)/lightdm/lightdm-module-greeter.conf"\"


# Packaging
EXTRA_DIST = \
			autogen.sh

DISTCLEANFILES = \
			aclocal.m4

MAINTAINERCLEANFILES = \
					Makefile.in \
					compile \
					defines.h.in \
					configure \
					depcomp \
					install-sh \
					missing

# Data Files
xgreetersdir = $(datadir)/xgreeters
dist_xgreeters_DATA = data/lightdm-module-greeter.desktop

configdir = $(sysconfdir)/lightdm
dist_config_DATA = data/lightdm-module-greeter.conf

#AttachExampleScript
scriptdir = $(sysconfdir)/lightdm/scripts
dist_script_DATA = data/scripts/example.sh

# Greeter Executable
greeterdir = $(bindir)
greeter_PROGRAMS = lightdm-module-greeter

lightdm_module_greeter_SOURCES = \
							src/main.c \
							src/app.c \
							src/callbacks.c \
							src/compat.c \
							src/config.c \
							src/ui.c \
							src/utils.c 

lightdm_module_greeter_CFLAGS = \
							$(AM_CFLAGS) \
							$(GTK_CFLAGS) \
							$(LIGHTDM_CFLAGS)
lightdm_module_greeter_LDADD = \
							$(GTK_LIBS) \
							$(LIGHTDM_LIBS)

# Add execute permission to example script
.PHONY: install
install: all
	chmod +x $(sysconfdir)/lightdm/scripts/example.sh