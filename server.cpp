#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include "game.hpp"

#define BUFFER_SIZE 1024

class Server {
private:
    int server_fd;
    int client_sockets[2];
    std::string player_names[2];
    char player_pieces[2];
    Game* game;
    int current_turn;
    
    void send_message(int client_idx, const std::string& msg) {
        std::string msg_with_newline = msg + "\n";
        send(client_sockets[client_idx], msg_with_newline.c_str(), msg_with_newline.length(), 0);
    }
    
    std::string receive_message(int client_idx) {
        char buffer[BUFFER_SIZE] = {0};
        int valread = read(client_sockets[client_idx], buffer, BUFFER_SIZE);
        if (valread <= 0) {
            return "";
        }
        return std::string(buffer);
    }
    
    bool check_client_connected(int client_idx) {
        fd_set read_fds;
        struct timeval tv;
        FD_ZERO(&read_fds);
        FD_SET(client_sockets[client_idx], &read_fds);
        
        tv.tv_sec = 0;
        tv.tv_usec = 0;
        
        int result = select(client_sockets[client_idx] + 1, &read_fds, NULL, NULL, &tv);
        
        if (result > 0) {
            char buffer[1];
            int n = recv(client_sockets[client_idx], buffer, 1, MSG_PEEK | MSG_DONTWAIT);
            if (n == 0) {
                return false;
            }
        }
        
        return true;
    }
    
public:
    Server() {
        server_fd = -1;
        client_sockets[0] = -1;
        client_sockets[1] = -1;
        game = new Game();
        current_turn = 0;
    }
    
    ~Server() {
        if (game) delete game;
        if (client_sockets[0] != -1) close(client_sockets[0]);
        if (client_sockets[1] != -1) close(client_sockets[1]);
        if (server_fd != -1) close(server_fd);
    }
    
    bool start(const std::string& ip, int port) {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd == 0) {
            std::cerr << "Socket creation failed\n";
            return false;
        }
        
        int opt = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
            std::cerr << "Setsockopt failed\n";
            return false;
        }
        
        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = inet_addr(ip.c_str());
        address.sin_port = htons(port);
        
        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            std::cerr << "Bind failed\n";
            return false;
        }
        
        if (listen(server_fd, 2) < 0) {
            std::cerr << "Listen failed\n";
            return false;
        }
        
        std::cout << "Server started on " << ip << ":" << port << "\n";
        std::cout << "Waiting for players...\n";
        
        return true;
    }
    
    void wait_for_players() {
        struct sockaddr_in address;
        int addrlen = sizeof(address);
        
        client_sockets[0] = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (client_sockets[0] < 0) {
            std::cerr << "Accept failed for player 1\n";
            return;
        }
        
        player_names[0] = receive_message(0);
        std::cout << "Player 1 connected: " << player_names[0] << "\n";
        send_message(0, "WAIT:Waiting for another player...");
        
        client_sockets[1] = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (client_sockets[1] < 0) {
            std::cerr << "Accept failed for player 2\n";
            return;
        }
        
        player_names[1] = receive_message(1);
        std::cout << "Player 2 connected: " << player_names[1] << "\n";
        
        srand(time(NULL));
        current_turn = rand() % 2;
        player_pieces[current_turn] = 'X';
        player_pieces[1 - current_turn] = 'O';
        
        std::string start_msg_0 = "START:" + player_names[1] + ":" + std::string(1, player_pieces[0]);
        std::string start_msg_1 = "START:" + player_names[0] + ":" + std::string(1, player_pieces[1]);
        
        send_message(0, start_msg_0);
        send_message(1, start_msg_1);
        
        std::cout << player_names[current_turn] << " (" << player_pieces[current_turn] << ") goes first!\n";
    }
    
    void run_game() {
        while (true) {
            int opponent = 1 - current_turn;
            
            if (!check_client_connected(current_turn)) {
                std::cout << player_names[current_turn] << " disconnected\n";
                send_message(opponent, "OPPONENT_DISCONNECT:");
                break;
            }
            if (!check_client_connected(opponent)) {
                std::cout << player_names[opponent] << " disconnected\n";
                send_message(current_turn, "OPPONENT_DISCONNECT:");
                break;
            }
            
            if (!game->has_valid_moves(player_pieces[current_turn])) {
                if (!game->has_valid_moves(player_pieces[opponent])) {
                    std::string result = game->get_result();
                    std::string end_msg = "END:" + result + ":" + game->get_board_state();
                    send_message(0, end_msg);
                    send_message(1, end_msg);
                    std::cout << "Game over: " << result << "\n";
                    break;
                } else {
                    std::cout << player_names[current_turn] << " has no valid moves, skipping...\n";
                    std::string skip_msg = "SKIP:" + game->get_board_state();
                    send_message(current_turn, skip_msg);
                    send_message(opponent, "OPPONENT_SKIP:" + game->get_board_state());
                    current_turn = opponent;
                    continue;
                }
            }
            
            std::string state_current = "YOUR_TURN:" + game->get_board_state();
            std::string state_opponent = "OPPONENT_TURN:" + game->get_board_state();
            
            send_message(current_turn, state_current);
            send_message(opponent, state_opponent);
            
            bool valid_move = false;
            while (!valid_move) {
                if (!check_client_connected(current_turn)) {
                    std::cout << player_names[current_turn] << " disconnected\n";
                    send_message(opponent, "OPPONENT_DISCONNECT:");
                    return;
                }
                if (!check_client_connected(opponent)) {
                    std::cout << player_names[opponent] << " disconnected\n";
                    send_message(current_turn, "OPPONENT_DISCONNECT:");
                    return;
                }
                
                std::string move = receive_message(current_turn);
                if (move.empty()) {
                    std::cout << player_names[current_turn] << " disconnected\n";
                    send_message(opponent, "OPPONENT_DISCONNECT:");
                    return;
                }
                
                int row, col;
                if (!game->parse_move(move, row, col)) {
                    send_message(current_turn, "INVALID:Invalid position format");
                    continue;
                }
                
                if (!game->is_valid_move(row, col, player_pieces[current_turn])) {
                    send_message(current_turn, "INVALID:Invalid move");
                    continue;
                }
                
                game->make_move(row, col, player_pieces[current_turn]);
                std::cout << player_names[current_turn] << " (" << player_pieces[current_turn] 
                         << ") played " << move << "\n";
                
                send_message(current_turn, "MOVE_OK:" + move);
                
                valid_move = true;
            }
            
            current_turn = opponent;
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <ip> <port>\n";
        return 1;
    }
    
    std::string ip = argv[1];
    int port = atoi(argv[2]);
    
    Server server;
    if (!server.start(ip, port)) {
        return 1;
    }
    
    server.wait_for_players();
    server.run_game();
    
    return 0;
}