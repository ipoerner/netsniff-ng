/*
 * netsniff-ng - the packet sniffing beast
 * By Daniel Borkmann <daniel@netsniff-ng.org>
 * Copyright 2009, 2010 Daniel Borkmann.
 * Copyright 2009, 2010 Emmanuel Roullit.
 * Subject to the GPL, version 2.
 */

/*
 * Copyright (c) 1990, 1991, 1992, 1994, 1995, 1996
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the University of California,
 * Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <arpa/inet.h>

#include "bpf.h"
#include "xmalloc.h"
#include "strlcpy.h"
#include "die.h"

/* This is a bug in libpcap, they actually use 'unsigned long' instead
 * of short! */
#define EXTRACT_SHORT(packet)						\
		((unsigned short) ntohs(*(unsigned short *) packet))
#define EXTRACT_LONG(packet)						\
		(ntohl(*(unsigned long *) packet))

/*
 * Number of scratch memory words for: BPF_ST and BPF_STX
 */
#ifndef BPF_MEMWORDS
# define BPF_MEMWORDS 16
#endif /* BPF_MEMWORDS */

static char *bpf_dump(const struct sock_filter bpf, int n)
{
	int v;
	const char *fmt, *op;
	static char image[256];
	char operand[64];

	v = bpf.k;

	switch (bpf.code) {
	default:
		op = "unimp";
		fmt = "0x%x";
		v = bpf.code;
		break;

	case BPF_RET | BPF_K:
		op = "ret";
		fmt = "#%d";
		break;

	case BPF_RET | BPF_A:
		op = "ret";
		fmt = "";
		break;

	case BPF_LD | BPF_W | BPF_ABS:
		op = "ld";
		fmt = "[%d]";
		break;

	case BPF_LD | BPF_H | BPF_ABS:
		op = "ldh";
		fmt = "[%d]";
		break;

	case BPF_LD | BPF_B | BPF_ABS:
		op = "ldb";
		fmt = "[%d]";
		break;

	case BPF_LD | BPF_W | BPF_LEN:
		op = "ld";
		fmt = "#pktlen";
		break;

	case BPF_LD | BPF_W | BPF_IND:
		op = "ld";
		fmt = "[x + %d]";
		break;

	case BPF_LD | BPF_H | BPF_IND:
		op = "ldh";
		fmt = "[x + %d]";
		break;

	case BPF_LD | BPF_B | BPF_IND:
		op = "ldb";
		fmt = "[x + %d]";
		break;

	case BPF_LD | BPF_IMM:
		op = "ld";
		fmt = "#0x%x";
		break;

	case BPF_LDX | BPF_IMM:
		op = "ldx";
		fmt = "#0x%x";
		break;

	case BPF_LDX | BPF_MSH | BPF_B:
		op = "ldxb";
		fmt = "4*([%d]&0xf)";
		break;

	case BPF_LD | BPF_MEM:
		op = "ld";
		fmt = "M[%d]";
		break;

	case BPF_LDX | BPF_MEM:
		op = "ldx";
		fmt = "M[%d]";
		break;

	case BPF_ST:
		op = "st";
		fmt = "M[%d]";
		break;

	case BPF_STX:
		op = "stx";
		fmt = "M[%d]";
		break;

	case BPF_JMP | BPF_JA:
		op = "ja";
		fmt = "%d";
		v = n + 1 + bpf.k;
		break;

	case BPF_JMP | BPF_JGT | BPF_K:
		op = "jgt";
		fmt = "#0x%x";
		break;

	case BPF_JMP | BPF_JGE | BPF_K:
		op = "jge";
		fmt = "#0x%x";
		break;

	case BPF_JMP | BPF_JEQ | BPF_K:
		op = "jeq";
		fmt = "#0x%x";
		break;

	case BPF_JMP | BPF_JSET | BPF_K:
		op = "jset";
		fmt = "#0x%x";
		break;

	case BPF_JMP | BPF_JGT | BPF_X:
		op = "jgt";
		fmt = "x";
		break;

	case BPF_JMP | BPF_JGE | BPF_X:
		op = "jge";
		fmt = "x";
		break;

	case BPF_JMP | BPF_JEQ | BPF_X:
		op = "jeq";
		fmt = "x";
		break;

	case BPF_JMP | BPF_JSET | BPF_X:
		op = "jset";
		fmt = "x";
		break;

	case BPF_ALU | BPF_ADD | BPF_X:
		op = "add";
		fmt = "x";
		break;

	case BPF_ALU | BPF_SUB | BPF_X:
		op = "sub";
		fmt = "x";
		break;

	case BPF_ALU | BPF_MUL | BPF_X:
		op = "mul";
		fmt = "x";
		break;

	case BPF_ALU | BPF_DIV | BPF_X:
		op = "div";
		fmt = "x";
		break;

	case BPF_ALU | BPF_AND | BPF_X:
		op = "and";
		fmt = "x";
		break;

	case BPF_ALU | BPF_OR | BPF_X:
		op = "or";
		fmt = "x";
		break;

	case BPF_ALU | BPF_LSH | BPF_X:
		op = "lsh";
		fmt = "x";
		break;

	case BPF_ALU | BPF_RSH | BPF_X:
		op = "rsh";
		fmt = "x";
		break;

	case BPF_ALU | BPF_ADD | BPF_K:
		op = "add";
		fmt = "#%d";
		break;

	case BPF_ALU | BPF_SUB | BPF_K:
		op = "sub";
		fmt = "#%d";
		break;

	case BPF_ALU | BPF_MUL | BPF_K:
		op = "mul";
		fmt = "#%d";
		break;

	case BPF_ALU | BPF_DIV | BPF_K:
		op = "div";
		fmt = "#%d";
		break;

	case BPF_ALU | BPF_AND | BPF_K:
		op = "and";
		fmt = "#0x%x";
		break;

	case BPF_ALU | BPF_OR | BPF_K:
		op = "or";
		fmt = "#0x%x";
		break;

	case BPF_ALU | BPF_LSH | BPF_K:
		op = "lsh";
		fmt = "#%d";
		break;

	case BPF_ALU | BPF_RSH | BPF_K:
		op = "rsh";
		fmt = "#%d";
		break;

	case BPF_ALU | BPF_NEG:
		op = "neg";
		fmt = "";
		break;

	case BPF_MISC | BPF_TAX:
		op = "tax";
		fmt = "";
		break;

	case BPF_MISC | BPF_TXA:
		op = "txa";
		fmt = "";
		break;
	}

	/* XXX: Tell gcc that this here is okay for us */
	slprintf(operand, sizeof(operand), fmt, v);
	slprintf(image, sizeof(image),
		 (BPF_CLASS(bpf.code) == BPF_JMP &&
		  BPF_OP(bpf.code) != BPF_JA) ?
		 "(%03d) %-8s %-16s jt %d\tjf %d" : "(%03d) %-8s %s",
		 n, op, operand, n + 1 + bpf.jt, n + 1 + bpf.jf);
	return image;
}

