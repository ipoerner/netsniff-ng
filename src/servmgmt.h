/*
 * netsniff-ng - the packet sniffing beast
 * By Daniel Borkmann <daniel@netsniff-ng.org>
 * Copyright 2011 Daniel Borkmann.
 * Subject to the GPL.
 */

#ifndef SERVMGMT_H
#define SERVMGMT_H

#include <stdio.h>

extern void parse_userfile_and_generate_serv_store_or_die(char *homedir);
extern void dump_serv_store(void);
extern void get_serv_store_entry_by_alias(char *alias, size_t len,
					  char **host, char **port, int *udp);
extern void destroy_serv_store(void);

#endif /* SERVMGMT_H */
