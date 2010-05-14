/*
 * Copyright (C) 2009, 2010  Daniel Borkmann <daniel@netsniff-ng.org> and 
 *                           Emmanuel Roullit <emmanuel@netsniff-ng.org>
 *
 * This program is free software; you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation; either version 2 of the License, or (at 
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License 
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc., 
 * 51 Franklin St, Fifth Floor, Boston, MA 02110, USA
 */

#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <arpa/inet.h>

#include <netsniff-ng/macros.h>
#include <netsniff-ng/hash.h>

#include <netsniff-ng/oui.h>
#include <netsniff-ng/ports_udp.h>
#include <netsniff-ng/ports_tcp.h>
#include <netsniff-ng/ether_types.h>

/*
 * Hash function API
 */

int hashtable_init(hashtable_t ** ht, size_t size, hashtable_callbacks_t * f)
{
	if (ht == NULL || f == NULL || size == 0)
		return -EINVAL;

	/* Check hash function pointer validity */
	if (f->key_copy == NULL || f->key_to_hash == NULL || f->key_equal == NULL)
		return -EINVAL;

	*ht = malloc(sizeof(**ht));
	if (*ht == NULL)
		return -ENOMEM;

	(*ht)->size = size;
	(*ht)->elems = 0;
	(*ht)->f = f;

	(*ht)->table = malloc(sizeof(*(*ht)->table) * size);
	if ((*ht)->table == NULL) {
		free(*ht);
		return -ENOMEM;
	}

	memset((*ht)->table, 0, sizeof(*(*ht)->table) * size);

	return 0;
}

void hashtable_destroy(hashtable_t * ht)
{
	int i;
	hashtable_bucket_t *hb, *hb_prev;

	assert(ht);

	for (i = 0; i < ht->size; ++i) {
		for (hb = ht->table[i]; hb != NULL;) {
			hb_prev = hb;
			ht->f->key_free(hb->key);
			hb = hb->next;
			free(hb_prev);
		}
	}

	free(ht->table);
	free(ht);
}

void *hashtable_insert(hashtable_t * ht, void * key, void *data)
{
	uintptr_t val;
	hashtable_bucket_t *hb;

	assert(ht);
	assert(ht->size);

	val = ht->f->key_to_hash(key) % ht->size;

	for (hb = ht->table[val]; hb != NULL; hb = hb->next)
		if (ht->f->key_equal(key, hb->key))
			return hb->data;

	hb = malloc(sizeof(*hb));
	if (hb == NULL)
		return NULL;

	hb->next = ht->table[val];
	hb->data = data;
	hb->key = ht->f->key_copy(key);
	ht->table[val] = hb;

	ht->elems++;

	return data;
}

void *hashtable_find(hashtable_t * ht, void *key)
{
	uintptr_t val;
	hashtable_bucket_t *hb;

	assert(ht);
	assert(ht->size);

	val = ht->f->key_to_hash(key) % ht->size;

	for (hb = ht->table[val]; hb != NULL; hb = hb->next)
		if (ht->f->key_equal(key, hb->key))
			return hb->data;
	return NULL;
}

void *hashtable_delete(hashtable_t * ht, void *key)
{
	uintptr_t val;
	hashtable_bucket_t *hb, *hb_prev;
	void *data = NULL;

	assert(ht);
	assert(ht->size);

	val = ht->f->key_to_hash(key) % ht->size;

	for (hb_prev = NULL, hb = ht->table[val]; hb != NULL; hb_prev = hb, hb = hb->next) {
		if (ht->f->key_equal(key, hb->key)) {
			data = hb->data;

			if (hb_prev)
				hb_prev->next = hb->next;
			else
				ht->table[val] = hb->next;

			ht->f->key_free(hb->key);
			free(hb);
			ht->elems--;
		}
	}

	return data;
}

int hashtable_foreach(hashtable_t * ht, void (*callback) (void *key, void *data))
{
	int i;
	hashtable_bucket_t *hb;

	assert(ht);
	assert(callback);

	for (i = 0; i < ht->size; ++i)
		for (hb = ht->table[i]; hb != NULL; hb = hb->next)
			callback(hb->key, hb->data);

	return 0;
}

/*
 * Some callback implementations
 */

void *no_copy(void *key)
{
	return key;
}

void no_free(void *key)
{
	return;
}

uintptr_t raw_key_to_hash(void *key)
{
	return (uintptr_t) key;
}

int raw_key_equal(void *key1, void *key2)
{
	return (key1 == key2);
}

void debug_callback_strings(void *key, void *data)
{
	printf("0x%x - %s\n", (uintptr_t) key, (char *) data);
}

/*
 * Specific hash function implementations
 */

/*
 * IEEE vendor table
 */

static hashtable_t *ieee_vendor_db;
static hashtable_callbacks_t ieee_vendor_cbs = {
	.key_copy = no_copy,
	.key_free = no_free,
	.key_to_hash = raw_key_to_hash,
	.key_equal = raw_key_equal,
};

