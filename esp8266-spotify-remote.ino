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

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <MiniGrafx.h>
#include <ILI9341_SPI.h>
#include <FS.h>
#include <JPEGDecoder.h>

#include "SpotifyClient.h"
#include "settings.h"
#include "TouchControllerWS.h"
#include "images.h"

 // Statements like:
 // #pragma message(Reminder "Fix this problem!")
 // Which will cause messages like:
 // C:\Source\Project\main.cpp(47): Reminder: Fix this problem!
 // to show up during compiles. Note that you can NOT use the
 // words "error" or "warning" in your reminders, since it will
 // make the IDE think it should abort execution. You can double
 // click on these messages and jump to the line in question.
 //
 // see https://stackoverflow.com/questions/5966594/how-can-i-use-pragma-message-so-that-the-message-points-to-the-filelineno
 //
#define Stringize( L )     #L 
#define MakeString( M, L ) M(L)
#define $Line MakeString( Stringize, __LINE__ )
#define Reminder __FILE__ "(" $Line ") : Reminder: "
#ifdef LOAD_SD_LIBRARY
#pragma message(Reminder "Comment out the line with LOAD_SD_LIBRARY /JPEGDecoder/src/User_config.h !")
#endif

int BUFFER_WIDTH = 240;
int BUFFER_HEIGHT = 160;
// Limited to 4 colors due to memory constraints
int BITS_PER_PIXEL = 2; // 2^2 =  4 colors

XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);
TouchControllerWS touchController(&ts);

ILI9341_SPI tft = ILI9341_SPI(TFT_CS, TFT_DC);
MiniGrafx gfx = MiniGrafx(&tft, BITS_PER_PIXEL, palette, BUFFER_WIDTH, BUFFER_HEIGHT);

WiFiClientSecure wifiClient;
SpotifyClient client(clientId, clientSecret, redirectUri, &wifiClient);
SpotifyData data;
SpotifyAuth auth;

String currentImageUrl = "";
uint32_t lastDrawingUpdate = 0;
uint16_t counter = 0;
long lastUpdate = 0;
TS_Point lastTouchPoint;
uint32_t lastTouchMillis = 0;
boolean isDownloadingCover = false;

void setClock();
void drawJPEGFromSpiffs(String filename, MiniGrafx *gfx, int xpos, int ypos, DrawingCallback *drawingCallback);
void calibrationCallback(int16_t x, int16_t y);
CalibrationCallback calibration = &calibrationCallback;
String formatTime(uint32_t time);
void saveRefreshToken(String refreshToken);
String loadRefreshToken();
void displayLogo();

void drawSongInfo();
DrawingCallback drawSongInfoCallback = &drawSongInfo;

void printFreeHeap(String msg) {
  Serial.println("*** Memory stats " + msg + " ***");
  Serial.printf("\tFree heap: %d\n", ESP.getFreeHeap());
  Serial.printf("\tMax free block size: %d\n", ESP.getMaxFreeBlockSize());
  Serial.printf("\tHeap fragmentation: %d%\n\n", ESP.getHeapFragmentation());
}

