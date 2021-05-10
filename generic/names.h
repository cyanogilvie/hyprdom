#ifndef _NAMES_H
#define _NAMES_H

const char* name(const char* prefix, const void* thing);	// name given thing
void* thing(const char* name);		// thing given name
void free_thing(const void* thing);	// let go of the thing

#endif
