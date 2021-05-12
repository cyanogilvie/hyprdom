puts "hyprdom parse:"
puts "Start: [exec cat /proc/[pid]/status | grep VmRSS]"
set here	[file dirname [file normalize [info script]]]
package require hyprdom
#load [lindex $argv 0]

puts "Loaded pacakge: [exec cat /proc/[pid]/status | grep VmRSS]"
proc main {} {
	global here

	set h	[open [file join $here ../xpath/xpath-grammar.xml]]
	try {
		set xml	[read $h]
	} finally {close $h}

	puts "100 times: [time {
		hyprdom free [hyprdom parse $xml]
	} 100]"
	set root [hyprdom parse $xml]
	puts "Peak: [exec cat /proc/[pid]/status | grep VmRSS]"
	puts [memory info]
	hyprdom free $root
	puts "After free: [exec cat /proc/[pid]/status | grep VmRSS]"
}

main
puts [memory info]
puts ""