void setup() {
  Serial.begin(115200);
  Serial.println("");
  printFreeHeap("right after setup()");

  Serial.println();
  Serial.print("connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected at IP address: ");
  Serial.println(WiFi.localIP());

  printFreeHeap("after WiFi connection");

  setClock();

  // TEST CONNECTION TO SPOTIFY ***************************************************
  uint32_t freeStackStart = ESP.getFreeContStack();
  X509List cert(digicertRootCaCert);
  wifiClient.setTrustAnchors(&cert);
  const char *host = "accounts.spotify.com";
  const uint16_t port = 443;
  wifiClient.connect(host, port);
  if (wifiClient.connected()) {
    Serial.println("Connection to Spotify established.");
  } else {
    Serial.printf("Connection to %s:%d failed; returning.\n", host, port);
    return;  
  }
  wifiClient.stop();
  uint32_t freeStackEnd = ESP.getFreeContStack();
  Serial.printf("Stack used: %d\n\n", freeStackStart - freeStackEnd);
  printFreeHeap("after socket connection established");
  // END TEST CONNECTION TO SPOTIFY ***********************************************

  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, HIGH);    // HIGH to Turn on;
  gfx.init();
  gfx.fillBuffer(MINI_BLACK);
  gfx.setColor(MINI_YELLOW);
  gfx.setFont(ArialMT_Plain_16);
  gfx.setTextAlignment(TEXT_ALIGN_CENTER);
  displayLogo();
  gfx.fillBuffer(MINI_BLACK);
  gfx.commit(0, 160);
  
  printFreeHeap("after 1st GFX commit");
  
  boolean mounted = SPIFFS.begin();
  if (!mounted) {
    Serial.println("FS not formatted. Doing that now");
    gfx.drawString(120, 10, "Formatting file system.\nPlease stand by...");
    gfx.commit(0, 0);
    SPIFFS.format();
    Serial.println("FS formatted...");
    SPIFFS.begin();
  }

  printFreeHeap("after SPIFFS mounted");

  // init the touch screen
  ts.begin();

  boolean isCalibrationAvailable = touchController.loadCalibration();
  if (!isCalibrationAvailable) {
    Serial.println("Calibration not available");
    touchController.startCalibration(&calibration);
    while (!touchController.isCalibrationFinished()) {
      touchController.continueCalibration();
      yield();
    }
    touchController.saveCalibration();
  }

  printFreeHeap("after touch screen initialized");

  String code = "";
  String grantType = "";
  String refreshToken = loadRefreshToken();
  if (refreshToken == "") {
    Serial.println("No refresh token found. Requesting through browser");
    gfx.fillBuffer(MINI_BLACK);
    gfx.setColor(MINI_YELLOW);
    gfx.setTextAlignment(TEXT_ALIGN_CENTER);
    gfx.setFont(ArialMT_Plain_16);
  
    gfx.drawString(120, 10, "Authentication required.\nOpen browser at\nhttp://" + espotifierNodeName + ".local");
    gfx.commit(0, 0);

    code = client.startConfigPortal(espotifierNodeName);
    grantType = "authorization_code";
  } else {
    Serial.println("Using refresh token found on the FS");
    code = refreshToken;
    grantType = "refresh_token";
  }
  client.getToken(&auth, grantType, code);
  Serial.printf("Refresh token: %s\nAccess Token: %s\n", auth.refreshToken.c_str(), auth.accessToken.c_str());
  if (auth.refreshToken != "") {
    saveRefreshToken(auth.refreshToken);
  }
  client.setDrawingCallback(&drawSongInfoCallback);
}

void loop() {
  if (millis() - lastUpdate > 1000) {
    uint16_t responseCode = client.update(&data, &auth);
    Serial.printf("HREF: %s\n", data.image300Href.c_str());
    lastUpdate = millis();
    Serial.printf("--------Response Code: %d\n", responseCode);
    Serial.printf("--------Free mem: %d\n", ESP.getFreeHeap());
    if (responseCode == 401) {
      client.getToken(&auth, "refresh_token", auth.refreshToken);
      if (auth.refreshToken != "") {
        saveRefreshToken(auth.refreshToken);
      }
    }
    if (responseCode == 200) {
      String selectedImageHref = data.image300Href;
      selectedImageHref.replace("https", "http");

      if (selectedImageHref != currentImageUrl) {

        Serial.println("New Image. Downloading it");

        isDownloadingCover = true;
        client.downloadFile(selectedImageHref, "/cover.jpg");
        isDownloadingCover = false;
        currentImageUrl = selectedImageHref;
        drawJPEGFromSpiffs("/cover.jpg", 45, 0);
        gfx.setColor(ILI9341_YELLOW);
      }
    }
    if (responseCode == 400) {
      gfx.fillBuffer(MINI_BLACK);
      gfx.setColor(MINI_WHITE);
      gfx.setTextAlignment(TEXT_ALIGN_CENTER);
      gfx.drawString(120, 20, "Please define\nclientId and clientSecret");
      gfx.commit(0, 160);
    }
  }
  drawSongInfo();

  if (touchController.isTouched(500) && millis() - lastTouchMillis > 1000) {
    TS_Point p = touchController.getPoint();
    lastTouchPoint = p;
    lastTouchMillis = millis();
    String command = "";
    String method = "";
    if (p.y > 160) {
      if (p.x < 80) {
        method = "POST";
        command = "previous";
      } else if (p.x >= 80 && p.x <= 160) {
        method = "PUT";
        command = "play";
        if (data.isPlaying) {
          command = "pause";
        }
        data.isPlaying = !data.isPlaying;
      } else if (p.x > 160) {
        method = "POST";
        command = "next";
      }
      uint16_t responseCode = client.playerCommand(&auth, method, command);
      Serial.print("playerCommand response =");
      Serial.println(responseCode);
    }
  }
}

