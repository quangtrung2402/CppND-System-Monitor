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

Process::Process(int pid) : m_pid(pid) {
  try {
    m_utilization = LinuxParser::ActiveJiffies(pid) / LinuxParser::Jiffies();
  } catch (...) {
    m_utilization = 0;
  }
}

int Process::Pid() const { return m_pid; }

float Process::CpuUtilization() const {
  return m_utilization;
}

string Process::Command() const {
  return LinuxParser::Command(m_pid);
}

string Process::Ram() const {
  return LinuxParser::Ram(m_pid);
}

string Process::User() const {
  return LinuxParser::User(m_pid);
}

long int Process::UpTime() const {
  return LinuxParser::UpTime(m_pid);
}

bool Process::operator<(Process const& a) const {
  return m_utilization < a.m_utilization;
}
