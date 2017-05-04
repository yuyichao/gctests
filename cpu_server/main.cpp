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

static int find_cpu(int *counts, int ncpu)
{
    int min_idx = 0;
    int min_count = INT_MAX;
    for (int i = 0;i < ncpu;i++) {
        int c = counts[i];
        if (c == 0) {
            printf("Idx %d is free\n", i);
            counts[i] = 1;
            return i;
        }
        if (c < min_count) {
            min_count = c;
            min_idx = i;
        }
    }
    printf("Idx %d: %d -> %d\n", min_idx, min_count, min_count + 1);
    counts[min_idx] = min_count + 1;
    return min_idx;
}

struct client_t {
    int fd;
    int cpu_idx;
};

static void main_loop(const char *name, const int *cpuids, int ncpu)
{
    if (ncpu <= 1) {
        fprintf(stderr, "Expect more than one CPU, got %d\n", ncpu);
        exit(1);
    }
    std::vector<int> counts(ncpu);
    for (auto &c: counts)
        c = 0;
    int listenfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listenfd == -1) {
        perror("socket");
        exit(1);
    }
    struct sockaddr_un serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, name, sizeof(serv_addr.sun_path) - 1);
    if (name[0] == '@') {
        serv_addr.sun_path[0] = 0;
    } else {
        unlink(name);
    }
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

    std::vector<client_t> clients;

    printf("Start listening\n");
    auto poll_clients = [&] () {
        for (int i = clients.size() - 1;i >= 0;i--) {
            auto &client = clients[i];
            char buff[10];
            ssize_t ret = recv(client.fd, buff, 10, 0);
            if (ret == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
                continue;
            if (ret > 0)
                continue;
            printf("Client %d on %d is disconnected\n", client.fd, cpuids[client.cpu_idx]);
            close(client.fd);
            counts[client.cpu_idx] -= 1;
            clients.erase(clients.begin() + i);
        }
    };

    auto assign_client = [&] (int fd) {
        printf("New client: %d\n", fd);
        int ret = fcntl(fd, F_SETFL, O_NONBLOCK);
        if (ret == -1) {
            perror("fcntl");
            fprintf(stderr, "Failed to set nonblock\n");
            exit(1);
        }
        poll_clients();
        int cpu_idx = find_cpu(&counts[0], ncpu);
        printf("Assigned cpu %d to client %d\n", cpuids[cpu_idx], fd);
        char buff[] = "012345678901234567890123456789\n";
        int len = snprintf(buff, sizeof(buff), "%d\n", cpuids[cpu_idx]);
        ret = send(fd, buff, len, 0);
        if (ret == -1) {
            perror("send");
            close(fd);
            counts[cpu_idx] -= 1;
            return;
        }
        clients.push_back(client_t{fd, cpu_idx});
    };

    while (1) {
        int fd = accept(listenfd, nullptr, nullptr);
        if (fd == -1) {
            perror("accept");
            exit(1);
        }
        assign_client(fd);
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