// Set time via NTP, as required for x.509 validation
void setClock() {
  configTime(TIMEZONE, "pool.ntp.org");
  
  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}

void drawSongInfo() {
  if (millis() - lastDrawingUpdate < 333) {
    return;
  }
  lastDrawingUpdate = millis();
  long timeSinceUpdate = 0;
  if (data.isPlaying) {
    timeSinceUpdate = millis() - lastUpdate;
  }
  drawProgress(min(data.progressMs + timeSinceUpdate, data.durationMs), data.durationMs, data.title, data.artistName, data.isPlaying, data.isPlayerActive);
}

void drawProgress(uint64_t progressMs, uint64_t durationMs, String songTitle, String artistName, boolean isPlaying, boolean isPlayerActive) {
  counter++;

  if (isDownloadingCover) {
    gfx.fillBuffer(MINI_BLACK);
    gfx.setColor(MINI_WHITE);
    gfx.drawCircle(100, 80, 5);
    gfx.drawCircle(120, 80, 5);
    gfx.drawCircle(140, 80, 5);
    gfx.fillCircle(100 + 20 * (counter % 3), 80, 5);
    gfx.commit(0, 0);
  }

  gfx.fillBuffer(MINI_BLACK);
  if (isPlayerActive) {
    gfx.setTextAlignment(TEXT_ALIGN_LEFT);

    uint8_t percentage = 100.0 * progressMs / durationMs;
    gfx.setColor(MINI_WHITE);
    gfx.setTextAlignment(TEXT_ALIGN_LEFT);
    gfx.drawString(10, 70, formatTime(progressMs));
    gfx.setTextAlignment(TEXT_ALIGN_RIGHT);
    gfx.drawString(230, 70, formatTime(data.durationMs));
    gfx.setColor(MINI_WHITE);
    gfx.setColor(MINI_GRAY);
    uint16_t progressX = 10 + 220 * percentage / 100;
    uint16_t progressY = 65;
    gfx.drawLine(progressX, progressY, 230, progressY);
    gfx.setColor(MINI_WHITE);
    gfx.drawLine(10, progressY, progressX, progressY);
    gfx.fillCircle(progressX, progressY, 5);

    gfx.setTextAlignment(TEXT_ALIGN_LEFT);
    String animatedTitle = songTitle;
    uint8_t maxChar = 30;
    uint8_t excessChars = songTitle.length() - maxChar;
    uint8_t currentPos = counter % excessChars;
    if (songTitle.length() > maxChar) {
      animatedTitle = songTitle.substring(currentPos, currentPos + maxChar);
    }
    gfx.setTextAlignment(TEXT_ALIGN_CENTER);
    gfx.drawString(120, 10, animatedTitle);
    gfx.setColor(MINI_GRAY);
    gfx.drawString(120, 30, artistName);
    gfx.setColor(MINI_YELLOW);
    gfx.drawPalettedBitmapFromPgm(88, 90, isPlaying ? miniPause : miniPlay);

    gfx.drawPalettedBitmapFromPgm(30, 90, miniPrevious);
    gfx.drawPalettedBitmapFromPgm(190, 90, miniNext);
    if (millis() - lastTouchMillis < SHOW_TOUCH_POINT_MILLIS) {
      gfx.setColor(MINI_YELLOW);
      gfx.fillCircle(lastTouchPoint.x, lastTouchPoint.y - 160, 5);
    }
    gfx.commit(0, 160);
  } else {
    displayLogo();
  }
}

