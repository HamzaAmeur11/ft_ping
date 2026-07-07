#include "ft_ping.h"

uint16_t indian(uint16_t x)
{
    return (uint16_t)(((x & 0x00ff) << 8) | ((x & 0xff00) >> 8));
}

uint16_t checksum(const uint8_t *pkt, uint16_t size)
{
    uint32_t acc = 0;
    const uint16_t *p = (const uint16_t *)pkt;

    while (size > 1) {
        acc += *p++;
        size -= 2;
    }
    if (size > 0)
        acc += *(const uint8_t *)p;

    while (acc >> 16)
        acc = (acc & 0xffff) + (acc >> 16);

    return (uint16_t)(~acc);
}

t_ip *make_ip_packet(type kind, const uint8_t *src_addr, const uint8_t *dst_addr,
                      uint16_t id, uint16_t *counter)
{
    if (!src_addr || !dst_addr || (kind != ICMP && kind != TCP && kind != UDP))
        return NULL;

    t_ip *packet = malloc(sizeof(t_ip));
    if (!packet)
        return NULL;

    bzero((uint8_t *)packet, sizeof(t_ip));
    packet->kind = kind;
    packet->id = id ? id : *counter;
    packet->src_addr = inet_addr((const char *)src_addr);
    packet->dst_addr = inet_addr((const char *)dst_addr);
    if (packet->dst_addr == INADDR_NONE) {
        free(packet);
        return NULL;
    }

    packet->icmp_payload = NULL;
    return packet;
}

t_icmp *make_icmp_packet(type kind, uint8_t *data, uint16_t size)
{
    if (!size || !data)
        return NULL;

    t_icmp *packet = malloc(sizeof(t_icmp));
    if (!packet)
        return NULL;

    bzero((uint8_t *)packet, sizeof(t_icmp));
    packet->kind = kind;
    packet->size = size;
    packet->data = data;
    return packet;
}

uint8_t *evaluate_icmp_packet(t_icmp *packet)
{
    if (!packet || !packet->data)
        return NULL;

    uint16_t size = (uint16_t)(sizeof(t_rawicmp) + packet->size);
    if (size % 2)
        size++;

    uint8_t *ret = malloc(size);
    if (!ret)
        return NULL;

    bzero(ret, size);

    t_rawicmp raw_packet;
    bzero((uint8_t *)&raw_packet, sizeof(raw_packet));

    switch (packet->kind) {
        case ECHO_REQUEST:
            raw_packet.type = 8;
            raw_packet.code = 0;
            break;
        case ECHO_REPLY:
            raw_packet.type = 0;
            raw_packet.code = 0;
            break;
        default:
            free(ret);
            return NULL;
    }

    raw_packet.checksum = 0;
    memcpy(ret, (uint8_t *)&raw_packet, sizeof(t_rawicmp));
    memcpy(ret + sizeof(t_rawicmp), packet->data, packet->size);
    ((t_rawicmp *)ret)->checksum = checksum(ret, size);
    return ret;
}

uint8_t *evaluate_ip_packet(t_ip *packet)
{
    if (!packet)
        return NULL;

    uint8_t protocol = 0;
    switch (packet->kind) {
        case ICMP: protocol = 1; break;
        case TCP:  protocol = 6; break;
        case UDP:  protocol = 17; break;
        default:   return NULL;
    }

    uint16_t total_len = sizeof(t_rawip);
    if (packet->icmp_payload)
        total_len = (uint16_t)(sizeof(t_rawip) + sizeof(t_rawicmp) + packet->icmp_payload->size);

    if (total_len % 2)
        total_len++;

    uint8_t *ret = malloc(total_len);
    if (!ret)
        return NULL;

    bzero(ret, total_len);

    t_rawip raw_packet;
    bzero((uint8_t *)&raw_packet, sizeof(raw_packet));
    raw_packet.version = 4;
    raw_packet.ihl = (uint8_t)(sizeof(t_rawip) / 4);
    raw_packet.dscp = 0;
    raw_packet.ecn = 0;
    raw_packet.total_length = indian(total_len);
    raw_packet.id = indian(packet->id);
    raw_packet.flags = 0;
    raw_packet.flags_offset = 0;
    raw_packet.ttl = packet->ttl;
    raw_packet.protocol = protocol;
    raw_packet.checksum = 0;
    raw_packet.src_addr = packet->src_addr;
    raw_packet.dst_addr = packet->dst_addr;

    memcpy(ret, (uint8_t *)&raw_packet, sizeof(t_rawip));
    if (packet->icmp_payload) {
        uint8_t *icmp_raw = evaluate_icmp_packet(packet->icmp_payload);
        if (!icmp_raw) {
            free(ret);
            return NULL;
        }
        memcpy(ret + sizeof(t_rawip), icmp_raw, sizeof(t_rawicmp) + packet->icmp_payload->size);
        free(icmp_raw);
    }

    ((t_rawip *)ret)->checksum = checksum(ret, total_len);
    return ret;
}