int ieee_vendors_init(void)
{
	int i, ret;
	size_t len;

	ret = hashtable_init(&ieee_vendor_db, 14000, &ieee_vendor_cbs);
	if (ret < 0) {
		warn("Could not create vendor hashtable! No mem left.\n");
		return ret;
	}

	len = sizeof(vendor_db) / sizeof(vendor_id_t);

	for (i = 0; i < len; ++i) {
		hashtable_insert(ieee_vendor_db, (void *)vendor_db[i].id, vendor_db[i].vendor);
	}

	return 0;
}

void ieee_vendors_destroy(void)
{
	hashtable_destroy(ieee_vendor_db);
}

const char *ieee_vendors_find(const uint8_t * mac_addr)
{
	char *vendor;
	uintptr_t key = 0;
	uint8_t *keyp = (uint8_t *) & key;

	memcpy(&keyp[1], mac_addr, 3);

	key = ntohl(key);

	vendor = hashtable_find(ieee_vendor_db, (void *)key);
	if (!vendor)
		vendor = vendor_unknown;

	return vendor;
}

/*
 * UDP port table
 */

static hashtable_t *ports_udp_db;
static hashtable_callbacks_t ports_udp_cbs = {
	.key_copy = no_copy,
	.key_free = no_free,
	.key_to_hash = raw_key_to_hash,
	.key_equal = raw_key_equal,
};

int ports_udp_init(void)
{
	int i, ret;
	size_t len;

	ret = hashtable_init(&ports_udp_db, 14000, &ports_udp_cbs);
	if (ret < 0) {
		warn("Could not create udp ports hashtable! No mem left.\n");
		return ret;
	}

	len = sizeof(ports_udp) / sizeof(port_udp_t);

	for (i = 0; i < len; ++i) {
		hashtable_insert(ports_udp_db, (void *)ports_udp[i].id, ports_udp[i].port);
	}

	return 0;
}

void ports_udp_destroy(void)
{
	hashtable_destroy(ports_udp_db);
}

const char *ports_udp_find(uint16_t port)
{
	uintptr_t key = 0;
	uint8_t *keyp = (uint8_t *) &key;

	keyp[3] = (port >> 8) & 0xFF;
	keyp[2] = (port)      & 0xFF;
	key = ntohl(key);

	return hashtable_find(ports_udp_db, (void *)key);
}

/*
 * TCP port table
 */

static hashtable_t *ports_tcp_db;
static hashtable_callbacks_t ports_tcp_cbs = {
	.key_copy = no_copy,
	.key_free = no_free,
	.key_to_hash = raw_key_to_hash,
	.key_equal = raw_key_equal,
};

int ports_tcp_init(void)
{
	int i, ret;
	size_t len;

	ret = hashtable_init(&ports_tcp_db, 14000, &ports_tcp_cbs);
	if (ret < 0) {
		warn("Could not create tcp ports hashtable! No mem left.\n");
		return ret;
	}

	len = sizeof(ports_tcp) / sizeof(port_tcp_t);

	for (i = 0; i < len; ++i) {
		hashtable_insert(ports_tcp_db, (void *)ports_tcp[i].id, ports_tcp[i].port);
	}

	return 0;
}

void ports_tcp_destroy(void)
{
	hashtable_destroy(ports_tcp_db);
}

const char *ports_tcp_find(uint16_t port)
{
	uintptr_t key = 0;
	uint8_t *keyp = (uint8_t *) &key;

	keyp[3] = (port >> 8) & 0xFF;
	keyp[2] = (port)      & 0xFF;
	key = ntohl(key);

	return hashtable_find(ports_tcp_db, (void *)key);
}

/*
 * Ether types table
 */

static hashtable_t *ether_types_db;
static hashtable_callbacks_t ether_types_cbs = {
	.key_copy = no_copy,
	.key_free = no_free,
	.key_to_hash = raw_key_to_hash,
	.key_equal = raw_key_equal,
};

int ether_types_init(void)
{
	int i, ret;
	size_t len;

	ret = hashtable_init(&ether_types_db, 14000, &ether_types_cbs);
	if (ret < 0) {
		warn("Could not create ether types hashtable! No mem left.\n");
		return ret;
	}

	len = sizeof(ether_types) / sizeof(ether_type_t);

	for (i = 0; i < len; ++i) {
		hashtable_insert(ether_types_db, (void *)ether_types[i].id, ether_types[i].type);
	}

	return 0;
}

void ether_types_destroy(void)
{
	hashtable_destroy(ether_types_db);
}

const char *ether_types_find(uint16_t type)
{
	char *type_str;
	uintptr_t key = 0;
	uint8_t *keyp = (uint8_t *) &key;

	keyp[3] = (type >> 8) & 0xFF;
	keyp[2] = (type)      & 0xFF;
	key = ntohl(key);

	type_str = hashtable_find(ether_types_db, (void *)key);
	if (!type_str)
		type_str = type_unknown;

	return type_str;
}

