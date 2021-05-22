set here	[file dirname [file normalize [info script]]]
package require hyprdom
#load [lindex $argv 0]
package require tdom

proc main {} {
	global here

	set h	[open [file join $here ../xpath/xpath-grammar.xml]]
	try {
		set xml	[read $h]
	} finally {close $h}

	if 0 {
	puts "Manual test hyprdom parse: [timerate {
		set root	[hyprdom parse $xml]
	} 1 1]"
	puts "Manual test hyprdom free: [timerate {
		hyprdom free $root
	} 1 1]"
	puts "Manual test hyprdom parse: [timerate {
		set root	[hyprdom parse $xml]
	} 1 1]"
	puts "Manual test hyprdom free: [timerate {
		hyprdom free $root
	} 1 1]"
	puts "Manual test tdom parse: [timerate {
		set doc	[dom parse $xml]
	} 1 1]"
	puts "Manual test tdom free: [timerate {
		domDoc $doc delete
	} 1 1]"
	puts "Manual test tdom parse: [timerate {
		set doc	[dom parse $xml]
	} 1 1]"
	puts "Manual test tdom free: [timerate {
		domDoc $doc delete
	} 1 1]"
	}

	set count	0
	puts "Parsing xpath-grammar.xml for 10 seconds"
	set start	[clock microseconds]
	while {[clock microseconds] - $start < 10e6} {
		domDoc [dom parse $xml] delete
		incr count
	}
	puts "Parsed and freed $count times"
}

main
