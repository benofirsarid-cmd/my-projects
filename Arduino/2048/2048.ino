w#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

// --- SCREEN PINS ---
#define TFT_CS    10
#define TFT_DC    9
#define TFT_MOSI  11
#define TFT_SCLK  12
#define TFT_RST   -1  

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// --- BUTTON PINS ---
#define BTN_UP    4
#define BTN_DOWN  5
#define BTN_LEFT  6
#define BTN_RIGHT 7

// --- GAME VARIABLES ---
int gameState = 0; // 0 = Menu, 1 = Playing, 2 = Game Over
int menuHover = 0; // 0 = Start, 1 = Mode
int difficulty = 0; // 0 = Normal, 1 = Hard, 2 = Chaos
int highScores[3] = {0, 0, 0}; 

int board[4][4];
int score = 0;
bool isGameOver = false;

// --- CUSTOM COLORS (RGB565 Format) ---
#define COLOR_EMPTY 0x4208
#define COLOR_2     0xFFFF
#define COLOR_4     0xFFE0
#define COLOR_8     0xFD20
#define COLOR_16    0xF800
#define COLOR_32    0xF81F
#define COLOR_64    0x001F
#define COLOR_128   0x07FF
#define COLOR_256   0x07E0
#define COLOR_512   0xE6E5
#define COLOR_1024  0xEE4A
#define COLOR_2048  0xEE25
#define COLOR_4096  0xF020
#define COLOR_8192  0xF000
#define COLOR_SUPER 0x0000 

// ==========================================
// MENU & GAME OVER UI
// ==========================================

void drawMenu() {
  tft.fillScreen(ST77XX_BLACK);
  
  tft.drawRect(2, 2, 236, 236, ST77XX_YELLOW);
  tft.drawRect(4, 4, 232, 232, ST77XX_ORANGE);
  
  tft.setCursor(70, 30);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(4);
  tft.print("2048");

  tft.setTextSize(2); 

  tft.setCursor(54, 100);
  if (menuHover == 0) tft.setTextColor(ST77XX_WHITE); else tft.setTextColor(ST77XX_ORANGE);
  tft.print("[START GAME]");

  tft.setCursor(20, 140);
  if (menuHover == 1) tft.setTextColor(ST77XX_WHITE); else tft.setTextColor(ST77XX_ORANGE);
  tft.print("Mode:");
  if (difficulty == 0) tft.print(" < NORMAL >");
  else if (difficulty == 1) tft.print(" <  HARD  >");
  else tft.print(" < CHAOS  >");

  tft.setCursor(20, 190);
  tft.setTextColor(ST77XX_GREEN);
  tft.print("High Score: ");
  tft.print(highScores[difficulty]);
}

void showGameOver() {
  tft.fillScreen(ST77XX_BLACK);
  
  if (score > highScores[difficulty]) {
    highScores[difficulty] = score;
  }
  
  tft.setCursor(39, 60); 
  tft.setTextColor(ST77XX_RED); 
  tft.setTextSize(3); 
  tft.print("GAME OVER");
  
  String scoreStr = "Score: " + String(score);
  int scoreWidth = scoreStr.length() * 12; 
  int scoreX = (240 - scoreWidth) / 2;
  
  tft.setCursor(scoreX, 110); 
  tft.setTextColor(ST77XX_WHITE); 
  tft.setTextSize(2); 
  tft.print(scoreStr);

  tft.setCursor(24, 160);
  tft.setTextColor(ST77XX_YELLOW);
  tft.print("Press UP to Menu");
}

// ==========================================
// GRID DRAWING LOGIC
// ==========================================

