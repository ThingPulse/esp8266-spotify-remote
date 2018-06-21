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

<img src="/images/SpotifyDashboard.png" width="500">

<img src="/images/SpotifyAppSignUp1.png" width="500">

<img src="/images/SpotifyppSignUp3.png" width="500">

<img src="/images/SpotifyCredentials.png" width="500">

<img src="/images/SpotifyAppSettings.png" width="500">

<img src="/images/SpotifyAppSettingsSave.png" width="500">

<img src="/images/SpotifyConnectScreen.png" width="500">
