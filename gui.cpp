#include <gtk/gtk.h>
#include <string>
#include <sstream>
#include "game.hpp"
#include "network.hpp"

// 全域變數
struct AppData {
    GtkWidget* window;
    GtkWidget* board_buttons[8][8];
    GtkWidget* status_label;
    GtkWidget* player1_label;
    GtkWidget* player2_label;
    GtkWidget* black_count_label;
    GtkWidget* white_count_label;
    GtkWidget* server_entry;
    GtkWidget* port_entry;
    GtkWidget* name_entry;
    GtkWidget* connect_button;
    GtkWidget* history_view;
    GtkTextBuffer* history_buffer;
    
    Game* game;
    NetworkClient* network;
    
    bool is_my_turn;
    char my_piece;
    std::string my_name;
    std::string opponent_name;
    
    guint network_timer_id;
};

AppData app_data;

// 輔助函數
std::string position_to_string(int row, int col) {
    char col_char = 'a' + col;
    char row_char = '8' - row;
    std::stringstream ss;
    ss << col_char << row_char;
    return ss.str();
}

void add_history(const std::string& text) {
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(app_data.history_buffer, &iter);
    std::string line = text + "\n";
    gtk_text_buffer_insert(app_data.history_buffer, &iter, line.c_str(), -1);
    
    GtkTextMark* mark = gtk_text_buffer_get_insert(app_data.history_buffer);
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(app_data.history_view), mark, 0.0, TRUE, 0.0, 1.0);
}

void update_board() {
    auto valid_moves = app_data.game->get_valid_moves(app_data.my_piece);
    
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            char piece = app_data.game->get_piece(i, j);
            GtkWidget* button = app_data.board_buttons[i][j];
            
            GtkStyleContext* context = gtk_widget_get_style_context(button);
            gtk_style_context_remove_class(context, "black-piece");
            gtk_style_context_remove_class(context, "white-piece");
            gtk_style_context_remove_class(context, "valid-move");
            
            if (piece == 'X') {
                gtk_button_set_label(GTK_BUTTON(button), "⬤");
                gtk_style_context_add_class(context, "black-piece");
            } else if (piece == 'O') {
                gtk_button_set_label(GTK_BUTTON(button), "⬤");
                gtk_style_context_add_class(context, "white-piece");
            } else {
                gtk_button_set_label(GTK_BUTTON(button), "");
                
                if (app_data.is_my_turn) {
                    for (auto move : valid_moves) {
                        if (move.first == i && move.second == j) {
                            gtk_button_set_label(GTK_BUTTON(button), "+");
                            gtk_style_context_add_class(context, "valid-move");
                            break;
                        }
                    }
                }
            }
        }
    }
}

void update_info() {
    std::stringstream ss1, ss2;
    ss1 << app_data.my_name << " (You): " << app_data.my_piece;
    ss2 << app_data.opponent_name << ": " << (app_data.my_piece == 'X' ? 'O' : 'X');
    
    gtk_label_set_text(GTK_LABEL(app_data.player1_label), ss1.str().c_str());
    gtk_label_set_text(GTK_LABEL(app_data.player2_label), ss2.str().c_str());
    
    std::stringstream ss3, ss4;
    ss3 << "X: " << app_data.game->get_black_count();
    ss4 << "O: " << app_data.game->get_white_count();
    
    gtk_label_set_text(GTK_LABEL(app_data.black_count_label), ss3.str().c_str());
    gtk_label_set_text(GTK_LABEL(app_data.white_count_label), ss4.str().c_str());
    
    if (app_data.is_my_turn) {
        gtk_label_set_text(GTK_LABEL(app_data.status_label), "It's your turn!");
    } else {
        gtk_label_set_text(GTK_LABEL(app_data.status_label), "Waiting for opponent's move...");
    }
}

void enable_board(bool enable) {
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            gtk_widget_set_sensitive(app_data.board_buttons[i][j], enable);
        }
    }
}

