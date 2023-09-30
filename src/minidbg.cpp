#include <iostream>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <vector>
#include <sstream>

#include "linenoise.h"

#include "debugger.hpp"

using namespace minidbg;

std::vector<std::string> split(const std::string &s, char delimiter)
{
    std::vector<std::string> out{}; //initialization
    std::stringstream ss {s}; //s is the parameter to the constructor
    std::string item;

    while(std::getline(ss, item, delimiter))
    {
        out.push_back(item);
    }

    return out;
}

bool is_prefix(const std::string& s, const std::string& of)
{
    if(s.size() > of.size()) return false;
    return std::equal(s.begin(), s.end(), of.begin());
}

void debugger::continue_execution()
{
    ptrace(PTRACE_CONT, m_pid, nullptr, nullptr);

    int wait_status;
    auto options = 0;
    waitpid(m_pid, &wait_status, options);
}

void debugger::handle_command(const std::string& line)
{
    auto args = split(line, ' ');
    auto command = args[0];

    if (is_prefix(command, "cont"))
    {
        continue_execution();
    }
    else if (is_prefix(command, "break"))
    {
        std::string addr {args[1], 2};
        set_breakpoint_at_address(std::stol(addr, 0, 16));
    }
    else
    {
        std::cerr << "Unknown command\n";
    }
}

void debugger::run()
{
    int wait_status;
    auto options = 0;
    waitpid(m_pid, &wait_status, options);

    char* line = nullptr;
    while((line = linenoise("minidbg > ")) != nullptr)
    {
        handle_command(line);
        linenoiseHistoryAdd(line);
        linenoiseFree(line);
    }
}

void debugger::set_breakpoint_at_address(std::intptr_t addr)
{
    std::cout << "Set breakpoint at address 0x" << std::hex << addr << std::endl;
    breakpoint bp {m_pid, addr};
    bp.enable();
    m_breakpoints[addr] = bp;
}

void execute_debugee(const std::string& prog_name)
{
    if(ptrace(PTRACE_TRACEME, 0, 0, 0) < 0)
    {
        std::cerr << "Error in ptrace\n";
        return;
    }
    execl(prog_name.c_str(), prog_name.c_str(), nullptr);
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Program name not specified \n";
        return -1;
    }

    auto prog = argv[1];

    auto pid = fork();

    if(pid == 0)
    {
        //child
        execute_debugee(prog);
    }
    else if(pid >= 1)
    {
        //parent
        std::cout << "Started debugging process " << pid << '\n';

        debugger dbg{prog, pid}; //c++11

        dbg.run();
    }
}