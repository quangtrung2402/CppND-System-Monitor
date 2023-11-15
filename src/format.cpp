#include <string>

#include "format.h"

using std::string;
using std::to_string;

string Format::ElapsedTime(long seconds) {
  string strHours{to_string(seconds / 3600)};

  int minutes = (seconds % 3600) / 60;
  string strMinutes{to_string(minutes)};
  if (minutes < 10) {
    strMinutes = "0" + strMinutes;
  }

  seconds = seconds % 60;
  string strSeconds{to_string(seconds)};
  if (seconds < 10) {
    strSeconds = "0" + strSeconds;
  }
  return strHours + ":" + strMinutes + ":" + strSeconds;
}