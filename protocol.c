#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "protocol.h"

/* ------------------------------------------------------------------ */
/* Sender helpers                                                       */
/* ------------------------------------------------------------------ */

void send_helo(int fd, int ticks, int netheight, const char *name)
{
    dprintf(fd, "HELO %s %d %d %.40s\r\n",
            SPPBTP_VERSION, ticks, netheight, name);
}

void send_name(int fd, const char *name)
{
    dprintf(fd, "NAME %s %.40s\r\n", SPPBTP_VERSION, name);
}

void send_serv(int fd, int n_balls)
{
    dprintf(fd, "SERV %d\r\n", n_balls);
}

void send_ball(int fd, int net_pos, int xttm, int yttm, int ydir, int ppb_char)
{
    if (ppb_char && ppb_char != 'O')
        dprintf(fd, "BALL %d %d %d %d %c\r\n",
                net_pos, xttm, yttm, ydir, ppb_char);
    else
        dprintf(fd, "BALL %d %d %d %d\r\n",
                net_pos, xttm, yttm, ydir);
}

void send_miss(int fd, const char *msg)
{
    if (msg && *msg)
        dprintf(fd, "MISS %.120s\r\n", msg);
    else
        dprintf(fd, "MISS\r\n");
}

void send_done(int fd, const char *msg)
{
    if (msg && *msg)
        dprintf(fd, "DONE %.120s\r\n", msg);
    else
        dprintf(fd, "DONE\r\n");
}

void send_quit(int fd, const char *msg)
{
    if (msg && *msg)
        dprintf(fd, "QUIT %.120s\r\n", msg);
    else
        dprintf(fd, "QUIT\r\n");
}

void send_err(int fd, const char *msg)
{
    if (msg && *msg)
        dprintf(fd, "?ERR %.120s\r\n", msg);
    else
        dprintf(fd, "?ERR\r\n");
}

/* ------------------------------------------------------------------ */
/* Receiver                                                             */
/* ------------------------------------------------------------------ */

/* upper-case the 4-char keyword in-place */
static void upcase4(char *s)
{
    int i;
    for (i = 0; i < 4 && s[i]; i++)
        s[i] = (char)toupper((unsigned char)s[i]);
}

int recv_msg(FILE *fp, struct sppbtp_msg *out)
{
    char line[256];
    char keyword[8];
    char rest[128]; /* RFC §3: total line ≤ 128 chars including CRLF */
    int  n;

    memset(out, 0, sizeof(*out));
    out->kind = MSG_NONE;

    if (!fgets(line, sizeof(line), fp))
        return 0;

    /* strip trailing CR/LF */
    n = (int)strlen(line);
    while (n > 0 && (line[n-1] == '\r' || line[n-1] == '\n'))
        line[--n] = '\0';

    if (n == 0)
        return 1; /* empty line — caller should call again */

    /* split into keyword and optional rest */
    keyword[0] = '\0';
    rest[0]    = '\0';
    if (sscanf(line, "%7s%*[ ]%127[^\n]", keyword, rest) < 1)
        return 1;
    upcase4(keyword);

    if (strcmp(keyword, "HELO") == 0) {
        out->kind = MSG_HELO;
        sscanf(rest, "%7s %d %d %40s",
               out->version, &out->ticks_per_sec,
               &out->net_height, out->name);
    } else if (strcmp(keyword, "NAME") == 0) {
        out->kind = MSG_NAME;
        sscanf(rest, "%7s %40s", out->version, out->name);
    } else if (strcmp(keyword, "SERV") == 0) {
        out->kind = MSG_SERV;
        sscanf(rest, "%d", &out->n_balls);
    } else if (strcmp(keyword, "BALL") == 0) {
        char pchar[4] = {0};
        out->kind = MSG_BALL;
        n = sscanf(rest, "%d %d %d %d %3s",
                   &out->net_position, &out->xttm,
                   &out->yttm, &out->ydir, pchar);
        out->ppb_char = (n == 5 && pchar[0]) ? pchar[0] : '\0';
    } else if (strcmp(keyword, "MISS") == 0) {
        out->kind = MSG_MISS;
        snprintf(out->message, sizeof(out->message), "%s", rest);
    } else if (strcmp(keyword, "DONE") == 0) {
        out->kind = MSG_DONE;
        snprintf(out->message, sizeof(out->message), "%s", rest);
    } else if (strcmp(keyword, "QUIT") == 0) {
        out->kind = MSG_QUIT;
        snprintf(out->message, sizeof(out->message), "%s", rest);
    } else if (keyword[0] == '?') {
        out->kind = MSG_ERR;
        snprintf(out->message, sizeof(out->message), "%s", rest);
    } else {
        out->kind = MSG_NONE;
        snprintf(out->message, sizeof(out->message), "unknown: %s", keyword);
    }

    return 1;
}
