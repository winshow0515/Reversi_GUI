#ifndef GAME_HPP
#define GAME_HPP

#include <string>
#include <vector>
#include <utility>

class Game {
private:
    char board[8][8];
    char current_player;
    int black_count;
    int white_count;
    
    const int dx[8] = {-1, 1, 0, 0, -1, -1, 1, 1};
    const int dy[8] = {0, 0, -1, 1, -1, 1, -1, 1};
    
    bool is_valid_pos(int row, int col) const {
        return row >= 0 && row < 8 && col >= 0 && col < 8;
    }
    
    bool check_direction(int row, int col, int dir, char player) {
        char opponent = (player == 'X') ? 'O' : 'X';
        int r = row + dx[dir];
        int c = col + dy[dir];
        
        if (!is_valid_pos(r, c) || board[r][c] != opponent) {
            return false;
        }
        
        r += dx[dir];
        c += dy[dir];
        
        while (is_valid_pos(r, c)) {
            if (board[r][c] == '*') {
                return false;
            }
            if (board[r][c] == player) {
                return true;
            }
            r += dx[dir];
            c += dy[dir];
        }
        
        return false;
    }
    
    void flip_direction(int row, int col, int dir, char player) {
        char opponent = (player == 'X') ? 'O' : 'X';
        int r = row + dx[dir];
        int c = col + dy[dir];
        
        while (is_valid_pos(r, c) && board[r][c] == opponent) {
            board[r][c] = player;
            r += dx[dir];
            c += dy[dir];
        }
    }
    
    void count_pieces() {
        black_count = 0;
        white_count = 0;
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                if (board[i][j] == 'X') black_count++;
                else if (board[i][j] == 'O') white_count++;
            }
        }
    }

public:
    Game() {
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                board[i][j] = '*';
            }
        }
        
        board[3][3] = 'X';
        board[3][4] = 'O';
        board[4][3] = 'O';
        board[4][4] = 'X';
        
        current_player = 'X';
        black_count = 2;
        white_count = 2;
    }
    
    bool is_valid_move(int row, int col, char player) {
        if (!is_valid_pos(row, col) || board[row][col] != '*') {
            return false;
        }
        
        for (int dir = 0; dir < 8; dir++) {
            if (check_direction(row, col, dir, player)) {
                return true;
            }
        }
        
        return false;
    }
    
    bool make_move(int row, int col, char player) {
        if (!is_valid_move(row, col, player)) {
            return false;
        }
        
        board[row][col] = player;
        
        for (int dir = 0; dir < 8; dir++) {
            if (check_direction(row, col, dir, player)) {
                flip_direction(row, col, dir, player);
            }
        }
        
        count_pieces();
        current_player = (player == 'X') ? 'O' : 'X';
        return true;
    }
    
    bool parse_move(const std::string& move, int& row, int& col) {
        if (move.length() != 2) return false;
        
        col = move[0] - 'a';
        row = 8 - (move[1] - '0');
        
        return is_valid_pos(row, col);
    }
    
    std::vector<std::pair<int, int>> get_valid_moves(char player) {
        std::vector<std::pair<int, int>> moves;
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                if (is_valid_move(i, j, player)) {
                    moves.push_back(std::make_pair(i, j));
                }
            }
        }
        return moves;
    }
    
    bool has_valid_moves(char player) {
        return !get_valid_moves(player).empty();
    }
    
    bool is_game_over() {
        return !has_valid_moves('X') && !has_valid_moves('O');
    }
    
    std::string get_board_state() {
        std::string state;
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                state += board[i][j];
            }
        }
        return state;
    }
    
    void set_board_state(const std::string& state) {
        if (state.length() != 64) return;
        
        int idx = 0;
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                board[i][j] = state[idx++];
            }
        }
        count_pieces();
    }
    
    char get_piece(int row, int col) const {
        if (!is_valid_pos(row, col)) return '*';
        return board[row][col];
    }
    
    char get_current_player() const { return current_player; }
    void set_current_player(char player) { current_player = player; }
    int get_black_count() const { return black_count; }
    int get_white_count() const { return white_count; }
    
    std::string get_result() {
        count_pieces();
        if (black_count > white_count) {
            return "X wins!";
        } else if (white_count > black_count) {
            return "O wins!";
        } else {
            return "Draw!";
        }
    }
};

#endif // GAME_HPP