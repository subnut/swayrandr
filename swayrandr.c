#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <json-c/json.h>

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
        /* To avoid uninitialized variables */
        reply->size = 0;
        reply->payload = NULL;

        void *buf;
        void *ptr;
        uint32_t payload_len;
        size_t bufsize;

        bufsize = sizeof magic + sizeof (uint32_t) * 2;
        buf = malloc(bufsize);
        if (buf == NULL)
        {
                perror("Failed to allocate memory for reply");
                return;
        }

        if (recv(sockfd, buf, bufsize, MSG_PEEK) == -1)
        {
                perror("Failed to read from socket");
                return;
        }

        ptr = buf + sizeof magic;
        memmove(&payload_len, ptr, sizeof (uint32_t));
        free(buf);

        reply->size = payload_len;
        bufsize += (size_t)payload_len;

        buf = malloc(bufsize);
        if (buf == NULL)
        {
                perror("Failed to allocate memory for reply");
                return;
        }

        if (recv(sockfd, buf, bufsize, 0) == -1)    // 0 means no flags
        {
                perror("Failed to read from socket");
                return;
        }

        reply->payload = malloc(reply->size);
        if (reply->payload == NULL)
        {
                perror("Failed to allocate memory for reply");
                return;
        }

        ptr = buf + sizeof magic + sizeof (uint32_t) * 2;
        memmove(reply->payload, ptr, (size_t)reply->size);
        free(buf);
}

struct json_object *
get_outputs(void)
{
        char *envvar;
        int sockfd;
        struct sockaddr_un sockaddr;
        sockaddr.sun_family = AF_UNIX;
        size_t sizeof_sun_path = sizeof sockaddr.sun_path/sizeof sockaddr.sun_path[0];
        struct json_object *outputs;
        outputs = NULL;

        envvar = getenv("SWAYSOCK");
        if (envvar == NULL)
        {
                fputs("SWAYSOCK not set\nIs sway running?\n", stderr);
                goto exit1;
        }
        if (memcmp(envvar, "", 1) == 0)
        {
                fputs("SWAYSOCK is empty\n", stderr);
                goto exit1;
        }
        if (strlen(envvar) > sizeof_sun_path - 1)       // -1 for '\0'
        {
                fprintf(stderr, "Length of SWAYSOCK exceeds %zu\n", sizeof_sun_path - 1);
                goto exit1;
        }
        memmove(sockaddr.sun_path, envvar, sizeof_sun_path);

        if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
        {
                perror("Could not get file descriptor for socket");
                goto exit1;
        }

        if (connect(sockfd, (struct sockaddr *)&sockaddr, sizeof sockaddr) == -1)
        {
                perror("Could not connect to socket");
                goto exit2;
        }

        struct message message;
        create_message(&message, 3, NULL, 0);
        if (message.payload == NULL)
        {
                perror("Failed to allocate memory for payload");
                goto exit3;
        }

        int retcode;
        retcode = send(sockfd, message.payload, message.size, MSG_NOSIGNAL);
        free(message.payload);
        if (retcode == -1)
        {
                perror("Failed to write to socket");
                goto exit3; /* Since message.payload has already been freed */
        }

        struct reply reply;
        get_reply(&reply, sockfd);
        if (reply.payload == NULL)
                goto exit3;

        outputs = json_tokener_parse(reply.payload);
        free(reply.payload);
        if (outputs == NULL)
                fputs("Failed to parse JSON\n", stderr);

exit3:  shutdown(sockfd, SHUT_RDWR);
exit2:  close(sockfd);
exit1:  return outputs;
}

int
main(void)
{
        struct json_object *outputs;
        outputs = get_outputs();
        if (outputs == NULL)
                return EXIT_FAILURE;

        puts(json_object_to_json_string_ext(outputs, JSON_C_TO_STRING_PRETTY));
        if (json_object_put(outputs) == 1)
                outputs = NULL;

        return EXIT_SUCCESS;
}

// vim:set et ts=8 sw=8 sts=4 nowrap path+=/usr/include/* fdm=syntax:
