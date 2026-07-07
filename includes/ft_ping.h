#ifndef FT_PING_H
#define FT_PING_H

#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <math.h>
#define _GNU_SOURCE

#define DEFAULT_PAYLOAD_SIZE 56
#define HELLO "Hello, World!\n\0"
#define packed __attribute__((packed))
#define printVar(x) _Generic((x),                            \
    t_ip *: print_ip_packet((uint8_t *)#x, (t_ip *)x),       \
    t_icmp *: print_icmp_packet((uint8_t *)#x, (t_icmp *)x), \
    default: printf("%s = (unknown type)\n", #x))

typedef enum e_type
{
    UNASSIGNED,
    ECHO_REPLY,
    ECHO_REQUEST,
    ICMP,
    TCP,
    UDP,
} type;

typedef struct s_args
{
    char            *destination;
    char            ip_str[INET_ADDRSTRLEN];
    struct in_addr  addr;
    bool            verbose;
    bool            help;
    bool            numeric;        // -n
    uint8_t         ttl;         // -t / --ttl
    size_t          payload_size; // -s
    bool            dont_route;     // -r
    double          timeout_sec;  // -W
} t_args;

typedef struct s_stats
{
    int             transmitted;
    int             received;
    double          rtt_min;
    double          rtt_max;
    double          rtt_sum;
    double          rtt_sum2; // for mdev
    struct timeval  start_time;
} t_stats;

extern volatile sig_atomic_t g_stop;

typedef struct packed s_rawicmp
{
    uint8_t         type;
    uint8_t         code;
    uint16_t        checksum;
    uint8_t         data[];
} t_rawicmp;

typedef struct packed s_icmp
{
    type            kind : 3;
    uint16_t        size;
    uint8_t         *data;
} t_icmp;

typedef struct packed s_ip
{
    uint32_t        src_addr;
    uint32_t        dst_addr;
    uint16_t        id;
    uint8_t         ttl;
    type            kind : 3;
    t_icmp          *icmp_payload;
} t_ip;

typedef struct packed s_rawip
{
    uint8_t         ihl : 4;
    uint8_t         version : 4;
    uint8_t         dscp : 6;
    uint8_t         ecn : 2;
    uint16_t        total_length;
    uint16_t        id;
    uint8_t         flags : 3;
    uint16_t        flags_offset : 13;
    uint8_t         ttl;
    uint8_t         protocol;
    uint16_t        checksum;
    uint32_t        src_addr;
    uint32_t        dst_addr;
    uint8_t         options[];
} t_rawip;

typedef struct packed s_ping
{
    uint16_t        id;
    uint16_t        seq;
    uint8_t         data[];
} t_ping;

typedef struct s_ping_ctx
{
    t_ping          *ping_pkt;
    t_icmp          *icmp_pkt;
    t_ip            *ip_pkt;
} t_ping_ctx;

uint16_t checksum(const uint8_t *pkt, uint16_t size);
uint16_t indian(uint16_t);
void printhex(const void *data, size_t len);
void sniff_icmp_packets(uint32_t sockfd, size_t max_packets);

// icmp
t_icmp *make_icmp_packet(type, uint8_t *, uint16_t);
uint8_t *evaluate_icmp_packet(t_icmp *);
void print_icmp_packet(uint8_t *, t_icmp *);
void print_raw_icmp_event(const uint8_t *packet, ssize_t len);

// ip
t_ip *make_ip_packet(type, const uint8_t *, const uint8_t *, uint16_t, uint16_t *);
uint8_t *evaluate_ip_packet(t_ip *);
void print_ip_packet(uint8_t *, t_ip *);
bool addPayload(t_ip *, t_icmp *);
bool sendIp(uint32_t sockfd, t_ip *packet);

// parsing
void parse_args(int argc, char **argv, t_args *args);

// socket.c
uint32_t setup(void);
bool sendIp(uint32_t sockfd, t_ip *packet);
bool apply_socket_options(uint32_t sockfd, t_args *args);

// ping_send.c
bool init_ping_ctx(t_ping_ctx *ctx, t_args *args, uint16_t id);
void destroy_ping_ctx(t_ping_ctx *ctx);
bool send_ping(uint32_t sockfd, t_ping_ctx *ctx, uint16_t seq);

// ping_recv.c
bool recv_ping(uint32_t sockfd, uint16_t expected_id, double *rtt_ms_out,
               uint16_t *seq_out, uint8_t *ttl_out, size_t *icmp_bytes_out,
               struct timeval *deadline, bool verbose);

// ping_loop.c
void run_ping_loop(uint32_t sockfd, t_args *args);


#endif // FT_PING_H
