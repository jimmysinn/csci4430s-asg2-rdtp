#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include "rdtp_common.h"

#define set_header(type, seq) ((((type << 28) | (seq & (~(0xF << 28))))))
#define get_type(header) (((header) >> 28) & (0xF))
#define get_seq(header) ((header) & (~(0xF << 28)))

#define set_abstimer(timer, sec)            \
    do {                                    \
        struct timeval now;                 \
        gettimeofday(&now, NULL);           \
        timer.tv_sec = now.tv_sec + sec;    \
        timer.tv_nsec = now.tv_usec * 1000; \
    } while (timer.tv_sec == time(NULL))

#define SYN 0
#define SYN_ACK 1
#define FIN 2
#define FIN_ACK 3
#define ACK 4
#define DATA 5

#define RTO 1
#define TIME_WAIT 10

#ifndef BUILD
#define printe(fmt, arg ...) \
           fprintf(stderr, "[01;35m[%ld] [%s:%s():%3d][0m " fmt, time(NULL), __FILE__, __FUNCTION__, __LINE__, ##arg)
#else
#define printe(fmt, ...) (0)
#endif

struct rdtp_argv {
    int sockfd;
    struct sockaddr_in *addr;
};