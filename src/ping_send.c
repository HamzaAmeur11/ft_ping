#include "ft_ping.h"

bool init_ping_ctx(t_ping_ctx *ctx, t_args *args, uint16_t id)
{
    size_t payload_size = args->payload_size;

    ctx->ping_pkt = malloc(sizeof(t_ping) + payload_size);
    if (!ctx->ping_pkt)
        return false;

    ctx->ping_pkt->id = indian(id);
    // fill padding once — timestamp bytes get overwritten each send, rest stays constant
    bzero(ctx->ping_pkt->data, payload_size);
    if (payload_size > sizeof(struct timeval))
            memset(ctx->ping_pkt->data + sizeof(struct timeval), 0xA5,
                   payload_size - sizeof(struct timeval));

    ctx->icmp_pkt = make_icmp_packet(ECHO_REQUEST, (uint8_t *)ctx->ping_pkt,
                                     (uint16_t)(sizeof(t_ping) + payload_size));
    if (!ctx->icmp_pkt)
    {
        free(ctx->ping_pkt);
        return false;
    }

    ctx->ip_pkt = malloc(sizeof(t_ip));
    if (!ctx->ip_pkt)
    {
        free(ctx->icmp_pkt);
        free(ctx->ping_pkt);
        return false;
    }
    bzero(ctx->ip_pkt, sizeof(t_ip));
    ctx->ip_pkt->kind = ICMP;
    ctx->ip_pkt->dst_addr = args->addr.s_addr;
    ctx->ip_pkt->src_addr = 0;
    ctx->ip_pkt->ttl = args->ttl;
    ctx->ip_pkt->icmp_payload = ctx->icmp_pkt;

    return true;
}

void destroy_ping_ctx(t_ping_ctx *ctx)
{
    free(ctx->ip_pkt);
    free(ctx->icmp_pkt);
    free(ctx->ping_pkt);
}

bool send_ping(uint32_t sockfd, t_ping_ctx *ctx, uint16_t seq)
{
    ctx->ping_pkt->seq = indian(seq);
    ctx->ip_pkt->id = (uint16_t)rand();

    // capture the timestamp as late as possible, right before the packet leaves
    struct timeval now;
    gettimeofday(&now, NULL);
    memcpy(ctx->ping_pkt->data, &now, sizeof(struct timeval));

    return sendIp(sockfd, ctx->ip_pkt);
}