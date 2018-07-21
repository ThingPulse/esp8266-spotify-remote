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

#pragma once
#include <Arduino.h>
#include <JsonListener.h>
#include <JsonStreamingParser.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <base64.h>

typedef void (*DrawingCallback)();

typedef struct SpotifyAuth {
  // "access_token": "XXX"
  String accessToken;
   // "token_type":"Bearer",
  String tokenType;
  // "expires_in":3600,
  uint16_t expiresIn;
  // "refresh_token":"XX",
  String refreshToken;
  // "scope":"user-modify-playback-state user-read-playback-state user-read-currently-playing user-read-private
  String scope;

} SpotifyAuth;

typedef struct SpotifyData {
  // timestamp = 1527690461156
  // progress_ms = 37516
  uint32_t progressMs;
  // is_playing = true
  boolean isPlaying;
  boolean isPlayerActive;
  // Starting new object: item
  // Starting new object: album
  // album_type = album
  // Starting new object: artists
  // Starting new object: external_urls
  // spotify = https://open.spotify.com/artist/0n94vC3S9c3mb2HyNAOcjg
  // href = https://api.spotify.com/v1/artists/0n94vC3S9c3mb2HyNAOcjg
  // id = 0n94vC3S9c3mb2HyNAOcjg
  // name = The Head and the Heart item.album.artists.name
  String artistName;
  // type = artist
  // uri = spotify:artist:0n94vC3S9c3mb2HyNAOcjg
  // Starting new object: external_urls
  // spotify = https://open.spotify.com/album/0xWfhCMYmaiCXtLOuyPoLF
  // href = https://api.spotify.com/v1/albums/0xWfhCMYmaiCXtLOuyPoLF
  String image640Href;
  String image300Href;
  String image64Href;
  // id = 0xWfhCMYmaiCXtLOuyPoLF
  // Starting new object: images
  // height = 639
  // url = https://i.scdn.co/image/c08c25d478dc3a645c04abc000602201ce1634d8
  // width = 630
  // Starting new object: width
  // height = 300
  // url = https://i.scdn.co/image/6b0e2580ede853b1173198693301f0df1979b5cf
  // width = 296
  // Starting new object: width
  // height = 64
  // url = https://i.scdn.co/image/7d20358db1b1502a09a05f860ba9d3641e552f8d
  // width = 63
  // name = The Head and the Heart
  // release_date = 2010
  // release_date_precision = year
  // type = album
  // uri = spotify:album:0xWfhCMYmaiCXtLOuyPoLF
  // Starting new object: artists
  // Starting new object: external_urls
  // spotify = https://open.spotify.com/artist/0n94vC3S9c3mb2HyNAOcjg
  // href = https://api.spotify.com/v1/artists/0n94vC3S9c3mb2HyNAOcjg
  // id = 0n94vC3S9c3mb2HyNAOcjg
  // name = The Head and the Heart
  // type = artist
  // uri = spotify:artist:0n94vC3S9c3mb2HyNAOcjg
  // disc_number = 1
  // duration_ms = 259120
  uint32_t durationMs;
  // explicit = false
  // Starting new object: external_ids
  // isrc = USSUB1191507
  // Starting new object: external_urls
  // spotify = https://open.spotify.com/track/3gvAGvbMCRvVDDp8ZaIPV5
  // href = https://api.spotify.com/v1/tracks/3gvAGvbMCRvVDDp8ZaIPV5
  // id = 3gvAGvbMCRvVDDp8ZaIPV5
  // is_local = false
  // name = Lost in My Mind
  String title;
  // popularity = 68
  // preview_url = null
  // track_number = 7
  // type = track
  // uri = spotify:track:3gvAGvbMCRvVDDp8ZaIPV5
  // Starting new object: context
  // Starting new object: external_urls
  // spotify = https://open.spotify.com/user/spotify/playlist/37i9dQZF1DWUNIrSzKgQbP
  // href = https://api.spotify.com/v1/users/spotify/playlists/37i9dQZF1DWUNIrSzKgQbP
  // type = playlist
  // uri = spotify:user:spotify:playlist:37i9dQZF1DWUNIrSzKgQbP
  

} SpotifyData;

class SpotifyClient: public JsonListener {
  private:

  String currentKey;
  String currentParent;
  SpotifyData *data;
  SpotifyAuth *auth;
  bool isDataCall;
  String rootPath[10];
  uint8_t level = 0;
  uint16_t currentImageHeight;
  DrawingCallback *drawingCallback;
  String clientId;
  String clientSecret;
  String redirectUri;
  ESP8266WebServer server;

  String getRootPath();
  void executeCallback();

  public:
    SpotifyClient(String clientId, String clientSecret, String redirectUri);
    uint16_t update(SpotifyData *data, SpotifyAuth *auth);

    uint16_t playerCommand(SpotifyAuth *auth, String method, String command);

    void getToken(SpotifyAuth *auth, String grantType, String code);

    void downloadFile(String url, String filename);

    void setDrawingCallback(DrawingCallback *drawingCallback) {
      this->drawingCallback = drawingCallback;
    }

    String startConfigPortal();



    virtual void whitespace(char c);

    virtual void startDocument();

    virtual void key(String key);

    virtual void value(String value);

    virtual void endArray();

    virtual void endObject();

    virtual void endDocument();

    virtual void startArray();

    virtual void startObject();

    
};

