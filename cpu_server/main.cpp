//

#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <vector>
#include <iostream>
#include <sstream>

static int find_cpu(const int *cpuids, int *counts, int ncpu)
{
    int min_idx = 0;
    int min_count = INT_MAX;
    for (int i = 0;i < ncpu;i++) {
        int c = counts[i];
        if (c == 0) {
            counts[i] = 1;
            return cpuids[i];
        }
        if (c < min_count) {
            min_count = c;
            min_idx = i;
        }
    }
    counts[min_idx] = min_count + 1;
    return cpuids[min_idx];
}

static void main_loop(const char *name, const int *cpuids, int ncpu)
{
    if (ncpu <= 1) {
        fprintf(stderr, "Expect more than one CPU, got %d\n", ncpu);
        exit(1);
    }
    std::vector<int> counts(ncpu);
    int listenfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listenfd == -1) {
        perror("socket");
        exit(1);
    }
    struct sockaddr_un serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, name, sizeof(serv_addr.sun_path) - 1);
    if (name[0] == '@')
        serv_addr.sun_path[0] = 0;
    int ret = bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if (ret == -1) {
        perror("bind");
        exit(1);
    }
    ret = listen(listenfd, 10);
    if (ret == -1) {
        perror("listen");
        exit(1);
    }
}

static std::vector<int> parse_cpu_list(std::istream &cpuset)
{
    auto fail = [&] () {
        fprintf(stderr, "Unable to parse CPU list\n");
        exit(1);
    };
    std::vector<int> cpus;
    while (true) {
        int cpu1;
        cpuset >> cpu1;
        if (cpuset.fail())
            fail();
        cpus.push_back(cpu1);
        char c = cpuset.get();
        if (cpuset.eof() || c == '\n') {
            break;
        } else if (c == ',') {
            continue;
        } else if (c != '-') {
            fail();
        }
        int cpu2;
        cpuset >> cpu2;
        if (cpuset.fail())
            fail();
        for (int cpu = cpu1 + 1; cpu <= cpu2; cpu++) {
            cpus.push_back(cpu);
        }
        c = cpuset.get();
        if (cpuset.eof() || c == '\n') {
            break;
        } else if (c != ',') {
            fail();
        }
    }
    return cpus;
}

static std::vector<int> parse_cpu_list(std::istream &&cpuset)
{
    return parse_cpu_list(cpuset);
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Wrong number of arguments: Usage: cpu_server socketpath cpu-list\n");
        exit(1);
    }
    char *path = argv[1];
    auto cpus = parse_cpu_list(std::istringstream(argv[2]));
    main_loop(path, &cpus[0], cpus.size());
    return 0;
}
