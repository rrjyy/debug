#include <iostream>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <vector>
#include <sstream>
#include <iomanip>
#include <sys/personality.h>

#include "linenoise.h"

#include "debugger.hpp"
#include "registers.hpp"

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

uint64_t debugger::get_pc()
{
    return get_register_value(m_pid, reg::rip);
}

void debugger::set_pc(uint64_t pc)
{
    set_register_value(m_pid, reg::rip, pc);
}

void debugger::step_over_breakpoint()
{
    // -1 because execution will go past the breakpoint
    auto possible_breakpoint_location = get_pc() - 1;

    if (m_breakpoints.count(possible_breakpoint_location))
    {
        auto& bp = m_breakpoints[possible_breakpoint_location];
        if (bp.is_enabled())
        {
            auto previous_instruction_address = possible_breakpoint_location;
            set_pc(previous_instruction_address);

            bp.disable();
            ptrace(PTRACE_SINGLESTEP, m_pid, nullptr, nullptr);
            wait_for_signal();
            bp.enable();
        }
    }
}

void debugger::wait_for_signal()
{
    int wait_status;
    auto options = 0;
    waitpid(m_pid, &wait_status, options);
}

uint64_t debugger::read_memory(uint64_t address)
{
    return ptrace(PTRACE_PEEKDATA, m_pid, address, nullptr);
}

void debugger::write_memory(uint64_t address, uint64_t value)
{
    ptrace(PTRACE_POKEDATA, m_pid, address, value);
}

void debugger::dump_registers()
{
    for (const auto& rd : g_register_descriptots)
    {
        std::cout << rd.name << " 0x"
                    << std::setfill('0') << std::setw(16) << std::hex << get_register_value(m_pid, rd.r) << std::endl;
    }
}

void debugger::continue_execution()
{
    step_over_breakpoint();
    ptrace(PTRACE_CONT, m_pid, nullptr, nullptr);
    wait_for_signal();
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
    else if (is_prefix(command, "register"))
    {
        if(is_prefix(args[1], "dump"))
        {
            dump_registers();
        }
        else if (is_prefix(args[1], "read"))
        {
            std::cout << get_register_value(m_pid, get_register_from_name(args[2])) << std::endl;
        }
        else if (is_prefix(args[1], "write"))
        {
            std::string val {args[3], 2}; //assume 0xVAL
            set_register_value(m_pid, get_register_from_name(args[2]), std::stol(val, 0, 16));
        }
    }
    else if(is_prefix(command, "memory"))
    {
        std::string addr {args[2], 2}; //assume 0xADDRESS

        if (is_prefix(args[1], "read"))
        {
            std::cout << std::hex << read_memory(std::stol(addr, 0, 16)) << std::endl;
        }
        if (is_prefix(args[1], "write"))
        {
            std::string val {args[3], 2}; //assume 0xVAL
            write_memory(std::stol(addr, 0, 16), std::stol(val, 0, 16));
        }
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
        personality(ADDR_NO_RANDOMIZE);
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