#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>


// --- NEW SAFE SCREEN PINS ---
#define TFT_SCLK  19
#define TFT_MOSI  20
#define TFT_DC    21
#define TFT_CS    47
#define TFT_RST   -1  

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// --- BUTTON PINS ---
#define BTN_UP    4
#define BTN_DOWN  5
#define BTN_LEFT  6
#define BTN_RIGHT 7

// --- OS VARIABLES ---
int osState = 0;       // 0 = Boot, 1 = Game Selector, 2 = Snake, 3 = Under Const, 4 = 2048
int osSelection = 0;   // 0=Snake, 1=Tetris, 2=2048, 3=PacMan, 4=Sokoban

// ==========================================
// SNAKE VARIABLES & SETTINGS
// ==========================================
#define GRID_SIZE 10
#define MAX_SNAKE_LENGTH 100

int snakeGameState = 0;       
int menuHoverSnake = 0;       
int diffSettingSnake = 1;     
int appleSetting = 0;    
int modeSetting = 0;     

int currentSpeed = 120;  
int numApples = 1;
int frameCount = 0;      
int highScoresSnake[3][3][3] = {0};

int snakeX[MAX_SNAKE_LENGTH];
int snakeY[MAX_SNAKE_LENGTH];
int snakeLength;
int dirX, dirY;
int lastDirX, lastDirY; // The Axis-Lock Variables
int foodX[5]; 
int foodY[5]; 

unsigned long lastSnakeMove = 0; 

// ==========================================
// 2048 VARIABLES & SETTINGS
// ==========================================
int game2048State = 0; 
int menuHover2048 = 0; 
int diffSetting2048 = 0; 
int highScores2048[3] = {0, 0, 0}; 

int board2048[4][4];
int score2048 = 0;

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
// ARCADE OS FUNCTIONS
// ==========================================

void playBootAnimation() {
  tft.fillScreen(ST77XX_BLACK);
  int colors[] = {ST77XX_RED, ST77XX_YELLOW, ST77XX_GREEN, ST77XX_CYAN, ST77XX_BLUE, ST77XX_MAGENTA};
  char letters[] = {'A', 'R', 'C', 'A', 'D', 'E'};
  tft.setTextSize(5);
  for(int i = 0; i < 6; i++) {
    tft.setTextColor(colors[i]);
    tft.setCursor(30 + (i * 30), 100);
    tft.print(letters[i]);
    delay(200); 
  }
  delay(500);
  tft.setCursor(48, 170); tft.setTextColor(ST77XX_WHITE); tft.setTextSize(2); tft.print("Press UP for");
  tft.setCursor(36, 200); tft.print("Game Selection");
  while(digitalRead(BTN_UP) == LOW) { delay(10); } 
  while(digitalRead(BTN_UP) == HIGH) { delay(10); } 
  while(digitalRead(BTN_UP) == LOW) { delay(10); } 
}

