#include <iostream>
#include <cstring>      // for memset, memcpy
#include <sys/socket.h> // for socket functions
#include <netinet/in.h> // for sockaddr_in, ntohl, ntohs
#include <arpa/inet.h>  // for inet_ntoa
#include <unistd.h>     // for close

// 1. Define the shared struct exactly as it is on the ESP side
// __attribute__((packed)) is crucial to match the 26-byte size
struct __attribute__((packed)) TelemetryPacket {
    uint32_t timestamp;     // Time (millis)
    int32_t steps;          // Encoder steps
    int32_t direction;      // Encoder direction
    int16_t ax, ay, az;     // MPU6050 Accel
    int16_t gx, gy, gz;     // MPU6050 Gyro
    uint8_t button;         // Encoder button (1/0)
    uint8_t ir_detected;    // IR Sensor (1/0)
};

int main() {
    const int UDP_PORT = 20025; // Must match sender's 'udpPort'
    const int BUFFER_SIZE = 1024;

    // 2. Create socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return 1;
    }

    // 3. Bind to port
    struct sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Listen on 0.0.0.0
    serverAddr.sin_port = htons(UDP_PORT);

    if (bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error binding socket: " << strerror(errno) << std::endl;
        close(sockfd);
        return 1;
    }

    std::cout << "Listening for UDP packets on port " << UDP_PORT << "..." << std::endl;
    std::cout << "Expected Packet Size: " << sizeof(TelemetryPacket) << " bytes." << std::endl;

    uint8_t buffer[BUFFER_SIZE];
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);

    while (true) {
        // 4. Receive Data
        ssize_t bytesReceived = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                                         (struct sockaddr*)&clientAddr, &clientAddrLen);

        if (bytesReceived < 0) {
            std::cerr << "Recvfrom error" << std::endl;
            continue;
        }

        // 5. Process Packet
        if (bytesReceived == sizeof(TelemetryPacket)) {
            // Copy buffer into struct to avoid alignment issues
            TelemetryPacket packet;
            std::memcpy(&packet, buffer, sizeof(TelemetryPacket));

            // 6. Convert Endianness (Network -> Host)
            // ntohl for 32-bit, ntohs for 16-bit
            uint32_t timestamp = (packet.timestamp);
            int32_t steps      = (int32_t)(packet.steps);
            int32_t direction  = (int32_t)(packet.direction);

            int16_t ax = (int16_t)(packet.ax);
            int16_t ay = (int16_t)(packet.ay);
            int16_t az = (int16_t)(packet.az);
            int16_t gx = (int16_t)(packet.gx);
            int16_t gy = (int16_t)(packet.gy);
            int16_t gz = (int16_t)(packet.gz);

            bool btn = (packet.button == 1);
            bool ir  = (packet.ir_detected == 1);

            // Display
            char* clientIP = inet_ntoa(clientAddr.sin_addr);
            std::cout << "[" << clientIP << "] "
                      << "Time: " << timestamp << " | "
                      << "IR: " << (ir ? "H" : "L") << " | "
                      << "Enc: " << steps << " (" << direction << ") " << (btn ? "H" : "L") << " | "
                      << "Accel: " << ax << "," << ay << "," << az << " | "
                      << "Gyro: " << gx << "," << gy << "," << gz
                      << std::endl;
        } else {
            std::cerr << "Ignored packet of size " << bytesReceived 
                      << " (Expected " << sizeof(TelemetryPacket) << ")" << std::endl;
        }
    }

    close(sockfd);
    return 0;
}
