/*
 *  HTTP over TLS (HTTPS) example sketch
 *
 *  This example demonstrates how to use
 *  WiFiClientSecure class to access HTTPS API.
 *  We fetch and display the status of
 *  esp8266/Arduino project continuous integration
 *  build.
 *
 *  Limitations:
 *    only RSA certificates
 *    no support of Perfect Forward Secrecy (PFS)
 *    TLSv1.2 is supported since version 2.4.0-rc1
 *
 *  Created by Ivan Grokhotkov, 2015.
 *  This example is in public domain.
 */

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include "SpotifyClient.h"
#include <MiniGrafx.h>
#include <Carousel.h>
#include <ILI9341_SPI.h>
#include <FS.h>
#include <JPEGDecoder.h>
#include "TouchControllerWS.h"
#include "util.h"
#include "images.h"

#define TFT_DC D2
#define TFT_CS D1
#define TFT_LED D8

#define TOUCH_CS D3
#define TOUCH_IRQ  D4

const char* ssid = "yourssid";
const char* password = "yourpassw0rd";

const char* host = "api.spotify.com";
const int httpsPort = 443;

// Use web browser to view and copy
// SHA1 fingerprint of the certificate
const char* fingerprint = "35 85 74 EF 67 35 A7 CE 40 69 50 F3 C0 F6 80 CF 80 3B 2E 19";
String token = "BQB28GFqKl1n8VStxjRezneZu3lDiE4SHeXIyjLe4RQzUsAD20jPl517y9bQqEtIU8B0MCri64AuU1M9huAXKkKS88PV4k9_tAH0jtoSQLOTjewfmNGrZb1pN4CiOoyL_X9tlWOICb285v46hLiMPcsF06GJe2t1SH-1";

#define MINI_BLACK 0
#define MINI_WHITE 1
#define MINI_YELLOW 2
#define MINI_GRAY 3

#define MAX_FORECASTS 12

// defines the colors usable in the paletted 16 color frame buffer
uint16_t palette[] = {ILI9341_BLACK, // 0
                      ILI9341_WHITE, // 1
                      ILI9341_YELLOW, // 2
                      0x840F
                      }; //3

int SCREEN_WIDTH = 240;
int SCREEN_HEIGHT = 320;
// Limited to 4 colors due to memory constraints
int BITS_PER_PIXEL = 2; // 2^2 =  4 colors

ADC_MODE(ADC_VCC);

XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);
TouchControllerWS touchController(&ts);

ILI9341_SPI tft = ILI9341_SPI(TFT_CS, TFT_DC);
MiniGrafx gfx = MiniGrafx(&tft, BITS_PER_PIXEL, palette, 240, 160);
MiniGrafx gfxImage = MiniGrafx(&tft, 16, palette, 8, 8);

SpotifyClient client;
SpotifyData data;
SpotifyAuth auth;

String currentImageUrl = "";
uint32_t lastDrawingUpdate = 0;

void drawJPEGFromSpiffs(String filename, MiniGrafx *gfx, int xpos, int ypos, DrawingCallback *drawingCallback);
void calibrationCallback(int16_t x, int16_t y);
CalibrationCallback calibration = &calibrationCallback;

void drawSongInfo();
DrawingCallback drawSongInfoCallback = &drawSongInfo;

uint16_t refreshTokenUsed = 0;

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
  while(f.available()) {
      //Lets read line by line from the file
      String token = f.readStringUntil('\r');
      Serial.printf("Refresh Token: %s\n", token.c_str());
      f.close();
      return token;
  }
  return "";
}

String formatTime(uint32_t time) {
  char time_str[10];
  uint8_t minutes = time / (1000 * 60);
  uint8_t seconds = (time / 1000) % 60;
  sprintf(time_str, "%2d:%02d", minutes, seconds);
  return String(time_str);
}




