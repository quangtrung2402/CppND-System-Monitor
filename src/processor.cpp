#include "processor.h"
#include "linux_parser.h"

float Processor::Utilization() {
  return (float)LinuxParser::ActiveJiffies() / LinuxParser::Jiffies();
}