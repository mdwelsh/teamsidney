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
    escher_(escher), last_x_(0), last_y_(0), eof_(false) {}

  bool Open(const char *filename);
  void Parse();
  bool Feed();

  private:
  bool readCommand();
  bool processCommand();
  void doArc(float posx, float posy, float x, float y, float cx, float cy, bool cw);
  double atan3(double dy, double dx);
  
  EscherStepper& escher_;
  File file_;
  bool eof_;
  char curCommand_[MAX_COMMAND_LINE_LENGTH];
  float last_x_;
  float last_y_;

};

#endif _ESCHERPARSER_H
