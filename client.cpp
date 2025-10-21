// Simple console client to send JSON command strings to the server.
// Usage: ./client <command>
// commands: start stop last next view
// "view" will spawn a gst-launch command to view UDP stream.

#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>

const char* SERVER_IP = "127.0.0.1";
const int SERVER_PORT = 5001;

bool send_json(const std::string& json) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error\n";
        return false;
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    if(inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr)<=0) {
        std::cerr << "Invalid address/ Address not supported\n";
        close(sock);
        return false;
    }
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection Failed\n";
        close(sock);
        return false;
    }
    send(sock, json.c_str(), json.size(), 0);
    close(sock);
    return true;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: client <start|stop|last|next|view>\n";
        return 0;
    }
    std::string cmd = argv[1];
    if (cmd == "start") {
        send_json("{\"command\":\"START_STREAM\"}");
    } else if (cmd == "stop") {
        send_json("{\"command\":\"STOP_STREAM\"}");
    } else if (cmd == "last") {
        send_json("{\"command\":\"RECORD_LAST_10\"}");
    } else if (cmd == "next") {
        send_json("{\"command\":\"RECORD_NEXT_10\"}");
    } else if (cmd == "view") {
        // spawn gst-launch viewer (receive RTP H264)
        std::string viewcmd = 
          "gst-launch-1.0 -v "
          "udpsrc port=5000 caps=\"application/x-rtp,media=video,encoding-name=H264,payload=96\" ! "
          "rtph264depay ! avdec_h264 ! videoconvert ! autovideosink sync=false";
        std::cout << "Launching viewer (gst-launch)...\n";
        // run in foreground so user can see video; if you want background append &
        int rc = system(viewcmd.c_str());
        (void)rc;
    } else {
        std::cout << "Unknown command\n";
    }
    return 0;
}