void bpf_dump_all(struct sock_fprog *bpf)
{
	int i;
	for (i = 0; i < bpf->len; ++i)
		printf("%s\n", bpf_dump(bpf->filter[i], i));
}

void bpf_attach_to_sock(int sock, struct sock_fprog *bpf)
{
	/* See Documentation/networking/filter.txt */
	int ret = setsockopt(sock, SOL_SOCKET, SO_ATTACH_FILTER, bpf,
			     sizeof(*bpf));
	if (ret < 0)
		error_and_die(EXIT_FAILURE, "Cannot attach filter to "
			      "socket!\n");
}

void bpf_detach_from_sock(int sock)
{
	int ret, empty = 0;
	/* See Documentation/networking/filter.txt */
	ret = setsockopt(sock, SOL_SOCKET, SO_DETACH_FILTER, &empty,
			 sizeof(empty));
	if (ret < 0)
		error_and_die(EXIT_FAILURE, "Cannot detach filter from "
			      "socket!\n");
}

int bpf_validate(const struct sock_fprog *bpf)
{
	uint32_t i, from;
	const struct sock_filter *p;

	if (!bpf)
		return 0;
	if (bpf->len < 1)
		return 0;

	for (i = 0; i < bpf->len; ++i) {
		p = &bpf->filter[i];
		switch (BPF_CLASS(p->code)) {
			/*
			 * Check that memory operations use valid addresses.
			 */
		case BPF_LD:
		case BPF_LDX:
			switch (BPF_MODE(p->code)) {
			case BPF_IMM:
				break;
			case BPF_ABS:
			case BPF_IND:
			case BPF_MSH:
				/*
				 * There's no maximum packet data size
				 * in userland.  The runtime packet length
				 * check suffices.
				 */
				break;
			case BPF_MEM:
				if (p->k >= BPF_MEMWORDS)
					return 0;
				break;
			case BPF_LEN:
				break;
			default:
				return 0;
			}
			break;
		case BPF_ST:
		case BPF_STX:
			if (p->k >= BPF_MEMWORDS)
				return 0;
			break;
		case BPF_ALU:
			switch (BPF_OP(p->code)) {
			case BPF_ADD:
			case BPF_SUB:
			case BPF_MUL:
			case BPF_OR:
			case BPF_AND:
			case BPF_LSH:
			case BPF_RSH:
			case BPF_NEG:
				break;
			case BPF_DIV:
				/*
				 * Check for constant division by 0.
				 */
				if (BPF_RVAL(p->code) == BPF_K && p->k == 0)
					return 0;
				break;
			default:
				return 0;
			}
			break;
		case BPF_JMP:
			/*
			 * Check that jumps are within the code block,
			 * and that unconditional branches don't go
			 * backwards as a result of an overflow.
			 * Unconditional branches have a 32-bit offset,
			 * so they could overflow; we check to make
			 * sure they don't.  Conditional branches have
			 * an 8-bit offset, and the from address is <=
			 * BPF_MAXINSNS, and we assume that BPF_MAXINSNS
			 * is sufficiently small that adding 255 to it
			 * won't overflow.
			 *
			 * We know that len is <= BPF_MAXINSNS, and we
			 * assume that BPF_MAXINSNS is < the maximum size
			 * of a u_int, so that i + 1 doesn't overflow.
			 *
			 * For userland, we don't know that the from
			 * or len are <= BPF_MAXINSNS, but we know that
			 * from <= len, and, except on a 64-bit system,
			 * it's unlikely that len, if it truly reflects
			 * the size of the program we've been handed,
			 * will be anywhere near the maximum size of
			 * a u_int.  We also don't check for backward
			 * branches, as we currently support them in
			 * userland for the protochain operation.
			 */
			from = i + 1;
			switch (BPF_OP(p->code)) {
			case BPF_JA:
				if (from + p->k >= bpf->len)
					return 0;
				break;
			case BPF_JEQ:
			case BPF_JGT:
			case BPF_JGE:
			case BPF_JSET:
				if (from + p->jt >= bpf->len ||
				    from + p->jf >= bpf->len)
					return 0;
				break;
			default:
				return 0;
			}
			break;
		case BPF_RET:
			break;
		case BPF_MISC:
			break;
		default:
			return 0;
		}
	}

	return BPF_CLASS(bpf->filter[bpf->len - 1].code) == BPF_RET;
}