void displayLogo() {
  gfx.setTextAlignment(TEXT_ALIGN_CENTER);
  gfx.setFont(ArialMT_Plain_16);
  gfx.setColor(MINI_YELLOW);
  gfx.drawPalettedBitmapFromPgm(20, 20, ThingPulseLogo);
  gfx.drawString(120, 110, "ESPotify-Remote");
  gfx.commit(0, 0);
}

void calibrationCallback(int16_t x, int16_t y) {
  gfx.fillBuffer(MINI_BLACK);

  gfx.drawString(120, 10, "Please calibrate\ntouch screen by\ntouch point");
  gfx.setColor(MINI_WHITE);
  if (y < BUFFER_HEIGHT) {
    gfx.fillCircle(x, y, 10);
    gfx.commit(0, 0);
    gfx.fillBuffer(MINI_BLACK);
    gfx.commit(0, BUFFER_HEIGHT);
  } else {
    gfx.fillCircle(x, y - BUFFER_HEIGHT, 10);
    gfx.commit(0, BUFFER_HEIGHT);
    gfx.fillBuffer(MINI_BLACK);
    gfx.commit(0, 0);
  }
}

String formatTime(uint32_t time) {
  char time_str[10];
  uint8_t minutes = time / (1000 * 60);
  uint8_t seconds = (time / 1000) % 60;
  sprintf(time_str, "%2d:%02d", minutes, seconds);
  return String(time_str);
}

void saveRefreshToken(String refreshToken) {
  File f = SPIFFS.open("/refreshToken.txt", "w+");
  if (!f) {
    Serial.println("Failed to open config file");
    return;
  }
  f.println(refreshToken);
  f.close();
}

String loadRefreshToken() {
  Serial.println("Loading config");
  File f = SPIFFS.open("/refreshToken.txt", "r");
  if (!f) {
    Serial.println("Failed to open config file");
    return "";
  }
  while (f.available()) {
    // Lets read line by line from the file
    String token = f.readStringUntil('\r');
    Serial.printf("Refresh Token: %s\n", token.c_str());
    f.close();
    return token;
  }
  return "";
}

void drawJPEGFromSpiffs(String filename, int xpos, int ypos) {
  char buffer[filename.length() + 1];
  filename.toCharArray(buffer, filename.length() + 1);

  JpegDec.decodeFile(buffer);
  uint8_t zoomFactor = 2;
  uint16_t *pImg;
  uint16_t mcu_w = JpegDec.MCUWidth;
  uint16_t mcu_h = JpegDec.MCUHeight;
  Serial.printf("MCU W/H: %d, %d\n", mcu_w, mcu_h);
  // uint32_t mcu_pixels = mcu_w * mcu_h; // total pixels
  // TODO immplmenet something to track drawtime performance
  // uint32_t drawTime = millis();

  while (JpegDec.read()) {
    pImg = JpegDec.pImage;
    int mcu_x = (JpegDec.MCUx * mcu_w) / zoomFactor + xpos;
    int mcu_y = (JpegDec.MCUy * mcu_h) / zoomFactor + ypos;

    tft.setAddrWindow(mcu_x, mcu_y, mcu_x + (mcu_w / zoomFactor) - 1, mcu_y + (mcu_h / zoomFactor) - 1);

    for (uint8_t y = 0; y < mcu_h; y++) {
      for (uint8_t x = 0; x < mcu_w; x++) {
        if (x % zoomFactor == 0 && y % zoomFactor == 0) {
          tft.pushColor(*pImg);
        }
        *pImg++;
      }
    }
    drawSongInfo();
  }
}