void drawGameCarousel() {
  tft.fillScreen(ST77XX_BLACK);
  tft.drawRect(2, 2, 236, 236, ST77XX_WHITE);
  tft.drawRect(4, 4, 232, 232, ST77XX_RED);
  tft.fillTriangle(20, 120, 30, 110, 30, 130, ST77XX_YELLOW);
  tft.fillTriangle(220, 120, 210, 110, 210, 130, ST77XX_YELLOW);

  if (osSelection == 0) {
    tft.setCursor(75, 40); tft.setTextColor(ST77XX_GREEN); tft.setTextSize(3); tft.print("SNAKE");
    tft.fillRect(100, 100, 15, 15, ST77XX_GREEN); tft.fillRect(115, 100, 15, 15, ST77XX_GREEN);
    tft.fillRect(115, 115, 15, 15, ST77XX_GREEN); tft.fillRect(130, 115, 15, 15, ST77XX_GREEN);
    tft.fillRect(130, 130, 15, 15, ST77XX_GREEN); tft.fillRect(160, 130, 15, 15, ST77XX_RED);   
  } else if (osSelection == 1) {
    tft.setCursor(65, 40); tft.setTextColor(ST77XX_CYAN); tft.setTextSize(3); tft.print("TETRIS");
    tft.fillRect(95, 110, 50, 15, ST77XX_CYAN); tft.fillRect(112, 125, 16, 15, ST77XX_CYAN);
  } else if (osSelection == 2) {
    tft.setCursor(85, 40); tft.setTextColor(ST77XX_YELLOW); tft.setTextSize(3); tft.print("2048");
    tft.fillRect(100, 100, 18, 18, ST77XX_ORANGE); tft.fillRect(122, 100, 18, 18, ST77XX_YELLOW);
    tft.fillRect(100, 122, 18, 18, ST77XX_YELLOW); tft.fillRect(122, 122, 18, 18, ST77XX_WHITE);
  } else if (osSelection == 3) {
    tft.setCursor(55, 40); tft.setTextColor(ST77XX_YELLOW); tft.setTextSize(3); tft.print("PAC-MAN");
    tft.fillCircle(120, 120, 20, ST77XX_YELLOW); tft.fillTriangle(120, 120, 145, 100, 145, 140, ST77XX_BLACK); 
    tft.fillCircle(155, 120, 5, ST77XX_WHITE); 
  } else if (osSelection == 4) {
    tft.setCursor(55, 40); tft.setTextColor(ST77XX_ORANGE); tft.setTextSize(3); tft.print("SOKOBAN");
    tft.fillRect(100, 100, 40, 40, ST77XX_ORANGE); tft.drawRect(100, 100, 40, 40, ST77XX_WHITE);
    tft.drawLine(100, 100, 140, 140, ST77XX_WHITE); tft.drawLine(140, 100, 100, 140, ST77XX_WHITE);
  }
  tft.setCursor(24, 200); tft.setTextColor(ST77XX_WHITE); tft.setTextSize(2); tft.print("Press UP to Play");
}

void showUnderConstruction() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(10, 100); tft.setTextColor(ST77XX_RED); tft.setTextSize(3); tft.print("COMING SOON!");
  tft.setCursor(24, 160); tft.setTextColor(ST77XX_YELLOW); tft.setTextSize(2); tft.print("Press UP to Menu");
  while (digitalRead(BTN_UP) == LOW) { delay(10); }  
  while (digitalRead(BTN_UP) == HIGH) { delay(10); } 
  while (digitalRead(BTN_UP) == LOW) { delay(10); }  
  osState = 1; drawGameCarousel(); 
}

// ==========================================
// 2048 ENGINE
// ==========================================

void draw2048Menu() {
  tft.fillScreen(ST77XX_BLACK);
  tft.drawRect(2, 2, 236, 236, ST77XX_YELLOW);
  tft.drawRect(4, 4, 232, 232, ST77XX_ORANGE);
  tft.setCursor(70, 30); tft.setTextColor(ST77XX_YELLOW); tft.setTextSize(4); tft.print("2048");
  tft.setTextSize(2); 
  tft.setCursor(54, 100); if (menuHover2048 == 0) tft.setTextColor(ST77XX_WHITE); else tft.setTextColor(ST77XX_ORANGE); tft.print("[START GAME]");
  tft.setCursor(20, 140); if (menuHover2048 == 1) tft.setTextColor(ST77XX_WHITE); else tft.setTextColor(ST77XX_ORANGE); tft.print("Mode:");
  if (diffSetting2048 == 0) tft.print(" < NORMAL >"); else if (diffSetting2048 == 1) tft.print(" <  HARD  >"); else tft.print(" < CHAOS  >");
  tft.setCursor(20, 175); tft.setTextColor(ST77XX_GREEN); tft.print("High Score: "); tft.print(highScores2048[diffSetting2048]);
  tft.setCursor(30, 215); if (menuHover2048 == 2) tft.setTextColor(ST77XX_RED); else tft.setTextColor(ST77XX_WHITE); tft.print("[EXIT TO ARCADE]");
}

