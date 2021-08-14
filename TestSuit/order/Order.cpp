#include <mutex>
#include <csignal>
#include "TiRPC.hpp"

bool g_wait = true;

void SignalHandler(int signum)
{
    switch (signum) {
    case SIGINT:
        g_wait = false;
        break;
    default:
        break;
    }
}

int main(int argc, char* argv[])
{
    std::map<std::string, std::string> configs;
    configs["ip"] = "127.0.0.1";
    configs["p1"] = "6021";
    configs["p2"] = "6022";
    configs["workers"] = "1";  // default worker count: 1
    configs["timeout"] = "60"; // default timeout: 60s

    std::string command;
    for (int argn = 1; argn < argc; argn++) {
        std::string config = argv[argn];
        if (config == "command") {
            // The part after `command` is the command line content.
            for (int rest = argn + 1; rest < argc; rest++) {
                command += std::string(argv[rest]) + " ";
            }
            command.pop_back(); // Pop up the last space.
            break;
        } else {
            size_t split = config.find_first_of('=');
            std::string key = config.substr(0, split);
            std::string value = config.substr(split + 1);
            configs[key] = value;
        }
    }

    signal(SIGINT, SignalHandler);

    return 0;
}
