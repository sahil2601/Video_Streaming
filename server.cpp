// Simple TCP server that accepts JSON commands and controls gst-launch processes.
// Commands:
//   {"command":"START_STREAM"}
//   {"command":"STOP_STREAM"}
//   {"command":"RECORD_LAST_10"}
//   {"command":"RECORD_NEXT_10"}
// Streams to localhost UDP 5000 and saves 10-second segments to "segments" folder.

#include <arpa/inet.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <signal.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <vector>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <algorithm>

namespace fs = std::filesystem;
using namespace std::chrono;

const int PORT = 5001; // TCP port for commands
const std::string STREAM_PID_FILE = "/tmp/gst_stream.pid";
const std::string SEGMENTS_DIR = "segments";
const std::string RECORDINGS_DIR = "recordings";

// helper: get timestamp string
std::string ts_now() {
    auto t = system_clock::now();
    std::time_t tt = system_clock::to_time_t(t);
    std::tm tm = *std::localtime(&tt);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return oss.str();
}

void ensure_dirs() {
    if (!fs::exists(SEGMENTS_DIR)) fs::create_directory(SEGMENTS_DIR);
    if (!fs::exists(RECORDINGS_DIR)) fs::create_directory(RECORDINGS_DIR);
}

// start streaming: test video -> encode -> UDP + splitmuxsink segments
void start_stream() {
    ensure_dirs();
    if (fs::exists(STREAM_PID_FILE)) {
        std::cout << "Stream already running.\n";
        return;
    }

    
    std::string pipeline = "gst-launch-1.0 -e -v " 
   "videotestsrc pattern=ball num-buffers=300 ! videoconvert ! video/x-raw,width=640,height=480,framerate=30/1 ! "      
   "x264enc tune=zerolatency bitrate=500 speed-preset=ultrafast ! h264parse ! tee name=t " 
   "t. ! queue ! rtph264pay ! udpsink host=127.0.0.1 port=5000 sync=false async=false " 
   "t. ! queue ! splitmuxsink location=" + SEGMENTS_DIR + "/segment%03d.mp4 max-size-time=10000000000"; 
 
// run in background and save pid
 std::string cmd = pipeline + " > /tmp/gst_stream.log 2>&1 & echo $! > " + STREAM_PID_FILE; 
int rc = system(cmd.c_str()); 
if (rc == -1) { 
std::cerr << "Failed to start gst-launch pipeline\n"; return; 
} 
std::cout << "Started stream (pid file: " << STREAM_PID_FILE << ").\n";
}

// stop streaming by PID
void stop_stream() {
    if (!fs::exists(STREAM_PID_FILE)) {
        std::cout << "Stream not running.\n";
        return;
    }
    std::ifstream in(STREAM_PID_FILE);
    pid_t pid;
    in >> pid;
    in.close();
    if (pid > 0) {
        std::string killcmd = "kill " + std::to_string(pid) + " 2>/dev/null || true";
        system(killcmd.c_str());
        system("pkill -f gst-launch-1.0 2>/dev/null || true");
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    fs::remove(STREAM_PID_FILE);
    std::cout << "Stopped stream.\n";
}

// return segment files sorted by last modified time descending
std::vector<fs::path> list_segments_sorted() {
    std::vector<fs::path> v;
    if (!fs::exists(SEGMENTS_DIR)) return v;
    for (auto &p: fs::directory_iterator(SEGMENTS_DIR)) {
        if (p.path().extension() == ".mp4") v.push_back(p.path());
    }
    std::sort(v.begin(), v.end(), [](const fs::path& a, const fs::path& b){
        return fs::last_write_time(a) > fs::last_write_time(b);
    });
    return v;
}

// copy latest segment as last 10s
void record_last_10() {
    auto segs = list_segments_sorted();
    if (segs.empty()) {
        std::cout << "No segments available.\n";
        return;
    }
    auto latest = segs.front();
    std::string out = RECORDINGS_DIR + "/last_" + ts_now() + ".mp4";
    try {
        fs::copy_file(latest, out);
        std::cout << "Copied last 10s: " << out << "\n";
    } catch (const std::exception &e) {
        std::cerr << "Failed to copy: " << e.what() << "\n";
    }
}

// copy next segment after 10s (simulate next 10s recording)
void record_next_10() {
    std::this_thread::sleep_for(std::chrono::seconds(10)); // wait 10s for next segment to appear
    auto segs = list_segments_sorted();
    if (segs.empty()) {
        std::cout << "No segments available yet.\n";
        return;
    }
    auto latest = segs.front();
    std::string out = RECORDINGS_DIR + "/next_" + ts_now() + ".mp4";
    try {
        fs::copy_file(latest, out);
        std::cout << "Saved next 10s segment: " << out << "\n";
    } catch (const std::exception &e) {
        std::cerr << "Failed to save next 10s: " << e.what() << "\n";
    }
}

void handle_command(const std::string& cmd) {
    if (cmd.find("START_STREAM") != std::string::npos) {
        start_stream();
    } else if (cmd.find("STOP_STREAM") != std::string::npos) {
        stop_stream();
    } else if (cmd.find("RECORD_LAST_10") != std::string::npos) {
        record_last_10();
    } else if (cmd.find("RECORD_NEXT_10") != std::string::npos) {
        record_next_10();
    } else {
        std::cout << "Unknown command: " << cmd << "\n";
    }
}

int main() {
    ensure_dirs();
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address))<0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    std::cout << "Command server listening on 0.0.0.0:" << PORT << "\n";

    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen))<0) {
            perror("accept");
            continue;
        }
        char buffer[4096] = {0};
        ssize_t valread = read(new_socket, buffer, sizeof(buffer)-1);
        if (valread > 0) {
            std::string s(buffer, buffer + valread);
            std::cout << "Received: " << s << "\n";
            handle_command(s);
        }
        close(new_socket);
    }

    return 0;
}
