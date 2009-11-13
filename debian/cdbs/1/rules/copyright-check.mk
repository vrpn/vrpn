# -*- mode: makefile; coding: utf-8 -*-
# Copyright © 2005-2008 Jonas Smedegaard <dr@jones.dk>
# Description: Check for changes to copyright notices in source
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

ifndef _cdbs_rules_copyright_check
_cdbs_rules_copyright_check := 1

include $(_cdbs_rules_path)/buildcore.mk$(_cdbs_makefile_suffix)

CDBS_BUILD_DEPENDS := $(CDBS_BUILD_DEPENDS), devscripts (>= 2.10.7)

# Set to yes to fail on changed/new hints are found
#DEB_COPYRIGHT_CHECK_STRICT := yes

# Single regular expression for files to include or ignore
DEB_COPYRIGHT_CHECK_REGEX = .*
#DEB_COPYRIGHT_CHECK_IGNORE_REGEX = ^(debian/.*|(.*/)?config\.(guess|sub|rpath)(\..*)?)$
DEB_COPYRIGHT_CHECK_IGNORE_REGEX = ^debian/(changelog|copyright(|_hints|_newhints))$

pre-build:: debian/stamp-copyright-check

debian/stamp-copyright-check:
	@echo 'Scanning upstream source for new/changed copyright notices...'
	@echo licensecheck -c '$(DEB_COPYRIGHT_CHECK_REGEX)' -r --copyright -i '$(DEB_COPYRIGHT_CHECK_IGNORE_REGEX)' * \
		"| some-output-filtering..."

# Perl in shell in make requires extra care:
#  * Single-quoting ('...') protects against shell expansion
#  * Double-dollar ($$) expands to plain dollar ($) in make
	@licensecheck -c '$(DEB_COPYRIGHT_CHECK_REGEX)' -r --copyright -i '$(DEB_COPYRIGHT_CHECK_IGNORE_REGEX)' * \
		| LC_ALL=C perl -e \
	'print "Format-Specification: http://wiki.debian.org/Proposals/CopyrightFormat?action=recall&rev=XXX\n";'\
	'print "Upstream-Name: Untrusted draft - double-check copyrights yourself!\n\n";'\
	'$$n=0; while (<>) {'\
	'	s/[^[:print:]]//g;'\
	'	if (/^([^:\s][^:]+):[\s]+(\S.*?)\s*$$/) {'\
	'		$$files[$$n]{name}=$$1;'\
	'		$$files[$$n]{license}=$$2;'\
	'	};'\
	'	if (/^\s*\[Copyright:\s*(\S.*?)\s*\]/) {'\
	'		$$files[$$n]{copyright}=$$1;'\
	'	};'\
	'	/^$$/ and $$n++;'\
	'};'\
	'foreach $$file (@files) {'\
	'	$$file->{license} =~ s/\s*\(with incorrect FSF address\)//;'\
	'	$$file->{license} =~ s/\s+\(v([^)]+) or later\)/-$$1+/;'\
	'	$$file->{license} =~ s/\s+\(v([^)]+)\)/-$$1/;'\
	'	$$file->{license} =~ s/\s*(\*No copyright\*)\s*// and $$file->{copyright} = $$1;'\
	'	$$file->{license} =~ s/^\s*(GENERATED FILE)/UNKNOWN ($$1)/;'\
	'	$$file->{license} =~ s/\s+(GENERATED FILE)/ ($$1)/;'\
	'	$$file->{copyright} =~ s/(?<=(\b\d{4}))(?{$$y=$$^N})\s*[,-]\s*((??{$$y+1}))\b/-$$2/g;'\
	'	$$file->{copyright} =~ s/(?<=\b\d{4})\s*-\s*\d{4}(?=\s*-\s*(\d{4})\b)//g;'\
	'	$$file->{copyright} =~ s/\b(\d{4})\s+([\S^\d])/$$1, $$2/g;'\
	'	$$file->{copyright} =~ s/^\W*\s+\/\s+//g;'\
	'	$$file->{copyright} =~ s/\s+\/\s+\W*$$//;'\
	'	$$file->{copyright} =~ s/\s+\/\s+/\n\t/g;'\
	'	$$pattern = "$$file->{license} [$$file->{copyright}]";'\
	'	push @{ $$patternfiles{"$$pattern"} }, $$file->{name};'\
	'};'\
	'foreach $$pattern ( sort {'\
	'			@{$$patternfiles{$$b}} <=> @{$$patternfiles{$$a}}'\
	'			||'\
	'			$$a cmp $$b'\
	'		} keys %patternfiles ) {'\
	'	($$license, $$copyright) = $$pattern =~ /(.*) \[(.*)\]/s;'\
	'	print "Files: ", join("\n\t", sort @{ $$patternfiles{$$pattern} }), "\n";'\
	'	print "Copyright: $$copyright\n";'\
	'	print "License: $$license\n\n";'\
	'};'\
		> debian/copyright_newhints
	@patterncount="`cat debian/copyright_newhints | sed 's/^[^:]*://' | LANG=C sort -u | grep . -c -`"; \
		echo "Found $$patterncount different copyright and licensing combinations."
	@if [ ! -f debian/copyright_hints ]; then touch debian/copyright_hints; fi
	@newstrings=`diff -u debian/copyright_hints debian/copyright_newhints | sed '1,2d' | egrep '^\+' - | sed 's/^\+//'`; \
		if [ -n "$$newstrings" ]; then \
			echo "$(if $(DEB_COPYRIGHT_CHECK_STRICT),ERROR,WARNING): The following new or changed copyright notices discovered:"; \
			echo; \
			echo "$$newstrings"; \
			echo; \
			echo "To fix the situation please do the following:"; \
			echo "  1) Investigate the above changes and update debian/copyright as needed"; \
			echo "  2) Replace debian/copyright_hints with debian/copyright_newhints"; \
			$(if $(DEB_COPYRIGHT_CHECK_STRICT),exit 1,:); \
		else \
			echo 'No new copyright notices found - assuming no news is good news...'; \
			rm -f debian/copyright_newhints; \
		fi
	touch $@

clean::
	rm -f debian/stamp-copyright-check

endif
