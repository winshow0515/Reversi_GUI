# Reversi GUI - 黑白棋線上對戰遊戲
Reversi_online_game 的 GUI 版本，使用 GTK+ 3 和 C++ 開發  

# 目錄
- [遊戲畫面](#遊戲畫面)
- [遊戲規則](#遊戲規則)
- [Prerequisite](#prerequisite)
- [使用方法](#使用方法)
- [技術細節](#技術細節)
- [心得](#心得)

# 遊戲畫面
![遊戲畫面](/Images/遊戲結束.png)
![遊戲結束畫面](/Images/遊戲結束.png)

# 遊戲規則
## 基本規則

1. **8×8 棋盤**：標準黑白棋棋盤，共 64 格
2. **起始位置**：中央 4 顆棋子（2 黑 2 白）
3. **目標**：當棋盤滿時，擁有較多棋子的一方獲勝
4. **合法移動**：必須至少翻轉一顆對手棋子
5. **翻轉**：新棋子和現有棋子之間的所有對手棋子都會翻面
6. **跳過**：如果沒有合法移動，回合自動跳過
7. **遊戲結束**：雙方都無法移動時遊戲結束

## 如何遊玩

- **你的回合**：綠色棋盤上會顯示黃色 `+` 在可下位置
- **點擊**：直接點擊有效位置來放置你的棋子
- **觀察**：對手的棋子會自動翻轉成你的顏色
- **獲勝**：棋子最多的玩家獲勝！

---

# Prerequisite

- **作業系統**: Linux (Ubuntu/Debian/Raspberry Pi OS)
- **編譯器**: g++ (支援 C++11)
- **函式庫**: GTK+ 3
- **網路**: TCP/IP 能力
- **顯示**: X11 或 Wayland（圖形介面需要）

---

# 使用方法
## 1. 安裝 GTK+ 3 開發函式庫
### 在 Raspberry Pi 上
```bash
sudo apt-get update
sudo apt-get install libgtk-3-dev build-essential
pkg-config --modversion gtk+-3.0
```

### 在 Ubuntu/Debian 上
```bash
sudo apt-get update
sudo apt-get install libgtk-3-dev build-essential
```

### 在 WSL 上
**Windows 11（WSLg - 內建 GUI 支援）：**
```bash
sudo apt-get update
sudo apt-get install libgtk-3-dev
```

## 2. 編譯
```bash
make
```

這會產生兩個執行檔：
- `reversi_gtk` - 圖形化客戶端
- `server` - 遊戲伺服器

## 3. 啟動伺服器
在一台機器上（例如樹莓派）：

```bash
./server 192.168.1.100 8888
# 將 192.168.1.100 替換成你的實際 IP
```

## 4. 啟動客戶端
在相同或不同的機器上：

```bash
./reversi_gtk
```

在 GUI 視窗中：
1. 輸入 **Server IP**（例如 `192.168.1.100`）
2. 輸入 **Port**（例如 `8888`）
3. 輸入 **Your name**（例如 `小明`）
4. 點擊 **Connect**

## 5. 開始遊戲！

- 等待第二個玩家連線
- 遊戲自動開始
- 按照遊戲規則遊玩直到結束

---

# 技術細節
## 網路通訊

- **非阻塞接收**：使用 `MSG_DONTWAIT` 旗標
- **訊息分隔符**：換行符號（`\n`）
- **輪詢間隔**：透過 `g_timeout_add()` 每 100ms 輪詢一次
- **編碼**：UTF-8（遊戲協定使用 ASCII）

## 遊戲邏輯

- **移動驗證**：8 個方向檢查
- **翻轉棋子**：所有有效方向自動翻轉
- **狀態管理**：64 位元組棋盤狀態字串
- **回合管理**：伺服器端強制執行


## 專案結構
```
Reversi_GTK/
├── game.hpp          # 遊戲邏輯類別
├── network.hpp       # 網路通訊類別
├── gui.cpp           # GTK+ GUI 主程式
├── server.cpp        # 遊戲伺服器
├── Makefile          # 編譯設定檔
└── README.md         # 本說明文件
```

## 檔案說明
### game.hpp
- 核心遊戲邏輯
- 棋盤管理
- 移動驗證
- 翻轉棋子演算法
- 遊戲狀態追蹤

### network.hpp
- TCP/IP 客戶端實作
- 非阻塞 I/O
- 訊息解析
- 連線管理

### gui.cpp
- GTK+ 介面建立
- 事件處理
- 棋盤繪製
- CSS 樣式
- 網路訊息處理

### server.cpp
- TCP/IP 伺服器
- 玩家配對
- 回合管理
- 移動驗證
- 遊戲流程控制

### Makefile
- 編譯自動化
- 編譯器旗標
- GTK+ pkg-config 整合

## 網路通訊協定
### 伺服器 → 客戶端訊息
| 訊息 | 格式 | 說明 |
|------|------|------|
| WAIT | `WAIT:<訊息>\n` | 等待對手 |
| START | `START:<對手名>:<棋子>\n` | 遊戲開始 |
| YOUR_TURN | `YOUR_TURN:<棋盤>\n` | 輪到你下棋 |
| OPPONENT_TURN | `OPPONENT_TURN:<棋盤>\n` | 對手回合 |
| MOVE_OK | `MOVE_OK:<移動>\n` | 移動已接受 |
| INVALID | `INVALID:<原因>\n` | 移動被拒絕 |
| OPPONENT_DISCONNECT | `OPPONENT_DISCONNECT:\n` | 對手離線 |
| END | `END:<結果>:<棋盤>\n` | 遊戲結束 |

### 客戶端 → 伺服器訊息
| 訊息 | 格式 | 說明 |
|------|------|------|
| 名字 | `<名字>` | 玩家名字（連線時） |
| 移動 | `<位置>` | 移動位置（例如 "d4", "e5"） |

### 棋盤狀態格式
64 字元字串代表 8×8 棋盤：
- `*` = 空格
- `X` = 黑棋
- `O` = 白棋

範例：`"***************************XO******OX*****************"`


## GTK+ 事件迴圈

```cpp
main()
  ├── gtk_init()              // 初始化 GTK+
  ├── create_ui()             // 建立介面
  ├── g_timeout_add()         // 設定網路訊息計時器（100ms）
  └── gtk_main()              // 進入 GTK 事件迴圈
      └── 事件處理：
          ├── on_connect_clicked()      // 連線按鈕
          ├── on_board_button_clicked() // 棋盤點擊
          └── check_network_messages()  // 網路訊息輪詢
```

---