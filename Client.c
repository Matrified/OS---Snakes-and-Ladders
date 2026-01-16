#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 5555

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in serv = {0};
    serv.sin_family = AF_INET;
    serv.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv.sin_addr);

    connect(sock, (struct sockaddr *)&serv, sizeof(serv));

    char buffer[256];

    while (1) {
        int n = read(sock, buffer, sizeof(buffer) - 1);
        if (n <= 0) break;

        buffer[n] = '\0';
        printf("%s", buffer);

        if (strstr(buffer, "Press ENTER")) {
            getchar();
            write(sock, "\n", 1);
        }
    }

    close(sock);
    return 0;
}

