#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "net.h"

int make_server_socket(int port)
{
    struct sockaddr_in addr;
    int                fd;
    int                opt = 1;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return -1;
    }

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons((unsigned short)port);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(fd);
        return -1;
    }

    if (listen(fd, 1) == -1) {
        perror("listen");
        close(fd);
        return -1;
    }

    return fd;
}

int wait_for_client(int listen_fd)
{
    struct sockaddr_in client_addr;
    socklen_t          addrlen = sizeof(client_addr);
    int                conn_fd;

    conn_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addrlen);
    close(listen_fd); /* stop accepting after one game */
    return conn_fd;
}

int connect_to_server(const char *host, int port)
{
    struct hostent    *hp;
    struct sockaddr_in addr;
    int                fd;

    hp = gethostbyname(host);
    if (hp == NULL) {
        herror("gethostbyname");
        return -1;
    }

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    memcpy(&addr.sin_addr, hp->h_addr_list[0], (size_t)hp->h_length);
    addr.sin_port = htons((unsigned short)port);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(fd);
        return -1;
    }

    return fd;
}
