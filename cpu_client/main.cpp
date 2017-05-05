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

static void main_loop(const char *name)
{
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
#define ENV_PREFIX "GC_TEST_BIND_CPU="
    const size_t prefix_len = strlen(ENV_PREFIX);
    char buff[prefix_len + 30] = ENV_PREFIX;
    auto real_buf = buff + prefix_len;
    ssize_t len = recv(serverfd, real_buf, 29, 0);
    real_buf[len] = 0;
    if (real_buf[len - 1] != '\n') {
        // TODO...
        fprintf(stderr, "Wrong cpuid format: %s\n", real_buf);
        exit(1);
    }
    putenv(buff);
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Wrong number of arguments: Usage: cpu_client socketpath command...\n");
        exit(1);
    }
    char *path = argv[1];
    main_loop(path);
    return execvp(argv[2], &argv[2]);
}
