/*
 * BASIC CLIENT — connects to server, sends password,
 * then lets you type commands interactively. (WINDOWS WINSOCK VERSION)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT     9000
#define BUF_SIZE 4096

int main(int argc, char *argv[]) {
    WSADATA wsa;
    SOCKET sock;
    struct sockaddr_in server_addr;
    char buffer[BUF_SIZE];

    char *server_ip = "127.0.0.1";
    if (argc == 2) server_ip = argv[1];

    /* 0. Initialize Winsock */
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("Failed. Error Code : %d\n", WSAGetLastError());
        return 1;
    }

    /* 1. Create socket */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("socket failed: %d\n", WSAGetLastError());
        return 1;
    }

    /* 2. Connect to server */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    printf("[client] Connecting to %s:%d ...\n", server_ip, PORT);
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("connect failed: %d\n", WSAGetLastError());
        return 1;
    }
    printf("[client] Connected!\n");

    /* 3. Authentication loop */
    int auth_success = 0;
    for (int attempts = 0; attempts < 3; attempts++) {
        char username[128], password[128], combined[256];

        printf("Enter username: ");
        if(fgets(username, sizeof(username), stdin) == NULL) break;
        username[strcspn(username, "\n")] = 0; username[strcspn(username, "\r")] = 0;

        printf("Enter password: ");
        if(fgets(password, sizeof(password), stdin) == NULL) break;
        password[strcspn(password, "\n")] = 0; password[strcspn(password, "\r")] = 0;

        /* Combine them securely to send in a single TCP packet */
        snprintf(combined, sizeof(combined), "%s:%s", username, password);
        send(sock, combined, strlen(combined), 0);

        /* 4. Read server response */
        memset(buffer, 0, BUF_SIZE);
        recv(sock, buffer, BUF_SIZE - 1, 0);

        if (strcmp(buffer, "AUTH_OK") == 0) {
            auth_success = 1;
            break;
        } else {
            printf("[client] Try again! (%s)\n", buffer);
        }
    }

    if (!auth_success) {
        printf("[client] Too many failed attempts. Exiting.\n");
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    printf("[client] Authenticated! Type commands (or 'exit' to quit):\n\n");

    /* 5. Command loop */
    while (1) {
        printf("cmd> ");
        fflush(stdout);

        char command[BUF_SIZE];
        memset(command, 0, BUF_SIZE);
        if (!fgets(command, sizeof(command), stdin)) break;
        
        command[strcspn(command, "\n")] = 0;   
        command[strcspn(command, "\r")] = 0;

        if (strlen(command) == 0) continue;

        send(sock, command, strlen(command), 0);
        
        if (strcmp(command, "exit") == 0) {
            memset(buffer, 0, BUF_SIZE);
            recv(sock, buffer, BUF_SIZE - 1, 0);
            printf("%s\n", buffer);
            break;
        }

        memset(buffer, 0, BUF_SIZE);
        int bytes_received = recv(sock, buffer, BUF_SIZE - 1, 0);
        if (bytes_received > 0) {
            printf("%s\n", buffer);
        } else if (bytes_received == 0 || bytes_received == SOCKET_ERROR) {
            printf("[client] Server disconnected.\n");
            break;
        }
    }

    closesocket(sock);
    WSACleanup();
    printf("[client] Connection closed.\n");
    return 0;
}
