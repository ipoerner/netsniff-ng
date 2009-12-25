/* 
 * netsniff-ng
 *
 * High performance network sniffer for packet inspection
 *
 * Copyright (C) 2009, 2010  Daniel Borkmann <danborkmann@googlemail.com>
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
 *
 * Note: Your kernel has to be compiled with CONFIG_PACKET_MMAP=y option in 
 *       order to use this.
 */

/*
 * Contains: 
 *    Some miscellaneous stuff
 */

#ifndef _NET_MISC_H_
#define _NET_MISC_H_

#include <netsniff-ng/system.h>

/* Function signatures */
/* XXX: All stuff is inlined ... since it is not _always_inline we let gcc 
        decide whether to inline or not */

static inline int timespec_subtract(struct timespec *result, 
        struct timespec *after, struct timespec *before);
static inline void help(void);
static inline void version(void);
static inline void header(void);

/**
 * timespec_subtract - Subtracts two timespecs
 * @result:           result
 * @after:            second timespec
 * @before:           first timespec
 */
static inline int timespec_subtract(struct timespec *result, 
        struct timespec *after, struct timespec *before)
{
        result->tv_nsec = after->tv_nsec - before->tv_nsec;
    
        if(result->tv_nsec < 0)
        {
                /* Borrow 1sec from 'tv_sec' if subtraction -ve */
                result->tv_nsec += 1000000000;
                result->tv_sec   = after->tv_sec - before->tv_sec - 1;
                return (1);
        }
        else
        {
                result->tv_sec = after->tv_sec - before->tv_sec;
                return (0);
        }
}

/**
 * help - Prints help
 */
static inline void help(void)
{
        printf("%s %s, <danborkmann@googlemail.com>\n\n", PROGNAME_STRING, VERSION_STRING);
        printf("%s is a high performance network sniffer for packet\n", PROGNAME_STRING);
        printf("inspection that acts as a raw socket sniffer with kernelspace\n");
        printf("bpf and zero copy mode (rx ring).\n");
        printf("\n");
        printf("Options, required:\n");
        printf("    -d <arg>    use device <arg> for capturing packets\n");
        printf("    -f <arg>    use file <arg> as bpf filter\n");
        printf("\n");
        printf("Options for sys deamon:\n");
        printf("    -D          run as sys daemon\n");
        printf("    -P <arg>    use file <arg> as pidfile, req if -D\n");
        printf("    -L <arg>    use file <arg> as logfile, req if -D\n");
        printf("    -S <arg>    use file <arg> as uds inode, req if -D\n");
        printf("\n");
        printf("Options for CPU affinity:\n");
        printf("    -b <arg>    bind process to specific CPU/CPU-range\n");
        printf("    -B <arg>    forbid process to use specific CPU/CPU-range\n");
        printf("\n");
        printf("Options for packet printing:\n");
        printf("    -c          print captured packets\n");
        printf("    -cc         print captured packets, more information\n");
        printf("    -ccc        print captured packets, most information\n");
        printf("\n");
        printf("Options, misc:\n");
        printf("    -v          prints out version\n");
        printf("    -h          prints out this help\n");
        printf("\n");
        printf("Info:\n");
        printf("    - Sending a SIGUSR1 will show current packet statistics\n");
        printf("    - Sending a SIGUSR2 will toggle silent and packet printing mode\n");
        printf("    - Rule creation can be done with \'tcpdump -dd <rule>\',\n");
        printf("      see examples, or, of course manually by hand\n");
        printf("    - To access the running sys daemon you can use ipc via AF_UNIX\n");
        printf("    - For more help type \'man netsniff-ng\'\n");
        printf("\n");
        printf("Please report bugs to <danborkmann@googlemail.com>\n");
        printf("Copyright (C) 2009, 2010 Daniel Borkmann\n");
        printf("License: GNU GPL version 2\n");
        printf("This is free software: you are free to change and redistribute it.\n");
        printf("There is NO WARRANTY, to the extent permitted by law.\n");
    
        exit(EXIT_SUCCESS);
}

/**
 * version - Prints version
 */
static inline void version(void)
{
        printf("%s %s, <danborkmann@googlemail.com>\n\n", PROGNAME_STRING, VERSION_STRING);
        printf("%s is a high performance network sniffer for packet\n", PROGNAME_STRING);
        printf("inspection that acts as a raw socket sniffer with kernelspace\n");
        printf("bpf and zero copy mode (rx ring).\n");
        printf("\n");
        printf("Please report bugs to <danborkmann@googlemail.com>\n");
        printf("Copyright (C) 2009, 2010 Daniel Borkmann\n");
        printf("License: GNU GPL version 2\n");
        printf("This is free software: you are free to change and redistribute it.\n");
        printf("There is NO WARRANTY, to the extent permitted by law.\n");
    
        exit(EXIT_SUCCESS);
}

/**
 * header - Prints program startup header
 */
static inline void header(void)
{
        int ret;
        size_t len;
        char *cpu_string;

        struct sched_param sp;

        len = sysconf(_SC_NPROCESSORS_CONF) + 1;

        cpu_string = malloc(len);
        if(!cpu_string)
        {
                perr("no mem left\n");
                exit(EXIT_FAILURE);
        }

        ret = sched_getparam(getpid(), &sp);
        if(ret)
        {
                perr("Cannot determine sched prio\n");
                exit(EXIT_FAILURE);
        }

        dbg("%s %s -- pid: %d\n", PROGNAME_STRING, VERSION_STRING, 
                (int) getpid());

        dbg("nice: %d, scheduler: %d prio %d\n", 
                getpriority(PRIO_PROCESS, getpid()), 
                sched_getscheduler(getpid()), 
                sp.sched_priority);

        dbg("%ld of %ld CPUs online, affinity bitstring: %s\n", 
                sysconf(_SC_NPROCESSORS_ONLN), 
                sysconf(_SC_NPROCESSORS_CONF), 
                get_cpu_affinity(cpu_string, len));
 
        free(cpu_string);
}

#endif /* _NET_MISC_H_ */