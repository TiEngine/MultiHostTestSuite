#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <mutex>
#include <csignal>
#include <fstream>
#include "TiRPC.hpp"

bool g_loop = true;
using Group_Cmd = std::pair<std::string, std::string>;

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

    void Task(const std::string& group, const std::string& cmd)
    {
        std::lock_guard<std::mutex> locker(mutex);
        cmds.emplace_back(Group_Cmd(group, cmd));
    }

    void SwapCmds(std::vector<Group_Cmd>& out)
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
    std::vector<Group_Cmd> cmds;
} g_worker;

int main(int argc, char* argv[])
{
    std::map<std::string, std::string> commands;

    commands["ip"] = "127.0.0.1";   //Maner's IP
    commands["p1"] = "6021";
    commands["p2"] = "6022";
    commands["name"] = "worker";    //This worker's name
    commands["group"] = " ";        //This worker's group

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

    std::vector<Group_Cmd> cmds;
    while (g_loop) {
        g_worker.SwapCmds(cmds);
        if (cmds.size() == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 100Hz
        }

        for (const auto& cmd : cmds) {
            if (!g_loop) {
                break;
            }
            std::string group = cmd.first;
            std::string command = cmd.second;
            std::string cmd2 = cmd.second;
            if(group != commands["group"] && group != ":")
            {
                continue;
            }
   
            auto start = std::chrono::system_clock::now();
            std::time_t startTime = std::chrono::system_clock::to_time_t(start);

            system("pwd");
            // std::string command = cmd2 + " >" + commands["name"] + "_output.log 2>&1";
            // int status = system(command.c_str());
            int status = 0;

            std::vector<std::string> cmdParam;
            std::istringstream strCommand(command);
            std::string out;
            while (strCommand >> out) {
                cmdParam.push_back(out);
            }

            const int paramSize = cmdParam.size() + 1;
            std::cout << "paramSize = " << paramSize << std::endl;

            char** param = new char* [paramSize];

            for (int paramInd = 0; paramInd < cmdParam.size(); paramInd++)
            {
                param[paramInd] = const_cast<char*>(cmdParam[paramInd].c_str());
                std::cout<<"param[" << paramInd << "] = " << param[paramInd] <<std::endl;
            }
            param[paramSize - 1] = NULL;
            
            std::string logPath = std::string(commands["name"] + "_output.log");

            if(fork() == 0){
                system(std::string("rm " + logPath).c_str());
                int fd = open(logPath.c_str(), O_RDWR | O_CREAT, 0666);
                std::cout << "fd = " << fd << std::endl;
                dup2(fd, 1);
                dup2(fd, 2);
                
                execvp(param[0], param);
                perror("execvp");
                exit(0);
            }

            int child_status;
            wait(&child_status);
            int retCode = WIFEXITED(child_status);
            if(retCode){
                status =  WEXITSTATUS(child_status);
            }
            else{
                // coredump
                status = -1;
            }
            delete[] param;


            // If the return value is not -1, it only means that the command line
            // successfully triggered and executed, and it does not mean that the
            // command line is executed successfully or the return value of the
            // command line execution.
            // Due to the differences in the implementation between the Windows
            // and the Linux, we send the result by using the output.log.
            std::stringstream log;
            log << "COMMAND: `"     << command          << "`, "  // command
                << "WORKER: `"      << commands["name"] << "`, "  // worker name
                << "STATUS: "       << status << "." << std::endl // system retv
                << "START TIME: "   << std::ctime(&startTime) 
                << "OUTPUT: "       << std::endl                  // output.log
                << g_worker.ReadFile(commands["name"] + "_output.log");
            if (rpc.CallFunc("Outp", log.str()) !=
                tirpc::rpc::RpcCallError::Success) {
                std::cout<<"CallFunc Outp failed!" << std::endl;
            }
            // Also output the result to the terminal.
            std::cout << log.str() << std::endl;
        }
        cmds.clear();
    }

    //rpc.CallFunc("Outp", "Worker `" + commands["name"] + "` exit!");

    if (!rpc.Stop()) {
        std::cout << "Error: RPC stop failed." << std::endl;
    }

    std::cout << "========== Worker closed! ==========" << std::endl;
    #if (defined WIN32) && (defined _DEBUG)
    system("pause");
    #endif
    return 0;
}
