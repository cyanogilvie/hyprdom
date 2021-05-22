%name LemonParseXML

%include {
#include "hyprdomInt.h"
}

%parse_accept {
	printf("LemonParseXML succeeded\n");
}

%parse_failure {
	fprintf(stderr, "LemonParseXML failed\n");
}

%start_symbol document

%syntax_error {
	fprintf(stderr, "LemonParseXML syntax error\n");
}

%token_prefix LTK_

%extra_argument { struct parse_context }

document		::= prolog element misctail.

misctail		::=.
misctail		::= misctail misc.

misc			::= Comment.
misc			::= PI.
misc			::= WS.

prolog			::= misctail.
prolog			::= XMLDecl misctail.
//prolog			::= XMLDecl misctail doctypedecl misctail.

element			::= EmptyElemTag.
element			::= STag content ETag.

content			::=.
content			::= content CharData.
content			::= content element.
content			::= content Reference.
content			::= content CDSect.
content			::= content PI.
content			::= content Comment.

/*
cp				::= Name cpsuffix.
cp				::= choice cpsuffix.
cp				::= seq cpsuffix.

cpsuffix		::=.
cpsuffix		::= Question.
cpsuffix		::= Star.
cpsuffix		::= Plus.

choice			::= OpenParen optspace cp choiceopts optspace CloseParen.

choiceopt		::= optspace Pipe optspace cp.
choiceopts		::= choiceopt.
choiceopts		::= choiceopts choiceopt.

optspace		::=.
optspace		::= WS.

children		::= choice cpsuffix.
children		::= seq cpsuffix.

seq				::= OpenParen optspace cp seqopts optspace CloseParen.

seqopt			::= optspace Comma optspace cp.
seqopts			::=.
seqopts			::= seqopt.
seqopts			::= seqopts seqopt.

contentspec		::= Empty.
contentspec		::= Any.
contentspec		::= Mixed.
contentspec		::= children.

elementdecl		::= StartElementDecl WS Name WS contentspec optspace Gt.

markupdecl		::= elementdecl.
markupdecl		::= AttlistDecl.
markupdecl		::= EntityDecl.
markupdecl		::= NotationDecl.
markupdecl		::= PI.
markupdecl		::= Comment.

extSubsetDecl_elem	::= markupdecl.
extSubsetDecl_elem	::= conditionalSect.
extSubsetDecl_elem	::= DeclSep.

extSubsetDecl	::=.
extSubsetDecl	::= extSubsetDecl_elem.
extSubsetDecl	::= extSubsetDecl extSubsetDecl_elem.

intSubset_elem	::= markupdecl.
intSubset_elem	::= DeclSep.

intSubset		::=.
intSubset		::= intSubset_elem.
intSubset		::= intSubset intSubset_elem.

conditionalSect	::= includeSect.
conditionalSect	::= ignoreSect.

includeSect		::= StartSect optspace INCLUDE optspace OpenBracket extSubsetDecl CloseSect.
ignoreSect		::= StartSect optspace IGNORE optspace OpenBracket ignoreSectContents_list CloseSect.

ignoreSectContents_list	::=.
ignoreSectContents_list	::= ignoreSectContents.
ignoreSectContents_list	::= ignoreSectContents_list ignoreSectContents.

ignoreSectContents	::= Ignore ignore_tail.
ignore_tail		::=.
ignore_tail		::= StartSect ignoreSectContents CloseSect Ignore.
ignore_tail		::= ignore_tail StartSect ignoreSectContents CloseSect Ignore.

doctypedecl		::= StartDoctype WS Name opt_externalid optspace int_subset_part Gt.
opt_externalid	::=.
opt_externalid	::= WS ExternalID.

int_subset_part	::=.
int_subset_part	::= OpenBracket intSubset CloseBracket optspace.

extSubset		::= extSubsetDecl.
extSubset		::= TextDecl extSubsetDecl.

extParsedEnt	::= content.
extParsedEnt	::= TextDecl content.
*/

// vim: ft=lemon
