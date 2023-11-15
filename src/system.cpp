#include <unistd.h>
#include <cstddef>
#include <set>
#include <string>
#include <vector>

#include "process.h"
#include "processor.h"
#include "system.h"
#include "linux_parser.h"

using std::set;
using std::size_t;
using std::string;
using std::vector;

Processor& System::Cpu() {
  return cpu_;
}

vector<Process>& System::Processes() {
  vector<int> pids(LinuxParser::Pids());

  processes_.clear();
  for (auto pid : pids) {
    if (!LinuxParser::Ram(pid).empty()) {
      processes_.emplace_back(Process(pid));
    }
  }
  std::sort(processes_.rbegin(), processes_.rend());
  return processes_;
}

string System::Kernel() {
  return LinuxParser::Kernel();
}

float System::MemoryUtilization() {
  return LinuxParser::MemoryUtilization();
}

string System::OperatingSystem() {
  return LinuxParser::OperatingSystem();
}

int System::RunningProcesses() {
  return LinuxParser::RunningProcesses();
}

int System::TotalProcesses() {
  return LinuxParser::TotalProcesses();
}

long int System::UpTime() {
  return LinuxParser::UpTime();
}