// 處理網路訊息
gboolean check_network_messages(gpointer user_data) {
    if (!app_data.network->is_connected()) {
        return TRUE;
    }
    
    std::string msg = app_data.network->receive_message();
    if (msg.empty()) {
        return TRUE;
    }
    
    // 可能一次收到多個訊息，用換行符分割
    std::stringstream ss(msg);
    std::string line;
    
    while (std::getline(ss, line)) {
        if (line.empty()) continue;
        
        // 移除可能的 \r
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        std::vector<std::string> parts;
        std::string cmd = app_data.network->parse_message(line, parts);
        
        if (cmd == "WAIT") {
            gtk_label_set_text(GTK_LABEL(app_data.status_label), "Waiting for opponent...");
        }
        else if (cmd == "START" && parts.size() >= 3) {
            app_data.opponent_name = parts[1];
            app_data.my_piece = parts[2][0];
            app_data.network->set_opponent_name(app_data.opponent_name);
            app_data.network->set_my_piece(app_data.my_piece);
            
            // 不要用對話框，直接更新介面
            gtk_label_set_text(GTK_LABEL(app_data.status_label), "Game started!");
            update_info();
            update_board();
        }
        else if (cmd == "YOUR_TURN" && parts.size() >= 2) {
            app_data.game->set_board_state(parts[1]);
            app_data.is_my_turn = true;
            
            update_board();
            update_info();
            enable_board(true);
        }
        else if (cmd == "OPPONENT_TURN" && parts.size() >= 2) {
            app_data.game->set_board_state(parts[1]);
            app_data.is_my_turn = false;
            
            update_board();
            update_info();
            enable_board(false);
        }
        else if (cmd == "MOVE_OK" && parts.size() >= 2) {
            std::string history = app_data.my_name + ": " + parts[1];
            add_history(history);
            
            app_data.is_my_turn = false;
            enable_board(false);
            update_info();
        }
        else if (cmd == "INVALID" && parts.size() >= 2) {
            gtk_label_set_text(GTK_LABEL(app_data.status_label), 
                ("Invalid move: " + parts[1]).c_str());
        }
        else if (cmd == "OPPONENT_DISCONNECT") {
            gtk_label_set_text(GTK_LABEL(app_data.status_label), "Opponent disconnected. You win!");
            enable_board(false);
        }
        else if (cmd == "END" && parts.size() >= 3) {
            app_data.game->set_board_state(parts[2]);
            update_board();
            
            GtkWidget* dialog = gtk_message_dialog_new(
                GTK_WINDOW(app_data.window),
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_INFO,
                GTK_BUTTONS_OK,
                "Game Over!\n%s\n\nX: %d\nO: %d",
                parts[1].c_str(),
                app_data.game->get_black_count(),
                app_data.game->get_white_count()
            );
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            
            enable_board(false);
        }
    }
    
    return TRUE;
}

// 回調函數
void on_board_button_clicked(GtkWidget* widget, gpointer data) {
    if (!app_data.is_my_turn) return;
    
    int row = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "row"));
    int col = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "col"));
    
    if (!app_data.game->is_valid_move(row, col, app_data.my_piece)) {
        gtk_label_set_text(GTK_LABEL(app_data.status_label), "Invalid move! Try another position.");
        return;
    }
    
    std::string move = position_to_string(row, col);
    app_data.network->send_move(move);
}

void on_connect_clicked(GtkWidget* widget, gpointer data) {
    const char* host = gtk_entry_get_text(GTK_ENTRY(app_data.server_entry));
    const char* port_str = gtk_entry_get_text(GTK_ENTRY(app_data.port_entry));
    const char* name = gtk_entry_get_text(GTK_ENTRY(app_data.name_entry));
    
    if (strlen(host) == 0 || strlen(port_str) == 0 || strlen(name) == 0) {
        gtk_label_set_text(GTK_LABEL(app_data.status_label), "Please fill all fields!");
        return;
    }
    
    int port = atoi(port_str);
    app_data.my_name = name;
    
    gtk_widget_set_sensitive(app_data.connect_button, FALSE);
    gtk_button_set_label(GTK_BUTTON(app_data.connect_button), "Connecting...");
    
    if (app_data.network->connect_to_server(host, port, name)) {
        gtk_label_set_text(GTK_LABEL(app_data.status_label), "Connected to server");
        app_data.network_timer_id = g_timeout_add(100, check_network_messages, NULL);
    } else {
        gtk_label_set_text(GTK_LABEL(app_data.status_label), "Connection failed!");
        gtk_widget_set_sensitive(app_data.connect_button, TRUE);
        gtk_button_set_label(GTK_BUTTON(app_data.connect_button), "Connect");
    }
}

void on_window_destroy(GtkWidget* widget, gpointer data) {
    if (app_data.network_timer_id > 0) {
        g_source_remove(app_data.network_timer_id);
    }
    gtk_main_quit();
}

