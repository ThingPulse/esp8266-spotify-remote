# ThingPulse esp8266-spotify-remote

[![ThingPulse logo](https://thingpulse.com/assets/ThingPulse-w300.svg)](https://thingpulse.com)

## Purpose of this project

This project lets you control a Spotify player (phone, browser, etc) from an ESP8266. Album artworks as well as title and artist name
are fetched from Spotify's Web API over WiFi and displayed on a ILI9341 color TFT screen. The currently played song can be
paused, played and skipped to the next or previous song in the playlist. 

A full OAuth 2.0 web flow is used to acquire the necessary access and refresh tokens to permit the user to control the player. In order to
run this project on your device you will have to setup an application on Spotify's developer dashboard.

## Features

 - Artwork Download
 - Control Player on touch screen: Play, Pause, Next, Prev
 - Authentication and Authorization (OAuth 2.0 flow) On device. 

## Recommended Hardware

We developed this project specifically for our [ESP8266 Color Kit](https://thingpulse.com/product/esp8266-wifi-color-display-kit-2-4/). If you apreciate the hard work
and our willingnes to open-source projects like these please support us by buying our hardware.

<a href="https://thingpulse.com/product/esp8266-wifi-color-display-kit-2-4/">
  <img src="https://thingpulse.com/wp-content/uploads/2018/01/BoxSmall.jpeg" width="300">
</a>


## Contributions

Please see our [Guidelines](CONTRIBUTING.md) if you want to contribute to this project. Contributions are more than welcome!


## Setup Instructions

### Prepare Project in Arduino IDE

1. Download this project either as ZIP file or check it out with GIT
2. Open the project in the Arduino IDE
3. Set your WiFi credentials in the settings.h file
3. Install the MiniGrafx library (by Daniel Eichhorn, V 1.0.0 or later)
4. Install the JPEGDecoder library (by Bodmer, Makoto Kurauchi, Rich Geldreich, V 1.7.8 or later)
5. Install the JSON Streaming Parser library (by Daniel Eichhorn, V. 1.0.5 or later)
6. Install the XPT2046_Touchscreen (by Paul Stoffregen, V 1.2.0 or later)

### Get Access to the Spotify API

1. Go to (https://developer.spotify.com/dashboard/login) and login to the Spotify Developer Dashboard

2. Click on "My New App" 
<img src="/images/SpotifyDashboard.png" width="200">

3. Fill out the form. Give your new app a name you can attribute to this project 
<img src="/images/SpotifyAppSignUp1.png" width="200">

4. In the end of the 3 steps click on "Submit" 
<img src="/images/SpotifyppSignUp3.png" width="200">

5. Copy the Client ID and the Client Secret into the variables in the settings.h
<img src="/images/SpotifyCredentials.png" width="200">

6. Click on "Edit Settings". Add "http://esp8266.local/callback/" to Redirect URIs section
<img src="/images/SpotifyAppSettings.png" width="200">

7. Don't forget to save your settings.
<img src="/images/SpotifyAppSettingsSave.png" width="200">


### Compile and run the application

After all this configuration it's about time to run the application! 

1. Attach your [ESP8266 Color Kit](https://thingpulse.com/product/esp8266-wifi-color-display-kit-2-4/) to your computer and select the correct serial port in the Arduino IDE

2. Upload the code to your ESP8266

3. When you run this the first time you'll have to go through additional steps. The display will ask you to open the browser at a specific location. This will redirect you to the dialog below.
<img src="/images/SpotifyConnectScreen.png" width="200">

4. The next step is to calibrate the screen. Click on the white circles

5. Now open your spotify player and start a song. If everything worked out you'll see the song information with artwork on the TFT screen!
