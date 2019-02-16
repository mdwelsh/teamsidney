#include "EscherParser.h"

bool EscherParser::Open(const char *filename) {
  if (!SPIFFS.exists(filename)) {
    Serial.printf("ERROR - command file %s does not exist\n", filename);
    return false;
  }
  file_ = SPIFFS.open(filename);
  Serial.printf("EscherParser opened %s\n", filename);
  return true;
}

bool EscherParser::Feed() {
  // Read a line from the command file.
  if (!readCommand()) {
    return false;
  }
  if (!processCommand()) {
    Serial.println("WARNING - Unable to process command");
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
      index++;
    }
    if (c == '\n') {
      if (index-1 < MAX_COMMAND_LINE_LENGTH-1) {
        // Go ahead and commit the command.
        curCommand_[index] = 0x0; // Null terminate.
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
  char* command = strtok(curCommand_, " \n");
  if (!strcmp(command, "START")) {
    // Do nothing.
    
  } else if (!strcmp(command, "MOVE")) {
    char *xs = strtok(NULL, " \n");
    char *ys = strtok(NULL, " \n");
    if (xs == NULL || ys == NULL) {
      Serial.println("WARNING - MOVE command missing x or y value");
      Serial.println(curCommand_);
      return false;
    }
    int x = atoi(xs);
    int y = atoi(ys);
    escher_.push(x, y);
    
  } else if (!strcmp(command, "END")) {
    // Do nothing.
    
  } else {
    Serial.printf("WARNING - Unrecognized command token %s\n", command);
    return false;
  }
  return true;
}
