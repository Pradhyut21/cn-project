/*
 * BASIC SERVER — listens for a client, checks password,
 * runs commands, sends output back. (WINDOWS WINSOCK VERSION)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <direct.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT      9000
#define USERNAME  "admin"
#define PASSWORD  "secret123"
#define BUF_SIZE  4096

int main() {
    WSADATA wsa;
    SOCKET server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUF_SIZE];
    int addrlen = sizeof(client_addr);

    /* 0. Initialize Winsock */
    printf("[server] Initializing Winsock...\n");
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("Failed. Error Code : %d\n", WSAGetLastError());
        return 1;
    }

    /* 1. Create socket */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) { 
        printf("socket failed: %d\n", WSAGetLastError()); 
        return 1; 
    }

    /* Allow reuse of port */
    char opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* 2. Bind to port */
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;   /* accept from any IP */
    server_addr.sin_port        = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("bind failed: %d\n", WSAGetLastError()); 
        return 1;
    }

    /* 3. Listen for connections */
    listen(server_fd, 5);
    printf("[server] Listening on port %d...\n", PORT);

    /* 4. LOOP forever to accept multiple clients one by one */
    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addrlen);
        if (client_fd == INVALID_SOCKET) {
            printf("accept failed: %d\n", WSAGetLastError());
            continue; /* Skip to next try instead of killing server */
        }
        printf("\n==================================\n");
        printf("[server] Client connected from %s\n", inet_ntoa(client_addr.sin_addr));

        /* 5. Authentication — receive creds from client */
        int auth_success = 0;
        for (int attempts = 0; attempts < 3; attempts++) {
            memset(buffer, 0, BUF_SIZE);
            int res = recv(client_fd, buffer, BUF_SIZE - 1, 0);
            if (res <= 0) break;
            
            buffer[strcspn(buffer, "\r")] = 0;
            buffer[strcspn(buffer, "\n")] = 0;

            char *colon = strchr(buffer, ':');
            if (colon != NULL) {
                *colon = '\0';
                char *user_attempt = buffer;
                char *pass_attempt = colon + 1;
                
                printf("[server] Login attempt %d - User: '%s'\n", attempts + 1, user_attempt);

                if (strcmp(user_attempt, USERNAME) == 0 && strcmp(pass_attempt, PASSWORD) == 0) {
                    send(client_fd, "AUTH_OK", 7, 0);
                    auth_success = 1;
                    break;
                }
            }
            send(client_fd, "WRONG_CREDS", 11, 0);
        }

        if (!auth_success) {
            printf("[server] Authentication failed! Dropping client.\n");
            closesocket(client_fd);
            continue; /* Go back to top of while(1) and wait for next client! */
        }
        printf("[server] Login successful! Session started.\n");

        /* 6. Command loop */
        while (1) {
            memset(buffer, 0, BUF_SIZE);
            int n = recv(client_fd, buffer, BUF_SIZE - 1, 0);

            if (n <= 0) {
                printf("[server] Client disconnected.\n");
                break;
            }

            /* Remove trailing newlines from command */
            buffer[strcspn(buffer, "\r")] = 0;
            buffer[strcspn(buffer, "\n")] = 0;

            printf("[server] Running command: '%s'\n", buffer);

            if (strcmp(buffer, "exit") == 0) {
                send(client_fd, "Goodbye!", 8, 0);
                printf("[server] Client said exit. Closing session.\n");
                break;
            }

            /* Handle 'cd' command to change the server's working directory */
            if (strncmp(buffer, "cd ", 3) == 0) {
                char *dir = buffer + 3;
                if (_chdir(dir) == 0) {
                    /* Print current directory to server log to confirm */
                    char cwd[256];
                    if (_getcwd(cwd, 256) != NULL) {
                        printf("[server] Directory changed to: %s\n", cwd);
                    }
                    send(client_fd, "Directory changed.\n", 19, 0);
                } else {
                    printf("[server] Failed to change directory to: %s\n", dir);
                    send(client_fd, "ERROR: could not change directory.\n", 35, 0);
                }
                continue;
            }

            /* Run the command and capture output (Windows uses _popen) */
            FILE *fp = _popen(buffer, "r");
            if (!fp) {
                send(client_fd, "ERROR: could not run command\n", 29, 0);
                continue;
            }

            char output[BUF_SIZE];
            memset(output, 0, BUF_SIZE);
            int total = 0;
            int bytes;
            while ((bytes = fread(output + total, 1, BUF_SIZE - total - 1, fp)) > 0) {
                total += bytes;
            }
            _pclose(fp);

            if (total == 0) {
                send(client_fd, "(no output)\n", 12, 0);
            } else {
                send(client_fd, output, total, 0);
            }
        }

        /* Clean up just this client, keep server alive */
        closesocket(client_fd);
        printf("[server] Session ended. Waiting for new client...\n");
    }

    closesocket(server_fd);
    WSACleanup();
    printf("[server] Shutdown.\n");
    return 0;
}
