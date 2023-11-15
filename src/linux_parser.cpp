#include <dirent.h>
#include <unistd.h>
#include <sstream>
#include <string>
#include <vector>

#include "linux_parser.h"

#define PATH(path) kProcDirectory + (path)

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
  ifstream filestream(kOSPath);
  if (filestream.is_open()) {
    while (getline(filestream, line)) {
      replace(line.begin(), line.end(), ' ', '_');
      replace(line.begin(), line.end(), '=', ' ');
      replace(line.begin(), line.end(), '"', ' ');
      istringstream linestream(line);
      while (linestream >> key >> value) {
        if (key == "PRETTY_NAME") {
          replace(value.begin(), value.end(), '_', ' ');
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
  ifstream stream(kProcDirectory + kVersionFilename);
  if (stream.is_open()) {
    getline(stream, line);
    istringstream linestream(line);
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
      if (all_of(filename.begin(), filename.end(), isdigit)) {
        int pid = stoi(filename);
        pids.emplace_back(pid);
      }
    }
  }
  closedir(directory);
  return pids;
}

float LinuxParser::MemoryUtilization() {
  float total{1}, available{1};

  ifstream memInfoFile(PATH(kMeminfoFilename));
  if(memInfoFile.is_open()) {
    string line, key;
    while(getline(memInfoFile, line)) {
      istringstream ss(line);
      ss >> key;
      if("MemTotal:" == key) {
        ss >> total;
      } else if("MemAvailable:" == key) {
        ss >> available;
        break;  //Don't need to read more
      }
    }
  }
  return (total - available) / total;
}

long LinuxParser::UpTime() {
  long uptime;
  ifstream uptimeFile(PATH(kUptimeFilename));
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
  long time{0};
  
  vector<string> values;
  ifstream statFile(PATH(to_string(pid) + kStatFilename));
  if (statFile.is_open()) {
    string line, value;
    getline(statFile, line);
    istringstream ss(line);
    while (ss >> value) {
      values.emplace_back(value);
    }
  }
  if (values.size() < 17) {
    return time;
  }

  long utime{0}, stime{0} , cutime{0}, cstime{0};
  if (all_of(values[13].begin(), values[13].end(), isdigit))
    utime = stol(values[13]);
  if (all_of(values[14].begin(), values[14].end(), isdigit))
    stime = stol(values[14]);
  if (all_of(values[15].begin(), values[15].end(), isdigit))
    cutime = stol(values[15]);
  if (all_of(values[16].begin(), values[16].end(), isdigit))
    cstime = stol(values[16]);

  time = utime + stime + cutime + cstime;
  return time / sysconf(_SC_CLK_TCK);
}

long LinuxParser::ActiveJiffies() {
  auto jiffies(CpuUtilization());

  return stol(jiffies[CPUStates::kUser_]) + stol(jiffies[CPUStates::kNice_]) +
         stol(jiffies[CPUStates::kSystem_]) + stol(jiffies[CPUStates::kIRQ_]) +
         stol(jiffies[CPUStates::kSoftIRQ_]) +
         stol(jiffies[CPUStates::kSteal_]);
}

long LinuxParser::IdleJiffies() {
  auto jiffies(CpuUtilization());
  return stol(jiffies[CPUStates::kIdle_]) + stol(jiffies[CPUStates::kIOwait_]);
}

vector<string> LinuxParser::CpuUtilization() {
  vector<string> jiffies;
  ifstream statFile(PATH(kStatFilename));
  if (statFile.is_open()) {
    string line, cpu, value;
    getline(statFile, line);
    istringstream ss(line);
    ss >> cpu;
    while (ss >> value) {
      jiffies.emplace_back(value);
    }
  }
  return jiffies;
}

int LinuxParser::TotalProcesses() {
  int processes;
  ifstream statFile(PATH(kStatFilename));
  if (statFile.is_open()) {
    string line, key;
    while (getline(statFile, line)) {
      istringstream ss(line);
      ss >> key;
      if ("processes" == key) {
        ss >> processes;
        break;
      }
    }
  }
  return processes;
}

int LinuxParser::RunningProcesses() {
  int processes;
  ifstream statFile(PATH(kStatFilename));
  if (statFile.is_open()) {
    string line, key;
    while (getline(statFile, line)) {
      istringstream ss(line);
      ss >> key;
      if (key == "procs_running") {
        ss >> processes;
        break;
      }
    }
  }
  return processes;
}

string LinuxParser::Command(int pid) {
  string command;
  ifstream cmdFile(PATH(to_string(pid) + kCmdlineFilename));
  if (cmdFile.is_open()) {
    getline(cmdFile, command);
  }
  return command;
}

string LinuxParser::Ram(int pid) {
  long mem;

  ifstream stsFile(PATH(to_string(pid) + kStatusFilename));
  if (stsFile.is_open()) {
    string line, key;
    while (getline(stsFile, line)) {
      istringstream ss(line);
      ss >> key;
      if (key == "VmSize:") {
        ss >> mem;
        mem /= 1024;
        break;
      }
    }
  }
  return to_string(mem);;
}

string LinuxParser::Uid(int pid) {
  string uid;
  ifstream stsFile(PATH(to_string(pid) + kStatusFilename));
  if (stsFile.is_open()) {
    string line, key;
    while (getline(stsFile, line)) {
      istringstream ss(line);
      ss >> key;
      if (key == "Uid:") {
        ss >> uid;
        break;
      }
    }
  }
  return uid;
}

string LinuxParser::User(int pid) {
  string uid = Uid(pid);
  string name;
  ifstream pwFile(kPasswordPath);
  if (pwFile.is_open()) {
    string line;
    while (getline(pwFile, line)) {
      replace(line.begin(), line.end(), ':', ' ');
      istringstream ss(line);
      string asterisk, id;
      ss >> name >> asterisk >> id;
      if (id == uid) {
        break;
      }
    }
  }
  return name;
}

long LinuxParser::UpTime(int pid) {
  long time{0};

  ifstream statFile(PATH(to_string(pid) + kStatFilename));
  if (statFile.is_open()) {
    string line, value;
    getline(statFile, line);
    istringstream ss(line);
    int index{0};
    for(; index <= 21; ++index) {
      ss >> value;
    }

    try {
      if (index < 21) {
        time = 0;
      } else {
        time = stol(value) / sysconf(_SC_CLK_TCK);
      }
    } catch (...) {
      time = 0;
    }
  }
  return time;
}