// 建立 UI
void create_ui() {
    app_data.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app_data.window), "Reversi Game");
    gtk_window_set_default_size(GTK_WINDOW(app_data.window), 900, 600);
    gtk_window_set_position(GTK_WINDOW(app_data.window), GTK_WIN_POS_CENTER);
    g_signal_connect(app_data.window, "destroy", G_CALLBACK(on_window_destroy), NULL);
    
    GtkWidget* main_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(main_hbox), 10);
    gtk_container_add(GTK_CONTAINER(app_data.window), main_hbox);
    
    // === 左側：棋盤 ===
    GtkWidget* board_frame = gtk_frame_new("Board");
    gtk_box_pack_start(GTK_BOX(main_hbox), board_frame, TRUE, TRUE, 0);
    
    GtkWidget* board_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(board_grid), 2);
    gtk_grid_set_column_spacing(GTK_GRID(board_grid), 2);
    gtk_container_set_border_width(GTK_CONTAINER(board_grid), 10);
    gtk_container_add(GTK_CONTAINER(board_frame), board_grid);
    
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            GtkWidget* button = gtk_button_new_with_label("");
            gtk_widget_set_size_request(button, 60, 60);
            
            g_object_set_data(G_OBJECT(button), "row", GINT_TO_POINTER(i));
            g_object_set_data(G_OBJECT(button), "col", GINT_TO_POINTER(j));
            
            g_signal_connect(button, "clicked", G_CALLBACK(on_board_button_clicked), NULL);
            
            gtk_grid_attach(GTK_GRID(board_grid), button, j, i, 1, 1);
            
            app_data.board_buttons[i][j] = button;
        }
    }
    
    // === 右側：資訊面板 ===
    GtkWidget* right_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_pack_start(GTK_BOX(main_hbox), right_vbox, FALSE, FALSE, 0);
    gtk_widget_set_size_request(right_vbox, 300, -1);
    
    // 玩家資訊
    GtkWidget* info_frame = gtk_frame_new("Player Info");
    gtk_box_pack_start(GTK_BOX(right_vbox), info_frame, FALSE, FALSE, 0);
    
    GtkWidget* info_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(info_vbox), 10);
    gtk_container_add(GTK_CONTAINER(info_frame), info_vbox);
    
    app_data.player1_label = gtk_label_new("Player 1 (You): ");
    gtk_label_set_xalign(GTK_LABEL(app_data.player1_label), 0.0);
    gtk_box_pack_start(GTK_BOX(info_vbox), app_data.player1_label, FALSE, FALSE, 0);
    
    app_data.player2_label = gtk_label_new("Player 2: ");
    gtk_label_set_xalign(GTK_LABEL(app_data.player2_label), 0.0);
    gtk_box_pack_start(GTK_BOX(info_vbox), app_data.player2_label, FALSE, FALSE, 0);
    
    GtkWidget* count_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_box_pack_start(GTK_BOX(info_vbox), count_hbox, FALSE, FALSE, 5);
    
    app_data.black_count_label = gtk_label_new("Black: 2");
    gtk_box_pack_start(GTK_BOX(count_hbox), app_data.black_count_label, FALSE, FALSE, 0);
    
    app_data.white_count_label = gtk_label_new("White: 2");
    gtk_box_pack_start(GTK_BOX(count_hbox), app_data.white_count_label, FALSE, FALSE, 0);
    
    app_data.status_label = gtk_label_new("Not connected");
    gtk_label_set_xalign(GTK_LABEL(app_data.status_label), 0.0);
    gtk_box_pack_start(GTK_BOX(info_vbox), app_data.status_label, FALSE, FALSE, 5);
    
    // 連線設定
    GtkWidget* connect_frame = gtk_frame_new("Connection");
    gtk_box_pack_start(GTK_BOX(right_vbox), connect_frame, FALSE, FALSE, 0);
    
    GtkWidget* connect_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(connect_grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(connect_grid), 5);
    gtk_container_set_border_width(GTK_CONTAINER(connect_grid), 10);
    gtk_container_add(GTK_CONTAINER(connect_frame), connect_grid);
    
    GtkWidget* server_label = gtk_label_new("Server IP:");
    gtk_label_set_xalign(GTK_LABEL(server_label), 0.0);
    gtk_grid_attach(GTK_GRID(connect_grid), server_label, 0, 0, 1, 1);
    
    app_data.server_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(app_data.server_entry), "127.0.0.1");
    gtk_grid_attach(GTK_GRID(connect_grid), app_data.server_entry, 1, 0, 1, 1);
    
    GtkWidget* port_label = gtk_label_new("Port:");
    gtk_label_set_xalign(GTK_LABEL(port_label), 0.0);
    gtk_grid_attach(GTK_GRID(connect_grid), port_label, 0, 1, 1, 1);
    
    app_data.port_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(app_data.port_entry), "8888");
    gtk_grid_attach(GTK_GRID(connect_grid), app_data.port_entry, 1, 1, 1, 1);
    
    GtkWidget* name_label = gtk_label_new("Your name:");
    gtk_label_set_xalign(GTK_LABEL(name_label), 0.0);
    gtk_grid_attach(GTK_GRID(connect_grid), name_label, 0, 2, 1, 1);
    
    app_data.name_entry = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(connect_grid), app_data.name_entry, 1, 2, 1, 1);
    
    app_data.connect_button = gtk_button_new_with_label("Connect");
    gtk_style_context_add_class(gtk_widget_get_style_context(app_data.connect_button), "connect-btn");  // 加這行！
    g_signal_connect(app_data.connect_button, "clicked", G_CALLBACK(on_connect_clicked), NULL);
    gtk_grid_attach(GTK_GRID(connect_grid), app_data.connect_button, 0, 3, 2, 1);
    
    // 歷史記錄
    GtkWidget* history_frame = gtk_frame_new("History");
    gtk_box_pack_start(GTK_BOX(right_vbox), history_frame, TRUE, TRUE, 0);
    
    GtkWidget* scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(history_frame), scrolled);
    
    app_data.history_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(app_data.history_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(app_data.history_view), FALSE);
    gtk_container_add(GTK_CONTAINER(scrolled), app_data.history_view);
    
    app_data.history_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app_data.history_view));
    
