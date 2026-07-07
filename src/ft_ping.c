
#include "ft_ping.h"

int main(int argc, char **argv)
{
    t_args args = {0};
    parse_args(argc, argv, &args);

    uint32_t sockfd = setup();
    if (sockfd <= 2)
        return fprintf(stderr, "ft_ping: setup failed. Are you root?\n") ,1;

    if (!apply_socket_options(sockfd, &args))
        return close((int)sockfd), 1;
    
    run_ping_loop(sockfd, &args);

    close((int)sockfd);
    return 0;
}