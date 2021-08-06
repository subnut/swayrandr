#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

/*
 * stdio.h      -   fprintf()
 * stdlib.h     -   getenv()
 * string.h     -   memcmp()
 * sys/types.h  -   u_int32_t
 * sys/socket.h -   socket(), connect(), etc
 * sys/un.h     -   sockaddr_un
 */

int main(void)
{
        if (getenv("SWAYSOCK") == NULL)
                return fputs("SWAYSOCK not set. Is sway running?\n", stderr),
                       EXIT_FAILURE;

        if (memcmp(getenv("SWAYSOCK"), "", 1) == 0)
                return fputs("SWAYSOCK is empty\n", stderr),
                       EXIT_FAILURE;

        int sockfd;
        struct sockaddr_un sockaddr;
        sockaddr.sun_family = AF_UNIX;

        const char MAGIC[] = {'i','3','-','i','p','c'};
        u_int32_t PAYLOAD_LEN = 0;
        u_int32_t PAYLOAD_TYPE = 1;

        void *buf;
        size_t len = sizeof MAGIC + sizeof PAYLOAD_LEN + sizeof PAYLOAD_TYPE;
        if ((buf = malloc(len)) == NULL)
                return perror("Could not allocate memory for message"),
                       EXIT_FAILURE;
        void *ptr;
        ptr = buf;
        memmove(ptr, MAGIC, sizeof MAGIC);
        ptr += sizeof MAGIC;

        memmove(ptr, &PAYLOAD_LEN, sizeof PAYLOAD_LEN);
        ptr += sizeof PAYLOAD_LEN;
        memmove(ptr, &PAYLOAD_TYPE, sizeof PAYLOAD_TYPE);
        ptr += sizeof PAYLOAD_TYPE;

        if (strlen(getenv("SWAYSOCK")) > (sizeof sockaddr.sun_path /
                                sizeof sockaddr.sun_path[0]) - 1)       // -1 for '\0'
        {
                fprintf(stderr, "Length of SWAYSOCK exceeds %zu\n",
                                sizeof sockaddr.sun_path / sizeof sockaddr.sun_path[0] - 1);
                return EXIT_FAILURE;
        }

        memmove(sockaddr.sun_path, getenv("SWAYSOCK"),
                        sizeof sockaddr.sun_path / sizeof sockaddr.sun_path[0]);

        if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
                return perror("Could not create socket"),
                       EXIT_FAILURE;

        if (connect(sockfd, (struct sockaddr *)&sockaddr, sizeof sockaddr) == -1)
                return perror("Could not connect to socket"),
                       EXIT_FAILURE;

        if (send(sockfd, buf, len, MSG_NOSIGNAL) == -1)
                return perror("Failed to write to socket"),
                       EXIT_FAILURE;

}

// vim:set et ts=8 sw=8 sts=4 nowrap path+=/usr/include/* fdm=syntax:
