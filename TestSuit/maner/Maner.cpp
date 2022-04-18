#include <mutex>
#include <csignal>
#include "TiRPC.hpp"

bool g_loop = true;

void SignalHandler(int signum)
{
    switch (signum) {
    case SIGINT:
        {
            static std::stringstream eof;
            std::cin.rdbuf(eof.rdbuf());
            // '\26' is equivalent to Ctrl+Z,
            // and is equivalent to input a EOF.
            eof << std::endl << '\26' << std::endl;
        }
        g_loop = false;
        break;
    default:
        break;
    }
}

class Maner {
public:
    void Outp(const std::string& log)
    {
        std::lock_guard<std::mutex> locker(mutex);
        logs.emplace_back(log);
    }

    void Task(const std::string& cmd, const std::string& group, const std::string& env)
    {
        // Maner's Task does not do anything, Maner
        // only distributes the order to the Worker.
        (void)cmd;
    }

    void SwapLogs(std::vector<std::string>& out)
    {
        std::lock_guard<std::mutex> locker(mutex);
        out.swap(logs);
    }

private:
    std::mutex mutex; // for logs
    std::vector<std::string> logs;
} g_maner;

int main(int argc, char* argv[])
{
    std::map<std::string, std::string> commands;

    commands["ip"] = "127.0.0.1";   //Maner's IP
    commands["p1"] = "6021";        //RPC port
    commands["p2"] = "6022";        //Multicast port

    for (int argn = 1; argn < argc; argn++) {
        std::string command = argv[argn];
        size_t split = command.find_first_of('=');
        std::string key = command.substr(0, split);
        std::string value = command.substr(split + 1);
        commands[key] = value;
    }

    std::cout << "========== This is Maner! ==========" << std::endl;
    std::cout << "Configuration information:"           << std::endl;
    for (const auto& config : commands) {
        std::cout << "- " << config.first
                  << ": " << config.second
                  << std::endl;
    }
    std::cout << "------------------------------------" << std::endl;

    signal(SIGINT, SignalHandler);

    tirpc::RpcAsyncBroadcast rpc;
    rpc.BindFunc("Outp", &Maner::Outp, g_maner);
    rpc.BindFunc("Task", &Maner::Task, g_maner);
    if (!rpc.Start(tirpc::RpcAsyncBroadcast::Role::Server, commands["ip"],
        atoi(commands["p1"].c_str()), atoi(commands["p2"].c_str()))) {
        std::cout << "Error: Async broadcast rpc start failed." << std::endl;
        return 1;
    }

    std::thread showLogs = std::thread([]() {
        std::vector<std::string> logs;
        while (g_loop) {
            g_maner.SwapLogs(logs);
            if (logs.size() > 0) {
                for (const auto& log : logs) {
                    std::stringstream ss;
                    ss << "===== [RECEIVE RESULT] =====" << std::endl
                       <<               log              << std::endl
                       << "----------------------------" << std::endl
                       << std::endl; // Add one more split line.
                    std::cout << ss.str();
                }
                logs.clear();
            } else {
                std::this_thread::sleep_for( // 100Hz
                    std::chrono::milliseconds(10));
            }
        }
    });

    while (g_loop) {
        std::string command;
        getline(std::cin, command);

        if (command.length() > 0) {
            if (command[0] == ':') {
                bool success = true;
                command = command.substr(1);
                if (rpc.CallFunc("Task", command, std::string(":"), std::string("")) !=
                    tirpc::rpc::RpcCallError::Success) {
                    success = false;
                }
                std::stringstream ss;
                ss << "Distribute order `" << command  << "` : "
                   << (success ? "success" : "failed") << std::endl;
                std::cout << ss.str();
            }
        }
    }

    if (!rpc.Stop()) {
        std::cout << "Error: Async broadcast rpc stop failed." << std::endl;
    }

    if (showLogs.joinable()) {
        showLogs.join();
    }

    std::cout << "========== Maner closed! ==========" << std::endl;
    #if (defined WIN32) && (defined _DEBUG)
    system("pause");
    #endif
    return 0;
}
