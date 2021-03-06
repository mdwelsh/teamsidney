#include <float.h>
#include <math.h>
#include "EscherParser.h"

bool EscherParser::Open(const char *filename) {
  filename_ = (char *)filename;
  if (!SPIFFS.exists(filename_)) {
    Serial.printf("ERROR - gcode file %s does not exist\n", filename_);
    return false;
  }
  file_ = SPIFFS.open(filename_);
  Serial.printf("EscherParser opened %s\n", filename_);
  eof_ = false;
  return true;
}

void EscherParser::Prepare() {
  minx_ = FLT_MAX;
  maxx_ = 0.0;
  miny_ = FLT_MAX;
  maxy_ = 0.0;
  last_minx_ = FLT_MAX;
  last_maxx_ = 0.0;
  last_miny_ = FLT_MAX;
  last_maxy_ = 0.0;
  preparing_ = true;
  while (Feed()) {}
  preparing_ = false;
  Serial.printf("Prepare: Got minx %f maxx %f miny %f maxy %f\n", minx_, maxx_, miny_, maxy_);
  escher_.setMinX(minx_);
  escher_.setMaxX(maxx_);
  escher_.setMinY(miny_);
  escher_.setMaxY(maxy_);
  escher_.computeScaleFactors();
  file_.close();
  Open(filename_);
  last_x_ = 0;
  last_y_ = 0;
}

void EscherParser::moveTo(float x, float y) {
  if (x == last_x_ && y == last_y_) {
    return;
  }
  last_x_ = x;
  last_y_ = y;
  // Save the previous value so that we can restore it after parsing
  // the file, to undo the final point.
  last_minx_ = minx_;
  last_maxx_ = maxx_;
  last_miny_ = miny_;
  last_maxy_ = maxy_;
  if (preparing_) {
    if (x < minx_) {
      minx_ = x;
    }
    if (x > maxx_) {
      maxx_ = x;
    }
    if (y < miny_) {
      miny_ = y;
    }
    if (y > maxy_) {
      maxy_ = y;
    }
  } else {
    escher_.push(x, y);
  }
}

// Try to feed the parser with more Gcode data.
// Returns false when there's nothing left to process.
bool EscherParser::Feed() {
  if (eof_) {
    return false;
  }

  while(true) {
    // Read a line from the command file.
    if (!readCommand()) {
      Serial.println("EscherParser::Feed - reached EOF.");
      // Ignore final (0, 0) added by Inkscape Gcode plugin.
      if (preparing_) {
        minx_ = last_minx_;
        maxx_ = last_maxx_;
        miny_ = last_miny_;
        maxy_ = last_maxy_;
      } else {
        escher_.pop();
      }
      eof_ = true;
      return false;
    }
    // The command we processed might have an error or be something we
    // skip, so read another command if that happens.
    if (processCommand()) {
      return true;
    }
  }
}

double EscherParser::atan3(double dy, double dx) {
   double a = atan2(dy, dx);
   if (a < 0) {
     a = (M_PI * 2.0) + a;
   }
   return a;
}

// Precision of arcs in centimeters per segment.
#define CM_PER_SEGMENT 5.0

// Adapted from:
//  https://www.marginallyclever.com/2014/03/how-to-improve-the-2-axis-cnc-gcode-interpreter-to-understand-arcs/
bool EscherParser::doArc(float posx, float posy, float x, float y, float cx, float cy, bool cw) {
  //Serial.printf("doArc: %f, %f, %f, %f, %f, %f, %d\n", posx, posy, x, y, cx, cy, cw);
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
  int num_segments = floor(l / CM_PER_SEGMENT);

  //Serial.printf("doArc: posx %f posy %f x %f y %f cx %f cy %f cw %f\n", posx, posy, x, y, cx, cy, cw);
  //Serial.printf("doArc: dx %f dy %f radius %f angle1 %f angle2 %f sweep %f\n", dx, dy, radius, angle1, angle2, sweep);
  //Serial.printf("doArc: l %f num_segments %d\n", l, num_segments);

  for (int i = 0; i < num_segments; i++) {
    // interpolate around the arc
    float fraction = (i * 1.0) / (num_segments * 1.0);
    float angle3 = (sweep * fraction) + angle1;

    // find the intermediate position
    float nx = cx + cos(angle3) * radius;
    float ny = cy + sin(angle3) * radius;

    // make a line to that intermediate position
    //Serial.printf("doArc: pushing %f %f\n", nx, ny);
    moveTo(nx, ny);
  }

  // one last line hit the end
  //Serial.printf("doArc: last pushing %f %f\n", x, y);
  moveTo(x, y);
  // We return false on a (0.0, 0.0) move, since that might be the
  // last point in the gCode file, which we want to read past and
  // get rid of before returning from Feed().
  return !(x == 0.0 && y == 0.0);
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
      Serial.println("EscherParser::readCommand() hit eof");
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
        Serial.printf("WARNING - Skipping long command line (%d bytes)\n", index);
        index = 0;
      }
    }
  }
}

// Process the current command. Returns false if there is an error
// parsing or processing the command. Returns true if the command pushed
// new points into the EscherStepper to be processed.
bool EscherParser::processCommand() {
  String cmd = String(curCommand_);
  //Serial.printf("Processing: %s\n", cmd.c_str());

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
    moveTo(x, y);
    // We return false on a (0.0, 0.0) move, since that might be the
    // last point in the gCode file, which we want to read past and
    // get rid of before returning from Feed().
    return !(x == 0.0 && y == 0.0);
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
    return doArc(last_x_, last_y_, x, y, last_x_+i, last_y_+j, cw);
  }
  // Ignore all other commands.
  return false;
}