#include "hyprdomInt.h"

/*!types:re2c*/

/*
   Non-terminals:
	cp				= (Name | choice | seq) ("?" | "*" | "+")?;
	choice			= "(" WS? cp (WS? "|" WS? cp)+ WS? ")";
	children		= (choice | seq) ("?" | "*" | "+")?;
	seq				= "(" WS? cp (WS? "," WS? cp)* WS? ")";
	contentspec		= "EMPTY" | "ANY" | Mixed | children;
	elementdecl		= "<!ELEMENT" WS Name WS contentspec WS? ">";
	markupdecl		= elementdecl | AttlistDecl | EntityDecl | NotationDecl | PI | Comment;
	extSubsetDecl	= (markupdecl | conditionalSect | DeclSep)*;
	intSubset		= (markupdecl | DeclSep)*;
	conditionalSect	= includeSect | ignoreSect;
	includeSect		= "<![" WS? "INCLUDE" WS? "[" extSubsetDecl "]]>";
	ignoreSect		= "<![" WS? "IGNORE" WS? "[" ignoreSectContents* "]]>";
	ignoreSectContents	= Ignore ("<![" ignoreSectContents "]]>" Ignore)*;
	doctypedecl		= "<!DOCTYPE" WS Name (WS ExternalID)? WS? ("[" intSubset "]" WS?)? ">";
	extSubset		= TextDecl? extSubsetDecl;
	extParsedEnt	= TextDecl? content;
	prolog			= XMLDecl? Misc* (doctypedecl Misc*)?;
	document		= prolog element Misc*;
	element			= EmptyElemTag | STag content ETag;
	content			= CharData? ((element | Reference | CDSect | PI | Comment) CharData?)*;

 */

/*!re2c
	WS				= [ \t\n\r]+;
	Char			= [\t\n\r\u0020-\uD7FF\uE000-\uFFFD];
	NameStartChar	= [:A-Z_a-z\xC0-\xF6\xF8-\u02FF\u0370-\u037D\u037F-\u1FFF\u200C-\u200D\u2070-\u218F\u2C00-\u2FEF\u3001-\uD7FF\uF900-\uFDCF\uFDF0-\uFFFD];
	NameChar		= NameStartChar | [.0-9\xB7\u0300-\u036F\u203F-\u2040-];
	Name			= NameStartChar NameChar*;
	Names			= Name ("\x20" Name)*;
	NmToken			= NameChar+;
	NmTokens		= NmToken ("\x20" NmToken)*;
	SystemLiteral	= ('"' [^"\x00]* '"') | ("'" [^'\x00]* "'");
	PubidChar		= [ \n\ta-zA-Z0-9'()+,./:=?;!*#@$_%-];
	PubidLiteral	= '"' PubidChar* '"' | "'" (PubidChar \ ['])* "'";
	CharData		= [^<&\x00]*;
	Comment			= "<!--" @mstart ((Char \ [-]) | ("-" (Char \ [-])))* @mend "-->";
	PITarget		= Name;
	PI				= "<?" @namestart PITarget @nameend WS @mstart Char* @mend "?>";
	PI_start		= "<?" @namestart PITarget @nameend WS @mstart;
	CDStart			= "<![CDATA[";
	CData			= Char*;
	CDEnd			= "]]>";
	CDSect			= CDStart @mstart CData @mend CDEnd;
	Eq				= WS? "=" WS?;
	VersionNum		= "1." [0-9]+;
	VersionInfo		= WS "version" Eq ("'" VersionNum "'" | '"' VersionNum '"');
	SDDecl			= WS "standalone" Eq (("'" ("yes" | "no") "'") | ('"' ("yes" | "no") '"'));
	ETag			= "</" Name WS? ">";
	Mixed			= "(" WS? "#PCDATA" (WS? "|" WS? Name)* WS? ")*" | "(" WS? "#PCDATA" WS? ")";
	StringType		= "CDATA";
	TokenizedType	= "ID" | "IDREF" | "IDREFS" | "ENTITY" | "ENTITIES" | "NMTOKEN" | "NMTOKENS";
	Enumeration		= "(" WS? NmToken (WS? "|" WS? NmToken)* WS? ")";
	NotationType	= "NOTATION" WS "(" WS? Name (WS? "|" WS? Name)* WS? ")";
	EnumeratedType	= NotationType | Enumeration;
	AttType			= StringType | TokenizedType | EnumeratedType;
	Ignore			= Char*;
	CharRef			= "&@" [0-9]+ ";" | "&#x" [0-9a-fA-F]+ ";";
	EntityRef		= "&" Name ";";
	Reference		= EntityRef | CharRef;
	PEReference		= "%" Name ";";
	DeclSep			= PEReference | WS;
	ExternalID		= "SYSTEM" WS SystemLiteral;
	NDataDecl		= WS "NDATA" WS Name;
	EncName			= [A-Za-z] [A-Za-z0-9._-]*;
	EncodingDecl	= WS "encoding" Eq ('"' EncName '"' | "'" EncName "'");
	XMLDecl			= "<?sml" VersionInfo EncodingDecl? SDDecl? WS? "?>";
	TextDecl		= "<?xml" VersionInfo? EncodingDecl WS? "?>";
	PublicID		= "PUBLIC" WS PubidLiteral;
	NotationDecl	= "<!NOTATION" WS Name WS (ExternalID | PublicID) WS? ">";
	EntityValue		= '"' ([^%&"\x00] | PEReference | Reference)* '"' |
					  "'" ([^%&'\x00] | PEReference | Reference)* "'";
	PEDef			= EntityValue | ExternalID;
	EntityDef		= EntityValue | (ExternalID NDataDecl?);
	GEDecl			= "<!ENTITY" WS Name WS EntityDef WS? ">";
	PEDecl			= "<!ENTITY" WS "%" WS Name WS PEDef WS? ">";
	EntityDecl		= GEDecl | PEDecl;
	AttValue		= '"' ([^<&"\x00] | Reference)* '"' |
					  "'" ([^<&'\x00] | Reference)* "'";
	Attribute		= Name Eq AttValue;
	DefaultDecl		= "#REQUIRED" | "#IMPLIED" | (("FIXED" WS)? AttValue);
	AttDef			= WS Name WS AttType WS DefaultDecl;
	AttlistDecl		= "<!ATTLIST" WS Name AttDef* WS? ">";
	STag			= "<" Name (WS Attribute)* WS? ">";
	EmptyElemTag	= "<" Name (WS Attribute)* WS? "/>";

	Question			= "?";
	Star				= "*";
	Plus				= "+";
	Comma				= ",";
	Pipe				= "|";
	OpenParen			= "(";
	CloseParen			= ")";
	OpenBracet			= "[";
	CloseBracket		= "]";
	Gt					= ">";
	StartElementDecl	= "<!ELEMENT";
	StartDoctype		= "<!DOCTYPE";
	Empty				= "EMPTY";
	Ant					= "ANY";
	CloseSect			= "]]>";
	StartSect			= "<![";
	INCLUDE				= "INCLUDE";
	IGNORE				= "IGNORE";

*/

