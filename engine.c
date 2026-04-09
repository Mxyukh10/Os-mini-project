#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <sys/wait.h>

#define SOCKET_PATH "/tmp/engine_socket"
#define MAX_CONTAINERS 10

typedef struct {
    char id[50];
    pid_t pid;
    int active;
} container_t;

container_t containers[MAX_CONTAINERS];

// SIGCHLD handler
void handle_sigchld(int sig) {
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < MAX_CONTAINERS; i++) {
            if (containers[i].pid == pid) {
                containers[i].active = 0;
                printf("[EXIT] %s (PID %d)\n", containers[i].id, pid);
            }
        }
    }
}

// start command
void start_container(char *id) {
    pid_t pid = fork();

    if (pid == 0) {
        printf("[RUNNING] %s (PID %d)\n", id, getpid());
        while (1) sleep(1);
    } else {
        for (int i = 0; i < MAX_CONTAINERS; i++) {
            if (!containers[i].active) {
                strcpy(containers[i].id, id);
                containers[i].pid = pid;
                containers[i].active = 1;
                break;
            }
        }
        printf("[START] %s (PID %d)\n", id, pid);
    }
}

// ps command
void list_containers() {
    printf("=== RUNNING CONTAINERS ===\n");
    int found = 0;

    for (int i = 0; i < MAX_CONTAINERS; i++) {
        if (containers[i].active) {
            printf("ID: %s | PID: %d\n", containers[i].id, containers[i].pid);
            found = 1;
        }
    }

    if (!found) {
        printf("No active containers\n");
    }
}

// stop command
void stop_container(char *id) {
    for (int i = 0; i < MAX_CONTAINERS; i++) {
        if (containers[i].active && strcmp(containers[i].id, id) == 0) {
            kill(containers[i].pid, SIGTERM);
            printf("[STOP] %s\n", id);
            return;
        }
    }
    printf("Container '%s' not found\n", id);
}

void run_supervisor() {
    int server_fd, client_fd;
    struct sockaddr_un addr;
    char buffer[256];

    signal(SIGCHLD, handle_sigchld);

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    unlink(SOCKET_PATH);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);

    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 5);

    printf("Supervisor ready...\n");

    while (1) {
        memset(buffer, 0, sizeof(buffer));

        client_fd = accept(server_fd, NULL, NULL);
        read(client_fd, buffer, sizeof(buffer));

        char *cmd = strtok(buffer, " ");
        char *arg = strtok(NULL, " ");

        if (strcmp(cmd, "start") == 0 && arg) {
            start_container(arg);
        } else if (strcmp(cmd, "ps") == 0) {
            list_containers();
        } else if (strcmp(cmd, "stop") == 0 && arg) {
            stop_container(arg);
        } else {
            printf("Invalid command. Use: start <id> | ps | stop <id>\n");
        }

        close(client_fd);
    }
}

void send_command(char *cmd) {
    int sock;
    struct sockaddr_un addr;

    sock = socket(AF_UNIX, SOCK_STREAM, 0);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Connect failed (Is supervisor running?)");
        exit(1);
    }

    write(sock, cmd, strlen(cmd));
    close(sock);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage:\n");
        printf("  ./engine supervisor\n");
        printf("  ./engine start <id>\n");
        printf("  ./engine ps\n");
        printf("  ./engine stop <id>\n");
        return 1;
    }

    if (strcmp(argv[1], "supervisor") == 0) {
        run_supervisor();
    } else {
        char command[100] = "";

        for (int i = 1; i < argc; i++) {
            strcat(command, argv[i]);
            strcat(command, " ");
        }

        send_command(command);
    }

    return 0;
}