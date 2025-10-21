# Video_Streaming
Its a video streaming project using gstreamer pipelines to records, save, exit on client orders.

# TCP-Controlled GStreamer Streaming Server

This project implements a **TCP server** that controls a GStreamer streaming pipeline and records video segments.  
The server accepts JSON commands from a client to start/stop streaming and save the last or next 10 seconds of video.

---

## Features

- Start a live video stream (simulated using `videotestsrc` or webcam)
- Stream encoded H264 video over UDP to `127.0.0.1:5000`
- Save rolling video segments (~10 seconds each) using `splitmuxsink`
- Commands supported via TCP JSON messages:
  - `{"command":"START_STREAM"}`
  - `{"command":"STOP_STREAM"}`
  - `{"command":"RECORD_LAST_10"}`
  - `{"command":"RECORD_NEXT_10"}`

---

## Requirements

- Linux environment
- C++17 compiler (g++ >= 9)
- GStreamer 1.18+ (`gst-launch-1.0`)
- CMake (optional, if building with CMake)
- Internet connection (for cloning repo & installing dependencies)

Install GStreamer on Ubuntu/Debian:

```bash
sudo apt update
sudo apt install gstreamer1.0-tools gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly

