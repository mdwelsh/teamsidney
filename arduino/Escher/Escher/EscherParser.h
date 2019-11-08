#ifndef _ESCHERPARSER_H
#define _ESCHERPARSER_H

#include "EscherStepper.h"
#include "FS.h"
#include "SPIFFS.h"

// Command lines longer than this are ignored.
#define MAX_COMMAND_LINE_LENGTH 128

class EscherParser {
  public:
  EscherParser(EscherStepper &escher) :
    escher_(escher), stopped_(false), cIndex_(0), last_x_(0), last_y_(0) {}

  bool Open(const char *filename);
  bool Feed();

  private:
  bool readCommand();
  bool processCommand();
  
  File file_;
  EscherStepper& escher_;
  bool stopped_;
  char curCommand_[MAX_COMMAND_LINE_LENGTH];
  char cIndex_;
  int last_x_;
  int last_y_;
};

#endif _ESCHERPARSER_H
