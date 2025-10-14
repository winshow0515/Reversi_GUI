#ifndef NETWORK_HPP
#define NETWORK_HPP

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include <errno.h>

class NetworkClient {
private:
    int sock;
    std::string player_name;
    std::string opponent_name;
    char my_piece;
    bool connected;
    
    std::vector<std::string> split(const std::string& s, char delimiter) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(s);
        while (std::getline(tokenStream, token, delimiter)) {
            tokens.push_back(token);
        }
        return tokens;
    }
    
public:
    NetworkClient() {
        sock = -1;
        connected = false;
        my_piece = ' ';
    }
    
    ~NetworkClient() {
        disconnect();
    }
    
    bool connect_to_server(const std::string& host, int port, const std::string& name) {
        player_name = name;
        
        std::cout << "[NETWORK] Connecting to " << host << ":" << port << std::endl;
        
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            std::cerr << "[NETWORK] Socket creation failed" << std::endl;
            return false;
        }
        
        struct sockaddr_in serv_addr;
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);
        
        if (inet_pton(AF_INET, host.c_str(), &serv_addr.sin_addr) <= 0) {
            std::cerr << "[NETWORK] Invalid address" << std::endl;
            close(sock);
            sock = -1;
            return false;
        }
        
        // 使用阻塞式連線（更可靠）
        if (::connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
            std::cerr << "[NETWORK] Connection failed: " << strerror(errno) << std::endl;
            close(sock);
            sock = -1;
            return false;
        }
        
        std::cout << "[NETWORK] Connected successfully" << std::endl;
        connected = true;
        
        // 發送玩家名字（不加換行符，server 會自己讀取）
        std::cout << "[NETWORK] Sending name: " << player_name << std::endl;
        ssize_t sent = send(sock, player_name.c_str(), player_name.length(), 0);
        std::cout << "[NETWORK] Sent " << sent << " bytes" << std::endl;
        
        return true;
    }
    
    void disconnect() {
        if (sock != -1) {
            close(sock);
            sock = -1;
        }
        connected = false;
    }
    
    bool is_connected() const {
        return connected && sock != -1;
    }
    
    void send_move(const std::string& move) {
        if (is_connected()) {
            send(sock, move.c_str(), move.length(), 0);
        }
    }
    
    std::string receive_message() {
        if (!is_connected()) return "";
        
        char buffer[1024] = {0};
        
        // 使用 MSG_DONTWAIT 標誌來非阻塞接收
        int n = recv(sock, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);
        
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 沒有資料可讀，正常情況
                return "";
            }
            // 其他錯誤，斷線
            std::cerr << "[NETWORK] recv error: " << strerror(errno) << std::endl;
            connected = false;
            return "";
        }
        
        if (n == 0) {
            // 連線關閉
            std::cerr << "[NETWORK] Connection closed by server" << std::endl;
            connected = false;
            return "";
        }
        
        buffer[n] = '\0';
        std::string result(buffer);
        std::cout << "[NETWORK] Received " << n << " bytes: " << result << std::endl;
        return result;
    }
    
    // 解析訊息並回傳命令類型
    std::string parse_message(const std::string& msg, std::vector<std::string>& parts) {
        parts = split(msg, ':');
        if (parts.empty()) return "";
        return parts[0];
    }
    
    std::string get_player_name() const { return player_name; }
    std::string get_opponent_name() const { return opponent_name; }
    void set_opponent_name(const std::string& name) { opponent_name = name; }
    char get_my_piece() const { return my_piece; }
    void set_my_piece(char piece) { my_piece = piece; }
    
    int get_socket() const { return sock; }
};

#endif // NETWORK_HPP