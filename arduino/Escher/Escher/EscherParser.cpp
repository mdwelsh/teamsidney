#include <math.h>
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

// Try to feed the parser with more Gcode data.
// Returns false when there's nothing left to process.
bool EscherParser::Feed() {
  if (eof_) {
    return false;
  }
  // Read a line from the command file.
  if (!readCommand()) {
    Serial.println("EscherParser::Feed - reached EOF.");
    // Remove final (0, 0) added by Inkscape Gcode plugin.
    escher_.pop();
    eof_ = true;
    return false;
  }
  if (!processCommand()) {
    Serial.println("EscherParser::Feed - error processing command.");
  }
  return true;
}

double EscherParser::atan3(double dy, double dx) {
   double a = atan2(dy, dx);
   if (a < 0) {
     a = (M_PI * 2.0) + a;
   }
   return a;
}

// Precision of arcs in centimeters per segment.
#define CM_PER_SEGMENT 0.1

// Adapted from:
//  https://www.marginallyclever.com/2014/03/how-to-improve-the-2-axis-cnc-gcode-interpreter-to-understand-arcs/
void EscherParser::doArc(float posx, float posy, float x, float y, float cx, float cy, bool cw) {
  float dx = posx - cx;
  float dy = posy - cy;
  float radius = sqrt((dx*dx)+(dy*dy));

  // find the sweep of the arc
  float angle1 = atan3(posy - cy, posx - cx);
  float angle2 = atan3(y - cy, x - cx);
  float sweep = angle2 - angle1;

  if (sweep < 0 && cw) {
    angle2 += 2.0 * M_PI;
  } else if (sweep > 0 && !cw) {
    angle1 += 2.0 * M_PI;
  }

  sweep = angle2 - angle1;

  // get length of arc
  float l = abs(sweep) * radius;
  int num_segments = int(l / CM_PER_SEGMENT);

  for (int i = 0; i < num_segments; i++) {
    // interpolate around the arc
    float fraction = (i * 1.0) / (num_segments * 1.0);
    float angle3 = (sweep * fraction) + angle1;

    // find the intermediate position
    float nx = cx + cos(angle3) * radius;
    float ny = cy + sin(angle3) * radius;

    // make a line to that intermediate position
    escher_.push(nx, ny);
  }

  // one last line hit the end
  escher_.push(x, y);
}

// Read a line from the command file. Returns false if EOF is hit.
// This means that the last command in the file will be dropped
// if the line is not newline-terminated.
bool EscherParser::readCommand() {
  int index = 0;
  while (true) {
    int c = file_.read();
    if (c == -1) {
      // Hit EOF.
      return false;
    }
    if (index <= MAX_COMMAND_LINE_LENGTH-1) {
      // Add the character read to the command.
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

// Process the current command. Returns false if there is an error
// parsing or processing the command.
bool EscherParser::processCommand() {
  String cmd = String(curCommand_);

  // Line command.
  if (cmd.startsWith("G00") || cmd.startsWith("G01")) {
    char* command = strtok(curCommand_, " \n");
    char* xs = strtok(NULL, " \n");
    char* ys = strtok(NULL, " \n");
    if (xs == NULL || ys == NULL) {
      Serial.println("WARNING - line command missing x or y value");
      Serial.println(curCommand_);
      return false;
    }
    if (!String(xs).startsWith("X") || !String(ys).startsWith("Y")) {
      Serial.println("WARNING - line command malformed X or Y value");
      Serial.println(curCommand_);
      return false;
    }
    float x = atof(xs+1);
    float y = atof(ys+1);
    Serial.printf("EscherParser - line to %f %f\n", x, y);
    escher_.push(x, y);
    last_x_ = x;
    last_y_ = y;
    return true;
  }

  // Curve.
  if (cmd.startsWith("G02") || cmd.startsWith("G03")) {
    char* command = strtok(curCommand_, " \n");
    char* xs = strtok(NULL, " \n");
    char* ys = strtok(NULL, " \n");
    if (!String(xs).startsWith("X") || !String(ys).startsWith("Y")) {
      Serial.println("WARNING - curve command malformed X or Y value");
      Serial.println(curCommand_);
      return false;
    }
    char* zs = strtok(NULL, " \n");
    char* is;
    if (String(zs).startsWith("Z")) {
      // This is a Z value - we ignore it.
      is = strtok(NULL, " \n");
    } else {
      is = zs;
    }
    char* js = strtok(NULL, " \n");
    if (!String(is).startsWith("I") || !String(js).startsWith("J")) {
      Serial.println("WARNING - curve command malformed I or J value");
      Serial.println(curCommand_);
      return false;
    }
    float x = atof(xs+1);
    float y = atof(ys+1);
    float i = atof(is+1);
    float j = atof(js+1);
    bool cw = false;
    if (cmd.startsWith("G03")) {
      cw = true;
    }
    Serial.printf("EscherParser - doArc %f %f %f %f %f %f cw %d\n", last_x_, last_y_, x, y, last_x_+i, last_y_+j, cw);
    doArc(last_x_, last_y_, x, y, last_x_+i, last_y_+j, cw);
    return true;
  }
  // Ignore all other commands.
  return true;
}