uint16_t getTileColor2048(int val) {
  switch (val) {
    case 0: return COLOR_EMPTY; case 2: return COLOR_2; case 4: return COLOR_4; case 8: return COLOR_8; case 16: return COLOR_16;
    case 32: return COLOR_32; case 64: return COLOR_64; case 128: return COLOR_128; case 256: return COLOR_256; case 512: return COLOR_512;
    case 1024: return COLOR_1024; case 2048: return COLOR_2048; case 4096: return COLOR_4096; case 8192: return COLOR_8192; default: return COLOR_SUPER;
  }
}

void drawBoard2048() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(20, 10); tft.setTextColor(ST77XX_WHITE); tft.setTextSize(2); tft.print("Score: "); tft.print(score2048);
  for (int row = 0; row < 4; row++) {
    for (int col = 0; col < 4; col++) {
      int val = board2048[row][col];
      int x = 20 + 4 + (col * 49); int y = 40 + 4 + (row * 49); 
      tft.fillRect(x, y, 45, 45, getTileColor2048(val));
      if (val > 0) {
        int numDigits = String(val).length(); int tSize = 2; int textWidth = numDigits * 12; 
        if (numDigits >= 4) { tSize = 1; textWidth = numDigits * 6; }
        int textX = x + ((45 - textWidth) / 2); int textY = y + (tSize == 2 ? 15 : 19); 
        if (val == 2 || val == 4) tft.setTextColor(ST77XX_BLACK); else tft.setTextColor(ST77XX_WHITE);
        tft.setTextSize(tSize); tft.setCursor(textX, textY); tft.print(val);
      }
    }
  }
}

void spawnTile2048() {
  int emptyX[16], emptyY[16]; int count = 0;
  for (int r = 0; r < 4; r++) { for (int c = 0; c < 4; c++) { if (board2048[r][c] == 0) { emptyX[count] = r; emptyY[count] = c; count++; } } }
  if (count > 0) {
    int pick = random(0, count); int val = 2; int chance = random(0, 100);
    if (diffSetting2048 == 0) val = (chance < 90) ? 2 : 4;
    else if (diffSetting2048 == 1) val = (chance < 50) ? 2 : 4;
    else { if (chance < 33) val = 2; else if (chance < 66) val = 4; else val = 8; }
    board2048[emptyX[pick]][emptyY[pick]] = val;
  }
}

void reset2048Game() { score2048 = 0; for (int r = 0; r < 4; r++) { for (int c = 0; c < 4; c++) board2048[r][c] = 0; } spawnTile2048(); spawnTile2048(); drawBoard2048(); }

bool checkGameOver2048() {
  for (int r = 0; r < 4; r++) { for (int c = 0; c < 4; c++) { if (board2048[r][c] == 0) return false; } }
  for (int r = 0; r < 4; r++) { for (int c = 0; c < 3; c++) { if (board2048[r][c] == board2048[r][c+1]) return false; } }
  for (int c = 0; c < 4; c++) { for (int r = 0; r < 3; r++) { if (board2048[r][c] == board2048[r+1][c]) return false; } }
  return true; 
}

bool slideLineLeft2048(int line[4]) {
  bool moved = false; int temp[4] = {0, 0, 0, 0}; int pos = 0;
  for (int i = 0; i < 4; i++) { if (line[i] != 0) { temp[pos] = line[i]; if (i != pos) moved = true; pos++; } }
  for (int i = 0; i < 4; i++) line[i] = temp[i];
  for (int i = 0; i < 3; i++) { if (line[i] != 0 && line[i] == line[i + 1]) { line[i] *= 2; score2048 += line[i]; line[i + 1] = 0; moved = true; } }
  int temp2[4] = {0, 0, 0, 0}; pos = 0;
  for (int i = 0; i < 4; i++) { if (line[i] != 0) { temp2[pos++] = line[i]; } }
  for (int i = 0; i < 4; i++) line[i] = temp2[i]; return moved;
}

