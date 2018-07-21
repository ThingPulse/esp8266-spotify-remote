/**The MIT License (MIT)
 
 Copyright (c) 2018 by ThingPulse Ltd., https://thingpulse.com
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 */

#include "TouchControllerWS.h"

TouchControllerWS::TouchControllerWS(XPT2046_Touchscreen *touchScreen) {
  this->touchScreen = touchScreen;
}
  
bool TouchControllerWS::loadCalibration() {
  // always use this to "mount" the filesystem
  bool result = SPIFFS.begin();
  Serial.println("SPIFFS opened: " + result);

  // this opens the file "f.txt" in read-mode
  File f = SPIFFS.open("/calibration.txt", "r");

  if (!f) {
    return false;
  } else {

      //Lets read line by line from the file
      String dxStr = f.readStringUntil('\n');
      String dyStr = f.readStringUntil('\n');
      String axStr = f.readStringUntil('\n');
      String ayStr = f.readStringUntil('\n');

      dx = dxStr.toFloat();
      dy = dyStr.toFloat();
      ax = axStr.toInt();
      ay = ayStr.toInt();

  }
  f.close();
  return true;
}

bool TouchControllerWS::saveCalibration() {
  bool result = SPIFFS.begin();
  if (result) {
	  Serial.println("SPIFFS started successfully");
	  // open the file in write mode
	  File f = SPIFFS.open("/calibration.txt", "w");
	  if (!f) {
		  Serial.println("file creation failed");
	  }
	  // now write two lines in key/value style with  end-of-line characters
	  f.println(dx);
	  f.println(dy);
	  f.println(ax);
	  f.println(ay);

	  f.close();
  }
  else {
	  Serial.println("SPIFFS failed to start!");
  }
  return result;
}

void TouchControllerWS::startCalibration(CalibrationCallback *calibrationCallback) {
  state = 0;
  this->calibrationCallback = calibrationCallback;
  (*calibrationCallback)(10, 10);

}

void TouchControllerWS::continueCalibration() {
    TS_Point p = touchScreen->getPoint();

    if (state == 0) {
      
      if (touchScreen->touched()) {
        p1 = p;
        state++;
        (*calibrationCallback)(230, 310);
        lastStateChange = millis();
      }

    } else if (state == 1) {
      if (touchScreen->touched() && (millis() - lastStateChange > 1000)) {

        p2 = p;
        state++;
        lastStateChange = millis();
        dx = 240.0 / abs(p1.y - p2.y);
        dy = 320.0 / abs(p1.x - p2.x);
        ax = p1.y < p2.y ? p1.y : p2.y;
        ay = p1.x < p2.x ? p1.x : p2.x;
        Serial.printf("dx: %f, dy: %f, ax: %f, ay: %f\n", dx, dy, ax, ay);
      }

    }
}
bool TouchControllerWS::isCalibrationFinished() {
  return state == 2;
}

bool TouchControllerWS::isTouched() {
	bool result;
	result = touchScreen->touched();
	return result;
}

bool TouchControllerWS::isTouched(int16_t debounceMillis) {
  if (touchScreen->touched() && (long)millis() - lastTouched > debounceMillis) {
    lastTouched = millis();
    return true;
  }
  return false;
}
TS_Point TouchControllerWS::getPoint() {
    TS_Point p = touchScreen->getPoint();
    int x = 240 - (p.y - ax) * dx;
    int y = (p.x - ay) * dy;
    p.x = x;
    p.y = y;
    return p;
}