uint16_t getTileColor(int val) {
  switch (val) {
    case 0: return COLOR_EMPTY;
    case 2: return COLOR_2;
    case 4: return COLOR_4;
    case 8: return COLOR_8;
    case 16: return COLOR_16;
    case 32: return COLOR_32;
    case 64: return COLOR_64;
    case 128: return COLOR_128;
    case 256: return COLOR_256;
    case 512: return COLOR_512;
    case 1024: return COLOR_1024;
    case 2048: return COLOR_2048;
    case 4096: return COLOR_4096;
    case 8192: return COLOR_8192;
    default: return COLOR_SUPER;
  }
}

void drawBoard() {
  tft.fillScreen(ST77XX_BLACK);

  // --- TOP LEFT SCORE ---
  tft.setCursor(20, 10);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.print("Score: ");
  tft.print(score);

  // --- DRAW 4x4 GRID ---
  // The grid is offset 20px from the left and 40px from the top
  for (int row = 0; row < 4; row++) {
    for (int col = 0; col < 4; col++) {
      int val = board[row][col];
      int x = 20 + 4 + (col * 49); // 45px cell + 4px gap = 49px 
      int y = 40 + 4 + (row * 49); 
      
      tft.fillRect(x, y, 45, 45, getTileColor(val));

      if (val > 0) {
        int numDigits = String(val).length();
        int tSize = 2;
        int textWidth = numDigits * 12; 
        
        // Dynamic Font Scaling: If the number is 4 digits or more (1024+), shrink the text to fit!
        if (numDigits >= 4) {
          tSize = 1;
          textWidth = numDigits * 6; // Size 1 text is 6px wide per char
        }

        int textX = x + ((45 - textWidth) / 2);
        int textY = y + (tSize == 2 ? 15 : 19); 
        
        if (val == 2 || val == 4) tft.setTextColor(ST77XX_BLACK);
        else tft.setTextColor(ST77XX_WHITE);
        
        tft.setTextSize(tSize);
        tft.setCursor(textX, textY);
        tft.print(val);
      }
    }
  }
}

// ==========================================
// GAMEPLAY LOGIC
// ==========================================

void spawnTile() {
  int emptyX[16], emptyY[16];
  int count = 0;
  
  for (int r = 0; r < 4; r++) {
    for (int c = 0; c < 4; c++) {
      if (board[r][c] == 0) {
        emptyX[count] = r;
        emptyY[count] = c;
        count++;
      }
    }
  }
  
  if (count > 0) {
    int pick = random(0, count);
    int val = 2;
    int chance = random(0, 100);
    
    if (difficulty == 0) { 
      val = (chance < 90) ? 2 : 4;
    } else if (difficulty == 1) { 
      val = (chance < 50) ? 2 : 4;
    } else if (difficulty == 2) { 
      if (chance < 33) val = 2;
      else if (chance < 66) val = 4;
      else val = 8;
    }
    board[emptyX[pick]][emptyY[pick]] = val;
  }
}

void resetGame() {
  score = 0;
  isGameOver = false;
  for (int r = 0; r < 4; r++) {
    for (int c = 0; c < 4; c++) board[r][c] = 0;
  }
  spawnTile();
  spawnTile();
  drawBoard();
}

bool checkGameOver() {
  for (int r = 0; r < 4; r++) {
    for (int c = 0; c < 4; c++) {
      if (board[r][c] == 0) return false;
    }
  }
  for (int r = 0; r < 4; r++) {
    for (int c = 0; c < 3; c++) {
      if (board[r][c] == board[r][c+1]) return false;
    }
  }
  for (int c = 0; c < 4; c++) {
    for (int r = 0; r < 3; r++) {
      if (board[r][c] == board[r+1][c]) return false;
    }
  }
  return true; 
}

bool slideLineLeft(int line[4]) {
  bool moved = false;
  
  int temp[4] = {0, 0, 0, 0};
  int pos = 0;
  for (int i = 0; i < 4; i++) {
    if (line[i] != 0) {
      temp[pos] = line[i];
      if (i != pos) moved = true;
      pos++;
    }
  }
  for (int i = 0; i < 4; i++) line[i] = temp[i];

  for (int i = 0; i < 3; i++) {
    if (line[i] != 0 && line[i] == line[i + 1]) {
      line[i] *= 2;
      score += line[i];
      line[i + 1] = 0;
      moved = true;
    }
  }

  int temp2[4] = {0, 0, 0, 0};
  pos = 0;
  for (int i = 0; i < 4; i++) {
    if (line[i] != 0) { temp2[pos++] = line[i]; }
  }
  for (int i = 0; i < 4; i++) line[i] = temp2[i];

  return moved;
}

