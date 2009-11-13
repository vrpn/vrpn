# -*- mode: makefile; coding: utf-8 -*-
# Copyright © 2004-2007 Jonas Smedegaard <dr@jones.dk>
# Description: Generate and include build information
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

ifndef _cdbs_rules_buildinfo
_cdbs_rules_buildinfo = 1

include $(_cdbs_rules_path)/buildcore.mk$(_cdbs_makefile_suffix)

CDBS_BUILD_DEPENDS := $(CDBS_BUILD_DEPENDS), dh-buildinfo

common-install-arch common-install-indep:: debian/stamp-buildinfo

debian/stamp-buildinfo:
	dh_buildinfo
	touch debian/stamp-buildinfo

clean::
	rm -f debian/stamp-buildinfo

endif