// Internal parser <<<
int parse_xml_re(struct parse_context* restrict pc, const char* xml /* Must be null terminated */, const int xmllen) //<<<
{
	int						c = yycdocument;
	const char*				s = xml;
	const char*				YYMARKER;
	uint32_t				namebase = 0;
	uint32_t   				fail;
	const char*				namestart;
	const char*				nameend;
	const char*				mstart;
	const char*				mend;
	const char*				attrvalend;
	const char*				textstart = NULL;
	struct radish_search	search;
	uint32_t				name_id;
	Tcl_DString				val;
	int						translated = 0;
	int						depth = 0;
	int						haveroot = 0;
	int						base;
	/*!stags:re2c format = "const char *@@{tag}; "; */

	Tcl_DStringInit(&val);

	radish_search_init(pc->interp, &pc->cx.doc->radish_names, &pc->on_error, &search);

#define radish_search_reset() \
	__builtin_prefetch(search.radish->nodes, 0, 3); \
	search.slot = 0; \
	search.divergence = 0; \
	search.value_ofs = 0;

top:
	/*!re2c
		re2c:api:style = free-form;
		re2c:define:YYCTYPE = "unsigned char";
		re2c:define:YYCURSOR = s;
		re2c:define:YYGETCONDITION = "c";
		re2c:define:YYSETCONDITION = "c = @@;";
		re2c:yyfill:enable = 0;
		re2c:flags:tags = 1;


		<document>	WS			:=> document
		<document>	Comment		{ goto gotcomment; }
		<document>	PI_start	{ goto gotpi; }
		<document>	"<!DOCTYPE" WS Name (WS ExternalID)? WS? ">"	{ goto yyc_document; }
		<document>	"<!DOCTYPE" WS Name (WS ExternalID)? WS? "["	{ goto eat_intSubset; }
		<document>	"<"			=> elem {
			if (haveroot) THROW_ERROR_LABEL(err, pc->rc, pc->interp, "Only one root element allowed %ld", s-xml-1);

			haveroot = 1;
			goto yyc_elem;
		}
		<document>	"\x00"		{ return TCL_OK; }

		<elem>		NameStartChar 		=> elemnametail {
			depth++;
			s--;
			goto getname;
		}

		<elemnametail>	@mstart NameChar* @mend (WS | ">" | "/") {
			s--;
			goto gotname;
		}

		<attribs>	WS? NameStartChar			=> attribtail {
			// Walk radish tree, advance s to first divergent char
			s--;
			namebase = s-xml;
			//radish_search_init(pc->interp, &pc->cx.doc->radish_names, &pc->on_error, &search);
			radish_search_reset();
			s += radish_search(&search, s);
			goto yyc_attribtail;
		}
		<attribs>	WS? ">" @textstart	=> elemcontent {
			goto yyc_elemcontent;
		}
		<attribs>	WS? "/>"	{
			struct node*			node = &pc->cx.doc->nodes[pc->cx.slot];

			pc->cx.slot = node->parent;

			depth--;
			if (pc->cx.slot != 0) {
				c = yycelemcontent;
				goto yyc_elemcontent;
			} else {
				c = yycdocument;
				goto yyc_document;
			}
		}

		<attribtail>	@mstart NameChar* @mend Eq	=> attrval {
			goto gotname;
		}

		<attrval>		'"' 		=> attrvaldquote {goto yyc_attrvaldquote;}
		<attrval>		"'" 		=> attrvalsquote {goto yyc_attrvalsquote;}

		<attrvaldquote>	@textstart [^"<&\x00]+ / "&"	{
			Tcl_DStringAppend(&val, xml+(textstart-xml), s-textstart);
			translated = 1;
			textstart = s;
			goto yyc_attrvaldquote;
		}
		<attrvaldquote>	@textstart [^"<&\x00]+ / '"'	{
			if (translated) {
				Tcl_DStringAppend(&val, textstart, s-textstart);
			}
			goto yyc_attrvaldquote;
		}
		<attrvaldquote>	"&amp;"	{
			Tcl_DStringAppend(&val, "&", 1);
			translated = 1;
			goto yyc_attrvaldquote;
		}
		<attrvaldquote>	"&lt;"	{
			Tcl_DStringAppend(&val, "<", 1);
			translated = 1;
			goto yyc_attrvaldquote;
		}
		<attrvaldquote>	"&gt;"	{
			Tcl_DStringAppend(&val, ">", 1);
			translated = 1;
			goto yyc_attrvaldquote;
		}
		<attrvaldquote>	"&apos;"	{
			Tcl_DStringAppend(&val, "'", 1);
			translated = 1;
			goto yyc_attrvaldquote;
		}
		<attrvaldquote>	"&quot;"	{
			Tcl_DStringAppend(&val, "\"", 1);
			translated = 1;
			goto yyc_attrvaldquote;
		}
		<attrvaldquote>	"&#x" "0"* @mstart [1-9A-Fa-f][0-9A-Fa-f]* @mend ";" {
			base = 16;
			goto got_char_ref;
		}
		<attrvaldquote>	"&#" "0"* @mstart [1-9][0-9]* @mend ";" {
			base = 10;
			goto got_char_ref;
		}
		<attrvaldquote> @attrvalend '"' {
			goto gotattrib;
		}

		<attrvalsquote>	@textstart [^'<&\x00]+ / "&"	{
			Tcl_DStringAppend(&val, xml+(textstart-xml), s-textstart);
			translated = 1;
			textstart = s;
			goto yyc_attrvalsquote;
		}
		<attrvalsquote>	@textstart [^'<&\x00]+ / "'"	{
			if (translated) {
				Tcl_DStringAppend(&val, textstart, s-textstart);
			}
			goto yyc_attrvalsquote;
		}
		<attrvalsquote>	"&amp;"	{
			Tcl_DStringAppend(&val, "&", 1);
			translated = 1;
			goto yyc_attrvalsquote;
		}
		<attrvalsquote>	"&lt;"	{
			Tcl_DStringAppend(&val, "<", 1);
			translated = 1;
			goto yyc_attrvalsquote;
		}
		<attrvalsquote>	"&gt;"	{
			Tcl_DStringAppend(&val, ">", 1);
			translated = 1;
			goto yyc_attrvalsquote;
		}
		<attrvalsquote>	"&apos;"	{
			Tcl_DStringAppend(&val, "'", 1);
			translated = 1;
			goto yyc_attrvalsquote;
		}
		<attrvalsquote>	"&quot;"	{
			Tcl_DStringAppend(&val, "\"", 1);
			translated = 1;
			goto yyc_attrvalsquote;
		}
		<attrvalsquote>	"&#x" "0"* @mstart [1-9A-Fa-f][0-9A-Fa-f]* @mend ";" {
			base = 16;
			goto got_char_ref;
		}
		<attrvalsquote>	"&#" "0"* @mstart [1-9][0-9]* @mend ";" {
			base = 10;
			goto got_char_ref;
		}
		<attrvalsquote> @attrvalend "'" {
			goto gotattrib;
		}

		<elemcontent>	Comment		{ goto gotcomment; }
		<elemcontent>	PI_start	{ goto gotpi; }
		<elemcontent>	"<[CDATA["	{ goto readcdata; }
		<elemcontent>	"<"			:=> elem

		<elemcontent>	@textstart [^<&\x00]+ / "&"	{
			Tcl_DStringAppend(&val, xml+(textstart-xml), s-textstart);
			translated = 1;
			goto yyc_elemcontent;
		}
		<elemcontent>	@textstart [^<&\x00]+ / "<"	{
			struct node*			node = NULL;
			struct node_slot		myslot = {};
			const char*				text_val;
			int						text_len;

			node = new_node(pc->cx.doc, &myslot);
			node->type = TEXT;

			if (translated) {
				Tcl_DStringAppend(&val, xml+(textstart-xml), s-textstart);
				text_val = Tcl_DStringValue(&val);
				text_len = Tcl_DStringLength(&val);
			} else {
				text_val = textstart;
				text_len = s-textstart;
			}

			replace_tclobj(&node->value, get_string(pc->cx.doc->dedup_pool, text_val, text_len));

			node->name_id = pc->text_node_name_id;
			TEST_OK_LABEL(elemcontent_text_err, pc->rc, attach_node(pc->interp, &myslot, &pc->cx, NULL));

			if (translated) {
				translated = 0;
				Tcl_DStringFree(&val);
			}

			goto yyc_elemcontent;

		elemcontent_text_err:
			if (node) {
				free_node(node);
				node = NULL;
			}
			longjmp(pc->on_error, 1);
		}
		<elemcontent>	"&amp;"	{
			Tcl_DStringAppend(&val, "&", 1);
			translated = 1;
			goto yyc_elemcontent;
		}
		<elemcontent>	"&lt;"	{
			Tcl_DStringAppend(&val, "<", 1);
			translated = 1;
			goto yyc_elemcontent;
		}
		<elemcontent>	"&gt;"	{
			Tcl_DStringAppend(&val, ">", 1);
			translated = 1;
			goto yyc_elemcontent;
		}
		<elemcontent>	"&apos;"	{
			Tcl_DStringAppend(&val, "'", 1);
			translated = 1;
			goto yyc_elemcontent;
		}
		<elemcontent>	"&quot;"	{
			Tcl_DStringAppend(&val, "\"", 1);
			translated = 1;
			goto yyc_elemcontent;
		}
		<elemcontent>	"&#x" "0"* @mstart [1-9A-Fa-f][0-9A-Fa-f]* @mend ";" {
			base = 16;
			goto got_char_ref;
		}
		<elemcontent>	"&#" "0"* @mstart [1-9][0-9]* @mend ";" {
			base = 10;
			goto got_char_ref;
		}

		<elemcontent>	"</"					=> etagtail {
			if (translated) {
				struct node*			node = NULL;
				struct node_slot		myslot = {};
				const char*				text_val;
				int						text_len;

				node = new_node(pc->cx.doc, &myslot);
				node->type = TEXT;

				text_val = Tcl_DStringValue(&val);
				text_len = Tcl_DStringLength(&val);

				replace_tclobj(&node->value, get_string(pc->cx.doc->dedup_pool, text_val, text_len));

				node->name_id = pc->text_node_name_id;
				TEST_OK_LABEL(elemcontent_text_err, pc->rc, attach_node(pc->interp, &myslot, &pc->cx, NULL));

				translated = 0;
				Tcl_DStringFree(&val);
			}

			goto getname;
		}

		<etagtail>	@mstart NameChar* @mend WS? ">"	{
			struct node*			node = &pc->cx.doc->nodes[pc->cx.slot];

			if (mstart == mend) {
				name_id = (intptr_t)radish_get_value(&search);
			} else {
				Tcl_Obj* nameObj = NULL;

				TEST_OK_JMP(pc->on_error, pc->rc, Tcl_ListObjIndex(pc->interp, pc->cx.doc->names, node->name_id, &nameObj));
				THROW_ERROR_JMP(pc->on_error, pc->rc, pc->interp, "End tag at %d doesn't match start tag \"%s\"", namebase, Tcl_GetString(nameObj));
			}

			if (name_id != node->name_id) {
				Tcl_Obj* nameObj = NULL;

				TEST_OK_JMP(pc->on_error, pc->rc, Tcl_ListObjIndex(pc->interp, pc->cx.doc->names, node->name_id, &nameObj));
				THROW_ERROR_JMP(pc->on_error, pc->rc, pc->interp, "End tag at %d doesn't match start tag \"%s\"", namebase, Tcl_GetString(nameObj));
			}

			pc->cx.slot = node->parent;

			depth--;
			if (pc->cx.slot != 0) {
				c = yycelemcontent;
				goto yyc_elemcontent;
			} else {
				c = yycdocument;
				goto yyc_document;
			}
		}


		<*>	*	{ goto general_parse_error; }
	*/

general_parse_error:
	fail = s-xml-1;
	if (pc->interp) Tcl_SetObjResult(pc->interp, Tcl_ObjPrintf("XML parse error at %d", fail));
	return TCL_ERROR;

eat_intSubset: //<<<
	{
		int depth = 1;
		while (depth > 0) {
			/*!re2c
				re2c:api:style = free-form;
				re2c:define:YYCTYPE = "unsigned char";
				re2c:define:YYCURSOR = s;
				re2c:define:YYGETCONDITION = "c";
				re2c:define:YYSETCONDITION = "c = @@;";
				re2c:yyfill:enable = 0;
				re2c:flags:tags = 1;

			"["					{ depth++; }
			"]"					{ depth--; }
			[^\x5b\x5d\x00]+	{}
			*					{ goto general_parse_error; }

			*/
		}
		/*!re2c
			re2c:api:style = free-form;
			re2c:define:YYCTYPE = "unsigned char";
			re2c:define:YYCURSOR = s;
			re2c:define:YYGETCONDITION = "c";
			re2c:define:YYSETCONDITION = "c = @@;";
			re2c:yyfill:enable = 0;
			re2c:flags:tags = 1;

		WS? ">"		{}
		*			{ goto general_parse_error; }

		*/
		goto yyc_document;
	}
	
	//>>>
getname: //<<<
	// Walk radish tree, advance s to first divergent char
	//radish_search_init(pc->interp, &pc->cx.doc->radish_names, &pc->on_error, &search);
	radish_search_reset();
	namebase = s-xml;
	s += radish_search(&search, s);
	goto top;

	//>>>
gotname: //<<<
	// Unique tail [@mstart .. @mend)
	if (mstart == mend && search.value_ofs) {
		name_id = (uintptr_t)radish_get_value(&search);
	} else {
		TEST_OK_JMP(pc->on_error, pc->rc, alloc_name(
					pc->interp,
					pc->cx.doc,
					xml+namebase,
					mend-xml - namebase,
					&name_id));

		set_radish(&search, mstart, mend-mstart, UINT2PTR(name_id));
	}

	if (c == yycelemnametail) {
		struct node*		node = NULL;
		struct node_slot	myslot;

		if (unlikely(pc->cx.slot == 0)) {
			// First element: make this the root
			pc->cx.doc->nodes[pc->cx.doc->root].name_id = name_id;
			node = &pc->cx.doc->nodes[pc->cx.doc->root];
			TEST_OK_JMP(pc->on_error, pc->rc, get_node_slot(pc->interp, node, &myslot));
		} else {
			node = new_node(pc->cx.doc, &myslot);
			node->name_id = name_id;
		}

		if (likely(pc->cx.slot != 0)) {
			//DBG("Attaching to parent %d\n", pc->cx.slot);
			TEST_OK_LABEL(err, pc->rc, attach_node(pc->interp, &myslot, &pc->cx, NULL));
		} else {
			//DBG("no parent %d\n", pc->cx.slot);
		}
		//DBG("Pushing pc->cx.slot to current element: %d -> %d (my parent: %d)\n", pc->cx.slot, myslot.slot, node->parent);
		pc->cx.slot = myslot.slot;

		c = yycattribs;
		goto yyc_attribs;
	}
	goto top;

	//>>>
gotattrib: //<<<
	{
		struct node*		attrnode = NULL;
		struct node_slot	attrslot = {};
		int					attrval_len;
		const char*			attrval_str;

		if (translated) {
			attrval_str = Tcl_DStringValue(&val);
			attrval_len = Tcl_DStringLength(&val);
		} else {
			attrval_str = textstart;
			attrval_len = attrvalend - textstart;
		}

		attrnode = new_node(pc->cx.doc, &attrslot);
		attrnode->type = ATTRIBUTE;
		attrnode->name_id = name_id;
		replace_tclobj(&attrnode->value, get_string(pc->cx.doc->dedup_pool, attrval_str, attrval_len));

		// TODO: Fix error path leaks
		TEST_OK_JMP(pc->on_error, pc->rc, attach_node(pc->interp, &attrslot, &pc->cx, NULL));
		attrnode = NULL;

		if (translated) {
			translated = 0;
			Tcl_DStringFree(&val);
		}
	}

	c = yycattribs;
	goto yyc_attribs;

	//>>>
gotcomment: //<<<
	{
		struct node*			node = NULL;
		struct node_slot		myslot = {};
		const char*				comment_str;
		int						comment_len;

		comment_str = mstart;
		comment_len = mend-mstart;

		node = new_node(pc->cx.doc, &myslot);
		node->type = COMMENT;
		replace_tclobj(&node->value, get_string(pc->cx.doc->dedup_pool, comment_str, comment_len));

		node->name_id = pc->comment_node_name_id;
		TEST_OK_LABEL(gotcomment_err, pc->rc, attach_node(pc->interp, &myslot, &pc->cx, NULL));

		goto top;

	gotcomment_err:
		if (node) {
			free_node(node);
			node = NULL;
		}

		longjmp(pc->on_error, 1);
	}

	//>>>
gotpi: //<<<
	{
		struct node*			node = NULL;
		struct node_slot		myslot = {};
		Tcl_Obj*				val = NULL;
		Tcl_Obj*				ov[2] = {NULL, NULL};

		while (1) {
			/*!re2c
				re2c:api:style = free-form;
				re2c:define:YYCTYPE = "unsigned char";
				re2c:define:YYCURSOR = s;
				re2c:define:YYGETCONDITION = "c";
				re2c:define:YYSETCONDITION = "c = @@;";
				re2c:yyfill:enable = 0;
				re2c:flags:tags = 1;

			@mend "?>"	{ break; }
			Char		{}
			*			{
				fail = s-xml-1;

				if (pc->interp)
					Tcl_SetObjResult(pc->interp, Tcl_ObjPrintf("XML parse error at %d", fail));

				return TCL_ERROR;
			}
			*/
		}

		node = new_node(pc->cx.doc, &myslot);
		node->type = PI;
		replace_tclobj(&ov[0], get_string(pc->cx.doc->dedup_pool, namestart, nameend-namestart));
		replace_tclobj(&ov[1], get_string(pc->cx.doc->dedup_pool, mstart,    mend-mstart));
		replace_tclobj(&node->value, Tcl_NewListObj(2, ov));

		node->name_id = pc->pi_node_name_id;
		TEST_OK_LABEL(gotpi_err, pc->rc, attach_node(pc->interp, &myslot, &pc->cx, NULL));

		replace_tclobj(&val, NULL);
		replace_tclobj(&ov[0], NULL);
		replace_tclobj(&ov[1], NULL);

		goto top;

	gotpi_err:
		if (node) {
			free_node(node);
			node = NULL;
		}
		replace_tclobj(&val, NULL);
		replace_tclobj(&ov[0], NULL);
		replace_tclobj(&ov[1], NULL);
		longjmp(pc->on_error, 1);
	}

	//>>>
got_char_ref: //<<<
	{
		long int	codepoint;
		int			stored;
		char		buf[TCL_UTF_MAX];

		// Too many digits for highest allowable unicode codepoint
		if (mend - mstart > 7)
			THROW_ERROR_JMP(pc->on_error, pc->rc, pc->interp, "Invalid character specified via CharRef at %ld", mstart-xml);

		codepoint = strtol(mstart, NULL, base);
		// TODO: Properly check against valid Char range
		if (codepoint > 0x10FFFF)
			THROW_ERROR_JMP(pc->on_error, pc->rc, pc->interp, "Invalid character specified via CharRef at %ld", mstart-xml);
		stored = Tcl_UniCharToUtf(codepoint, buf);
		Tcl_DStringAppend(&val, buf, stored);
		translated = 1;
		goto top;
	}

	//>>>
readcdata: //<<<
	{
		uint32_t	p = s-xml;

		while (p < xmllen-3) {
			switch (xml[p+2]) {
				case '>':
					if (
						xml[p]   == ']' &&
						xml[p+1] == ']'
					) {
						struct node*			node = NULL;
						struct node_slot		myslot = {};
						const char*				text_val;
						int						text_len;

						node = new_node(pc->cx.doc, &myslot);
						node->type = TEXT;

						text_val = s;
						text_len = p-(s-xml);

						replace_tclobj(&node->value, get_string(pc->cx.doc->dedup_pool, text_val, text_len));

						node->name_id = pc->text_node_name_id;
						TEST_OK_LABEL(readcdata_text_err, pc->rc, attach_node(pc->interp, &myslot, &pc->cx, NULL));

						p += 3;
						s = xml+p;
						goto top;

					readcdata_text_err:
						if (node) {
							free_node(node);
							node = NULL;
						}
						longjmp(pc->on_error, 1);
					}
					p += 3;
					break;

				case ']':
					p++;
					break;

				default:
					p += 3;
			}
		}
		THROW_ERROR_JMP(pc->on_error, pc->rc, pc->interp, "XML truncated: unterminated cdata at offset %ld", s-xml);
	}

	//>>>
err: //<<<
	return TCL_ERROR;
	//>>>
#undef radish_search_reset
}