void setup() {
  Serial.begin(115200);

  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, HIGH);    // HIGH to Turn on;
  gfx.init();
    boolean mounted = SPIFFS.begin();
  if (!mounted) {
    Serial.println("FS not formatted. Doing that now");
    SPIFFS.format();
    Serial.println("FS formatted...");
    SPIFFS.begin();
  }

  ts.begin();

  //SPIFFS.remove("/calibration.txt");
  boolean isCalibrationAvailable = touchController.loadCalibration();
  if (!isCalibrationAvailable) {
    Serial.println("Calibration not available");
    touchController.startCalibration(&calibration);
    while (!touchController.isCalibrationFinished()) {
      gfx.fillBuffer(0);
      gfx.setColor(MINI_YELLOW);
      gfx.setTextAlignment(TEXT_ALIGN_CENTER);
      gfx.drawString(120, 160, "Please calibrate\ntouch screen by\ntouch point");
      touchController.continueCalibration();
      gfx.commit();
      yield();
    }
    touchController.saveCalibration();
  }

  client.setDrawingCallback(&drawSongInfoCallback);

  gfx.fillBuffer(MINI_BLACK);
  gfx.commit(0, 0);
  gfx.commit(0, 160);
  
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
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  String code = "AQCHFrajnotCtcr0CZykumH5-j6Ifh3CwRUC1Zc9piXcg4KCBcpECwbUhIhmDC5wAXsyZEhQ8WT1EP1pp9WYyWGtmawhpPcbT1ip8_afCyz7DYNQ1orQLhnkTu6y9p4du4G7epFn-ntER3P1yEypUZYB_51hAMqFA6SBELcdIzXatp9vJzK6h2VvqUoRoBKhj3Y9KI0WnaQ_f55ZqRzrWrC07nP3T20m8Pe_vnFS9eprvyjFWytmu10xFTglZUR7PwU1-Sa2zmT6_RAvjZ30ftMsqUA70CNIwnWaojxFOzDPQqqwYNVvF0N6Zg5t_aThdCfq6U-AzQ";  String grantType = "authorization_code";
  String refreshToken = loadRefreshToken();
  if (refreshToken != "") {
    Serial.println("Using refresh token found on the FS");
    code = refreshToken;
    grantType = "refresh_token";
  }
  client.getToken(&auth, "2ddeb46ba2734ecbaaf04f50136edb62", "1a0bbcf52f0f41c9bb2d1d00dff0acf9", "http%3A%2F%2Fhttpbin.org%2Fanything", grantType, code);
  Serial.printf("Refresh token: %s\nAccess Token: %s\n", auth.refreshToken.c_str(), auth.accessToken.c_str());
  if (auth.refreshToken != "") {
    saveRefreshToken(auth.refreshToken);
  }



}
uint16_t lastFreeMem = 0;
uint16_t counter = 0;
long lastUpdate = 0;
TS_Point lastTouchPoint;
void loop() {


    if (millis() - lastUpdate > 1000) {
      uint16_t responseCode = client.update(&data, &auth);
      Serial.printf("HREF: %s\n", data.image300Href.c_str());
      lastUpdate = millis();
      Serial.printf("--------Response Code: %d\n", responseCode);
      Serial.printf("--------Free mem: %d\n", ESP.getFreeHeap());
      if (responseCode == 401) {
        refreshTokenUsed++;
        client.getToken(&auth, "2ddeb46ba2734ecbaaf04f50136edb62", "1a0bbcf52f0f41c9bb2d1d00dff0acf9", "http%3A%2F%2Fhttpbin.org%2Fanything", "refresh_token", auth.refreshToken);
        if (auth.refreshToken != "") {
          saveRefreshToken(auth.refreshToken);
        }
      }
      if (responseCode == 200) {
         Serial.printf("640: %s\n", data.image640Href.c_str());
         Serial.printf("300: %s\n", data.image300Href.c_str());
         Serial.printf("64: %s\n", data.image64Href.c_str());

        String selectedImageHref = data.image300Href;
        selectedImageHref.replace("https", "http");
        
        
        if (selectedImageHref != currentImageUrl) {
          gfx.fillBuffer(MINI_BLACK);
          gfx.commit(0,0);
          Serial.println("New Image. Downloading it");
          gfxImage.init();
          client.downloadFile(selectedImageHref, "/cover.jpg");
          currentImageUrl = selectedImageHref;
          drawJPEGFromSpiffs("/cover.jpg", &gfxImage, 45, 10, &drawSongInfoCallback);
          gfx.setColor(ILI9341_YELLOW);
          gfx.init();
        }
  
  
        
  
  
      }

    }
    drawSongInfo();
    
    if (touchController.isTouched(500)) {
      TS_Point p = touchController.getPoint();
      lastTouchPoint = p;
      String command = "";
      String method = "";
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
    }
    counter++;
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
    //gfx.drawString(120, 100, isPlaying ? "Pause" : "Play");
    gfx.drawPalettedBitmapFromPgm(88, 90, isPlaying ?  miniPause : miniPlay);
    gfx.fillCircle(lastTouchPoint.x, lastTouchPoint.y - 160, 5);
    gfx.drawPalettedBitmapFromPgm(40, 90, miniPrevious);
    gfx.drawPalettedBitmapFromPgm(190, 90, miniNext);
    //gfx.drawString(40, 100, "Prev");
    //gfx.drawString(200, 100, "Next");
  } else {
    gfx.setTextAlignment(TEXT_ALIGN_CENTER);
    gfx.setFont(ArialMT_Plain_16);
    gfx.setColor(MINI_YELLOW);
    gfx.drawString(120, 10, "No player active");
  }
  gfx.commit(0, 160);
}

