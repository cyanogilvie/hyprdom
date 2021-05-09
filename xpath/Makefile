TCLSH=tclsh
PACKCC=packcc

all: xpath1.c

xpath1.xml: xpath-grammar.xml xpath1.tcl
	$(TCLSH) xpath1.tcl xpath-grammar.xml xpath1.xml

xpath1.peg: xpath1.xml xml2peg.tcl
	$(TCLSH) xml2peg.tcl xpath1.xml xpath1.peg

xpath1_to_ast.tcl: xpath1.peg peg2parser.tcl
	$(TCLSH) peg2parser.tcl xpath1.peg xpath1_to_ast.tcl

xpath1.c: xpath1.peg
	$(PACKCC) xpath1.peg

clean:
	-rm xpath1.xml xpath1.peg xpath1_to_ast.tcl xpath1.c xpath1.h

.PHONY: all clean test
