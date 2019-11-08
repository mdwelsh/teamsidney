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

// Adapted from:
//  https://www.marginallyclever.com/2014/03/how-to-improve-the-2-axis-cnc-gcode-interpreter-to-understand-arcs/
bool EscherParser::doArc(float posx, float posy, float x, float y, float cx, float cy, bool cw) {
  float dx = posx - cx;
  float dy = posy - cy;
  float radius = Math.sqrt((dx*dx)+(dy*dy));

  // XXX MDW STOPPED HERE.

  // find the sweep of the arc
  var angle1 = atan3(posy - cy, posx - cx);
  var angle2 = atan3(y - cy, x - cx);
  var sweep = angle2 - angle1;

  if (sweep < 0 && cw) {
    angle2 += 2.0 * Math.PI;
  } else if (sweep > 0 && !cw) {
    angle1 += 2.0 * Math.PI;
  }

  sweep = angle2 - angle1;

  // get length of arc
  var l = Math.abs(sweep) * radius;
  var num_segments = Math.floor(l / CM_PER_SEGMENT);

  for (i = 0; i < num_segments; i++) {
    // interpolate around the arc
    var fraction = (i * 1.0) / (num_segments * 1.0);
    var angle3 = (sweep * fraction) + angle1;

    // find the intermediate position
    var nx = cx + Math.cos(angle3) * radius;
    var ny = cy + Math.sin(angle3) * radius;

    // make a line to that intermediate position
    retval.push({x: nx, y: ny});
  }

  // one last line hit the end
  retval.push({x: x, y: y});
  return retval;
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
    last_x_ = (int)x;
    last_y_ = (int)y;
    escher_.push(last_x_, last_y_);
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