uint32_t bpf_run_filter(const struct sock_fprog * fcode, uint8_t * packet,
		        size_t plen)
{
	/* XXX: caplen == len */
	uint32_t A, X;
	uint32_t k;
	struct sock_filter *bpf;
	int32_t mem[BPF_MEMWORDS];

	if (fcode == NULL || fcode->filter == NULL || fcode->len == 0)
		return 0xFFFFFFFF;

	A = 0;
	X = 0;

	bpf = fcode->filter;

	--bpf;
	while (1) {

		++bpf;

		switch (bpf->code) {
		default:
			return 0;
		case BPF_RET | BPF_K:
			return (uint32_t) bpf->k;

		case BPF_RET | BPF_A:
			return (uint32_t) A;

		case BPF_LD | BPF_W | BPF_ABS:
			k = bpf->k;
			if (k + sizeof(int32_t) > plen)
				return 0;
			A = EXTRACT_LONG(&packet[k]);
			continue;

		case BPF_LD | BPF_H | BPF_ABS:
			k = bpf->k;
			if (k + sizeof(short) > plen)
				return 0;
			A = EXTRACT_SHORT(&packet[k]);
			continue;

		case BPF_LD | BPF_B | BPF_ABS:
			k = bpf->k;
			if (k >= plen)
				return 0;
			A = packet[k];
			continue;

		case BPF_LD | BPF_W | BPF_LEN:
			A = plen;
			continue;

		case BPF_LDX | BPF_W | BPF_LEN:
			X = plen;
			continue;

		case BPF_LD | BPF_W | BPF_IND:
			k = X + bpf->k;
			if (k + sizeof(int32_t) > plen)
				return 0;
			A = EXTRACT_LONG(&packet[k]);
			continue;

		case BPF_LD | BPF_H | BPF_IND:
			k = X + bpf->k;
			if (k + sizeof(short) > plen)
				return 0;
			A = EXTRACT_SHORT(&packet[k]);
			continue;

		case BPF_LD | BPF_B | BPF_IND:
			k = X + bpf->k;
			if (k >= plen)
				return 0;
			A = packet[k];
			continue;

		case BPF_LDX | BPF_MSH | BPF_B:
			k = bpf->k;
			if (k >= plen)
				return 0;
			X = (packet[bpf->k] & 0xf) << 2;
			continue;

		case BPF_LD | BPF_IMM:
			A = bpf->k;
			continue;

		case BPF_LDX | BPF_IMM:
			X = bpf->k;
			continue;

		case BPF_LD | BPF_MEM:
			A = mem[bpf->k];
			continue;

		case BPF_LDX | BPF_MEM:
			X = mem[bpf->k];
			continue;

		case BPF_ST:
			mem[bpf->k] = A;
			continue;

		case BPF_STX:
			mem[bpf->k] = X;
			continue;

		case BPF_JMP | BPF_JA:
			bpf += bpf->k;
			continue;

		case BPF_JMP | BPF_JGT | BPF_K:
			bpf += (A > bpf->k) ? bpf->jt : bpf->jf;
			continue;

		case BPF_JMP | BPF_JGE | BPF_K:
			bpf += (A >= bpf->k) ? bpf->jt : bpf->jf;
			continue;

		case BPF_JMP | BPF_JEQ | BPF_K:
			bpf += (A == bpf->k) ? bpf->jt : bpf->jf;
			continue;

		case BPF_JMP | BPF_JSET | BPF_K:
			bpf += (A & bpf->k) ? bpf->jt : bpf->jf;
			continue;

		case BPF_JMP | BPF_JGT | BPF_X:
			bpf += (A > X) ? bpf->jt : bpf->jf;
			continue;

		case BPF_JMP | BPF_JGE | BPF_X:
			bpf += (A >= X) ? bpf->jt : bpf->jf;
			continue;

		case BPF_JMP | BPF_JEQ | BPF_X:
			bpf += (A == X) ? bpf->jt : bpf->jf;
			continue;

		case BPF_JMP | BPF_JSET | BPF_X:
			bpf += (A & X) ? bpf->jt : bpf->jf;
			continue;

		case BPF_ALU | BPF_ADD | BPF_X:
			A += X;
			continue;

		case BPF_ALU | BPF_SUB | BPF_X:
			A -= X;
			continue;

		case BPF_ALU | BPF_MUL | BPF_X:
			A *= X;
			continue;

		case BPF_ALU | BPF_DIV | BPF_X:
			if (X == 0)
				return 0;
			A /= X;
			continue;

		case BPF_ALU | BPF_AND | BPF_X:
			A &= X;
			continue;

		case BPF_ALU | BPF_OR | BPF_X:
			A |= X;
			continue;

		case BPF_ALU | BPF_LSH | BPF_X:
			A <<= X;
			continue;

		case BPF_ALU | BPF_RSH | BPF_X:
			A >>= X;
			continue;

		case BPF_ALU | BPF_ADD | BPF_K:
			A += bpf->k;
			continue;

		case BPF_ALU | BPF_SUB | BPF_K:
			A -= bpf->k;
			continue;

		case BPF_ALU | BPF_MUL | BPF_K:
			A *= bpf->k;
			continue;

		case BPF_ALU | BPF_DIV | BPF_K:
			A /= bpf->k;
			continue;

		case BPF_ALU | BPF_AND | BPF_K:
			A &= bpf->k;
			continue;

		case BPF_ALU | BPF_OR | BPF_K:
			A |= bpf->k;
			continue;

		case BPF_ALU | BPF_LSH | BPF_K:
			A <<= bpf->k;
			continue;

		case BPF_ALU | BPF_RSH | BPF_K:
			A >>= bpf->k;
			continue;

		case BPF_ALU | BPF_NEG:
			A = -A;
			continue;

		case BPF_MISC | BPF_TAX:
			X = A;
			continue;

		case BPF_MISC | BPF_TXA:
			A = X;
			continue;
		}
	}
}

