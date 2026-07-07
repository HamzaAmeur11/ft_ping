#include "ft_ping.h"

bool apply_socket_options(uint32_t sockfd, t_args *args)
{
    if (args->dont_route) {
        int optval = 1;
        if (setsockopt((int)sockfd, SOL_SOCKET, SO_DONTROUTE, &optval, sizeof(optval)) < 0) {
            perror("setsockopt(SO_DONTROUTE)");
            return false;
        }
    }
    return true;
}

uint32_t setup(void)
{
    uint32_t sockfd = (uint32_t)socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if ((int)sockfd < 0)
        return 0;

    uint32_t optval = 1;
    if (setsockopt((int)sockfd, IPPROTO_IP, IP_HDRINCL, &optval, sizeof(optval)) < 0) {
        perror("setsockopt");
        close((int)sockfd);
        return 0;
    }
    return sockfd;
}

bool sendIp(uint32_t sockfd, t_ip *packet)
{
    if (!packet || sockfd <= 2)
        return false;

    uint8_t *raw = evaluate_ip_packet(packet);
    if (!raw)
        return false;

    struct sockaddr_in sock;
    bzero((uint8_t *)&sock, sizeof(sock));
    sock.sin_family = AF_INET;
    sock.sin_addr.s_addr = packet->dst_addr;
    sock.sin_port = 0;

    if (sock.sin_addr.s_addr == INADDR_NONE ||
        sock.sin_addr.s_addr == INADDR_ANY ||
        sock.sin_addr.s_addr == INADDR_BROADCAST) {
        free(raw);
        return false;
    }

    uint16_t len = (uint16_t)(sizeof(t_rawip) + sizeof(t_rawicmp) + packet->icmp_payload->size);
    if (len % 2)
        len++;

    ssize_t sent_bytes = sendto((int)sockfd, raw, (size_t)len, 0,
                                 (const struct sockaddr *)&sock, sizeof(sock));
    if (sent_bytes < 0)
        perror("sendto");

    free(raw);
    return (sent_bytes > 0);
}