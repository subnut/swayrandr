#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

/*
 * stdint.h     -   uint32_t
 * stdio.h      -   fprintf()
 * stdlib.h     -   getenv()
 * string.h     -   memcmp()
 * unistd.h     -   close()
 * sys/socket.h -   socket(), connect(), etc
 * sys/un.h     -   sockaddr_un
 */

static const char magic[] = {'i', '3', '-', 'i', 'p', 'c'};

struct message {
        size_t size;
        void *payload;
};

struct reply {
        uint32_t size;
        char *payload;
};

static void
create_message(struct message *message, uint32_t type, void *payload, size_t size)
{
        uint32_t len = (uint32_t)size;

        message->size = 0;
        message->payload = malloc(sizeof magic + (sizeof type * 2));
        if (message->payload == NULL)
                return;

        void *ptr;
        ptr = message->payload;

        memmove(ptr, &magic, sizeof magic);
        ptr += sizeof magic;
        message->size += sizeof magic;

        memmove(ptr, &len, sizeof len);
        ptr += sizeof len;
        message->size += sizeof len;

        memmove(ptr, &type, sizeof type);
        ptr += sizeof type;
        message->size += sizeof type;

        if (size != 0)
        memmove(ptr, payload, size);
        message->size += size;
}

static void
get_reply(struct reply *reply, int sockfd)
{
        void *buf;
        void *ptr;
        uint32_t payload_len;
        size_t bufsize;

        bufsize = sizeof magic + sizeof (uint32_t) * 2;
        buf = malloc(bufsize);
        if (buf == NULL)
                return perror("Failed to allocate memory for reply");

        if (recv(sockfd, buf, bufsize, MSG_PEEK) == -1)
                return perror("Failed to read from socket");

        ptr = buf + sizeof magic;
        memmove(&payload_len, ptr, sizeof (uint32_t));
        free(buf);

        reply->size = payload_len;
        bufsize += (size_t)payload_len;

        buf = malloc(bufsize);
        if (buf == NULL)
                return perror("Failed to allocate memory for reply");

        if (recv(sockfd, buf, bufsize, 0) == -1)    // 0 means no flags
                return perror("Failed to read from socket");

        reply->payload = malloc(reply->size);
        if (reply->payload == NULL)
                return perror("Failed to allocate memory for reply");

        ptr = buf + sizeof magic + sizeof (uint32_t) * 2;
        memmove(reply->payload, ptr, (size_t)reply->size);
        free(buf);
}

/* TODO: Don't just return immediately. close(), free(), etc and _then_ return */
int main(void)
{
        char *envvar;
        int sockfd;
        struct sockaddr_un sockaddr;
        sockaddr.sun_family = AF_UNIX;
        size_t sizeof_sun_path = sizeof sockaddr.sun_path/sizeof sockaddr.sun_path[0];

        envvar = getenv("SWAYSOCK");
        if (envvar == NULL)
                return fputs("SWAYSOCK not set. Is sway running?\n", stderr),
                       EXIT_FAILURE;
        if (memcmp(envvar, "", 1) == 0)
                return fputs("SWAYSOCK is empty\n", stderr),
                       EXIT_FAILURE;
        if (strlen(envvar) > sizeof_sun_path - 1)       // -1 for '\0'
                return fprintf(stderr, "Length of SWAYSOCK exceeds %zu\n", sizeof_sun_path - 1),
                       EXIT_FAILURE;
        memmove(sockaddr.sun_path, envvar, sizeof_sun_path);

        if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
                return perror("Could not create socket"),
                       EXIT_FAILURE;

        if (connect(sockfd, (struct sockaddr *)&sockaddr, sizeof sockaddr) == -1)
                return perror("Could not connect to socket"),
                       EXIT_FAILURE;

        struct message message;
        create_message(&message, 3, NULL, 0);
        if (message.payload == NULL)
                return perror("Failed to allocate memory for payload"),
                       EXIT_FAILURE;

        int retcode;
        retcode = send(sockfd, message.payload, message.size, MSG_NOSIGNAL);
        free(message.payload);
        if (retcode == -1)
                return perror("Failed to write to socket"),
                       EXIT_FAILURE;

        struct reply reply;
        get_reply(&reply, sockfd);
        if (reply.payload == NULL)
                return EXIT_FAILURE;

        for (uint32_t size = 0; size <= reply.size; size++)
                putchar(reply.payload[size]);

        return EXIT_SUCCESS;
}

// vim:set et ts=8 sw=8 sts=4 nowrap path+=/usr/include/* fdm=syntax:
