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
// using Group_Cmd = std::pair<std::string, std::string>;
struct Command {
    std::string command;
    std::string group;
    std::string env;
    int delay;
    int timeout;
};

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

    void Task(const std::string& cmd, const std::string& group, const std::string& env, int delay, int timeout)
    {
        std::lock_guard<std::mutex> locker(mutex);
        Command command = { cmd, group, env, delay, timeout };
        cmds.emplace_back(command);
    }

    void SwapCmds(std::vector<Command>& out)
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
    std::vector<Command> cmds;
} g_worker;

int main(int argc, char* argv[])
{
    std::map<std::string, std::string> commands;

    commands["ip"] = "127.0.0.1";   //Maner's IP
    commands["p1"] = "6021";
    commands["p2"] = "6022";
    commands["name"] = "worker";    //This worker's name
    commands["group"] = " ";        //This worker's group
    commands["mode"] = "pipe";      //This worker's work mode, pipe or file

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


    tirpc::RpcAsyncBroadcast rpc;
    rpc.BindFunc("Outp", &Worker::Outp, g_worker);
    rpc.BindFunc("Task", &Worker::Task, g_worker);
    if (!rpc.Start(tirpc::RpcAsyncBroadcast::Role::Client, commands["ip"],
        atoi(commands["p1"].c_str()), atoi(commands["p2"].c_str()))) {
        std::cout << "Error: RPC start failed." << std::endl;
        return 1;
    }

    int fdPipe[2];
    if(pipe(fdPipe) < 0) {
        std::cout << "pipe() failed!" << std::endl;
        return 1;
    }

    int getProp = fcntl(fdPipe[0], F_GETFL);
    fcntl(fdPipe[0], F_SETFL, getProp | O_NONBLOCK);

    char readBuf[1024] = {0};

    std::vector<Command> cmds;
    while (g_loop) {
        g_worker.SwapCmds(cmds);
        if (cmds.size() == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 100Hz
        }

        for (const auto& cmd : cmds) {
            if (!g_loop) {
                break;
            }

            std::string command = cmd.command;
            std::string group = cmd.group;
            std::string env = cmd.env;
            int delayTime = cmd.delay;
            int timeout = cmd.timeout;
            if (delayTime < 0) {
                std::cout << "delay(" << delayTime << ") is less than 0, correct it to 0ms." << std::endl;
                delayTime = 0;
            }
            if (timeout < 0) {
                std::cout << "timeout(" << timeout << ") is less than 0, correct it to 300s." << std::endl;
                timeout = 300;
            }
            if(group != commands["group"] && group != ":")
            {
                continue;
            }
   
            auto start = std::chrono::system_clock::now();
            std::time_t startTime = std::chrono::system_clock::to_time_t(start);

            system("pwd");
            // std::string cmd2 = cmd.second;
            // std::string command = cmd2 + " >" + commands["name"] + "_output.log 2>&1";
            // int status = system(command.c_str());
            int status = 0;

            std::vector<std::string> cmdParam;
            std::vector<std::string> cmdEnv;
            std::istringstream strCommand(command);
            std::istringstream strCommandEnv(env);
            std::string out;
            while (strCommand >> out) {
                cmdParam.push_back(out);
            }
            //split envs by ';'
            while (getline(strCommandEnv, out, ';')) {
                cmdEnv.push_back(out);
            }

            const int paramSize = cmdParam.size() + 1;
            const int envSize = cmdEnv.size() + 1;

            char** param = new char* [paramSize];
            char** commandEnv = new char* [envSize];

            for (int paramInd = 0; paramInd < paramSize - 1; paramInd++)
            {
                param[paramInd] = const_cast<char*>(cmdParam[paramInd].c_str());
            }
            param[paramSize - 1] = NULL;

            for (int envInd = 0; envInd < envSize - 1; envInd++)
            {
                commandEnv[envInd] = const_cast<char*>(cmdEnv[envInd].c_str());
            }
            commandEnv[envSize - 1] = NULL;
            
            std::string logPath = std::string(commands["name"] + "_output.log");

            pid_t childPid = fork();
            if(childPid == 0){
                if(commands["mode"] == "file") {
                    remove(logPath.c_str());
                    int fd = open(logPath.c_str(), O_RDWR | O_CREAT, 0666);
                    dup2(fd, 1);
                    dup2(fd, 2);
                }
                else {
                    dup2(fdPipe[1], 1);
                    dup2(fdPipe[1], 2);
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(delayTime));
                execve(param[0], param, commandEnv);
                perror("execvp");
                exit(0);
            }

            signal(SIGINT, SignalHandler);
            auto begin = std::chrono::system_clock::now();
            auto end = std::chrono::system_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - begin);

            while (duration.count() <= timeout) {
                std::this_thread::sleep_for(std::chrono::milliseconds(200)); // 5Hz
                end = std::chrono::system_clock::now();
                duration = std::chrono::duration_cast<std::chrono::seconds>(end - begin);

                if (duration.count() > timeout) {
                    kill(childPid, SIGINT);
                }

                int child_status;
                pid_t exitPid = waitpid(childPid, &child_status, WNOHANG);

                if (exitPid) {
                    int retCode = WIFEXITED(child_status);
                    if (retCode) {
                        status = WEXITSTATUS(child_status);
                    }
                    else {
                        // coredump
                        status = -1;
                    }
                    break;
                }
            }
            delete[] param;

            std::stringstream log;
            log << "COMMAND: `"     << command          << "`, "  // command
                << "WORKER: `"      << commands["name"] << "`, "  // worker name
                << "STATUS: "       << status << "." << std::endl // system retv
                << "START TIME: "   << std::ctime(&startTime) 
                << "OUTPUT: "       << std::endl;                  // output.log

            if(commands["mode"] == "file") {
                log << g_worker.ReadFile(logPath);
            }
            else {
                int readLength = 0;
                while((readLength = read(fdPipe[0], readBuf, sizeof(readBuf))) > 0) {
                    std::string readStr(readBuf, readLength);
                    log << readStr;
                }
            }

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
