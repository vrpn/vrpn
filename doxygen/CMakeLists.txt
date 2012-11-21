# Changes made by:
# 2009-2010 Ryan Pavlik <rpavlik@iastate.edu>
# http://academic.cleardefinition.com
# Iowa State University HCI Graduate Program/VRAC

include(DoxygenTargets)

#-----------------------------------------------------------------------------
# Create documentation
add_doxygen(Doxyfile
	INSTALL_DESTINATION
	share/doc/vrpn-${BRIEF_VERSION}/source-docs
	NO_PDF
	INSTALL_COMPONENT
	doc)	# For some reason, LaTeX bails with a nesting-too-deep error
