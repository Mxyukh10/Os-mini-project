#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define FIFO_PATH "/tmp/os_fifo"
#define MAX_CONTAINERS 100
#define MAX_NAME_LEN 64
#define BUFFER_SIZE 256

typedef struct {
    char name[MAX_NAME_LEN];
    pid_t pid;
    int active;
} Container;

static Container containers[MAX_CONTAINERS];

static int find_container_index(const char *name) {
    int i;

    for (i = 0; i < MAX_CONTAINERS; i++) {
        if (containers[i].active && strcmp(containers[i].name, name) == 0) {
            return i;
        }
    }

    return -1;
}

static int find_free_slot(void) {
    int i;

    for (i = 0; i < MAX_CONTAINERS; i++) {
        if (!containers[i].active) {
            return i;
        }
    }

    return -1;
}

static void remove_container_by_pid(pid_t pid) {
    int i;

    for (i = 0; i < MAX_CONTAINERS; i++) {
        if (containers[i].active && containers[i].pid == pid) {
            printf("Supervisor: container '%s' with PID %d exited\n",
                   containers[i].name, containers[i].pid);
            containers[i].active = 0;
            containers[i].name[0] = '\0';
            containers[i].pid = 0;
            return;
        }
    }
}

static void reap_children(void) {
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        remove_container_by_pid(pid);
    }
}

static void sigchld_handler(int sig) {
    (void)sig;
}

static void run_container(const char *name) {
    printf("Container '%s' started with PID %d\n", name, getpid());
    fflush(stdout);

    while (1) {
        pause();
    }
}

static void start_container(const char *name) {
    int index;
    pid_t pid;

    if (name == NULL || name[0] == '\0') {
        printf("Supervisor: invalid container name\n");
        return;
    }

    if (find_container_index(name) != -1) {
        printf("Supervisor: container '%s' already exists\n", name);
        return;
    }

    index = find_free_slot();
    if (index == -1) {
        printf("Supervisor: container limit reached\n");
        return;
    }

    pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        run_container(name);
        exit(0);
    }

    strncpy(containers[index].name, name, MAX_NAME_LEN - 1);
    containers[index].name[MAX_NAME_LEN - 1] = '\0';
    containers[index].pid = pid;
    containers[index].active = 1;

    printf("Supervisor: started container '%s' with PID %d\n", name, pid);
}

static void stop_container(const char *name) {
    int index = find_container_index(name);

    if (index == -1) {
        printf("Supervisor: container '%s' not found\n", name);
        return;
    }

    if (kill(containers[index].pid, SIGTERM) == -1) {
        perror("kill");
        return;
    }

    printf("Supervisor: stop signal sent to '%s' (PID %d)\n",
           containers[index].name, containers[index].pid);
}

static void list_containers(void) {
    int i;
    int found = 0;

    printf("Active containers:\n");
    for (i = 0; i < MAX_CONTAINERS; i++) {
        if (containers[i].active) {
            printf("Name: %s, PID: %d\n", containers[i].name, containers[i].pid);
            found = 1;
        }
    }

    if (!found) {
        printf("No active containers.\n");
    }
}

static void process_command(char *command) {
    char action[BUFFER_SIZE];
    char name[MAX_NAME_LEN];
    int fields;

    if (command == NULL) {
        return;
    }

    reap_children();

    fields = sscanf(command, "%255s %63s", action, name);

    if (fields <= 0) {
        return;
    }

    if (strcmp(action, "START") == 0) {
        if (fields < 2) {
            printf("Supervisor: START requires a container name\n");
            return;
        }
        start_container(name);
    } else if (strcmp(action, "STOP") == 0) {
        if (fields < 2) {
            printf("Supervisor: STOP requires a container name\n");
            return;
        }
        stop_container(name);
    } else if (strcmp(action, "LIST") == 0) {
        list_containers();
    } else {
        printf("Supervisor: unknown command '%s'\n", action);
    }

    fflush(stdout);
}

int main(void) {
    int fifo_fd;
    int dummy_fd;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    struct sigaction sa;

    memset(containers, 0, sizeof(containers));

    if (mkfifo(FIFO_PATH, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo");
        return 1;
    }

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    printf("Supervisor started. Listening on %s\n", FIFO_PATH);
    fflush(stdout);

    fifo_fd = open(FIFO_PATH, O_RDONLY);
    if (fifo_fd == -1) {
        perror("open");
        return 1;
    }

    dummy_fd = open(FIFO_PATH, O_WRONLY);
    if (dummy_fd == -1) {
        perror("open");
        close(fifo_fd);
        return 1;
    }

    while (1) {
        reap_children();

        bytes_read = read(fifo_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            process_command(buffer);
        } else if (bytes_read == 0) {
            close(fifo_fd);
            fifo_fd = open(FIFO_PATH, O_RDONLY);
            if (fifo_fd == -1) {
                perror("open");
                break;
            }
        } else if (errno != EINTR) {
            perror("read");
            break;
        }
    }

    close(dummy_fd);
    close(fifo_fd);
    unlink(FIFO_PATH);
    return 0;
}
