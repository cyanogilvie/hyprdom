struct parse_context {
	Tcl_Interp*			interp;
	XML_Parser			parser;
	int					rc;
	uint32_t			slots_estimate;
	uint32_t			depth;
	struct node_slot	cx;
	uint32_t			text_node_name_id;
	uint32_t			comment_node_name_id;
	uint32_t			pi_node_name_id;
	jmp_buf				on_error;
};


void pcb_start_element(void* userData, const XML_Char* name, const XML_Char** atts);
void pcb_end_element(void* userData, const XML_Char* name);
void pcb_text(void* userData, const XML_Char* s, int len);
void pcb_pi(void* userData, const XML_Char* target, const XML_Char* data);
void pcb_comment(void* userData, const XML_Char* data);

int parse_xml_re(struct parse_context* restrict pc, const char* restrict xml /* Must be null terminated */, const int xmllen);
int parse_xml(struct parse_context* restrict pc, const char* restrict xml /* Must be null terminated */, const int xmllen);
