#include "ft_ping.h"

static void print_usage(const char *prog_name, int exit_code)
{
    printf("Usage: %s [-v] [-?] [-n] [-t ttl] [--ttl ttl] [-s size] [-r] [-W timeout] <destination>\n", prog_name);
    exit(exit_code);
}

static long parse_long_arg(int argc, char **argv, int *i, const char *name)
{
    if (*i + 1 >= argc)
    {
        fprintf(stderr, "ft_ping: option %s requires an argument\n", name);
        exit(EXIT_FAILURE);
    }
    (*i)++;
    char *endptr;
    long val = strtol(argv[*i], &endptr, 10);
    if (*endptr != '\0' || argv[*i][0] == '\0')
    {
        fprintf(stderr, "ft_ping: invalid argument for %s: %s\n", name, argv[*i]);
        exit(EXIT_FAILURE);
    }
    return val;
}

static bool resolve_destination(const char *destination, t_args *args)
{
    struct addrinfo hints;

    struct addrinfo *res;
    int status;

    bzero(&hints, sizeof(hints));
    hints.ai_family = AF_INET; // mandatory part only needs IPv4
    hints.ai_socktype = SOCK_RAW;
    hints.ai_protocol = IPPROTO_ICMP;

    status = getaddrinfo(destination, NULL, &hints, &res);
    if (status != 0)
    {
        fprintf(stderr, "ft_ping: unknown host %s: %s\n",
                destination, gai_strerror(status));
        return false;
    }

    struct sockaddr_in *sin = (struct sockaddr_in *)res->ai_addr;
    args->addr = sin->sin_addr;
    inet_ntop(AF_INET, &args->addr, args->ip_str, sizeof(args->ip_str));

    freeaddrinfo(res);
    return true;
}

static void handle_options_and_address(int argc, char **argv, t_args *args)
{
    bool got_destination = false;
    int i = 0;
    while (++i < argc)
    {
        if (strcmp(argv[i], "-v") == 0)
            args->verbose = true;
        else if (strcmp(argv[i], "-n") == 0)
            args->numeric = true;
        else if (strcmp(argv[i], "-r") == 0)
            args->dont_route = true;

        else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--ttl") == 0)
        {
            long ttl = parse_long_arg(argc, argv, &i, argv[i]);
            if (ttl < 1 || ttl > 255)
            {
                fprintf(stderr, "ft_ping: invalid ttl: %ld (must be 1-255)\n", ttl);
                exit(EXIT_FAILURE);
            }
            args->ttl = (uint8_t)ttl;
        }
        else if (strcmp(argv[i], "-s") == 0)
        {
            long size = parse_long_arg(argc, argv, &i, argv[i]);
            if (size < 0 || size > 65507)
            {
                fprintf(stderr, "ft_ping: invalid packet size: %ld\n", size);
                exit(EXIT_FAILURE);
            }
            args->payload_size = (size_t)size;
        }

        else if (strcmp(argv[i], "-W") == 0)
        {
            long secs = parse_long_arg(argc, argv, &i, argv[i]);
            if (secs < 0)
            {
                fprintf(stderr, "ft_ping: invalid timeout: %ld\n", secs);
                exit(EXIT_FAILURE);
            }
            args->timeout_sec = (double)secs;
        }
        else
        {
            if (got_destination)
            {
                fprintf(stderr, "ft_ping: only one destination allowed\n");
                exit(EXIT_FAILURE);
            }
            if (!resolve_destination(argv[i], args))
                exit(EXIT_FAILURE);
            args->destination = argv[i];
            got_destination = true;
        }
    }

    if (!got_destination && !args->help)
        print_usage(argv[0], EXIT_FAILURE);
}

void parse_args(int argc, char **argv, t_args *args)
{
    if (argc < 2)
        print_usage(argv[0], EXIT_FAILURE );

    args->ttl = 64;
    args->payload_size = DEFAULT_PAYLOAD_SIZE;
    args->timeout_sec = 1.0;
    int i = -1;

    while (++i < argc)
        if (strcmp(argv[i], "-?") == 0)
            print_usage(argv[0], EXIT_SUCCESS);

    handle_options_and_address(argc, argv, args);
}