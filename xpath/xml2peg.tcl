#!/usr/bin/env tclsh

catch {package require common_sighandler}
package require parse_args
interp alias {} ::parse_args {} ::parse_args::parse_args
package require tdom
package require chantricks
namespace import ::chantricks::*

parse_args $argv {
	-format	{-enum {tcllib packcc} -default packcc}
	in		{-default -}
	out		{-default -}
}

set exit	0
set reffed	{}

proc xpath {cx xpath} { #<<<
	tailcall domNode $cx selectNodes $xpath
}

#>>>
proc pe {rule {glue { }} {needs_grouping 0}} { #<<<
	global reffed
	set res				{}

	#append res	[lmap e [domNode $rule childNodes] {domNode $e nodeName}]
	foreach part [domNode $rule childNodes] {
		switch -exact -- [domNode $part nodeName] {
			g:sequence {
				lappend res [pe $part]
			}
			g:zeroOrMore {
				lappend res "([pe $part])*"
			}
			g:oneOrMore {
				lappend res "([pe $part])+"
			}
			g:optional {
				lappend res "([pe $part])?"
			}
			g:complement {
				lappend res "!([pe $part])"
			}
			g:ref {
				set name	[domNode $part getAttribute name]
				if {[xpath $part {string(@unfold)}] eq "yes"} {
					lappend res	[pe [xpath $part {//g:production[@name=$name]}]]
				} else {
					lappend res $name
					dict set reffed $name	1
				}
			}
			g:choice {
				lappend res [pe $part { / } 1]
			}
			g:token {
				lappend res [pe $part]
			}
			g:string {
				lappend res [pe $part]
				#error "Adding g:string: [domNode $part asXML]:\n($res)"
			}
			g:char {
				lappend res [domNode $part text]
			}
			g:charRange {
				lappend res "[domNode $part getAttribute minChar]-[domNode $part getAttribute maxChar]"
			}
			g:charCode {
				lappend res \\u[domNode $part getAttribute value]
			}
			g:charCodeRange {
				lappend res "\\u[domNode $part getAttribute minValue]-\\u[domNode $part getAttribute maxValue]"
			}
			g:charClass {
				lappend res "\[[pe $part {}]\]"
			}

			"#text" {
				#puts stderr "#text: ([domNode $part nodeValue])"
				set string	[domNode $part nodeValue]
				if {[string match *'* $string]} {
					lappend res \"[domNode $part nodeValue]\"
				} else {
					lappend res '[domNode $part nodeValue]'
				}
			}

			default {
				error "Unhandled part: [domNode $part nodeName]"
			}
		}
	}

	if {$needs_grouping && [llength $res] > 1} {
		set res	([join $res $glue])
	} else {
		set res	[join $res $glue]
	}

	set res
}

#>>>
proc gen_exprpe rule { #<<<
	set res	{}

	set next_level_name	[xpath $rule {
		string(
			ancestor::g:level/following-sibling::g:level/*[1]/@name
		)
	}]
	switch -exact -- [domNode $rule nodeName] {
		g:binary {
			append res "$next_level_name ([pe $rule] $next_level_name)*"
		}

		g:prefix {
			append res "([pe $rule])[domNode $rule getAttribute prefix-seq-type] $next_level_name"
		}

		g:primary {
			append res [pe $rule]
		}

		default {
			error "Unhandled exprrule: [domNode $rule nodeName]"
		}
	}

	set res
}

#>>>

if {$in eq "-"} {
	set indoc	[read stdin]
} else {
	set indoc	[readfile $in]
}

set outdoc	{}

set doc		[dom parse $indoc]
set root	[domDoc $doc documentElement]
try {
	if {$format eq "tcllib"} {
		append outdoc "PEG [xpath $root {string(g:language[1]/@id)}] ([xpath $root {string(g:start[1]/@name)}])" \n
	} elseif {$format eq "packcc"} {
		dict set reffed ImplicitlyAllowedWhitespace 1
		dict set reffed CommentStart 1
		dict set reffed CommentContent 1
		dict set reffed CommentEnd 1
		dict set reffed ExprSingle 1
		dict set reffed Char 1
		dict set reffed Whitespace 1
		dict set reffed WhitespaceChar 1
		append outdoc	"\t\tXpath\t<-\t^ Ignored* OrExpr Ignored* \$" \n
		append outdoc	"\t\tIgnored\t<-\tComment / ImplicitlyAllowedWhitespace" \n
		append outdoc	"\t\tComment\t<-\tCommentStart CommentContent CommentEnd" \n
	}
	foreach rule [domNode $root childNodes] {
		switch -- [domNode $rule nodeName] {
			g:language - g:start - g:state-list {}
			g:production -
			g:token {
				if {[string match _* [domNode $rule getAttribute name]]} continue
				if {![dict exists $reffed [domNode $rule getAttribute name]]} continue

				set type	[xpath $rule string(@node-type)]
				if {$type eq "" && [domNode $rule nodeName] eq "g:token"} {
					set type	leaf	;# TODO: not sure about this
				}
				if {$format eq "tcllib" && $type ni {value ""}} {
					append outdoc "$type:"
				} else {
					# Implicit "value" type
					append outdoc \t
				}
				append outdoc	"\t[xpath $rule string(@name)]\t<-\t[pe $rule]"
				if {$format eq "tcllib"} {
					append outdoc	";"
				}
				append outdoc	\n
			}
			g:exprProduction {
				if 0 {
				append outdoc "\t\t[xpath $rule string(@name)]\t<-\t[xpath $rule {string(g:level[1]/*[1]/@name)}]"
				if {$format eq "tcllib"} {
					append outdoc	";"
				}
				append outdoc	\n
				}
				foreach level [xpath $rule g:level] {
					append outdoc "\t\t[xpath $level {string(*[1]/@name)}]\t<-\t[gen_exprpe [xpath $level {*[1]}]]"
					if {$format eq "tcllib"} {
						append outdoc	";"
					}
					append outdoc	\n
				}
			}
			default {
				error "Unhandled node: [domNode $rule nodeName]:\n[domNode $rule asXML]"
			}
		}
	}
	if {$format eq "tcllib"} {
		append outdoc "END;" \n
	}
} on ok {} {
	if {$out eq "-"} {
		puts $outdoc
	} else {
		writefile $out $outdoc
	}
} on error {errmsg options} {
	puts stderr [dict get $options -errorinfo]
	set exit	1
} finally {
	$doc delete
}

exit $exit

# vim: ft=tcl foldmethod=marker foldmarker=<<<,>>> ts=4 shiftwidth=4
