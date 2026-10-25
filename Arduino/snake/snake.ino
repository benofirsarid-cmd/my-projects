#include <Adafruit_GFX.h>
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

// --- GAME SETTINGS ---
#define GRID_SIZE 10
#define MAX_SNAKE_LENGTH 100

// --- STATE & MENU VARIABLES ---
int gameState = 0;       
int menuHover = 0;       
int diffSetting = 1;     // 0 = Easy, 1 = Med, 2 = Hard
int appleSetting = 0;    // 0 = 1, 1 = 3, 2 = 5
int modeSetting = 0;     // 0 = Border, 1 = Wrap, 2 = Magnet

int currentSpeed = 120;  
int numApples = 1;
int frameCount = 0;      

// --- HIGH SCORES ARRAY (3x3x3 Grid) ---
int highScores[3][3][3] = {0};

// --- SNAKE & FOOD ARRAYS ---
int snakeX[MAX_SNAKE_LENGTH];
int snakeY[MAX_SNAKE_LENGTH];
int snakeLength;
int dirX, dirY;
int foodX[5]; 
int foodY[5]; 

// ==========================================
// CUSTOM FUNCTIONS
// ==========================================

void drawMenu() {
  tft.fillScreen(ST77XX_BLACK);
  
  tft.drawRect(2, 2, 236, 236, ST77XX_GREEN);
  tft.drawRect(4, 4, 232, 232, ST77XX_GREEN);
  
  tft.setCursor(60, 20);
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(4);
  tft.print("SNAKE");

  tft.setTextSize(2); 

  // Option 0: Start Game
  tft.setCursor(54, 80);
  if (menuHover == 0) tft.setTextColor(ST77XX_YELLOW); else tft.setTextColor(ST77XX_WHITE);
  tft.print("[START GAME]");

  // Option 1: Difficulty
  tft.setCursor(20, 120);
  if (menuHover == 1) tft.setTextColor(ST77XX_YELLOW); else tft.setTextColor(ST77XX_WHITE);
  tft.print("Spd:");
  if (diffSetting == 0) tft.print(" < EASY >");
  else if (diffSetting == 1) tft.print(" < MED  >");
  else tft.print(" < HARD >");

  // Option 2: Apples
  tft.setCursor(20, 160);
  if (menuHover == 2) tft.setTextColor(ST77XX_YELLOW); else tft.setTextColor(ST77XX_WHITE);
  tft.print("Appls:");
  if (appleSetting == 0) tft.print(" < 1 >");
  else if (appleSetting == 1) tft.print(" < 3 >");
  else tft.print(" < 5 >");

  // Option 3: Game Mode
  tft.setCursor(20, 200);
  if (menuHover == 3) tft.setTextColor(ST77XX_YELLOW); else tft.setTextColor(ST77XX_WHITE);
  tft.print("Mode:");
  if (modeSetting == 0) tft.print(" <BORDER>");
  else if (modeSetting == 1) tft.print(" < WRAP >");
  else tft.print(" <MAGNET>");
}

void spawnFood(int index) {
  bool safeToSpawn = false;
  
  while (!safeToSpawn) {
    foodX[index] = random(1, 23) * GRID_SIZE; 
    foodY[index] = random(1, 23) * GRID_SIZE;
    safeToSpawn = true;
    
    for (int i = 0; i < snakeLength; i++) {
      if (foodX[index] == snakeX[i] && foodY[index] == snakeY[i]) {
        safeToSpawn = false; 
        break;
      }
    }
  }
  tft.fillRect(foodX[index], foodY[index], GRID_SIZE, GRID_SIZE, ST77XX_RED);
}

void resetGame() {
  tft.fillScreen(ST77XX_BLACK); 
  frameCount = 0;
  
  if (appleSetting == 0) numApples = 1;
  else if (appleSetting == 1) numApples = 3;
  else numApples = 5;

  if (modeSetting != 1) { 
    tft.drawRect(9, 9, 222, 222, ST77XX_BLUE);
  }
  
  snakeLength = 3;
  dirX = 1; 
  dirY = 0;

  for(int i = 0; i < snakeLength; i++) {
    snakeX[i] = 50 - (i * GRID_SIZE);
    snakeY[i] = 120;
    tft.fillRect(snakeX[i], snakeY[i], GRID_SIZE, GRID_SIZE, ST77XX_GREEN);
  }
  
  foodX[0] = 150;
  foodY[0] = 120;
  tft.fillRect(foodX[0], foodY[0], GRID_SIZE, GRID_SIZE, ST77XX_RED);
  
  for (int i = 1; i < numApples; i++) {
    spawnFood(i);
  }
}

// ==========================================
// MAIN SETUP
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

