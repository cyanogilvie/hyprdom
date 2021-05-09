#!/usr/bin/env tclsh

catch {package require common_sighandler}
package require tdom
package require chantricks
namespace import ::chantricks::*
package require parse_args
interp alias {} ::parse_args {} ::parse_args::parse_args

parse_args $argv {
	in		{-default -}
	out		{-default -}
}

if {$in eq "-"} {
	set indoc	[read stdin]
} else {
	set indoc	[readfile $in]
}

set doc	[dom parse $indoc]
set root	[$doc documentElement]
#puts [[$root selectNodes {g:start[@if="xpath1"]}] asXML]
if 0 {
	while 1 {
		set next	[domNode $root selectNodes {
			//*[@if and not(contains(@if, "xpath1"))][1] |
			//*[@not-if and contains(@not-if, "xpath1")][1]
		}]
		if {[llength $next] == 0} break
		set next	[lindex $next 0]
		domNode $next delete
	}
} else {
	# Delete nodes not relevant to xpath1
	foreach node [lreverse [domNode $root selectNodes {
		//*[@if and not(contains(@if, "xpath1") or starts-with(@if, "_"))] |
		//*[@not-if and contains(@not-if, "xpath1")] |
		//comment()
	}]] {
		domNode $node delete
	}

	# Delete g:level nodes with no attribs or children
	foreach node [domNode $root selectNodes {
		//g:level[not(*) and not(@*)]
	}] {
		domNode $node delete
	}
}

if {$out eq "-"} {
	puts [domNode $root asXML]
} else {
	writefile $out [domNode $root asXML]
}

