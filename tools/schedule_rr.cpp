//

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <vector>

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "Too few arguments for schedule_rr\n");
        return -1;
    }
    auto path = argv[1];
    argv += 2;
    argc -= 2;
    auto under_rr = getenv("RUNNING_UNDER_RR");
    if (under_rr && *under_rr)
        return execvp(argv[0], argv);
    std::vector<char*> new_argv{(char*)"cpu_client", path, (char*)"rr_wrapper"};
    for (int i = 0; i < argc; i++)
        new_argv.push_back(argv[i]);
    new_argv.push_back(nullptr);
    return execvp(new_argv[0], &new_argv[0]);
}
