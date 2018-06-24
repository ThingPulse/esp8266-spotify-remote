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

#include <FS.h>
#include <XPT2046_Touchscreen.h>

#ifndef _TOUCH_CONTROLLERWSH_
#define _TOUCH_CONTROLLERWSH_

typedef void (*CalibrationCallback)(int16_t x, int16_t y);

class TouchControllerWS {
  public:
    TouchControllerWS(XPT2046_Touchscreen *touchScreen);
    bool loadCalibration();
    bool saveCalibration();
    void startCalibration(CalibrationCallback *callback);
    void continueCalibration();
    bool isCalibrationFinished();
    bool isTouched();
    bool isTouched(int16_t debounceMillis);
    TS_Point getPoint();

  private:
    XPT2046_Touchscreen *touchScreen;
    float dx = 0.0;
    float dy = 0.0;
    int ax = 0;
    int ay = 0;
    int state = 0;
    long lastStateChange = 0;
    long lastTouched = 0;
    CalibrationCallback *calibrationCallback;
    TS_Point p1, p2;

};

#endif
