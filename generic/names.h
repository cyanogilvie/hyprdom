#ifndef _NAMES_H
#define _NAMES_H

const char* name(const void *const thing);	// name given thing
void* thing(const char *const name);		// thing given name
void free_thing(const void *const thing);	// let go of the thing

#endif
