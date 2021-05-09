#!/usr/bin/env tclsh

catch {package require common_sighandler}
package require pt::pgen
package require chantricks
namespace import chantricks::*
package require parse_args
interp alias {} ::parse_args {} ::parse_args::parse_args

parse_args $argv {
	-name	{}
	in		{-default -}
	out		{-default -}
}

if {$in eq "-"} {
	set grammar	[read stdin]
} else {
	if {![info exists name]} {
		set name	[file rootname $in]
	}
	set grammar	[readfile $in]
}

if {![info exists name]} {
	error "Must supply -name"
}

set parser	[pt::pgen peg $grammar snit -class $name -file $name.peg $name]
#set parser	[pt::pgen peg $grammar critcl -file $name.peg -name $name]

if {$out eq "-"} {
	puts $parser
} else {
	writefile $out $parser
}
