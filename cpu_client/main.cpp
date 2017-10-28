//

#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <vector>
#include <iostream>
#include <sstream>

static int get_cpu_binding(const char *name)
{
    // Do not close this socket, this is used for keeping track of the process from the server.
    int serverfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (serverfd == -1) {
        perror("socket");
        exit(1);
    }
    struct sockaddr_un serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, name, sizeof(serv_addr.sun_path) - 1);
    if (name[0] == '@')
        serv_addr.sun_path[0] = 0;
    int ret = connect(serverfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if (ret == -1) {
        perror("connect");
        exit(1);
    }
    char buff[30];
    ssize_t len = recv(serverfd, buff, 29, 0);
    buff[len] = 0;
    char *endptr = buff;
    int val = strtol(buff, &endptr, 10);
    if (endptr == buff) {
        fprintf(stderr, "Wrong cpuid format: %s\n", buff);
        exit(1);
    }
    return val;
}

static void put_cpu_binding_env(int cpu)
{
    char buff[100];
    sprintf(buff, "GC_TEST_BIND_CPU=%d", cpu);
    putenv(buff);
}

static int cpu_client(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Wrong number of arguments: Usage: cpu_client socketpath command...\n");
        exit(1);
    }
    put_cpu_binding_env(get_cpu_binding(argv[0]));
    return execvp(argv[1], &argv[1]);
}

int main(int argc, char *argv[])
{
    return cpu_client(argc - 1, argv + 1);
}
