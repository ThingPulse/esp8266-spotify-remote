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
#include <ESP8266HTTPClient.h>
#include "SpotifyClient.h"

#define min(X, Y) (((X)<(Y))?(X):(Y))



SpotifyClient::SpotifyClient(String clientId, String clientSecret, String redirectUri) {
  this->clientId = clientId;
  this->clientSecret = clientSecret;
  this->redirectUri = redirectUri;
}


uint16_t SpotifyClient::update(SpotifyData *data, SpotifyAuth *auth) {
  this->data = data;

  level = 0;
  isDataCall = true;
  currentParent = "";
  WiFiClientSecure client = WiFiClientSecure();
  JsonStreamingParser parser;
  parser.setListener(this);

  String host = "api.spotify.com";
  const int port = 443;
  String url = "/v1/me/player/currently-playing";
  if (!client.connect(host.c_str(), port)) {
    Serial.println("connection failed");
    return 0;
  }

   Serial.print("Requesting URL: ");
  //Serial.println(url);
  String request = "GET " + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Authorization: Bearer " + auth->accessToken + "\r\n" +
               "Connection: close\r\n\r\n";
  // This will send the request to the server
  Serial.println(request);
  client.print(request);
  
  int retryCounter = 0;
  while(!client.available()) {
    executeCallback();
    retryCounter++;
    if (retryCounter > 10) {
      return 0;
    }
    delay(10);
  }
  uint16_t bufLen = 1024;
  unsigned char buf[bufLen];
  boolean isBody = false;
  char c = ' ';

  int size = 0;
  client.setNoDelay(false);
  // while(client.connected()) {
  uint16_t httpCode = 0;
  while(client.connected() || client.available()) {
    while((size = client.available()) > 0) {
   
      if (isBody) {
        uint16_t len = min(bufLen, size);
        c = client.readBytes(buf, len);
        for (uint16_t i = 0; i < len; i++) {
          parser.parse(buf[i]);
          //Serial.print((char)buf[i]);
        }
      } else {
        String line = client.readStringUntil('\r');
        Serial.println(line);
        if (line.startsWith("HTTP/1.")) {
          httpCode = line.substring(9, line.indexOf(' ', 9)).toInt();
          Serial.printf("HTTP Code: %d\n", httpCode); 
        }
        if (line == "\r" || line == "\n" || line == "") {
          Serial.println("Body starts now");
          isBody = true;
        }
      }
      executeCallback();
    }
  }
  if (httpCode == 200) {
    this->data->isPlayerActive = true;
  } else if (httpCode == 204) {
    this->data->isPlayerActive = false;
  }
  //client.flush();
  //client.stop();
  this->data = nullptr;
  return httpCode;
}


uint16_t SpotifyClient::playerCommand(SpotifyAuth *auth, String method, String command) {

  level = 0;
  isDataCall = true;
  currentParent = "";
  WiFiClientSecure client = WiFiClientSecure();
  JsonStreamingParser parser;
  parser.setListener(this);

  String host = "api.spotify.com";
  const int port = 443;
  String url = "/v1/me/player/" + command;
  if (!client.connect(host.c_str(), port)) {
    Serial.println("connection failed");
    return 0;
  }

   Serial.print("Requesting URL: ");
  //Serial.println(url);
  String request = method + " " + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Authorization: Bearer " + auth->accessToken + "\r\n" +
               "Content-Length: 0\r\n" + 
               "Connection: close\r\n\r\n";
  // This will send the request to the server
  Serial.println(request);
  client.print(request);
  
  int retryCounter = 0;
  while(!client.available()) {
    executeCallback();

    retryCounter++;
    if (retryCounter > 10) {
      return 0;
    }
    delay(10);
  }
  uint16_t bufLen = 1024;
  unsigned char buf[bufLen];
  boolean isBody = false;
  char c = ' ';

  int size = 0;
  client.setNoDelay(false);
  // while(client.connected()) {
  uint16_t httpCode = 0;
  while(client.connected() || client.available()) {
    while((size = client.available()) > 0) {
      if (isBody) {
        uint16_t len = min(bufLen, size);
        c = client.readBytes(buf, len);
        for (uint16_t i = 0; i < len; i++) {
          parser.parse(buf[i]);
          //Serial.print((char)buf[i]);
        }
      } else {
        String line = client.readStringUntil('\r');
        Serial.println(line);
        if (line.startsWith("HTTP/1.")) {
          httpCode = line.substring(9, line.indexOf(' ', 9)).toInt();
          Serial.printf("HTTP Code: %d\n", httpCode); 
        }
        if (line == "\r" || line == "\n" || line == "") {
          Serial.println("Body starts now");
          isBody = true;
        }
      }
    }
    executeCallback();
  }
  return httpCode;
}