void calibrationCallback(int16_t x, int16_t y) {
  gfx.setColor(1);
  gfx.fillCircle(x, y, 10);
}

void drawJPEGFromSpiffs(String filename, MiniGrafx *gfx, int xpos, int ypos, DrawingCallback *drawingCallback) {
    Serial.println(filename);
    char buffer[filename.length() + 1];
    filename.toCharArray(buffer, filename.length() + 1);
  
    JpegDec.decodeFile(buffer);
  
    uint16_t *pImg;
    int x,y,bx,by;
    uint16_t mcu_w = JpegDec.MCUWidth;
    uint16_t mcu_h = JpegDec.MCUHeight;
    
    // Output CSV
    //sprintf(str,"#SIZE,%d,%d",JpegDec.width,JpegDec.height);
    //Serial.println(str);
    long lastDrawingUpdate = 0;

    while(JpegDec.read()){
        pImg = JpegDec.pImage ;
        int mcu_x = JpegDec.MCUx * (mcu_w);
        int mcu_y = JpegDec.MCUy * (mcu_h);
        for(by=0; by<JpegDec.MCUHeight; by++){
            yield();
        
            for(bx=0; bx<JpegDec.MCUWidth; bx++){
                yield();
            
                x = JpegDec.MCUx * JpegDec.MCUWidth + bx;
                y = JpegDec.MCUy * JpegDec.MCUHeight + by;
                
                if(x<JpegDec.width && y<JpegDec.height){

                    if(JpegDec.comps == 1){ // Grayscale
                    
                        //sprintf(str,"#RGB,%d,%d,%u", x, y, pImg[0]);
                        //Serial.println(str);
                        gfx->setColor(*pImg);
                        gfx->setPixel(bx * 0.5 , by * 0.5);

                    }else{ // RGB

                        //gfx->setColor(color565(pImg[0], pImg[1], pImg[2]));
                        //gfx->setPixel(x , y);

//                        sprintf(str,"#RGB,%d,%d,%u,%u,%u", x, y, pImg[0], pImg[1], pImg[2]);
//                        Serial.println(str);
                    }
                }
                pImg += 1; //JpegDec.comps ;
            }
        }
        gfx->commit(mcu_x * 0.5 + xpos, mcu_y * 0.5 + ypos);
        drawSongInfo();

    }
  

}
