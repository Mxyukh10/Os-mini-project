#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define FIFO_PATH "/tmp/os_fifo"
#define BUFFER_SIZE 128

static void print_usage(const char *program_name) {
    printf("Usage:\n");
    printf("  %s start <name>\n", program_name);
    printf("  %s stop <name>\n", program_name);
    printf("  %s list\n", program_name);
}

int main(int argc, char *argv[]) {
    int fifo_fd;
    char command[BUFFER_SIZE];

    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "start") == 0) {
        if (argc != 3) {
            print_usage(argv[0]);
            return 1;
        }
        snprintf(command, sizeof(command), "START %s", argv[2]);
    } else if (strcmp(argv[1], "stop") == 0) {
        if (argc != 3) {
            print_usage(argv[0]);
            return 1;
        }
        snprintf(command, sizeof(command), "STOP %s", argv[2]);
    } else if (strcmp(argv[1], "list") == 0) {
        if (argc != 2) {
            print_usage(argv[0]);
            return 1;
        }
        snprintf(command, sizeof(command), "LIST");
    } else {
        print_usage(argv[0]);
        return 1;
    }

    fifo_fd = open(FIFO_PATH, O_WRONLY);
    if (fifo_fd == -1) {
        perror("open");
        printf("Make sure the supervisor is running.\n");
        return 1;
    }

    if (write(fifo_fd, command, strlen(command)) == -1) {
        perror("write");
        close(fifo_fd);
        return 1;
    }

    close(fifo_fd);
    return 0;
}