//>>>
#if 0
static int check_name_tail(const char* tail, int at_start, uint32_t *fail, uint32_t *adv) //<<<
{
	int			c = (at_start) ? yycinit : yycnametail;
	const char*	s = tail;
	const char*	YYMARKER;

	/* For some reason the inclusion of the range \U00010000-\U000EFFFF causes the pattern to incorrectly match characters way outside that range
			Char			= [\t\n\r\u0020-\uD7FF\uE000-\uFFFD\U00010000-\U0010FFFF]
			NameStartChar	= [:A-Z_a-z\xC0-\xF6\xF8-\u02FF\u0370-\u037D\u037F-\u1FFF\u200C-\u200D\u2070-\u218F\u2C00-\u2FEF\u3001-\uD7FF\uF900-\uFDCF\uFDF0-\uFFFD\U00010000-\U000EFFFF]
			NameChar		= NameStartChar | [.0-9\xB7\u0300-\u036F\u203F-\u2040-]
	*/

	/*!re2c
		re2c:api:style = free-form;
		re2c:define:YYCTYPE = "unsigned char";
		re2c:define:YYCURSOR = s;
		re2c:define:YYGETCONDITION = "c";
		re2c:define:YYSETCONDITION = "c = @@;";
		re2c:yyfill:enable = 0;
		re2c:flags:tags = 1;

		end_start_tag	= WS [">/"];

		<init>		NameStartChar				:=> nametail

		<nametail>	end_start_tag				{ *adv = s-tail-1; return 1; }
		<nametail>	NameChar					:=> nametail

		<*>			*							{ *fail = s-tail-1; return 0; }
	*/
}

