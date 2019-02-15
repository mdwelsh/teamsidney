#include "EscherParser.h"

bool EscherParser::Init() {
  if (!SPIFFS.exists(fname_)) {
    Serial.printf("ERROR - command file %s does not exist\n", fname_);
    return false;
  }
  file_ = SPIFFS.open(fname_);
  return true;
}

bool EscherParser::Feed() {
  // Read a line from the command file.
  if (!readCommand()) {
    Serial.println("EscherParser: Command file done");
    return false;
  }
  if (!processCommand()) {
    Serial.println("EscherParser: Unable to process command");
    Serial.println(curCommand_);
    // Ignore it
  }
}

// Read a line from the command file.
bool EscherParser::readCommand() {
  int index = 0;
  while (true) {
    int c = file_.read();
    if (c == -1) {
      // File finished
      if (index == 0) {
        // Reached end of file with no command buffered.
        return false;
      } else {
        return true;
      }
    }
    if (index <= MAX_COMMAND_LINE_LENGTH-1) {
      curCommand_[index] = c;
      curCommand_[index+1] = 0x0; // Proactively null-terminate.
    }
    if (c == '\n') {
      if (index < MAX_COMMAND_LINE_LENGTH-1) {
        // Go ahead and commit the command.
        return true;
      } else {
        // Command ran into the limit, so we skip this line.
        Serial.printf("WARNING - Skipping long command line (%d bytes)\n",
            index);
        index = 0;
      }
    }
  }
}

// Process the current command.
bool EscherParser::processCommand() {
}
