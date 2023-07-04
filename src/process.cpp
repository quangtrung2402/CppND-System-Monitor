#include <unistd.h>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

#include "process.h"
#include "linux_parser.h"

using std::string;
using std::to_string;
using std::vector;

Process::Process(int pid) :
m_pid(pid),
m_utilization(0),
m_uptime(LinuxParser::UpTime(pid)),
m_ram(LinuxParser::Ram(pid)),
m_user(LinuxParser::User(pid)),
m_command(LinuxParser::Command(pid))
  {
  long seconds{LinuxParser::UpTime() - m_uptime};
  long time{LinuxParser::ActiveJiffies(pid)};
  try {
    m_utilization = float(time) / float(seconds);
  } catch (...) {
    m_utilization = 0;
  }
}

int Process::Pid() const { return m_pid; }

float Process::CpuUtilization() const { return m_utilization; }

string Process::Command() const { return m_command; }

string Process::Ram() const { return m_ram; }

string Process::User() const { return m_user; }

long int Process::UpTime() const { return m_uptime; }

bool Process::operator<(Process const& a) const {
  return m_utilization < a.m_utilization;
}