bool moveGrid2048(String dir) {
  bool gridChanged = false;
  for (int i = 0; i < 4; i++) {
    int line[4];
    if (dir == "LEFT")  for(int j=0; j<4; j++) line[j] = board2048[i][j];
    if (dir == "RIGHT") for(int j=0; j<4; j++) line[j] = board2048[i][3-j]; 
    if (dir == "UP")    for(int j=0; j<4; j++) line[j] = board2048[j][i];
    if (dir == "DOWN")  for(int j=0; j<4; j++) line[j] = board2048[3-j][i]; 
    if (slideLineLeft2048(line)) gridChanged = true;
    if (dir == "LEFT")  for(int j=0; j<4; j++) board2048[i][j] = line[j];
    if (dir == "RIGHT") for(int j=0; j<4; j++) board2048[i][3-j] = line[j];
    if (dir == "UP")    for(int j=0; j<4; j++) board2048[j][i] = line[j];
    if (dir == "DOWN")  for(int j=0; j<4; j++) board2048[3-j][i] = line[j];
  }
  return gridChanged;
}

void run2048Engine() {
  if (game2048State == 0) {
    if (digitalRead(BTN_UP) == LOW) { while(digitalRead(BTN_UP) == LOW) delay(10); menuHover2048--; if (menuHover2048 < 0) menuHover2048 = 2; draw2048Menu(); }
    if (digitalRead(BTN_DOWN) == LOW) { while(digitalRead(BTN_DOWN) == LOW) delay(10); menuHover2048++; if (menuHover2048 > 2) menuHover2048 = 0; draw2048Menu(); }
    if (digitalRead(BTN_LEFT) == LOW && menuHover2048 == 1) { while(digitalRead(BTN_LEFT) == LOW) delay(10); diffSetting2048--; if (diffSetting2048 < 0) diffSetting2048 = 2; draw2048Menu(); }
    if (digitalRead(BTN_RIGHT) == LOW) {
      while(digitalRead(BTN_RIGHT) == LOW) delay(10); 
      if (menuHover2048 == 0) { game2048State = 1; reset2048Game(); } 
      else if (menuHover2048 == 1) { diffSetting2048++; if (diffSetting2048 > 2) diffSetting2048 = 0; draw2048Menu(); }
      else if (menuHover2048 == 2) { osState = 1; menuHover2048 = 0; drawGameCarousel(); }
    }
    return;
  }
  if (game2048State == 2) { if (digitalRead(BTN_UP) == LOW) { while(digitalRead(BTN_UP) == LOW) delay(10); game2048State = 0; draw2048Menu(); } return; }

  bool moved = false;
  if (digitalRead(BTN_UP) == LOW) { moved = moveGrid2048("UP"); while(digitalRead(BTN_UP) == LOW) delay(10); }
  else if (digitalRead(BTN_DOWN) == LOW) { moved = moveGrid2048("DOWN"); while(digitalRead(BTN_DOWN) == LOW) delay(10); }
  else if (digitalRead(BTN_LEFT) == LOW) { moved = moveGrid2048("LEFT"); while(digitalRead(BTN_LEFT) == LOW) delay(10); }
  else if (digitalRead(BTN_RIGHT) == LOW) { moved = moveGrid2048("RIGHT"); while(digitalRead(BTN_RIGHT) == LOW) delay(10); }

  if (moved) {
    spawnTile2048();
    if (checkGameOver2048()) {
      game2048State = 2; tft.fillScreen(ST77XX_BLACK);
      if (score2048 > highScores2048[diffSetting2048]) highScores2048[diffSetting2048] = score2048;
      tft.setCursor(39, 60); tft.setTextColor(ST77XX_RED); tft.setTextSize(3); tft.print("GAME OVER");
      String scoreStr = "Score: " + String(score2048);
      int scoreWidth = scoreStr.length() * 12; int scoreX = (240 - scoreWidth) / 2;
      tft.setCursor(scoreX, 110); tft.setTextColor(ST77XX_WHITE); tft.setTextSize(2); tft.print(scoreStr);
      tft.setCursor(24, 160); tft.setTextColor(ST77XX_YELLOW); tft.print("Press UP to Menu");
    } else {
      drawBoard2048();
    }
  }
}

// ==========================================
// SNAKE ENGINE
// ==========================================

