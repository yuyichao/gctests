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
#include <sys/stat.h>

#include <vector>
#include <iostream>
#include <sstream>

// Generic helper functions
static char *check_env(const char *name)
{
    if (auto val = getenv(name))
        return *val ? val : nullptr;
    return nullptr;
}

__attribute__((noreturn)) static void checked_execvp(char *const *argv)
{
    execvp(argv[0], argv);
    perror("execvp");
    exit(1);
}

static void mkdir_recursive(const char *dir)
{
    std::string buff(dir);
    while (buff.back() == '/')
        buff.pop_back();
    for (auto &c: buff) {
        if (c == '/') {
            c = 0;
            mkdir(buff.c_str(), S_IRWXU);
            c = '/';
        }
    }
    mkdir(buff.c_str(), S_IRWXU);
}

static bool check_command(const char *arg, const char *cmd)
{
    auto larg = strlen(arg);
    auto lcmd = strlen(cmd);
    if (larg == lcmd)
        return memcmp(arg, cmd, lcmd) == 0;
    if (larg < lcmd)
        return false;
    return arg[larg - lcmd - 1] == '/' && memcmp(cmd, &arg[larg - lcmd], lcmd)  == 0;
}

template<typename T>
static void append_tail_args(T &args, int argc, char **argv)
{
    for (int i = 0; i < argc; i++)
        args.push_back(argv[i]);
    args.push_back(nullptr);
}

// RR/GC test specific helper functions
static void exec_if_rr(char *const *argv)
{
    if (check_env("RUNNING_UNDER_RR")) {
        checked_execvp(argv);
    }
}

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

template<typename T>
static void add_cpu_binding_rr_arg(T &args, const char *v)
{
    if (v && *v) {
        args.push_back((char*)"--bind-to-cpu");
        args.push_back((char*)v);
    }
}

template<typename T>
static void add_cpu_binding_rr_arg(T &args, int num)
{
    char *str;
    if (asprintf(&str, "%d", num) < 0) {
        fprintf(stderr, "asprintf failed\n");
        exit(1);
    }
    add_cpu_binding_rr_arg(args, str);
}

template<typename T>
static void add_chaos_rr_arg(T &args)
{
    if (check_env("GC_TEST_CHAOS")) {
        args.push_back((char*)"-h");
    }
}

static void ensure_rr_dir(void)
{
    if (auto rr_dir = check_env("_RR_TRACE_DIR")) {
        mkdir_recursive(rr_dir);
    }
}

template<typename F>
__attribute__((noreturn)) static void run_rr(int argc, char **argv, F &&cpu_binding)
{
    exec_if_rr(argv);
    std::vector<char*> new_argv{(char*)"rr", (char*)"-S", (char*)"record",
            (char*)"--ignore-nested"};
    add_chaos_rr_arg(new_argv);
    add_cpu_binding_rr_arg(new_argv, cpu_binding());
    append_tail_args(new_argv, argc, argv);
    ensure_rr_dir();
    checked_execvp(&new_argv[0]);
}

// Main entries
__attribute__((noreturn)) static void cpu_client(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Wrong number of arguments: Usage: cpu_client socketpath command...\n");
        exit(1);
    }
    exec_if_rr(&argv[1]);
    put_cpu_binding_env(get_cpu_binding(argv[0]));
    checked_execvp(&argv[1]);
}

__attribute__((noreturn)) static void schedule_rr(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Too few arguments for schedule_rr\n");
        exit(1);
    }
    auto path = argv[0];
    argv += 1;
    argc -= 1;
    run_rr(argc, argv, [=] { return get_cpu_binding(path); });
}

__attribute__((noreturn)) static void rr_wrapper(int argc, char **argv)
{
    if (argc < 1) {
        fprintf(stderr, "Too few arguments for rr_wrapper\n");
        exit(1);
    }
    run_rr(argc, argv, [] { return getenv("GC_TEST_BIND_CPU"); });
}

int main(int argc, char *argv[])
{
    auto self = argv[0];
    argc--;
    argv++;

    // Skip all arguments starting with `=` and use them to set envs
    // Treat a single `=` as the separator between envs and other arguments.
    while (argc > 0) {
        auto arg = *argv;
        if (*arg != '=')
            break;
        argc--;
        argv++;
        if (arg[1]) {
            putenv(&arg[1]);
        }
        else {
            break;
        }
    }

    if (check_command(self, "schedule_rr")) {
        schedule_rr(argc, argv);
    }
    else if (check_command(self, "rr_wrapper")) {
        rr_wrapper(argc, argv);
    }
    else {
        cpu_client(argc, argv);
    }
}
