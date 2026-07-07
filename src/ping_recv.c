#include "ft_ping.h"

static double timeval_diff_ms(struct timeval *start, struct timeval *end)
{
    return (end->tv_sec - start->tv_sec) * 1000.0 + (end->tv_usec - start->tv_usec) / 1000.0;
}


bool recv_ping(uint32_t sockfd, uint16_t expected_id, double *rtt_ms_out,
               uint16_t *seq_out, uint8_t *ttl_out, size_t *icmp_bytes_out,
               struct timeval *deadline, bool verbose)
{
    uint8_t buffer[4096];
    struct sockaddr_in peer;
    socklen_t peer_len = sizeof(peer);
    struct timeval now, remaining;
    struct iphdr *ip_hdr = NULL;
    struct icmphdr *icmp_hdr = NULL;
    uint8_t *payload = NULL;
    struct timeval sent_time;

    gettimeofday(&now, NULL);
    if (now.tv_sec > deadline->tv_sec ||
        (now.tv_sec == deadline->tv_sec && now.tv_usec >= deadline->tv_usec))
        return false;

    remaining.tv_sec = deadline->tv_sec - now.tv_sec;
    remaining.tv_usec = deadline->tv_usec - now.tv_usec;
    if (remaining.tv_usec < 0)
    {
        remaining.tv_sec -= 1;
        remaining.tv_usec += 1000000;
    }
    setsockopt((int)sockfd, SOL_SOCKET, SO_RCVTIMEO, &remaining, sizeof(remaining));

    ssize_t received = recvfrom((int)sockfd, buffer, sizeof(buffer), 0,
                                (struct sockaddr *)&peer, &peer_len);
    if (received < 0 || (size_t)received < sizeof(struct iphdr))
        return false;

    ip_hdr = (struct iphdr *)buffer;
    size_t ip_hdr_len = (size_t)ip_hdr->ihl * 4;
    if ((size_t)received < ip_hdr_len + sizeof(struct icmphdr))
        return false;

    icmp_hdr = (struct icmphdr *)(buffer + ip_hdr_len);
    if (icmp_hdr->type != ICMP_ECHOREPLY)
    {
        if (verbose && icmp_hdr->type == ICMP_TIME_EXCEEDED)
        {
            char src[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &ip_hdr->saddr, src, sizeof(src));
            fprintf(stderr, "From %s: Time to live exceeded\n", src);
        }
        return false;
    }
    if (ntohs(icmp_hdr->un.echo.id) != expected_id)
        return false;

    payload = buffer + ip_hdr_len + sizeof(struct icmphdr);
    memcpy(&sent_time, payload, sizeof(struct timeval));

    gettimeofday(&now, NULL);
    *rtt_ms_out = timeval_diff_ms(&sent_time, &now);
    *seq_out = ntohs(icmp_hdr->un.echo.sequence);
    *ttl_out = ip_hdr->ttl;
    *icmp_bytes_out = (size_t)received - ip_hdr_len;
    return true;
}