void drawSnakeMenu() {
  tft.fillScreen(ST77XX_BLACK);
  tft.drawRect(2, 2, 236, 236, ST77XX_GREEN);
  tft.setCursor(60, 20); tft.setTextColor(ST77XX_GREEN); tft.setTextSize(4); tft.print("SNAKE"); tft.setTextSize(2); 
  tft.setCursor(54, 70); if (menuHoverSnake == 0) tft.setTextColor(ST77XX_YELLOW); else tft.setTextColor(ST77XX_WHITE); tft.print("[START GAME]");
  tft.setCursor(20, 105); if (menuHoverSnake == 1) tft.setTextColor(ST77XX_YELLOW); else tft.setTextColor(ST77XX_WHITE); tft.print("Spd:");
  if (diffSettingSnake == 0) tft.print(" < EASY >"); else if (diffSettingSnake == 1) tft.print(" < MED  >"); else tft.print(" < HARD >");
  tft.setCursor(20, 140); if (menuHoverSnake == 2) tft.setTextColor(ST77XX_YELLOW); else tft.setTextColor(ST77XX_WHITE); tft.print("Appls:");
  if (appleSetting == 0) tft.print(" < 1 >"); else if (appleSetting == 1) tft.print(" < 3 >"); else tft.print(" < 5 >");
  tft.setCursor(20, 175); if (menuHoverSnake == 3) tft.setTextColor(ST77XX_YELLOW); else tft.setTextColor(ST77XX_WHITE); tft.print("Mode:");
  if (modeSetting == 0) tft.print(" <BORDER>"); else if (modeSetting == 1) tft.print(" < WRAP >"); else tft.print(" <MAGNET>");
  tft.setCursor(30, 210); if (menuHoverSnake == 4) tft.setTextColor(ST77XX_RED); else tft.setTextColor(ST77XX_WHITE); tft.print("[EXIT TO ARCADE]");
}

void spawnFoodSnake(int index) {
  bool safeToSpawn = false;
  while (!safeToSpawn) {
    foodX[index] = random(1, 23) * GRID_SIZE; foodY[index] = random(1, 23) * GRID_SIZE; safeToSpawn = true;
    for (int i = 0; i < snakeLength; i++) { if (foodX[index] == snakeX[i] && foodY[index] == snakeY[i]) { safeToSpawn = false; break; } }
  }
  tft.fillRect(foodX[index], foodY[index], GRID_SIZE, GRID_SIZE, ST77XX_RED);
}

void resetSnakeGame() {
  tft.fillScreen(ST77XX_BLACK); frameCount = 0; lastSnakeMove = millis(); 
  if (appleSetting == 0) numApples = 1; else if (appleSetting == 1) numApples = 3; else numApples = 5;
  if (modeSetting != 1) { tft.drawRect(9, 9, 222, 222, ST77XX_BLUE); }
  
  snakeLength = 3; dirX = 1; dirY = 0; 
  lastDirX = 1; lastDirY = 0; // Initialize Lock

  for(int i = 0; i < snakeLength; i++) {
    snakeX[i] = 50 - (i * GRID_SIZE); snakeY[i] = 120;
    tft.fillRect(snakeX[i], snakeY[i], GRID_SIZE, GRID_SIZE, ST77XX_GREEN);
  }
  foodX[0] = 150; foodY[0] = 120; tft.fillRect(foodX[0], foodY[0], GRID_SIZE, GRID_SIZE, ST77XX_RED);
  for (int i = 1; i < numApples; i++) { spawnFoodSnake(i); }
}