//>>>
int parse_xml(struct parse_context* restrict pc, const char* restrict xml /* Must be null terminated */, const int xmllen) //<<<
{
	int			tokstart = 0;
	int			chunkstart = 0;
	int			p = 0;
	Tcl_DString	ds;
	int			transformed = 0;	// True if the value that will be emitted is transformed.  In that case ds contains the accumulated value
	jmp_buf		on_error;

	if (setjmp(on_error)) goto done;

	Tcl_DStringInit(&ds);

	// lexical closure functions (GCC extension, sadly)
	void emit_chunk() { // Copy the text [chunkstart, p) to ds, move chunkstart to p and flag as transformed <<<
		if (p > chunkstart) {
			Tcl_DStringAppend(&ds, xml+chunkstart, p-chunkstart);
			chunkstart = p;
			transformed = 1;
		}
	}

	//>>>
	/*
	int is_whitespace() { //<<<
		switch (xml[p]) {
			case ' ':
			case '\t':
			case '\n':
			case '\r':
				return 1;
			default:
				return 0;
		}
	}

	//>>>
	*/
	int consume_char() { // <<<
		uint32_t	codepoint = 0;
		uint32_t	first = p;

		if (unlikely(xml[p] == 0))
			THROW_ERROR_JMP(on_error, pc->rc, pc->interp, "XML truncated: %d", p);

		if (likely((xml[p] & 0x80) == 0)) return xml[p++];

		/*
		 * We omit the test of the continuation bytes below (xml[p] & 0xC0 != 0x80),
		 * because we have faith that Tcl won't hand us a string with invalid UTF-8
		 * sequences.
		 */
		if ((xml[p] & 0xE0) == 0xC0) { // Two byte encoding
			codepoint  = (xml[p++] & 0x1F) << 16;	// if (xml[p] & 0xC0 != 0x80) goto err;
			codepoint |=  xml[p++] & 0x3F;

			if (unlikely(codepoint == 0)) // MUTF-8 encoded NULL, XML does not permit them
				THROW_ERROR_JMP(on_error, pc->rc, pc->interp, "XML syntax error: NULL character at %d", p);
		} else if ((xml[p] & 0xF0) == 0xE0) { // Three byte encoding
			codepoint  = (xml[p++] & 0x0F) << 12;	// if (xml[p] & 0xC0 != 0x80) goto err;
			codepoint |= (xml[p++] & 0x3F) << 6;	// if (xml[p] & 0xC0 != 0x80) goto err;
			codepoint |=  xml[p++] & 0x3F;
		} else if ((xml[p] & 0xF8) == 0xF0) { // Four byte encoding
			codepoint  = (xml[p++] & 0x07) << 18;	// if (xml[p] & 0xC0 != 0x80) goto err;
			codepoint |= (xml[p++] & 0x3F) << 12;	// if (xml[p] & 0xC0 != 0x80) goto err;
			codepoint |= (xml[p++] & 0x3F) << 6;	// if (xml[p] & 0xC0 != 0x80) goto err;
			codepoint |=  xml[p++] & 0x3F;
			/*
		} else if ((xml[p] & 0xFC) == 0xF8) { // Five byte encoding
			codepoint  = (xml[p++] & 0x03) << 24;	// if (xml[p] & 0xC0 != 0x80) goto err;
			codepoint |= (xml[p++] & 0x3F) << 18;	// if (xml[p] & 0xC0 != 0x80) goto err;
			codepoint |= (xml[p++] & 0x3F) << 12;	// if (xml[p] & 0xC0 != 0x80) goto err;
			codepoint |= (xml[p++] & 0x3F) << 6;	// if (xml[p] & 0xC0 != 0x80) goto err;
			codepoint |=  xml[p++] & 0x3F;
		} else if ((xml[p] & 0xFE) == 0xFC) { // Six byte encoding
			codepoint  = (xml[p++] & 0x01) << 30;	// if (xml[p] & 0xC0 != 0x80) goto err;
			codepoint |= (xml[p++] & 0x3F) << 24;	// if (xml[p] & 0xC0 != 0x80) goto err;
			codepoint |= (xml[p++] & 0x3F) << 18;	// if (xml[p] & 0xC0 != 0x80) goto err;
			codepoint |= (xml[p++] & 0x3F) << 12;	// if (xml[p] & 0xC0 != 0x80) goto err;
			codepoint |= (xml[p++] & 0x3F) << 6;	// if (xml[p] & 0xC0 != 0x80) goto err;
			codepoint |=  xml[p++] & 0x3F;
			*/
		} else {
			goto err;
		}

		return codepoint;

err:
		THROW_ERROR_JMP(on_error, pc->rc, pc->interp, "XML syntax error: Invalid character encoding at %d", first);
	}

	//>>>
	uint32_t consume_name(const char* term_symbols) { // Consume a name token, look up or create it in the document name registry and return the name_id //<<<
		uint32_t				radish_slot = 0;	// Start the radish walk at the root
		Tcl_DString*			rn = &pc->cx.doc->radish_names[radish_slot];
		unsigned char* restrict	radish_node = Tcl_DStringValue(rn);
		uint32_t				rp = 0;				// Radish pointer - offset into rn->string
		const uint32_t			name_base = p;		// Offset into xml where name begins
		uint32_t				id;
		uint32_t				unique_name_suffix = 0;
		enum radish_match		match_state;

		// First walk the radish trie

		p += radish_resolve(pc->cx.doc->radish_names, xml+p, &radish_slot, &rp, &match_state);
		// radish_slot is the deepest matching slot, at offset fp

walk_node:
		while (1) {
			while (xml[p] && radish_node[rp] && xml[p] == radish_node[rp]) {
				p++;
				rp++;
			}

			if (xml[p] == 0)
				THROW_ERROR_JMP(on_error, pc->rc, pc->interp, "XML truncated: unterminated name at offset %d", chunkstart);

			if (radish_node[rp] == 0) {
				if ( /* only these chars are valid to end a start tag name */
						p > name_base &&
						index(term_symbols, xml[p])
				) {
					// Match: read and return the name_id
					int32_t 	id = *(int32_t *)(radish_node + ++rp);

					if (id == -1) {
						// Not a leaf node (yet) - no name allocated.  Allocate a
						// new name and store it to the radish node
						TEST_OK_JMP(on_error, pc->rc, alloc_name(
									pc->interp,
									pc->cx.doc,
									xml + name_base,
									p - name_base,
									&id));
						*(int32_t *)(radish_node + rp) = id;
					}

					return id;
				}

				/*
				 * Ran to the end of this radish node prefix, but more name
				 * remains.  Search the child radish nodes for the node
				 * matching the next byte of the name
				 */
				const char		next_name_byte = xml[p];

				rp += 5;	// Skip over the prefix null terminator and slot
				while (1) {
					const char		child_firstchar = radish_node[rp];
					int				adv;

					if (child_firstchar == 0) {
						// No more child nodes for this radish_node.  From xml[p] name is unique
						unique_name_suffix = p;
						// leave rp pointing at the end of radish_node, where we will
						// Tcl_DStringAppend the new child for this node suffix (if
						// it validates).
						goto check_unique_tail;
					}

					rp++;
					adv = read_radish_slot(radish_node+rp, &radish_slot);
					if (unlikely(adv == 0))
						THROW_ERROR_JMP(on_error, pc->rc, pc->interp, "Corrupted slot encoding for radish node");
					rp += adv;

					if (child_firstchar == next_name_byte) {
						// Found next radish node to walk
						rn = &pc->cx.doc->radish_names[radish_slot];
						radish_node = Tcl_DStringValue(rn);
						rp = 0;
						goto walk_node;
					}

					// Loop back around to check the next child node in sequence, looking
					// for the first char that matches xml[p]
				}
			} else {
				/*
				 * More chars remain on this radish prefix node, leave radish_node[rp] pointing
				 * to the first divergent character in the existing prefix.  If the tail of
				 * the name survives the valid chars checks below and is properly terminated,
				 * we'll split this radish node at rp and allocate a new branch for make from
				 * unique_name_suffix.
				 */
				unique_name_suffix = p;
				goto check_unique_tail;
			}
		}

check_unique_tail:
		{
			uint32_t	fail, adv;

			if (!check_name_tail(xml+p, p==name_base, &fail, &adv)) {
				const int	c = consume_char();
				THROW_ERROR_JMP(pc->on_error, pc->rc, pc->interp, "Name character invalid at offset %d: U+%04X", p+fail, c);
			}

			p += adv;
		}
		//if (index(term_symbols, xml[p])) break;

		// Register a new name [namebase..p)
		TEST_OK_JMP(on_error, pc->rc, alloc_name(pc->interp, pc->cx.doc, xml+namebase, p-namebase, &id));

		// Graft it into the radish trie:
		add_radish_node(pc->cx.doc, radish_slot, rp, xml+unique_name_suffix+1, p-unique_name_suffix-1, id);

		return id;
	}

	//>>>
	void consume_whitespace() { // Step over any whitespace <<<
		do {
			switch (xml[p]) {
				case ' ':
				case '\t':
				case '\n':
				case '\r':	// XML requires that \r be replaced before further processing, but we're discarding all this input anyway
					p++;
					continue;

					/*
				case '\r':	// XML spec requires \r to be folded with or into \n before any further processing
					emit_chunk();
					chunkstart++;
					if (
							xml[p+1] != '\n'
					) {
						// Lone \r (no following \n): translate it to \n
						Tcl_DStringAppend(&ds, "\n", 1);
					}
					p++;
					continue;
					*/
			}
		} while(0);
		chunkstart = p;
	}

	//>>>
	void consume_pi() { // Consume a processing instruction <<<
		const uint32_t name_id = consume_name(" ?");

		consume_whitespace();

		do {
			switch (xml[p]) {
				case '?':
					if (xml[p+1] == '>') goto end;
					break;
				case '&':	// possible start of entity encoding;
					consume_entity();
					break;
				case 0:		// truncated input inside a PI
					THROW_ERROR_JMP(on_error, pc->rc, pc->interp, "XML truncated");
			}
			p++;
		} while(0);

end:
		// TODO: create PI node
		if (likely(translated == 0)) {
			//replace(&node->value, get_string(pc->cx.d->dedup_pool, xml+chunkstart, p-1-chunkstart));
		} else {
			emit_chunk();
			//replace(&node->value, get_string(pc->cx.d->dedup_pool, Tcl_DStringValue(&ds), Tcl_DStringLength(&ds)));
			Tcl_DStringFree(&ds);
			transformed = 0;
		}
		p += 2;
		chunkstart = p;
	}

	//>>>
	void consume_comment() { //<<<
		while (p < xmllen-3) {
			switch (xml[p+2]) {
				case '>':
					if (
						xml[p]   == '-' &&
						xml[p+1] == '-'
					) {
						// TODO:
						// - create comment node, containing [chunkstart, p)
						// - attach to parent
						p += 3;
						return;
					}
					p += 3;
					break;

				case '-':
					p++;
					break;

				default:
					/*
					 * XML prohibits comments containing the sequence --,
					 * but we don't reject them because the check would spoil
					 * the efficiency of this hard-coded Boyer-Moore search.
					 */
					/*
					if (
						xml[p]   == '-' &&
						xml[p+1] == '-'
					) THROW_ERROR_JMP(on_error, pc->rc, pc->interp, "XML syntax error: -- in comment at offset %d", p);
					*/
					p += 3;
			}
		}
		THROW_ERROR_JMP(on_error, pc->rc, pc->interp, "XML truncated: unterminated comment at offset %d", chunkstart);
	}

	//>>>
	void consume_cdata() { //<<<
		while (p < xmllen-3) {
			switch (xml[p+2]) {
				case '>':
					if (
						xml[p]   == ']' &&
						xml[p+1] == ']'
					) {
						// TODO:
						// - create text node, containing [chunkstart, p)
						// - attach to parent
						p += 3;
						return;
					}
					p += 3;
					break;

				case ']':
					p++;
					break;

				default:
					p += 3;
			}
		}
		THROW_ERROR_JMP(on_error, pc->rc, pc->interp, "XML truncated: unterminated cdata at offset %d", chunkstart);
	}

	//>>>
	void consume_element() { // <<<
		uint32_t name_id

		// Divert to handler for PI, CDATA, declaration or comment if we're looking at one of those rather than an element <<<
		switch (xml[p]) {
			case '?':
				p++;
				consume_pi();
				return;

			case '!':
				if (
						xml[p+1] == '-' &&
						xml[p+2] == '-' 
				) {
					p += 3;
					chunkstart = p;
					consume_comment();
					return;
				} else if (
						xml[p+1] == '[' &&
						xml[p+2] == 'C' &&
						xml[p+3] == 'D' &&
						xml[p+4] == 'A' &&
						xml[p+5] == 'T' &&
						xml[p+6] == 'A' &&
						xml[p+7] == '['
				) {
					p += 8;
					chunkstart = p;
					consume_cdata();
					return;
				}
				p++;
				chunkstart = p;
				consume_declaration();
				return;
		}
		// Divert to handler for PI, CDATA, declaration or comment if we're looking at one of those rather than an element >>>

		name_id = consume_name(" \t\n\r>/");

		// TODO: create element node, attach to parent if not the root, and set it as the context

		do {
			consume_whitespace();

			switch (xml[p]) {
				case '>':
					break;

				case '/':
					if (xml[p+1] != '>') 
						THROW_ERROR_JMP(on_error, pc->rc, pc->interp, "XML syntax error: \"/\" in tag");

					p += 2; goto end;

				case 0:		// truncated input inside a tag
					THROW_ERROR_JMP(on_error, pc->rc, pc->interp, "XML truncated");

				default:	// start of attrib
					{
						const uint32_t	attrib_name_id = consume_name(" \t\n\r=");

						consume_whitespace();

						if (xml[p] != '=')
							THROW_ERROR_JMP(on_error, pc->rc, pc->interp, "XML syntax error: attribute followed by %c instead of =", xml[p]);
						p++;

						consume_whitespace();
						{
							const char	quote = xml[p];

							switch (quote) {
								case '"':
								case '\'':
									break;
								case 0:
									THROW_ERROR_JMP(on_error, pc->rc, pc->interp, "XML truncated");
								default:
									THROW_ERROR_JMP(on_error, pc->rc, pc->interp, "XML syntax error: expecting \" or ' to start attribute value, got: %c", xml[p]);
							}
							p++;
							chunk_start = p;
							do {
								switch (xml[p]) {
									case quote: // Found the matching quote
										// TODO: Add this attibute to the element
										break;

									case 0:
										THROW_ERROR_JMP(on_error, pc->rc, pc->interp, "XML truncated");

									case '<':
										THROW_ERROR_JMP(on_error, pc->rc, pc->interp, "XML syntax error: bare < in attribute value");

									case '&':	// Entity
										consume_entity();
										break;
									default:
										p++;
										continue;
								}
							} while(0);
						}
					}
			}
			p++;
		} while(0);

		chunkstart = p;

		// p points to the first byte of child contents for the element
		switch (xml[p]) {
			case '<':
				if (xml[p+1] == '/') { // Closing tag, matching our open tag.  Confirm that the names match
					p += 2;
					const uint32_t	close_name = consume_name_id(" \t\n\r>");
					if (close_name_id != name_id)
						THROW_ERROR_JMP(on_error, pc->rc, pc->interp, "XML syntax error: closing tag name doesn't match opening tag");
					consume_whitespace();
					if (xml[p] != '>')
						THROW_ERROR_JMP(on_error, pc->rc, pc->interp, "XML syntax error: expecting >, got: %c", xml[p]);
					p++;
					goto end;
				}

				// Open tag for a child element
				p++;
				consume_element();
				break;

			default:
		}
end:
	}

	//>>>

	while (p < xmllen) {
		// Context: outside of an element: scan forward, ignoring anything before the first tag
		//while (xml[p] != '<' && xml[p] != 0) p++;

		consume_whitespace();

		switch (xml[p]) {
			case '<':	p++; consume_element(); break;
			case 0:		goto done;
			default:
				THROW_ERROR_JMP(on_error, pc->rc, pc->interp, "XML syntax error: content outside of root element");
		}
	}

done:
	Tcl_DStringFree(&ds);
	return pc->rc;
}

