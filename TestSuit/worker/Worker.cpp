#include <mutex>
#include <csignal>
#include <fstream>
#include "TiRPC.hpp"

bool g_loop = true;

void SignalHandler(int signum)
{
    switch (signum) {
    case SIGINT:
        g_loop = false;
        break;
    default:
        break;
    }
}

class Worker {
public:
    void Outp(const std::string& log)
    {
        // Worker does not care about logs.
        (void)log;
    }

    void Task(const std::string& cmd)
    {
        std::lock_guard<std::mutex> locker(mutex);
        cmds.emplace_back(cmd);
    }

    void SwapCmds(std::vector<std::string>& out)
    {
        std::lock_guard<std::mutex> locker(mutex);
        out.swap(cmds);
    }

    std::string ReadFile(const std::string& file)
    {
        std::ifstream ifs(file); // text file (default)
        if (ifs.is_open()) {
            ifs.seekg(0, std::ios::end);
            std::ifstream::pos_type size = ifs.tellg();
            ifs.seekg(0, std::ios::beg);

            std::vector<char> chars(size);
            ifs.read(chars.data(), size);
            ifs.close();

            return std::string(chars.data(), size);
        }
        return "Error: Open output.log failed.";
    }

private:
    std::mutex mutex; // for cmds
    std::vector<std::string> cmds;
} g_worker;

int main(int argc, char* argv[])
{
    std::unordered_map<std::string, std::string> commands;

    commands["ip"] = "127.0.0.1";
    commands["p1"] = "6021";
    commands["p2"] = "6022";
    commands["name"] = "worker";

    for (int argn = 1; argn < argc; argn++) {
        std::string command = argv[argn];
        size_t split = command.find_first_of('=');
        std::string key = command.substr(0, split);
        std::string value = command.substr(split + 1);
        commands[key] = value;
    }

    std::cout << "========== This is Worker! ==========" << std::endl;
    std::cout << "Configuration information:"            << std::endl;
    for (const auto& config : commands) {
        std::cout << "- " << config.first
                  << ": " << config.second
                  << std::endl;
    }
    std::cout << "------------------------------------" << std::endl;

    signal(SIGINT, SignalHandler);

    tirpc::RpcAsyncBroadcast rpc;
    rpc.BindFunc("Outp", &Worker::Outp, g_worker);
    rpc.BindFunc("Task", &Worker::Task, g_worker);
    if (!rpc.Start(tirpc::RpcAsyncBroadcast::Role::Client, commands["ip"],
        atoi(commands["p1"].c_str()), atoi(commands["p2"].c_str()))) {
        std::cout << "Error: RPC start failed." << std::endl;
        return 1;
    }

    std::vector<std::string> cmds;
    while (g_loop) {
        g_worker.SwapCmds(cmds);
        if (cmds.size() == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 100Hz
        }

        for (const auto& cmd : cmds) {
            if (!g_loop) {
                break;
            }

            std::string command = cmd + " >output.log 2>&1";
            int status = system(command.c_str());
            // If the return value is not -1, it only means that the command line
            // successfully triggered and executed, and it does not mean that the
            // command line is executed successfully or the return value of the
            // command line execution.
            // Due to the differences in the implementation between the Windows
            // and the Linux, we send the result by using the output.log.
            std::stringstream log;
            log << "COMMAND: `" << command          << "`, "  // command
                << "WORKER: `"  << commands["name"] << "`, "  // worker name
                << "STATUS: "   << status << "." << std::endl // system retv
                << "OUTPUT: "   << std::endl                  // output.log
                << g_worker.ReadFile("output.log");
            rpc.CallFunc("Outp", log.str());
            // Also output the result to the terminal.
            std::cout << log.str() << std::endl;
        }
        cmds.clear();
    }

    rpc.CallFunc("Outp", "Worker `" + commands["name"] + "` exit!");

    if (!rpc.Stop()) {
        std::cout << "Error: RPC stop failed." << std::endl;
    }

    std::cout << "========== Worker closed! ==========" << std::endl;
    #if (defined WIN32) && (defined _DEBUG)
    system("pause");
    #endif
    return 0;
}
