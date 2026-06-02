#ifndef NET_H
#define NET_H

/* Returns a listening socket fd bound to port, or -1 on error. */
int make_server_socket(int port);

/* Blocks until one client connects; returns the connected fd, or -1 on error.
 * Closes the listening socket before returning. */
int wait_for_client(int listen_fd);

/* Returns a connected socket fd to host:port, or -1 on error. */
int connect_to_server(const char *host, int port);

#endif
