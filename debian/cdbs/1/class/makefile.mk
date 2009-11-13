# -*- mode: makefile; coding: utf-8 -*-
# Copyright © 2002,2003 Colin Walters <walters@debian.org>
# Copyright © 2003,2008 Jonas Smedegaard <dr@jones.dk>
# Description: Builds and cleans packages which have a Makefile
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

ifndef _cdbs_class_makefile
_cdbs_class_makefile = 1

include $(_cdbs_rules_path)/buildcore.mk$(_cdbs_makefile_suffix)
#include $(_cdbs_class_path)/makefile-vars.mk$(_cdbs_makefile_suffix)
include debian/cdbs/1/class/makefile-vars.mk

# TODO: Move these to buildcore.mk
cdbs_curpkgbuilddir = $(if $(DEB_BUILDDIR_$(cdbs_curpkg)),$(DEB_BUILDDIR_$(cdbs_curpkg)),$(DEB_BUILDDIR))
cdbs_curpkgdestdir = $(if $(DEB_DESTDIR_$(cdbs_curpkg)),$(DEB_DESTDIR_$(cdbs_curpkg)),$(DEB_DESTDIR))
 
cdbs_make_multibuilds = $(sort $(DEB_MAKE_FLAVORS))
cdbs_make_builddir_check = $(if $(call cdbs_streq,$(DEB_BUILDDIR),$(DEB_SRCDIR)),$(error DEB_MAKE_FLAVORS in use: DEB_BUILDDIR must be different from DEB_SRCDIR, and needs to be declared before including makefile.mk))
cdbs_make_build_stamps = $(if $(cdbs_make_multibuilds),$(cdbs_make_builddir_check)$(patsubst %,debian/stamp-makefile-build/%,$(cdbs_make_multibuilds)),debian/stamp-makefile-build)
cdbs_make_install_stamps = $(if $(cdbs_make_multibuilds),$(cdbs_make_builddir_check)$(patsubst %,debian/stamp-makefile-install/%,$(cdbs_make_multibuilds)),debian/stamp-makefile-install)
cdbs_make_check_stamps = $(if $(cdbs_make_multibuilds),$(cdbs_make_builddir_check)$(patsubst %,debian/stamp-makefile-check/%,$(cdbs_make_multibuilds)),debian/stamp-makefile-check)
cdbs_make_clean_nonstamps = $(if $(cdbs_make_multibuilds),$(patsubst %,makefile-clean/%,$(cdbs_make_multibuilds)),makefile-clean)
cdbs_make_curflavor = $(filter-out %/,$(subst /,/ ,$@))
cdbs_make_curbuilddir = $(if $(cdbs_make_multibuilds),$(subst @FLAVOR@,$(cdbs_make_curflavor),$(DEB_MAKE_BUILDDIRSKEL)),$(cdbs_curpkgbuilddir))

DEB_PHONY_RULES += makefile-clean $(cdbs_make_clean_nonstamps)

pre-build::
	$(if $(cdbs_make_multibuilds),mkdir -p debian/stamp-makefile-build debian/stamp-makefile-install)
	$(and $(cdbs_make_multibuilds),$(DEB_MAKE_CHECK_TARGET),mkdir -p debian/stamp-makefile-check)

common-build-arch common-build-indep:: $(cdbs_make_build_stamps)
$(cdbs_make_build_stamps):
	+$(DEB_MAKE_INVOKE) $(DEB_MAKE_BUILD_TARGET)
	touch $@

cleanbuilddir:: makefile-clean
makefile-clean:: $(if $(cdbs_make_multibuilds),$(cdbs_make_clean_nonstamps))
	$(if $(cdbs_make_multibuilds),-rmdir --ignore-fail-on-non-empty debian/stamp-makefile-build debian/stamp-makefile-install,rm -f debian/stamp-makefile-build debian/stamp-makefile-install)

$(cdbs_make_clean_nonstamps)::
	$(if $(DEB_MAKE_CLEAN_TARGET),+-$(DEB_MAKE_INVOKE) -k $(DEB_MAKE_CLEAN_TARGET),@echo "DEB_MAKE_CLEAN_TARGET unset, not running clean")
	$(if $(cdbs_make_multibuilds),rm -f $(@:makefile-clean%=debian/stamp-makefile-build%) $(@:makefile-clean%=debian/stamp-makefile-install%))

common-install-arch common-install-indep:: common-install-impl
common-install-impl:: $(cdbs_make_install_stamps)
$(cdbs_make_install_stamps)::
	$(if $(DEB_MAKE_INSTALL_TARGET),+$(DEB_MAKE_INVOKE) $(DEB_MAKE_INSTALL_TARGET),@echo "DEB_MAKE_INSTALL_TARGET unset, skipping default makefile.mk common-install target")
	$(if $(DEB_MAKE_INSTALL_TARGET),touch $@)

ifeq (,$(findstring nocheck,$(DEB_BUILD_OPTIONS)))
common-build-arch common-build-indep:: $(cdbs_make_check_stamps)
$(cdbs_make_check_stamps) : debian/stamp-makefile-check% : debian/stamp-makefile-build%
	$(if $(DEB_MAKE_CHECK_TARGET),+$(DEB_MAKE_INVOKE) $(DEB_MAKE_CHECK_TARGET),@echo "DEB_MAKE_CHECK_TARGET unset, not running checks")
	$(if $(DEB_MAKE_CHECK_TARGET),touch $@)

makefile-clean::
	$(if $(DEB_MAKE_CHECK_TARGET),$(if $(cdbs_make_multibuilds),-rmdir --ignore-fail-on-non-empty debian/stamp-makefile-check,rm -f debian/stamp-makefile-check))

$(cdbs_make_clean_nonstamps)::
	$(if $(cdbs_make_multibuilds),rm -f $(@:makefile-clean%=debian/stamp-makefile-check%))
endif

endif
