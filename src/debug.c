#include "ft_ping.h"

void print_ip_packet(uint8_t *identifier, t_ip *ip_packet)
{
    if (!ip_packet)
        return;

    char src[INET_ADDRSTRLEN];
    char dst[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ip_packet->src_addr, src, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &ip_packet->dst_addr, dst, INET_ADDRSTRLEN);

    if (identifier)
        printf("------ (IP *) %s = {\n", identifier);
    printf("\tIP Packet:\n");
    printf("\tKind: %s\t", ip_packet->kind == ICMP ? "ICMP" : (ip_packet->kind == TCP ? "TCP" : "UDP"));
    printf("\tID: %d\t", ip_packet->id);
    printf("\tSource: %s\t", src);
    printf("\tDestination: %s\n", dst);
    if (ip_packet->icmp_payload)
        printVar(ip_packet->icmp_payload);
    printf("}\n");
}

void print_icmp_packet(uint8_t *identifier, t_icmp *icmp_packet)
{
    if (!icmp_packet)
        return;

    if (identifier)
        printf("------ (ICMP *) %s = {\n", identifier);
    printf("\tICMP Packet:\n");
    printf("\tKind: %s\t", icmp_packet->kind == ECHO_REQUEST ? "ECHO_REQUEST" : "ECHO_REPLY");
    printf("\tSize: %d\n", icmp_packet->size);
    printf("\tData: ");
    printhex(icmp_packet->data, icmp_packet->size);
    printf("}\n");
}

void printhex(const void *data, size_t len)
{
    const unsigned char *bytes = data;
    for (size_t i = 0; i < len; i++)
        printf("%02x ", bytes[i]);
    printf("\n");
}

void print_raw_icmp_event(const uint8_t *packet, ssize_t len)
{
    if (!packet || len < (ssize_t)sizeof(struct iphdr))
        return;

    const struct iphdr *ip_header = (const struct iphdr *)packet;
    size_t ip_header_len = (size_t)ip_header->ihl * 4;
    if (len < (ssize_t)(ip_header_len + sizeof(struct icmphdr)))
        return;
    if (ip_header->protocol != IPPROTO_ICMP)
        return;

    const struct icmphdr *icmp_header = (const struct icmphdr *)(packet + ip_header_len);
    char src[INET_ADDRSTRLEN];
    char dst[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ip_header->saddr, src, sizeof(src));
    inet_ntop(AF_INET, &ip_header->daddr, dst, sizeof(dst));

    uint16_t total_len = ntohs(ip_header->tot_len);
    uint16_t icmp_len = (total_len > ip_header_len) ? (uint16_t)(total_len - ip_header_len) : 0;

    if (icmp_header->type == ICMP_ECHO)
        printf("IP %s > %s: ICMP echo request, id %u, seq %u, length %u\n",
               src, dst, ntohs(icmp_header->un.echo.id), ntohs(icmp_header->un.echo.sequence), icmp_len);
    else if (icmp_header->type == ICMP_ECHOREPLY)
        printf("IP %s > %s: ICMP echo reply, id %u, seq %u, length %u\n",
               src, dst, ntohs(icmp_header->un.echo.id), ntohs(icmp_header->un.echo.sequence), icmp_len);
    else
        printf("IP %s > %s: ICMP type %u, code %u, length %u\n",
               src, dst, icmp_header->type, icmp_header->code, icmp_len);
}

void sniff_icmp_packets(uint32_t sockfd, size_t max_packets)
{
    if (sockfd <= 2 || max_packets == 0)
        return;

    struct timeval timeout = { .tv_sec = 2, .tv_usec = 0 };
    if (setsockopt((int)sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
        perror("setsockopt");

    uint8_t buffer[4096];
    struct sockaddr_in peer;
    socklen_t peer_len = sizeof(peer);

    for (size_t i = 0; i < max_packets; i++) {
        ssize_t received = recvfrom((int)sockfd, buffer, sizeof(buffer), 0,
                                     (struct sockaddr *)&peer, &peer_len);
        if (received < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            perror("recvfrom");
            break;
        }
        print_raw_icmp_event(buffer, received);
    }
}