#!/usr/bin/tclsh
# $Id$
menu tool on
tool create tug falcon0 fbuttons0 ffeedback0
tool scale 30.0 0
tool scaleforce 20.0 0
tool scalespring 25.0

if {1} {
  # attach effector to second representation (AFM tip).
  tool rep 0 0 1
} else {
  # effector grabs atoms.
  tool rep 0 -1 -1
}