void runSnakeEngine() {
  if (snakeGameState == 0) {
    if (digitalRead(BTN_UP) == LOW) { while(digitalRead(BTN_UP) == LOW) delay(10); menuHoverSnake--; if (menuHoverSnake < 0) menuHoverSnake = 4; drawSnakeMenu(); }
    if (digitalRead(BTN_DOWN) == LOW) { while(digitalRead(BTN_DOWN) == LOW) delay(10); menuHoverSnake++; if (menuHoverSnake > 4) menuHoverSnake = 0; drawSnakeMenu(); }
    if (digitalRead(BTN_LEFT) == LOW) {
      while(digitalRead(BTN_LEFT) == LOW) delay(10);
      if (menuHoverSnake == 1) { diffSettingSnake--; if (diffSettingSnake < 0) diffSettingSnake = 2; }
      if (menuHoverSnake == 2) { appleSetting--; if (appleSetting < 0) appleSetting = 2; }
      if (menuHoverSnake == 3) { modeSetting--; if (modeSetting < 0) modeSetting = 2; } drawSnakeMenu();
    }
    if (digitalRead(BTN_RIGHT) == LOW) {
      while(digitalRead(BTN_RIGHT) == LOW) delay(10); 
      if (menuHoverSnake == 0) { 
        if (diffSettingSnake == 0) currentSpeed = 180; else if (diffSettingSnake == 1) currentSpeed = 120; else if (diffSettingSnake == 2) currentSpeed = 60;  
        snakeGameState = 1; resetSnakeGame();
      } else if (menuHoverSnake == 4) { osState = 1; menuHoverSnake = 0; drawGameCarousel(); } 
      else { 
        if (menuHoverSnake == 1) { diffSettingSnake++; if (diffSettingSnake > 2) diffSettingSnake = 0; }
        if (menuHoverSnake == 2) { appleSetting++; if (appleSetting > 2) appleSetting = 0; }
        if (menuHoverSnake == 3) { modeSetting++; if (modeSetting > 2) modeSetting = 0; } drawSnakeMenu();
      }
    }
    return; 
  }

  if (snakeGameState == 2) {
    tft.fillScreen(ST77XX_BLACK); int finalScore = snakeLength - 3;
    if (finalScore > highScoresSnake[diffSettingSnake][appleSetting][modeSetting]) { highScoresSnake[diffSettingSnake][appleSetting][modeSetting] = finalScore; }
    tft.setCursor(39, 40); tft.setTextColor(ST77XX_RED); tft.setTextSize(3); tft.print("GAME OVER");
    tft.setCursor(72, 90); tft.setTextColor(ST77XX_WHITE); tft.setTextSize(2); tft.print("Score: "); tft.print(finalScore);
    tft.setCursor(36, 125); tft.setTextColor(ST77XX_GREEN); tft.setTextSize(2); tft.print("High Score: "); tft.print(highScoresSnake[diffSettingSnake][appleSetting][modeSetting]);
    delay(2000); tft.setCursor(24, 175); tft.setTextColor(ST77XX_YELLOW); tft.setTextSize(2); tft.print("Press UP to Menu");
    while (digitalRead(BTN_UP) == LOW) { delay(10); } while (digitalRead(BTN_UP) == HIGH) { delay(10); } while (digitalRead(BTN_UP) == LOW) { delay(10); }
    snakeGameState = 0; drawSnakeMenu(); return;
  }

  // --- SNAKE CONTROLS WITH AXIS LOCK ---
  if (digitalRead(BTN_UP) == LOW && lastDirY != 1)     { dirX = 0; dirY = -1; }
  if (digitalRead(BTN_DOWN) == LOW && lastDirY != -1)  { dirX = 0; dirY = 1; }
  if (digitalRead(BTN_LEFT) == LOW && lastDirX != 1)   { dirX = -1; dirY = 0; }
  if (digitalRead(BTN_RIGHT) == LOW && lastDirX != -1) { dirX = 1; dirY = 0; }

  if (millis() - lastSnakeMove >= currentSpeed) {
    lastSnakeMove = millis(); 
    
    // LOCK IN THE DIRECTION FOR NEXT FRAME
    lastDirX = dirX;
    lastDirY = dirY;

    int nextX = snakeX[0] + (dirX * GRID_SIZE); int nextY = snakeY[0] + (dirY * GRID_SIZE);
    if (modeSetting == 1) { 
      if (nextX < 10) nextX = 220; else if (nextX > 220) nextX = 10;
      if (nextY < 10) nextY = 220; else if (nextY > 220) nextY = 10;
    } else { 
      if (nextX < 10 || nextX >= 230 || nextY < 10 || nextY >= 230) { snakeGameState = 2; return; }
    }
    for (int i = 0; i < snakeLength; i++) { if (nextX == snakeX[i] && nextY == snakeY[i]) { snakeGameState = 2; return; } }

    bool ateApple = false; int eatenIndex = -1;
    for (int i = 0; i < numApples; i++) { if (nextX == foodX[i] && nextY == foodY[i]) { ateApple = true; eatenIndex = i; break; } }
    if (ateApple) { if (snakeLength < MAX_SNAKE_LENGTH) { snakeLength++; } spawnFoodSnake(eatenIndex); } 
    else { tft.fillRect(snakeX[snakeLength - 1], snakeY[snakeLength - 1], GRID_SIZE, GRID_SIZE, ST77XX_BLACK); }

    for (int i = snakeLength - 1; i > 0; i--) { snakeX[i] = snakeX[i - 1]; snakeY[i] = snakeY[i - 1]; }
    snakeX[0] = nextX; snakeY[0] = nextY;
    tft.fillRect(snakeX[0], snakeY[0], GRID_SIZE, GRID_SIZE, ST77XX_GREEN);

    frameCount++;
    if (modeSetting == 2 && frameCount % 3 == 0) { 
      for (int i = 0; i < numApples; i++) {
        int oldX = foodX[i]; int oldY = foodY[i]; int newFoodX = foodX[i]; int newFoodY = foodY[i];
        if (foodX[i] < snakeX[0]) newFoodX += GRID_SIZE; else if (foodX[i] > snakeX[0]) newFoodX -= GRID_SIZE;
        else if (foodY[i] < snakeY[0]) newFoodY += GRID_SIZE; else if (foodY[i] > snakeY[0]) newFoodY -= GRID_SIZE;
        bool hitsSnake = false;
        for (int j = 0; j < snakeLength; j++) { if (newFoodX == snakeX[j] && newFoodY == snakeY[j]) { hitsSnake = true; break; } }
        if (!hitsSnake && newFoodX >= 10 && newFoodX < 230 && newFoodY >= 10 && newFoodY < 230) {
          tft.fillRect(oldX, oldY, GRID_SIZE, GRID_SIZE, ST77XX_BLACK); foodX[i] = newFoodX; foodY[i] = newFoodY;
          tft.fillRect(foodX[i], foodY[i], GRID_SIZE, GRID_SIZE, ST77XX_RED); 
        }
      }
    }
  } 
}