// 載入 CSS 樣式
    GtkCssProvider* css_provider = gtk_css_provider_new();
    const char* css_data = 
        "* { "
        "  font-family: Sans; "
        "} "
        "button { "
        "  background-image: none; "
        "  background-color: #228B22; "
        "  border: 2px solid #145214; "
        "  border-radius: 0px; "
        "  font-size: 50px; "
        "  font-weight: bold; "
        "  color: #ffffffff; "
        "  min-width: 60px; "
        "  min-height: 60px; "
        "  max-width: 60px; "              // 限制最大寬度
        "  max-height: 60px; "             // 限制最大高度
        "  padding: 0; "
        "  margin: 0; "
        "  box-shadow: none; "
        "  line-height: 0.8; "             // 減少行高！
        "} "
        "button label { "                  // 按鈕內的文字標籤
        "  margin: 0; "
        "  padding: 0; "
        "  line-height: 0.8; "             // 文字行高
        "} "
        "button:hover { "
        "  background-image: none; "
        "  background-color: #32CD32; "
        "} "
        "button.black-piece { "
        "  background-image: none; "
        "  color: #000000; "
        "  background-color: #228B22; "
        "  font-size: 45px; "
        "  line-height: 0.7; "             // 更緊密
        "} "
        "button.white-piece { "
        "  background-image: none; "
        "  color: #FFFFFF; "
        "  background-color: #228B22; "
        "  font-size: 45px; "
        "  line-height: 0.7; "             // 更緊密
        "  text-shadow: 0 0 6px #000000; "
        "} "
        "button.valid-move { "
        "  background-image: none; "
        "  color: #FFD700; "
        "  font-size: 24px; "
        "  background-color: #2E8B2E; "
        "  font-weight: bold; "
        "} "
        "button.connect-btn { "              // Connect 按鈕的專屬樣式
        "  background-color: #4CAF50; "      // 綠色背景
        "  color: #000000; "                 // 黑色文字
        "  font-size: 14px; "                // 小字體
        "  font-weight: normal; "            // 正常粗細
        "  min-width: 100px; "               // 取消最小寬度限制
        "  min-height: 30px; "               // 正常按鈕高度
        "  max-width: none; "                // 取消最大寬度限制
        "  max-height: none; "               // 取消最大高度限制
        "  padding: 5px 15px; "              // 加上內邊距
        "  border-radius: 3px; "             // 圓角
        "  line-height: normal; "            // 正常行高
        "} "
        "button.connect-btn:hover { "
        "  background-color: #45a049; "      // 懸停時深一點的綠色
        "} "
        "headerbar button.close { "
        "  background-color: #DC143C; "
        "  color: white; "
        "} "
        "headerbar button.close:hover { "
        "  background-color: #FF0000; "
        "}";

    gtk_css_provider_load_from_data(css_provider, css_data, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    
    gtk_widget_show_all(app_data.window);
}

int main(int argc, char* argv[]) {
    gtk_init(&argc, &argv);
    
    app_data.game = new Game();
    app_data.network = new NetworkClient();
    app_data.is_my_turn = false;
    app_data.my_piece = ' ';
    app_data.network_timer_id = 0;
    
    create_ui();
    
    update_board();
    enable_board(false);
    
    gtk_main();
    
    delete app_data.game;
    delete app_data.network;
    
    return 0;
}