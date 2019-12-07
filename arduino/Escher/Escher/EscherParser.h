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
    escher_(escher), preparing_(false), last_x_(0), last_y_(0), eof_(false) {}

  // Open the given Gcode file for reading.
  bool Open(const char *filename);

  // Prepare the parser by scanning the Gcode file and setting internal
  // parameters.
  void Prepare();

  // Invoked by the driver to feed data to the EscherStepper.
  // Returns false when the end of the file has been reached.
  bool Feed();

  private:
  bool readCommand();
  bool processCommand();
  void doArc(float posx, float posy, float x, float y, float cx, float cy, bool cw);
  double atan3(double dy, double dx);
  void moveTo(float x, float y);
  
  EscherStepper& escher_;
  char *filename_;
  File file_;
  bool preparing_;
  bool eof_;
  char curCommand_[MAX_COMMAND_LINE_LENGTH];
  float last_x_;
  float last_y_;
  float minx_, maxx_, miny_, maxy_;

};

#endif _ESCHERPARSER_H
