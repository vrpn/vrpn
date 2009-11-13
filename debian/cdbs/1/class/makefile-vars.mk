# -*- mode: makefile; coding: utf-8 -*-
# Copyright © 2002,2003 Colin Walters <walters@debian.org>
# Copyright © 2008 Jonas Smedegaard <dr@jones.dk>
# Description: Defines useful variables for packages which have a Makefile
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2, or (at
# your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
# 02111-1307 USA.

_cdbs_scripts_path ?= /usr/lib/cdbs
_cdbs_rules_path ?= /usr/share/cdbs/1/rules
_cdbs_class_path ?= /usr/share/cdbs/1/class

ifndef _cdbs_class_makefile_vars
_cdbs_class_makefile_vars = 1

include $(_cdbs_class_path)/langcore.mk$(_cdbs_makefile_suffix)

DEB_MAKE_MAKEFILE =
DEB_MAKE_ENVVARS = 
DEB_MAKE_INVOKE = $(DEB_MAKE_ENVVARS) $(MAKE) $(if $(DEB_MAKE_MAKEFILE), -f $(DEB_MAKE_MAKEFILE),) -C $(cdbs_make_curbuilddir) CFLAGS=$(if $(CFLAGS_$(cdbs_curpkg)),"$(CFLAGS_$(cdbs_curpkg))","$(CFLAGS)") CXXFLAGS=$(if $(CXXFLAGS_$(cdbs_curpkg)),"$(CXXFLAGS_$(cdbs_curpkg))","$(CXXFLAGS)") CPPFLAGS=$(if $(CPPFLAGS_$(cdbs_curpkg)),"$(CPPFLAGS_$(cdbs_curpkg))","$(CPPFLAGS)") LDFLAGS=$(if $(LDFLAGS_$(cdbs_curpkg)),"$(LDFLAGS_$(cdbs_curpkg))","$(LDFLAGS)")

# This variable is deprecated.
DEB_BUILD_MAKE_TARGET = 
_cdbs_deprecated_vars += DEB_BUILD_MAKE_TARGET
# New in 0.2.8.
DEB_MAKE_BUILD_TARGET = $(DEB_BUILD_MAKE_TARGET)

# If your Makefile provides an "install" target, you need to give the requisite commands
# here to install it into the staging directory.  For automake-using programs, this
# looks like: install DESTDIR=$(DEB_DESTDIR)
# If you're using automake though, you likely want to be including autotools.mk instead
# of this file.
# For multi-flavored builds (see below) installed per-flavor, it looks like this:
# install DESTDIR=$(cdbs_curpkgdestdir)

# This variable is deprecated.
DEB_CLEAN_MAKE_TARGET = clean
_cdbs_deprecated_vars += DEB_CLEAN_MAKE_TARGET
_cdbs_deprecated_DEB_CLEAN_MAKE_TARGET_default := $(DEB_CLEAN_MAKE_TARGET)
# New in 0.2.8.
DEB_MAKE_CLEAN_TARGET = $(DEB_CLEAN_MAKE_TARGET)

# This variable is deprecated.
DEB_MAKE_TEST_TARGET = 
_cdbs_deprecated_vars += DEB_MAKE_TEST_TARGET
# New in 0.2.8.
# New in 0.4.2.
DEB_MAKE_CHECK_TARGET = $(DEB_MAKE_TEST_TARGET)

# If DEB_MAKE_FLAVORS is set compilation is done once per flavor.
# NB! This must be declared _before_ including makefile.mk
#DEB_MAKE_FLAVORS = light normal enhanced

# If building multiple flavors, skeleton strings are used for
# DEB_BUILDDIR and DEB_DESTDIR, with @FLAVOR@ expanding to actual
# flavor.
DEB_MAKE_BUILDDIRSKEL = $(cdbs_curpkgbuilddir)/@FLAVOR@
DEB_MAKE_DESTDIRSKEL = $(cdbs_curpkgdestdir)/@FLAVOR@

endif
