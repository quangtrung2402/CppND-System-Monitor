#include <dirent.h>
#include <unistd.h>
#include <sstream>
#include <string>
#include <vector>

#include "linux_parser.h"

#define PROC_PATH(path) kProcDirectory + (path)

using std::stof;
using std::string;
using std::to_string;
using std::vector;
using std::ifstream;
using std::istringstream;
using std::getline;
using std::all_of;
using std::replace;

// DONE: An example of how to read data from the filesystem
string LinuxParser::OperatingSystem() {
  string line;
  string key;
  string value;
  std::ifstream filestream(kOSPath);
  if (filestream.is_open()) {
    while (std::getline(filestream, line)) {
      std::replace(line.begin(), line.end(), ' ', '_');
      std::replace(line.begin(), line.end(), '=', ' ');
      std::replace(line.begin(), line.end(), '"', ' ');
      std::istringstream linestream(line);
      while (linestream >> key >> value) {
        if (key == "PRETTY_NAME") {
          std::replace(value.begin(), value.end(), '_', ' ');
          return value;
        }
      }
    }
  }
  return value;
}

// DONE: An example of how to read data from the filesystem
string LinuxParser::Kernel() {
  string os, kernel, version;
  string line;
  std::ifstream stream(kProcDirectory + kVersionFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    linestream >> os >> version >> kernel;
  }
  return kernel;
}

// BONUS: Update this to use std::filesystem
vector<int> LinuxParser::Pids() {
  vector<int> pids;
  DIR* directory = opendir(kProcDirectory.c_str());
  struct dirent* file;
  while ((file = readdir(directory)) != nullptr) {
    // Is this a directory?
    if (file->d_type == DT_DIR) {
      // Is every character of the name a digit?
      string filename(file->d_name);
      if (std::all_of(filename.begin(), filename.end(), isdigit)) {
        int pid = stoi(filename);
        pids.push_back(pid);
      }
    }
  }
  closedir(directory);
  return pids;
}

float LinuxParser::MemoryUtilization() {
  float total{1}, available{1};
  ifstream memInfoFile(PROC_PATH(kMeminfoFilename));
  if(memInfoFile.is_open()) {
    string line, key;
    while(getline(memInfoFile, line)) {
      istringstream ss(line);
      ss >> key;
      if("MemTotal:" == key) {
        ss >> total;
      } else if("MemFree:" == key) {
        ss >> available;
        break;  //Don't need to read more
      }
    }
  }
  if (total != 0) {
    return (total - available) / total;
  }
  return -1;
}

long LinuxParser::UpTime() {
  long uptime;
  ifstream uptimeFile(PROC_PATH(kUptimeFilename));
  if(uptimeFile.is_open()) {
    string line;
    getline(uptimeFile, line);
    istringstream ss(line);
    ss >> uptime;
  }
  return uptime;
}

long LinuxParser::Jiffies() {
  return ActiveJiffies() + IdleJiffies();
}

long LinuxParser::ActiveJiffies(int pid) {
  // Referal: https://man7.org/linux/man-pages/man5/proc.5.html
  long utime{0}, stime{0} , cutime{0}, cstime{0};
  ifstream statFile(PROC_PATH(to_string(pid) + kStatFilename));
  if (statFile.is_open()) {
    string line, value;
    getline(statFile, line);
    istringstream ss(line);
    for (int i = 0; i <= CPUProcessStates::csTime_; ++i) {
      switch (i) {
        case CPUProcessStates::uTime_:  ss >> utime;  break;
        case CPUProcessStates::sTime_:  ss >> stime;  break;
        case CPUProcessStates::cuTime_:  ss >> cutime;  break;
        case CPUProcessStates::csTime_:  ss >> cstime;  break;
        default:
          ss >> value;  // Ignore this value
          break;
      }
    }
  }

  return (utime + stime + cutime + cstime) / sysconf(_SC_CLK_TCK);
}