//>>>
#endif
// Internal parser >>>

void pcb_start_element(void* userData, const XML_Char* name, const XML_Char** atts) //<<<
{
	struct parse_context*	pc = (struct parse_context*)userData;
	struct node*			node = NULL;
	const XML_Char**		attrib = atts;
	struct node_slot		myslot = {};
	struct node*			attrnode = NULL;

	//DBG("pcb_start_element: (%s)\n", name);
	if (unlikely(pc->cx.slot == 0)) {
		// First element: create the doc and make this the root
		TEST_OK_LABEL(err, pc->rc, set_node_name(pc->interp, &pc->cx.doc->nodes[pc->cx.doc->root], name));
		node = &pc->cx.doc->nodes[pc->cx.doc->root];
		//DBG("Created new doc %s with root %s\n", Tcl_GetString(pc->cx.doc->docname), name);
		TEST_OK_LABEL(err, pc->rc, get_node_slot(pc->interp, node, &myslot));
	} else {
		node = new_node(pc->cx.doc, &myslot);
		TEST_OK_LABEL(err, pc->rc, set_node_name(pc->interp, node, name));
	}

	while (*attrib) {
		const XML_Char*		attrname  = *attrib++;
		const XML_Char*		attrvalue = *attrib++;
		struct node_slot	attrslot = {};

		attrnode = new_node(pc->cx.doc, &attrslot);
		attrnode->type = ATTRIBUTE;
		TEST_OK_LABEL(err, pc->rc, set_node_name(pc->interp, attrnode, attrname));
		replace_tclobj(&attrnode->value, get_string(pc->cx.doc->dedup_pool, attrvalue, -1));

		TEST_OK_LABEL(err, pc->rc, attach_node(pc->interp, &attrslot, &myslot, NULL));
		attrnode = NULL;
	}

	if (likely(pc->cx.slot != 0)) {
		//DBG("Attaching to parent %d\n", pc->cx.slot);
		TEST_OK_LABEL(err, pc->rc, attach_node(pc->interp, &myslot, &pc->cx, NULL));
	} else {
		//DBG("no parent %d\n", pc->cx.slot);
	}
	//DBG("Pushing pc->cx.slot to current element: %d -> %d (my parent: %d)\n", pc->cx.slot, myslot.slot, node->parent);
	pc->cx.slot = myslot.slot;

	return;

err:
	if (attrnode) {
		free_node(attrnode);
		attrnode = NULL;
	}
	if (node) {
		free_node(node);
		node = NULL;
	}

	XML_StopParser(pc->parser, XML_FALSE);
	return;
}