void SpotifyClient::getToken(SpotifyAuth *auth, String grantType, String code) {
  this->auth = auth;
  isDataCall = false;
  JsonStreamingParser parser;
  parser.setListener(this);
  WiFiClientSecure client;
  //https://accounts.spotify.com/api/token
  const char* host = "accounts.spotify.com";
  const int port = 443;
  String url = "/api/token";
  if (!client.connect(host, port)) {
    Serial.println("connection failed");
    return;
  }

  Serial.print("Requesting URL: ");
  //Serial.println(url);
  String codeParam = "code";
  if (grantType == "refresh_token") {
    codeParam = "refresh_token"; 
  }
  String authorizationRaw = clientId + ":" + clientSecret;
  String authorization = base64::encode(authorizationRaw, false);
  // This will send the request to the server
  String content = "grant_type=" + grantType + "&" + codeParam + "=" + code + "&redirect_uri=" + redirectUri;
  String request = String("POST ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Authorization: Basic " + authorization + "\r\n" +
               "Content-Length: " + String(content.length()) + "\r\n" + 
               "Content-Type: application/x-www-form-urlencoded\r\n" + 
               "Connection: close\r\n\r\n" + 
               content;
  Serial.println(request);
  client.print(request);
      

  int retryCounter = 0;
  while(!client.available()) {
    executeCallback();
    retryCounter++;
    if (retryCounter > 10) {
      return;
    }
    delay(10);
  }

  int pos = 0;
  boolean isBody = false;
  char c;

  int size = 0;
  client.setNoDelay(false);
  while(client.connected() || client.available()) {
    while((size = client.available()) > 0) {
      c = client.read();
      if (c == '{' || c == '[') {
        isBody = true;
      }
      if (isBody) {
        parser.parse(c);
        Serial.print(c);
      } else {
        Serial.print(c);
      }
    }
    executeCallback();
  }

  this->data = nullptr;
}

String SpotifyClient::startConfigPortal() {
  String oneWayCode = "";

  server.on ( "/", [this]() {
    Serial.println(this->clientId);
    Serial.println(this->redirectUri);
    server.sendHeader("Location", String("https://accounts.spotify.com/authorize/?client_id=" 
      + this->clientId 
      + "&response_type=code&redirect_uri=" 
      + this->redirectUri 
      + "&scope=user-read-private%20user-read-currently-playing%20user-read-playback-state%20user-modify-playback-state"), true);
    server.send ( 302, "text/plain", "");
  } );

  server.on ( "/callback/", [this, &oneWayCode](){
    if(!server.hasArg("code")) {server.send(500, "text/plain", "BAD ARGS"); return;}
    
    oneWayCode = server.arg("code");
    Serial.printf("Code: %s\n", oneWayCode.c_str());
  
    String message = "<html><head></head><body>Succesfully authentiated This device with Spotify. Restart your device now</body></html>";
  
    server.send ( 200, "text/html", message );
  } );

  server.begin();

  if (WiFi.status() == WL_CONNECTED) {
	  Serial.println("WiFi connected!");
  } else {
	  Serial.println("WiFi not connected!");
  }

  Serial.println ( "HTTP server started" );

  while(oneWayCode == "") {
    server.handleClient();
    yield();
  }
  server.stop();
  return oneWayCode;
}

void SpotifyClient::whitespace(char c) {
  Serial.println("whitespace");
}

void SpotifyClient::startDocument() {
  Serial.println("start document");
  level = 0;
}

void SpotifyClient::key(String key) {
  currentKey = String(key);
  rootPath[level] = key;
  //Serial.println(getRootPath());
}

String SpotifyClient::getRootPath() {
  String path = "";
  for (uint8_t i = 1; i <= level; i++) {
    String currentLevel = rootPath[i];
    if (currentLevel == "") {
      break;
    }
    if (i > 1) {
      path += ".";
    }
    path += currentLevel;
  }
  return path;
}

void SpotifyClient::value(String value) {
  if (isDataCall) {
    
    String rootPath = this->getRootPath();
    //Serial.printf("%s = %s\n", rootPath.c_str(), value.c_str());
    //Serial.printf("%s = %s\n", currentKey.c_str(), value.c_str());
      // progress_ms = 37516 uint32_t progressMs;
    if (currentKey == "progress_ms") {
      data->progressMs = value.toInt();
    }
    // duration_ms = 259120 uint32_t durationMs;
    if (currentKey == "duration_ms") {
      data->durationMs = value.toInt();
    }
    // name = Lost in My MindString name;
    if (currentKey == "name") {
      data->title = value;
    }
    // is_playing = true boolean isPlaying;
    if (currentKey == "is_playing") {
      data->isPlaying = (value == "true" ? true : false);
    }
    if (currentKey == "height") {
      currentImageHeight = value.toInt();
    }
    if (currentKey == "url") {
      Serial.printf("HREF: %s = %s", rootPath.c_str(), value.c_str());

      if (rootPath == "item.album.images.url") {
        if (currentImageHeight == 640) {
          data->image640Href = value;
        }
        if (currentImageHeight > 250 && currentImageHeight < 350) {
          data->image300Href = value;
        }
        if (currentImageHeight == 64) {
          data->image64Href = value;
        }
      }
    }
    if (rootPath == "item.album.artists.name") {
      data->artistName = value;
    }

  } else {
    Serial.printf("\n%s=%s\n", currentKey.c_str(), value.c_str());
    // "access_token": "XXX" String accessToken;
    if (currentKey == "access_token") {
      auth->accessToken = value;
    }
    // "token_type":"Bearer", String tokenType;
    if (currentKey == "token_type") {
      auth->tokenType = value;
    }
    // "expires_in":3600, uint16_t expiresIn;
    if (currentKey == "expires_in") {
      auth->expiresIn = value.toInt();
    }
    // "refresh_token":"XX", String refreshToken;
    if (currentKey == "refresh_token") {
      auth->refreshToken = value;
    }
    // "scope":"user-modify-playback-state user-read-playback-state user-read-currently-playing user-read-private String scope;
    if (currentKey == "scope") {
      auth->scope = value;
    }
  }
}

void SpotifyClient::endArray() {

}


void SpotifyClient::startObject() {
  //Serial.println("Starting new object: " + currentKey);
  currentParent = currentKey;
  //rootPath[level] = currentKey;
  level++;
  
  //level++;*/
}

void SpotifyClient::endObject() {
  //rootPath[level] = "";
  level--;
  currentParent = "";
}

void SpotifyClient::endDocument() {

}

void SpotifyClient::startArray() {

}

void SpotifyClient::executeCallback() {
    if (drawingCallback != nullptr) {
      (*this->drawingCallback)();
    }
}

void SpotifyClient::downloadFile(String url, String filename) {
    Serial.println("Downloading " + url + " and saving as " + filename);

    // wait for WiFi connection
    // TODO - decide if there's a different action for first call or subsequent calls
	// boolean isFirstCall = true;
    HTTPClient http;

    Serial.print("[HTTP] begin...\n");

    // configure server and url
    http.begin(url);

    Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    int httpCode = http.GET();
    if(httpCode > 0) {
        //SPIFFS.remove(filename);
        fs::File f = SPIFFS.open(filename, "w+");
        if (!f) {
            Serial.println("file open failed");
            return;
        }
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);

        // file found at server
        if(httpCode == HTTP_CODE_OK) {

            // get lenght of document (is -1 when Server sends no Content-Length header)
            int total = http.getSize();
            int len = total;
            //progressCallback(filename, 0,total, true);
            // create buffer for read
            uint8_t buff[128] = { 0 };

            // get tcp stream
            WiFiClient * stream = http.getStreamPtr();

            // read all data from server
            while(http.connected() && (len > 0 || len == -1)) {
                // get available data size
                size_t size = stream->available();

                if(size) {
                    // read up to 128 byte
                    int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));

                    // write it to Serial
                    f.write(buff, c);

                    if(len > 0) {
                        len -= c;
                    }
                    //progressCallback(filename, total - len,total, false);
                    // isFirstCall = false;
                    executeCallback();
                }
                delay(1);
            }

            Serial.println();
            Serial.print("[HTTP] connection closed or file end.\n");

        }
        f.close();
    } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    
    http.end();
    
}
