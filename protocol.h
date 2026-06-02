#ifndef PROTOCOL_H
#define PROTOCOL_H

#define SPPBTP_VERSION "1.0"

/* All SPPBTP keywords are exactly 4 characters (RFC §3). */
enum msg_kind {
    MSG_HELO,
    MSG_NAME,
    MSG_SERV,
    MSG_BALL,
    MSG_MISS,
    MSG_DONE,
    MSG_QUIT,
    MSG_ERR,
    MSG_NONE   /* parse error or unknown */
};

struct sppbtp_msg {
    enum msg_kind kind;

    /* HELO fields */
    char  version[8];
    int   ticks_per_sec;
    int   net_height;
    char  name[41];

    /* SERV fields */
    int   n_balls;

    /* BALL fields */
    int   net_position;
    int   xttm;
    int   yttm;
    int   ydir;
    char  ppb_char; /* '\0' means use default */

    /* optional text for MISS / DONE / QUIT / ERR */
    char  message[128];
};

/* Sender helpers — each writes a CRLF-terminated line to fd. */
void send_helo(int fd, int ticks, int netheight, const char *name);
void send_name(int fd, const char *name);
void send_serv(int fd, int n_balls);
void send_ball(int fd, int net_pos, int xttm, int yttm, int ydir, int ppb_char);
void send_miss(int fd, const char *msg);
void send_done(int fd, const char *msg);
void send_quit(int fd, const char *msg);
void send_err(int fd, const char *msg);

/* Receive one CRLF-terminated line from fp and parse it into *out.
 * Returns 1 on success, 0 on EOF/error. */
int recv_msg(FILE *fp, struct sppbtp_msg *out);

#endif
