#include "ft_ping.h"

volatile sig_atomic_t g_stop = 0;

static void handle_sigint(int sig)
{
    (void)sig;
    g_stop = 1;
}

static double timeval_diff_ms(struct timeval *start, struct timeval *end)
{
    return (end->tv_sec - start->tv_sec) * 1000.0 + (end->tv_usec - start->tv_usec) / 1000.0;
}

static void print_startup(t_args *args)
{
    size_t total_bytes = args->payload_size + sizeof(struct icmphdr) + sizeof(struct iphdr);
    printf("PING %s (%s) %zu(%zu) bytes of data.\n",
           args->destination, args->ip_str, args->payload_size, total_bytes);
}

static void print_stats(t_args *args, t_stats *stats)
{
    struct timeval end;
    gettimeofday(&end, NULL);
    double elapsed_ms = timeval_diff_ms(&stats->start_time, &end);

    printf("\n--- %s ping statistics ---\n", args->destination);

    int loss_pct = stats->transmitted > 0
                       ? (int)(100.0 * (stats->transmitted - stats->received) / stats->transmitted)
                       : 0;

    printf("%d packets transmitted, %d received, %d%% packet loss, time %.0fms\n",
           stats->transmitted, stats->received, loss_pct, elapsed_ms);

    if (stats->received > 0)
    {
        double avg = stats->rtt_sum / stats->received;
        double variance = (stats->rtt_sum2 / stats->received) - (avg * avg);
        double mdev = variance > 0 ? sqrt(variance) : 0;
        printf("rtt min/avg/max/mdev = %.3f/%.3f/%.3f/%.3f ms\n",
               stats->rtt_min, avg, stats->rtt_max, mdev);
    }
}

static void wait_for_reply(uint32_t sockfd, uint16_t id, t_stats *stats,
                           t_args *args, struct timeval *next_send)
{
    struct timeval now, timeout_deadline, deadline;
    gettimeofday(&now, NULL);

    long whole_secs = (long)args->timeout_sec;
    long frac_usec = (long)((args->timeout_sec - whole_secs) * 1000000);
    timeout_deadline = now;
    timeout_deadline.tv_sec += whole_secs;
    timeout_deadline.tv_usec += frac_usec;
    if (timeout_deadline.tv_usec >= 1000000)
    {
        timeout_deadline.tv_sec += 1;
        timeout_deadline.tv_usec -= 1000000;
    }

    // never wait past the next scheduled send — take the earlier of the two
    bool timeout_is_earlier =
        timeout_deadline.tv_sec < next_send->tv_sec ||
        (timeout_deadline.tv_sec == next_send->tv_sec &&
         timeout_deadline.tv_usec < next_send->tv_usec);
    deadline = timeout_is_earlier ? timeout_deadline : *next_send;

    double rtt_ms;
    uint16_t reply_seq;
    uint8_t ttl;
    size_t icmp_bytes;

    while (1)
    {
        gettimeofday(&now, NULL);
        if (now.tv_sec > deadline.tv_sec ||
            (now.tv_sec == deadline.tv_sec && now.tv_usec >= deadline.tv_usec))
            break;
        if (g_stop)
            break;

        if (recv_ping(sockfd, id, &rtt_ms, &reply_seq, &ttl, &icmp_bytes,
                      &deadline, args->verbose))
        {
            stats->received++;
            if (stats->received == 1 || rtt_ms < stats->rtt_min)
                stats->rtt_min = rtt_ms;
            if (rtt_ms > stats->rtt_max)
                stats->rtt_max = rtt_ms;
            stats->rtt_sum += rtt_ms;
            stats->rtt_sum2 += rtt_ms * rtt_ms;

            printf("%zu bytes from %s: icmp_seq=%u ttl=%u time=%.3f ms\n",
                   icmp_bytes, args->ip_str, reply_seq, ttl, rtt_ms);
            break;
        }
    }
}

void run_ping_loop(uint32_t sockfd, t_args *args)
{
    signal(SIGINT, handle_sigint);
    srand((unsigned)getpid());

    uint16_t id = (uint16_t)(getpid() & 0xFFFF);
    uint16_t seq = 0;

    t_ping_ctx ctx;
    if (!init_ping_ctx(&ctx, args, id))
        return (void)fprintf(stderr, "ft_ping: failed to allocate packet context\n");

    t_stats stats = {0};
    gettimeofday(&stats.start_time, NULL);

    print_startup(args);

    struct timeval next_send;
    gettimeofday(&next_send, NULL);

    while (!g_stop)
    {
        seq++;

        if (send_ping(sockfd, &ctx, seq))
            stats.transmitted++;
        else if (args->verbose)
            fprintf(stderr, "ft_ping: failed to send packet seq=%u\n", seq);

        next_send.tv_sec += 1; // advance schedule BEFORE waiting, so wait_for_reply can see the real next tick
        wait_for_reply(sockfd, id, &stats, args, &next_send);

        struct timeval now;
        gettimeofday(&now, NULL);
        double sleep_ms = timeval_diff_ms(&now, &next_send);
        if (sleep_ms > 0 && !g_stop)
            usleep((useconds_t)(sleep_ms * 1000));
    }

    print_stats(args, &stats);
    destroy_ping_ctx(&ctx);
}

