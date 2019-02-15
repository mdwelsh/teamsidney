#ifndef _ESCHERPARSER_H
#define _ESCHERPARSER_H

#include "EscherStepper.h"
#include "FS.h"
#include "SPIFFS.h"

// Command lines longer than this are ignored.
#define MAX_COMMAND_LINE_LENGTH 128

class EscherParser {
  public:
  EscherParser(EscherStepper &escher, const char *filename) :
    escher_(escher),
    fname_(filename),
    stopped_(false),
    cIndex_(0) {}

  bool Init();
  bool Feed();

  private:
  const char *fname_;
  File file_;
  EscherStepper& escher_;
  char curCommand_[MAX_COMMAND_LINE_LENGTH];
  char cIndex_;
};

#endif _ESCHERPARSER_H
