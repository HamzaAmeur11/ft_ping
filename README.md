# ft_ping

A from-scratch reimplementation of `ping`, built on raw ICMP sockets, in C.
Reference behavior is modeled on `inetutils-2.0`'s `ping`.

## Building

Requires a Debian-family Linux with kernel > 3.14 and root privileges
(raw sockets need `CAP_NET_RAW`).

```bash
make        # build
make re     # clean rebuild
make clean  # remove object files
make fclean # remove object files + binary
```

A `Vagrantfile` is included for a matching, disposable test environment:

```bash
vagrant up
vagrant ssh
cd ft_ping
make
```

## Usage

```
sudo ./ft_ping [options] <destination>
```

`<destination>` may be an IPv4 address or a hostname (resolved once via
`getaddrinfo`; no DNS resolution is performed on returned packets).

### Options

| Flag | Description |
|---|---|
| `-?` | Print usage and exit |
| `-v` | Verbose — report packet-level problems (e.g. TTL exceeded) instead of failing silently |
| `-n` | Numeric output only (no reverse DNS — this is already the default behavior) |
| `-t <ttl>` / `--ttl <ttl>` | Set the outgoing IP TTL (1–255) |
| `-s <size>` | Set ICMP payload size in bytes (default: 56) |
| `-r` | Bypass routing table, send only to directly-connected hosts (`SO_DONTROUTE`) |
| `-W <seconds>` | Per-packet reply timeout (default: 1s); never exceeds normal send cadence |

The program runs until interrupted with `Ctrl+C`, at which point it prints
a summary (packets transmitted/received, loss %, RTT min/avg/max/mdev).

### Examples

```bash
sudo ./ft_ping google.com
sudo ./ft_ping -v -t 1 8.8.8.8      # forces a TTL-exceeded reply
sudo ./ft_ping -s 100 127.0.0.1
sudo ./ft_ping -W 2 192.0.2.1        # reserved, non-responsive test address
```

## Project layout

```
ft_ping.h      shared types, macros, prototypes
main.c         entry point
parsing.c      argument parsing, destination resolution (getaddrinfo)
packet.c       checksum + IP/ICMP packet construction (wire format encoding)
socket.c       raw socket setup, socket options, sendto wrapper
ping_send.c    packet lifecycle: build once, mutate + send per iteration
ping_recv.c    receive + match incoming ICMP replies
ping_loop.c    main send/receive loop, pacing, stats, SIGINT handling
debug.c        auxiliary printers, not part of the required output path
Makefile
Vagrantfile
```

## Design notes

- **Packet reuse, not per-packet allocation.** The IP/ICMP/ping structures are
  built once at startup and mutated in place each iteration (sequence number,
  embedded timestamp) — send-time allocator overhead was measured and shown
  to correlate with RTT outliers; this removed it.
- **Absolute send schedule.** Sends are scheduled against a fixed
  `next_send` timestamp advanced by exactly 1s each iteration, rather than
  measuring "how long did this iteration take" — this prevents drift
  accumulating after a lost/delayed reply.
- **`-W` is bounded by the send schedule.** A configured timeout longer than
  the standard ~1s cadence cannot stretch the interval between sends; it can
  only shorten how long an individual wait can run.
- **Verbose mode surfaces, never crashes.** TTL-exceeded and other non-echo
  ICMP responses are reported under `-v` rather than silently dropped, and
  send/receive failures never terminate the loop.

## Testing performed

- Cross-checked byte-for-byte output format (`PING ... bytes of data.`,
  reply line, statistics block) against real `ping` via manual diff.
- Verified packet contents at the wire level with `tcpdump -X` (payload
  pattern, TTL, checksum correctness via successful replies).
- `valgrind --leak-check=full` — clean: 0 leaks, 0 errors.
- Verified loopback (0% loss, sub-millisecond RTT) and real-network runs.
- Verified `-t 1` genuinely triggers ICMP Time Exceeded from the first hop.
- Verified `-W` no longer distorts send cadence (previously an over-long
  timeout could stretch every iteration; fixed by scheduling against the
  next absolute send time rather than the raw timeout value).

## Known limitations

- IPv4 only (no `-6` / IPv6 support — not required by the subject).
- Reverse DNS on the reply source is not implemented (explicitly not
  required per the subject and the grading scale).
- Bonus flags implemented: `-n`, `-t`/`--ttl`, `-s`, `-r`, `-W` (5 total,
  matching the project's 5-bonus cap).