// ==========================================
// MAIN ARDUINO SETUP & LOOP
// ==========================================

void setup() {
  Serial.begin(115200);
  pinMode(BTN_UP, INPUT_PULLUP); pinMode(BTN_DOWN, INPUT_PULLUP); pinMode(BTN_LEFT, INPUT_PULLUP); pinMode(BTN_RIGHT, INPUT_PULLUP);
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS); 
  tft.init(240, 240); tft.setRotation(2); 
  osState = 0; 
}

void loop() {
  if (osState == 0) { playBootAnimation(); osState = 1; drawGameCarousel(); return; }
  if (osState == 1) {
    if (digitalRead(BTN_LEFT) == LOW) { while(digitalRead(BTN_LEFT) == LOW) { delay(10); } osSelection--; if (osSelection < 0) osSelection = 4; drawGameCarousel(); }
    if (digitalRead(BTN_RIGHT) == LOW) { while(digitalRead(BTN_RIGHT) == LOW) { delay(10); } osSelection++; if (osSelection > 4) osSelection = 0; drawGameCarousel(); }
    if (digitalRead(BTN_UP) == LOW) {
      while(digitalRead(BTN_UP) == LOW) { delay(10); } 
      if (osSelection == 0) { osState = 2; snakeGameState = 0; drawSnakeMenu(); }
      else if (osSelection == 2) { osState = 4; game2048State = 0; draw2048Menu(); } 
      else { osState = 3; showUnderConstruction(); }
    }
    return;
  }
  if (osState == 2) { runSnakeEngine(); return; }
  if (osState == 3) { return; } 
  if (osState == 4) { run2048Engine(); return; }
}