bool moveGrid(String dir) {
  bool gridChanged = false;
  
  for (int i = 0; i < 4; i++) {
    int line[4];
    
    if (dir == "LEFT")  for(int j=0; j<4; j++) line[j] = board[i][j];
    if (dir == "RIGHT") for(int j=0; j<4; j++) line[j] = board[i][3-j]; 
    if (dir == "UP")    for(int j=0; j<4; j++) line[j] = board[j][i];
    if (dir == "DOWN")  for(int j=0; j<4; j++) line[j] = board[3-j][i]; 

    if (slideLineLeft(line)) gridChanged = true;

    if (dir == "LEFT")  for(int j=0; j<4; j++) board[i][j] = line[j];
    if (dir == "RIGHT") for(int j=0; j<4; j++) board[i][3-j] = line[j];
    if (dir == "UP")    for(int j=0; j<4; j++) board[j][i] = line[j];
    if (dir == "DOWN")  for(int j=0; j<4; j++) board[3-j][i] = line[j];
  }
  
  return gridChanged;
}

// ==========================================
// MAIN ARDUINO SETUP & LOOP
// ==========================================

void setup() {
  Serial.begin(115200);
  
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);

  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS); 
  tft.init(240, 240);
  tft.setRotation(2); 

  drawMenu();
}

void loop() {
  // --- STATE 0: MENU ---
  if (gameState == 0) {
    if (digitalRead(BTN_UP) == LOW) {
      menuHover--; if (menuHover < 0) menuHover = 1;
      drawMenu(); delay(200);
    }
    if (digitalRead(BTN_DOWN) == LOW) {
      menuHover++; if (menuHover > 1) menuHover = 0;
      drawMenu(); delay(200);
    }
    if (digitalRead(BTN_LEFT) == LOW && menuHover == 1) {
      difficulty--; if (difficulty < 0) difficulty = 2;
      drawMenu(); delay(200);
    }
    if (digitalRead(BTN_RIGHT) == LOW) {
      if (menuHover == 0) {
        gameState = 1;
        resetGame();
      } else if (menuHover == 1) {
        difficulty++; if (difficulty > 2) difficulty = 0;
        drawMenu();
      }
      delay(200);
    }
    return;
  }

  // --- STATE 2: GAME OVER ---
  if (gameState == 2) {
    if (digitalRead(BTN_UP) == LOW) {
      gameState = 0;
      drawMenu();
      delay(200);
    }
    return;
  }

  // --- STATE 1: PLAYING ---
  bool moved = false;

  if (digitalRead(BTN_UP) == LOW) {
    moved = moveGrid("UP");
    while(digitalRead(BTN_UP) == LOW) delay(10); 
  }
  else if (digitalRead(BTN_DOWN) == LOW) {
    moved = moveGrid("DOWN");
    while(digitalRead(BTN_DOWN) == LOW) delay(10); 
  }
  else if (digitalRead(BTN_LEFT) == LOW) {
    moved = moveGrid("LEFT");
    while(digitalRead(BTN_LEFT) == LOW) delay(10); 
  }
  else if (digitalRead(BTN_RIGHT) == LOW) {
    moved = moveGrid("RIGHT");
    while(digitalRead(BTN_RIGHT) == LOW) delay(10); 
  }

  if (moved) {
    spawnTile();
    isGameOver = checkGameOver();
    if (isGameOver) {
      gameState = 2;
      showGameOver();
    } else {
      drawBoard();
    }
  }
}