// ==========================================
// MAIN LOOP
// ==========================================
void loop() {
  
  // --- STATE 0: MAIN MENU ---
  if (gameState == 0) {
    if (digitalRead(BTN_UP) == LOW) {
      menuHover--;
      if (menuHover < 0) menuHover = 3; 
      drawMenu();
      delay(200); 
    }
    if (digitalRead(BTN_DOWN) == LOW) {
      menuHover++;
      if (menuHover > 3) menuHover = 0; 
      drawMenu();
      delay(200); 
    }

    if (digitalRead(BTN_LEFT) == LOW) {
      if (menuHover == 1) { diffSetting--; if (diffSetting < 0) diffSetting = 2; }
      if (menuHover == 2) { appleSetting--; if (appleSetting < 0) appleSetting = 2; }
      if (menuHover == 3) { modeSetting--; if (modeSetting < 0) modeSetting = 2; }
      drawMenu();
      delay(200);
    }
    
    if (digitalRead(BTN_RIGHT) == LOW) {
      if (menuHover == 0) { 
        if (diffSetting == 0) currentSpeed = 180;      
        else if (diffSetting == 1) currentSpeed = 120; 
        else if (diffSetting == 2) currentSpeed = 60;  
        
        gameState = 1; 
        resetGame();
      } else { 
        if (menuHover == 1) { diffSetting++; if (diffSetting > 2) diffSetting = 0; }
        if (menuHover == 2) { appleSetting++; if (appleSetting > 2) appleSetting = 0; }
        if (menuHover == 3) { modeSetting++; if (modeSetting > 2) modeSetting = 0; }
        drawMenu();
      }
      delay(200); 
    }
    return; 
  }

  // --- STATE 2: GAME OVER ---
  if (gameState == 2) {
    tft.fillScreen(ST77XX_BLACK);
    
    // Calculate final score
    int finalScore = snakeLength - 3;
    
    // Check and update High Score matrix
    if (finalScore > highScores[diffSetting][appleSetting][modeSetting]) {
      highScores[diffSetting][appleSetting][modeSetting] = finalScore;
    }
    int currentHigh = highScores[diffSetting][appleSetting][modeSetting];

    // Render Text
    tft.setCursor(39, 40); 
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(3);
    tft.print("GAME OVER");

    // Display Current Score
    tft.setCursor(72, 90);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(2);
    tft.print("Score: ");
    tft.print(finalScore);

    // Display High Score
    tft.setCursor(36, 125);
    tft.setTextColor(ST77XX_GREEN);
    tft.setTextSize(2);
    tft.print("High Score: ");
    tft.print(currentHigh);

    delay(2000); 

    tft.setCursor(24, 175);
    tft.setTextColor(ST77XX_YELLOW);
    tft.setTextSize(2);
    tft.print("Press UP to Menu");

    while (digitalRead(BTN_UP) == HIGH) { delay(10); }

    gameState = 0; 
    drawMenu();
    delay(200);
    return;
  }

  // --- STATE 1: PLAYING THE GAME ---
  
  if (digitalRead(BTN_UP) == LOW && dirY != 1)     { dirX = 0; dirY = -1; }
  if (digitalRead(BTN_DOWN) == LOW && dirY != -1)  { dirX = 0; dirY = 1; }
  if (digitalRead(BTN_LEFT) == LOW && dirX != 1)   { dirX = -1; dirY = 0; }
  if (digitalRead(BTN_RIGHT) == LOW && dirX != -1) { dirX = 1; dirY = 0; }

  int nextX = snakeX[0] + (dirX * GRID_SIZE);
  int nextY = snakeY[0] + (dirY * GRID_SIZE);

  if (modeSetting == 1) { // WRAP MODE
    if (nextX < 10) nextX = 220;
    else if (nextX > 220) nextX = 10;
    if (nextY < 10) nextY = 220;
    else if (nextY > 220) nextY = 10;
  } else { // BORDER OR MAGNET MODE
    if (nextX < 10 || nextX >= 230 || nextY < 10 || nextY >= 230) {
      gameState = 2; 
      return;
    }
  }

  for (int i = 0; i < snakeLength; i++) {
    if (nextX == snakeX[i] && nextY == snakeY[i]) {
      gameState = 2; 
      return;
    }
  }

  bool ateApple = false;
  int eatenIndex = -1;
  
  for (int i = 0; i < numApples; i++) {
    if (nextX == foodX[i] && nextY == foodY[i]) {
      ateApple = true;
      eatenIndex = i;
      break; 
    }
  }

  if (ateApple) {
    if (snakeLength < MAX_SNAKE_LENGTH) {
      snakeLength++; 
    }
    spawnFood(eatenIndex); 
  } else {
    tft.fillRect(snakeX[snakeLength - 1], snakeY[snakeLength - 1], GRID_SIZE, GRID_SIZE, ST77XX_BLACK);
  }

  for (int i = snakeLength - 1; i > 0; i--) {
    snakeX[i] = snakeX[i - 1];
    snakeY[i] = snakeY[i - 1];
  }

  snakeX[0] = nextX;
  snakeY[0] = nextY;
  tft.fillRect(snakeX[0], snakeY[0], GRID_SIZE, GRID_SIZE, ST77XX_GREEN);

  frameCount++;
  if (modeSetting == 2 && frameCount % 3 == 0) { 
    for (int i = 0; i < numApples; i++) {
      int oldX = foodX[i];
      int oldY = foodY[i];
      int newFoodX = foodX[i];
      int newFoodY = foodY[i];

      if (foodX[i] < snakeX[0]) newFoodX += GRID_SIZE;
      else if (foodX[i] > snakeX[0]) newFoodX -= GRID_SIZE;
      else if (foodY[i] < snakeY[0]) newFoodY += GRID_SIZE;
      else if (foodY[i] > snakeY[0]) newFoodY -= GRID_SIZE;

      bool hitsSnake = false;
      for (int j = 0; j < snakeLength; j++) {
        if (newFoodX == snakeX[j] && newFoodY == snakeY[j]) { hitsSnake = true; break; }
      }
      
      if (!hitsSnake && newFoodX >= 10 && newFoodX < 230 && newFoodY >= 10 && newFoodY < 230) {
        tft.fillRect(oldX, oldY, GRID_SIZE, GRID_SIZE, ST77XX_BLACK); 
        foodX[i] = newFoodX;
        foodY[i] = newFoodY;
        tft.fillRect(foodX[i], foodY[i], GRID_SIZE, GRID_SIZE, ST77XX_RED); 
      }
    }
  }

  delay(currentSpeed); 
}