void bpf_parse_rules(char *rulefile, struct sock_fprog *bpf)
{
	int ret;
	char buff[256];
	struct sock_filter sf_single = { 0x06, 0, 0, 0xFFFFFFFF };

	if (rulefile == NULL) {
		bpf->len = 1;
		bpf->filter = xmalloc(sizeof(sf_single));
		memcpy(&bpf->filter[0], &sf_single, sizeof(sf_single));
		return;
	}

	FILE *fp = fopen(rulefile, "r");
	if (!fp)
		error_and_die(EXIT_FAILURE, "Cannot read BPF rule file!\n");

	memset(buff, 0, sizeof(buff));

	while (fgets(buff, sizeof(buff), fp) != NULL) {
		buff[sizeof(buff) - 1] = 0;

		/* A comment. Skip this line */
		if (buff[0] != '{') {
			memset(buff, 0, sizeof(buff));
			continue;
		}

		memset(&sf_single, 0, sizeof(sf_single));
		ret = sscanf(buff, "{ 0x%x, %u, %u, 0x%08x },",
			     (unsigned int *) &sf_single.code,
			     (unsigned int *) &sf_single.jt,
			     (unsigned int *) &sf_single.jf,
			     (unsigned int *) &sf_single.k);
		if (ret != 4)
			/* No valid bpf opcode format or a syntax error */
			error_and_die(EXIT_FAILURE, "BPF syntax error!\n");

		bpf->len++;
		bpf->filter = xrealloc(bpf->filter, 1,
				       bpf->len * sizeof(sf_single));
		memcpy(&bpf->filter[bpf->len - 1], &sf_single,
		       sizeof(sf_single));

		memset(buff, 0, sizeof(buff));
	}

	fclose(fp);

	if (bpf_validate(bpf) == 0)
		error_and_die(EXIT_FAILURE, "This is not a valid BPF "
			      "program!\n");
}
