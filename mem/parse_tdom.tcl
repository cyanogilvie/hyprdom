puts "tdom parse:"
puts "Start: [exec cat /proc/[pid]/status | grep VmRSS]"
set here	[file dirname [file normalize [info script]]]
package require tdom

puts "Loaded pacakge: [exec cat /proc/[pid]/status | grep VmRSS]"
proc main {} {
	global here

	set h	[open [file join $here ../xpath/xpath-grammar.xml]]
	try {
		set xml	[read $h]
	} finally {close $h}

	puts "100 times: [time {
		domDoc [dom parse $xml] delete
	} 100]"
	set doc [dom parse $xml]
	puts "Peak: [exec cat /proc/[pid]/status | grep VmRSS]"
	puts [memory info]
	domDoc $doc delete
	puts "After free: [exec cat /proc/[pid]/status | grep VmRSS]"
}

main
puts [memory info]
puts ""
