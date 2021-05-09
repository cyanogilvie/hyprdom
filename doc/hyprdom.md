% hyprdom(n) 1.0 | High Performance DOM for Tcl with XML and XPath support
% Cyan Ogilvie
% 0.1

# NAME

hyprdom - High Performance DOM for Tcl with XML and XPath support

# SYNOPSIS

**package require hyprdom** ?0.1?

**hyprdom new** *rootname*

**hyprdom parse** *xml*

**hyprdom asXML** *node*

**hyprdom add** *node* *script*

**hyprdom current_node**

**hyprdom autonode_unknown** *cmd*

**hyprdom xpath** *node* *xpath*

**hyprdom free** *node*

# DESCRIPTION

This package aims to provide a high performance DOM for Tcl scripts, primarily
accessed through XPath and constructing new nodes through Tcl script commands.
While it supports parsing from and serializing to XML, its primary mission
isn't to be a general purpose XML processor (with all the surprises that
entails).  If working with XML documents is what you want then you may be
better served by the much more mature and complete tdom package.

This package aims to make querying and mutating the DOM from Tcl as performant
as possible, to enable its use in situations like just-in-time transformations
and recompilations of ASTs representing Tcl scripts, as they are executed by
a macro, or just to serve as a general datatype for complex structured data.

# COMMANDS

**hyprdom new** *rootname*
:   Create a new DOM with the root element name *rootname* and return a reference
    to the root node.

**hyprdom parse** *xml*
:   Parse the supplied *xml* and create a DOM from its contents, and return a
    reference to the root node.

**hyprdom asXML** *node*
:   Serialize the fragment of the DOM rooted at *node* as XML and return it
    as a string.

**hyprdom add** *node* *script*
:   Execute *script* in the current frame, with the current node context set
    to *node*, such that node construction commands (see **hyprdom autonode_unknown**)
    will be added as child nodes of *node*.

**hyprdom current_node**
:   Return a reference to the current context node.

**hyprdom autonode_unknown** *cmd*
:   Intended as a helper in the **::unknown** proc or a namespace unknown handler.
    If *cmd* matches a pattern for node construction commands, the corresponding
    command is created in the current namespace.  Returns true if a pattern was
    matched and a command created, or false if no match was found (and other
    unknown handlers should be tried).  See **AUTO NODE COMMANDS** for details.

**hyprdom xpath** *node* *xpath*
:   Evaluate the XPath expression *xpath* against the document with the context
    node set to *node*, and return the result as a list.

**hyprdom free** *node*
:   Destroy the DOM that *node* belongs to and free all the resources associated
    with it.

# AUTO NODE COMMANDS

TODO:
- element nodes
- text nodes
- comment nodes
- pi nodes
- node commands return a reference to the node

# EXAMPLES

Construct a DOM and serialize it as XML

```tcl
# Hook the global namespace unknown handler to create node commands
# just-in-time:
namespace eval :: {
    namespace unknown [list apply {
        {old cmd args} {
            if {[hyprdom autonode_unknown $cmd]} {
                return [list $cmd {*}$args]
            }
            uplevel #0 [list {*}$old $cmd {*}$args]
        }
    } [namespace unknown]]
}

set root    [hyprdom new myexample]

hyprdom add $root {
    <hello some "thing" id 1234 {
        <greeting style "traditional" {txt "world"}
        <greeting style "heretical" {txt "hyprdom"}
    }
}

puts [hyprdom asXML $root]

hyprdom free $root
```

Create a dom from XML and extract parts using xpath:

```tcl
set root    [hyprdom parse {
    <foo>
    </foo>
}]

# TODO
foreach node [hyprdom xpath $root {
}] {
}

hyprdom free $root
```

# SEE ALSO

tdom

# LICENSE

This package is Copyright 2021 Cyan Ogilvie, and is made available under
the same license terms as the Tcl Core.