long LinuxParser::ActiveJiffies() {
  // Referal: https://man7.org/linux/man-pages/man5/proc.5.html
  auto cpuUtil(CpuUtilization());
  if (cpuUtil.size() > CPUStates::kGuestNice_) {
    return stol(cpuUtil[CPUStates::kUser_]) +
          stol(cpuUtil[CPUStates::kNice_]) +
          stol(cpuUtil[CPUStates::kSystem_]) +
          stol(cpuUtil[CPUStates::kIRQ_]) +
          stol(cpuUtil[CPUStates::kSoftIRQ_]) +
          stol(cpuUtil[CPUStates::kSteal_]) +
          stol(cpuUtil[CPUStates::kGuest_]) +
          stol(cpuUtil[CPUStates::kGuestNice_]);
  }
  return 0;
}

long LinuxParser::IdleJiffies() {
  // Referal: https://man7.org/linux/man-pages/man5/proc.5.html
  auto cpuUtil(CpuUtilization());
  if (cpuUtil.size() > CPUStates::kIOwait_) {
    return stol(cpuUtil[CPUStates::kIdle_]) +
          stol(cpuUtil[CPUStates::kIOwait_]);
    }
  return 0;
}

vector<string> LinuxParser::CpuUtilization() {
  vector<string> jiffies;
  ifstream statFile(PROC_PATH(kStatFilename));
  if (statFile.is_open()) {
    string line, value;
    getline(statFile, line);  // summary cpu's infor is located at first line. Referal: https://man7.org/linux/man-pages/man5/proc.5.html
    istringstream ss(line);
    ss >> value;  // Ignore 'cpu' word
    while (ss >> value) {
      jiffies.emplace_back(value);
    }
  }
  return jiffies;
}

int LinuxParser::TotalProcesses() {
  ifstream statFile(PROC_PATH(kStatFilename));
  if (statFile.is_open()) {
    string line, key;
    int processes;
    while (getline(statFile, line)) {
      istringstream ss(line);
      ss >> key;
      if ("processes" == key) {
        ss >> processes;
        return processes;
      }
    }
  }
  return -1;
}

int LinuxParser::RunningProcesses() {
  ifstream statFile(PROC_PATH(kStatFilename));
  if (statFile.is_open()) {
    string line, key;
    int processes;
    while (getline(statFile, line)) {
      istringstream ss(line);
      ss >> key;
      if (key == "procs_running") {
        ss >> processes;
        return processes;
      }
    }
  }
  return -1;
}

string LinuxParser::Command(int pid) {
  string command;
  ifstream cmdFile(PROC_PATH(to_string(pid) + kCmdlineFilename));
  if (cmdFile.is_open()) {
    getline(cmdFile, command);
  }
  return command;
}

string LinuxParser::Ram(int pid) {
  ifstream stsFile(PROC_PATH(to_string(pid) + kStatusFilename));
  if (stsFile.is_open()) {
    string line, key;
    long mem;
    while (getline(stsFile, line)) {
      istringstream ss(line);
      ss >> key;
      if (key == "VmSize:") {
        ss >> mem;
        mem /= 1024;
        return to_string(mem);;
      }
    }
  }
  return "--";
}

string LinuxParser::Uid(int pid) {
  ifstream stsFile(PROC_PATH(to_string(pid) + kStatusFilename));
  if (stsFile.is_open()) {
    string line, key, uid;
    while (getline(stsFile, line)) {
      istringstream ss(line);
      ss >> key;
      if (key == "Uid:") {
        ss >> uid;
        return uid;
      }
    }
  }
  return "--";
}

string LinuxParser::User(int pid) {
  const auto uid = Uid(pid);
  ifstream pwFile(kPasswordPath);
  if (pwFile.is_open()) {
    string line, username{""}, asterisk, id;
    while (getline(pwFile, line)) {
      replace(line.begin(), line.end(), ':', ' ');
      istringstream ss(line);
      ss >> username >> asterisk >> id;
      if (id == uid) {
        return username;
      }
    }
  }
  return "";
}

long LinuxParser::UpTime(int pid) {
  long startTime{0};

  ifstream statFile(PROC_PATH(to_string(pid) + kStatFilename));
  if (statFile.is_open()) {
    string line, value;
    getline(statFile, line);
    istringstream ss(line);
    for(int index = 0; index < 22; ++index) {
      ss >> value;
    }
    startTime = stol(value) / sysconf(_SC_CLK_TCK);
  }
  return LinuxParser::UpTime() - startTime;
}