//>>>
void pcb_end_element(void* userData, const XML_Char* name) //<<<
{
	struct parse_context*	pc = (struct parse_context*)userData;
	struct node*			node = &pc->cx.doc->nodes[pc->cx.slot];

	//DBG("pcb_end_element: (%s)\n", name);
	//DBG("Popping pc->cx.slot to parent: %d <- %d\n", node->parent, pc->cx.slot);
	pc->cx.slot = node->parent;
}

//>>>
void pcb_text(void* userData, const XML_Char* s, int len) //<<<
{
	struct parse_context*	pc = (struct parse_context*)userData;
	struct node*			node = NULL;
	struct node_slot		myslot = {};

	//DBG("pcb_text\n");
	node = new_node(pc->cx.doc, &myslot);
	node->type = TEXT;
	replace_tclobj(&node->value, get_string(pc->cx.doc->dedup_pool, s, len));

	node->name_id = pc->text_node_name_id;
	TEST_OK_LABEL(err, pc->rc, attach_node(pc->interp, &myslot, &pc->cx, NULL));

	return;

err:
	if (node) {
		free_node(node);
		node = NULL;
	}

	XML_StopParser(pc->parser, XML_FALSE);
	return;
}

//>>>
void pcb_pi(void* userData, const XML_Char* target, const XML_Char* data) //<<<
{
	struct parse_context*	pc = (struct parse_context*)userData;
	struct node*			node = NULL;
	struct node_slot		myslot = {};
	Tcl_Obj*				val = NULL;
	Tcl_Obj*				ov[2] = {NULL, NULL};

	if (unlikely(pc->cx.doc == NULL)) return;
	//DBG("pcb_pi: (%s), (%s)\n", target, data);
	node = new_node(pc->cx.doc, &myslot);
	node->type = PI;
	replace_tclobj(&ov[0], get_string(pc->cx.doc->dedup_pool, target, -1));
	replace_tclobj(&ov[1], get_string(pc->cx.doc->dedup_pool, data,   -1));
	replace_tclobj(&node->value, Tcl_NewListObj(2, ov));

	node->name_id = pc->pi_node_name_id;
	TEST_OK_LABEL(err, pc->rc, attach_node(pc->interp, &myslot, &pc->cx, NULL));

	goto done;

err:
	if (node) {
		free_node(node);
		node = NULL;
	}
	XML_StopParser(pc->parser, XML_FALSE);

done:
	replace_tclobj(&val, NULL);
	replace_tclobj(&ov[0], NULL);
	replace_tclobj(&ov[1], NULL);

	return;
}

//>>>
void pcb_comment(void* userData, const XML_Char* data) //<<<
{
	struct parse_context*	pc = (struct parse_context*)userData;
	struct node*			node = NULL;
	struct node_slot		myslot = {};

	//DBG("pcb_comment: %s\n", data);
	if (unlikely(pc->cx.doc == NULL || pc->cx.slot == 0)) return;

	node = new_node(pc->cx.doc, &myslot);
	node->type = COMMENT;
	replace_tclobj(&node->value, get_string(pc->cx.doc->dedup_pool, data, -1));

	node->name_id = pc->comment_node_name_id;
	TEST_OK_LABEL(err, pc->rc, attach_node(pc->interp, &myslot, &pc->cx, NULL));

	return;

err:
	if (node) {
		free_node(node);
		node = NULL;
	}

	XML_StopParser(pc->parser, XML_FALSE);
	return;
}

//>>>

// vim: foldmethod=marker foldmarker=<<<,>>> ts=4 shiftwidth=4 ft=c
