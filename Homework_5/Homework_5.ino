//  Arduino LCD Platformer (Side-Scroller)
//  A Mario-style platformer game for 16x2 LCD
//  Features include:
//  - Custom physics (gravity, jumping, collision)
//  - Procedural-style level generation
//  - High score system using EEPROM
//  - Separation of Game Logic from Rendering and Input
 
#include <LiquidCrystal.h>
#include <EEPROM.h>

// Hardware Pins
const int PIN_RS = 9;
const int PIN_EN = 8;
const int PIN_D4 = 7;
const int PIN_D5 = 6;
const int PIN_D6 = 5;
const int PIN_D7 = 4;
const int PIN_JOY_X = A4;
const int PIN_JOY_Y = A5;
const int PIN_BTN = 2;

// Display Settings
const int LCD_COLS = 16;
const int LCD_ROWS = 2;
const int CAMERA_SCROLL_TRIGGER = 7; 

// Level Settings
const int LEVEL_LENGTH = 80; 
const int LEVEL_END_X = 76;  

// Physics Parameters
const float MOVE_SPEED = 0.3;
const int JUMP_DURATION_MS = 800;
const int PHYSICS_STEP_MS = 50; 
const int MSG_DISPLAY_MS = 3000; 

// Input Thresholds
const int JOYSTICK_LEFT_THRESHOLD = 400;
const int JOYSTICK_RIGHT_THRESHOLD = 600;
const int JOYSTICK_JUMP_THRESHOLD = 400;
const int BTN_DEBOUNCE_MS = 300;
const int JOY_DEBOUNCE_MS = 200;

// Scoring
const int SCORE_COIN = 50;
const int SCORE_KILL = 100;
const int TIME_BONUS_PAR_SEC = 80; 
const int TIME_BONUS_MULTIPLIER = 10;

// Tile IDs
const byte TILE_EMPTY = 0;
const byte TILE_PLATFORM = 1;
const byte TILE_COIN = 2;
const byte TILE_ENEMY = 3;
const byte TILE_FINISH = 4; // New Tile Type

// Sprite IDs (Shifted to 1-6 to avoid Null Byte 0 issues)
const byte SPRITE_MARIO_RUN = 1;
const byte SPRITE_MARIO_JUMP = 2;
const byte SPRITE_PLATFORM = 3;
const byte SPRITE_ENEMY = 4;
const byte SPRITE_COIN = 5;
const byte SPRITE_FINISH = 6; // New Sprite ID

// Game States
enum GameState { 
  STATE_MENU, 
  STATE_PLAYING, 
  STATE_HIGHSCORE, 
  STATE_GAMEOVER, 
  STATE_WIN 
};

struct HighScoreEntry {
  int score;
  int completionTime;
};

// Handles all game logic, physics, and state management
class GameModel {
public:
  struct Player {
    float x; 
    int y; 
    bool isJumping; 
    unsigned long jumpStartTime; 
    int score; 
    bool isDead;
    int prevY; 

    void update() {
      if (isJumping && (millis() - jumpStartTime > JUMP_DURATION_MS)) { 
        isJumping = false; 
      }
    }
  };

  struct Level {
    byte mapUpper[LEVEL_LENGTH];
    byte mapLower[LEVEL_LENGTH];

    // Populates the level randomly each time
    void generate() {
      randomSeed(analogRead(A0));

      // Clear Map
      for (int i = 0; i < LEVEL_LENGTH; i++) { 
        mapUpper[i] = TILE_EMPTY; 
        mapLower[i] = TILE_EMPTY; 
      }
      
      for (int i = 0; i < LEVEL_LENGTH; i++) {
        
        // --- START ZONE ---
        if (i < 10) {
          mapLower[i] = TILE_PLATFORM;
          continue; 
        }

        // --- FINISH ZONE ---
        // Place the Finish Flag at the very end
        if (i == LEVEL_END_X) {
          mapLower[i] = TILE_FINISH;
          mapUpper[i] = TILE_FINISH; // The Flag is on top
          continue;
        }
        // Safety zone around finish
        if (i > LEVEL_END_X) {
           mapLower[i] = TILE_PLATFORM;
           continue;
        }

        // --- GROUND GENERATION ---
        if (random(0, 100) < 10 && mapLower[i-1] == TILE_PLATFORM) {
           mapLower[i] = TILE_EMPTY; 
        } else {
           mapLower[i] = TILE_PLATFORM;  
        }

        // --- ENTITY GENERATION ---
        if (mapLower[i] == TILE_PLATFORM) {
          // Enemies
          if (random(0, 100) < 10) {
            mapLower[i] = TILE_ENEMY;
          }
          
          // Upper Structures
          if (mapUpper[i] == TILE_EMPTY) {
            if (random(0, 100) < 20) {
              mapUpper[i] = TILE_PLATFORM;
              if (random(0, 100) < 50 && i < LEVEL_LENGTH - 1) {
                 mapUpper[i+1] = TILE_COIN;
              }
            } 
            else if (random(0, 100) < 10) {
              mapUpper[i] = TILE_COIN;
            }
          }
        }
      }
    }

    byte getTile(int x, int y) {
      if (x < 0 || x >= LEVEL_LENGTH) return TILE_PLATFORM; 
      return (y == 0) ? mapUpper[x] : mapLower[x];
    }
    
    void setTile(int x, int y, byte val) {
      if (x < 0 || x >= LEVEL_LENGTH) return;
      if (y == 0) mapUpper[x] = val; else mapLower[x] = val;
    }
  };

  Player player;
  Level level;
  GameState currentState;
  unsigned long gameStartTime;
  unsigned long stateStartTime; 
  int elapsedTime;
  
  int menuCursor = 0;
  int hsRankViewer = 0;
  HighScoreEntry highScores[3];

  GameModel() {
    currentState = STATE_MENU;
    stateStartTime = 0; 
    loadHighScores();
  }

  void initGame() {
    level.generate();
    player.x = 1.0;
    player.y = 1;
    player.isJumping = false;
    player.score = 0;
    player.isDead = false;
    player.prevY = 1;
    gameStartTime = millis();
    elapsedTime = 0;
    currentState = STATE_PLAYING;
  }

  void updatePhysics() {
    if (currentState != STATE_PLAYING) return;

    elapsedTime = (millis() - gameStartTime) / 1000;
    player.prevY = player.y; 

    player.update(); 

    int px = (int)(player.x + 0.5);

    // Gravity
    if (player.isJumping) {
      player.y = 0; 
    } else {
      if (player.y == 0 && level.getTile(px, 0) == TILE_PLATFORM) {
        player.y = 0; 
      } else {
        player.y = 1; 
      }
    }

    // Pit Check
    if (player.y == 1 && level.getTile(px, 1) == TILE_EMPTY) { 
       gameOver(); 
       return; 
    }

    // Collision
    int tile = level.getTile(px, player.y);
    
    if (tile == TILE_FINISH) { // WIN CONDITION
       gameWin();
       return;
    }
    else if (tile == TILE_ENEMY) { 
      if (player.prevY == 0 && player.y == 1) {
        player.score += SCORE_KILL;
        level.setTile(px, 1, TILE_PLATFORM); 
        playerJump(); 
      } else {
        gameOver();
      }
    } else if (tile == TILE_COIN) { 
      player.score += SCORE_COIN;
      level.setTile(px, player.y, TILE_EMPTY); 
    }
  }

  void playerJump() {
    if (!player.isJumping) {
      player.isJumping = true;
      player.y = 0;
      player.jumpStartTime = millis();
    }
  }

  void gameOver() {
    player.isDead = true;
    updateHighScores(player.score, elapsedTime);
    currentState = STATE_GAMEOVER;
    stateStartTime = millis(); 
  }

  void gameWin() {
    player.isDead = true;
    int timeBonus = 0;
    if (elapsedTime < TIME_BONUS_PAR_SEC) {
      timeBonus = (TIME_BONUS_PAR_SEC - elapsedTime) * TIME_BONUS_MULTIPLIER;
    }
    player.score += timeBonus;
    updateHighScores(player.score, elapsedTime);
    currentState = STATE_WIN;
    stateStartTime = millis(); 
  }

  void loadHighScores() {
    for (int i = 0; i < 3; i++) {
      EEPROM.get(i * sizeof(HighScoreEntry), highScores[i]);
      if (highScores[i].score < 0) highScores[i] = {0, 0};
    }
  }

  void updateHighScores(int score, int time) {
    for (int i = 0; i < 3; i++) {
      if (score > highScores[i].score) {
        for (int j = 2; j > i; j--) highScores[j] = highScores[j-1];
        highScores[i] = {score, time};
        for (int k = 0; k < 3; k++) EEPROM.put(k * sizeof(HighScoreEntry), highScores[k]);
        break;
      }
    }
  }
};

// Interface for rendering to decouple logic from display hardware
class IRenderer {
public:
  virtual void init() = 0;
  virtual void render(GameModel& model) = 0;
};

// Implementation for 16x2 LCD
class LCDRenderer : public IRenderer {
private:
  LiquidCrystal lcd;
  int cameraX;
  GameState lastState; 
  
  // Custom Sprites
  byte marioRun[8] = {B01110, B01010, B01110, B10101, B00100, B01010, B10001, B11111};
  byte marioJump[8] = {B00100, B01010, B01110, B10101, B00100, B01010, B01110, B00000};
  byte platform[8] = {B00000, B00000, B00000, B00000, B00000, B00000, B00000, B11111};
  byte goomba[8] = {B01110, B10001, B00000, B01010, B00000, B11111, B01010, B11111};
  byte coin[8] = {B00000, B00100, B01010, B10101, B10101, B01010, B00100, B00000};
  byte finishFlag[8] = {B10101, B01010, B10101, B01010, B10101, B01010, B10101, B01010};

public:
  LCDRenderer() : lcd(PIN_RS, PIN_EN, PIN_D4, PIN_D5, PIN_D6, PIN_D7) {
    cameraX = 0;
    lastState = STATE_MENU;
  }

  void init() override {
    lcd.begin(LCD_COLS, LCD_ROWS);
    
    lcd.createChar(SPRITE_MARIO_RUN, marioRun);
    lcd.createChar(SPRITE_MARIO_JUMP, marioJump);
    lcd.createChar(SPRITE_PLATFORM, platform);
    lcd.createChar(SPRITE_ENEMY, goomba);
    lcd.createChar(SPRITE_COIN, coin);
    lcd.createChar(SPRITE_FINISH, finishFlag);
    
    lcd.clear();
  }

  void render(GameModel& model) override {
    if (model.currentState != lastState) {
      lcd.clear(); 
      if (model.currentState == STATE_PLAYING) cameraX = 0; 
      lastState = model.currentState;
    }

    if (model.currentState == STATE_MENU) {
      lcd.setCursor(0, 0); lcd.print(model.menuCursor == 0 ? "> Play       " : "  Play       ");
      lcd.setCursor(0, 1); lcd.print(model.menuCursor == 1 ? "> Highscores " : "  Highscores ");
      return;
    }
    
    if (model.currentState == STATE_HIGHSCORE) {
      lcd.setCursor(0, 0); lcd.print("Rank #"); lcd.print(model.hsRankViewer + 1);
      lcd.setCursor(0, 1); lcd.print("S:"); lcd.print(model.highScores[model.hsRankViewer].score);
      lcd.print(" T:"); lcd.print(model.highScores[model.hsRankViewer].completionTime);
      return;
    }
    
    if (model.currentState == STATE_GAMEOVER) {
      lcd.setCursor(3, 0); lcd.print("GAME OVER   ");
      lcd.setCursor(0, 1); lcd.print("S:"); lcd.print(model.player.score);
      lcd.print(" T:"); lcd.print(model.elapsedTime); lcd.print("s");
      return;
    }

    if (model.currentState == STATE_WIN) {
      lcd.setCursor(4, 0); lcd.print("YOU WIN!    ");
      lcd.setCursor(0, 1); lcd.print("S:"); lcd.print(model.player.score);
      lcd.print(" T:"); lcd.print(model.elapsedTime); lcd.print("s");
      return;
    }

    // Render Gameplay
    if (model.player.x - cameraX > CAMERA_SCROLL_TRIGGER) cameraX++;
    if (cameraX > LEVEL_LENGTH - LCD_COLS) cameraX = LEVEL_LENGTH - LCD_COLS; 

    for (int i = 0; i < LCD_COLS; i++) {
      int worldX = cameraX + i;
      drawTile(i, 0, model.level.getTile(worldX, 0));
      drawTile(i, 1, model.level.getTile(worldX, 1));
    }

    int screenX = (int)(model.player.x + 0.5) - cameraX;
    if (screenX >= 0 && screenX < LCD_COLS) {
      lcd.setCursor(screenX, model.player.y);
      bool onGround = (model.level.getTile((int)(model.player.x + 0.5), model.player.y) == TILE_PLATFORM);
      byte sprite = (model.player.isJumping || !onGround) ? SPRITE_MARIO_JUMP : SPRITE_MARIO_RUN;
      lcd.write(sprite);
    }

    lcd.setCursor(0, 0); lcd.print("T:"); lcd.print(model.elapsedTime);
    lcd.setCursor(10, 0); lcd.print("S:"); lcd.print(model.player.score);
  }

private:
  void drawTile(int x, int y, byte t) {
    lcd.setCursor(x, y);
    switch (t) {
      case TILE_PLATFORM: lcd.write(SPRITE_PLATFORM); break;
      case TILE_COIN: lcd.write(SPRITE_COIN); break;
      case TILE_ENEMY: lcd.write(SPRITE_ENEMY); break;
      case TILE_FINISH: lcd.write(SPRITE_FINISH); break;
      default: lcd.write(' '); break;
    }
  }
};

// Coordinates input processing and game loop timing
class GameController {
private:
  GameModel& model;
  IRenderer* renderer;
  unsigned long lastUpdate;
  unsigned long lastInputTime;

public:
  GameController(GameModel& m, IRenderer* r) : model(m), renderer(r) {
    lastUpdate = 0;
    lastInputTime = 0;
  }

  void setup() {
    renderer->init();
  }

  void loop() {
    unsigned long now = millis();
    if (now - lastUpdate > PHYSICS_STEP_MS) {
      handleInput();
      model.updatePhysics();
      renderer->render(model); 
      lastUpdate = now;
    }
  }

private:
  void handleInput() {
    int xVal = analogRead(PIN_JOY_X);
    int yVal = analogRead(PIN_JOY_Y);
    bool btn = (digitalRead(PIN_BTN) == LOW);
    
    if (model.currentState == STATE_MENU || model.currentState == STATE_HIGHSCORE) {
      if (millis() - lastInputTime < JOY_DEBOUNCE_MS) return;

      if (model.currentState == STATE_MENU) {
        if (yVal < JOYSTICK_LEFT_THRESHOLD) model.menuCursor = 0;
        if (yVal > JOYSTICK_RIGHT_THRESHOLD) model.menuCursor = 1;
        
        if (btn && millis() - lastInputTime > BTN_DEBOUNCE_MS) {
          if (model.menuCursor == 0) model.initGame();
          else model.currentState = STATE_HIGHSCORE;
          lastInputTime = millis();
        }
      } else { 
        if (yVal < JOYSTICK_LEFT_THRESHOLD && model.hsRankViewer > 0) model.hsRankViewer--;
        if (yVal > JOYSTICK_RIGHT_THRESHOLD && model.hsRankViewer < 2) model.hsRankViewer++;
        
        if (btn && millis() - lastInputTime > BTN_DEBOUNCE_MS) {
           model.currentState = STATE_MENU;
           lastInputTime = millis();
        }
      }
      if (yVal < JOYSTICK_LEFT_THRESHOLD || yVal > JOYSTICK_RIGHT_THRESHOLD) lastInputTime = millis();
      return;
    }

    if (model.currentState == STATE_GAMEOVER || model.currentState == STATE_WIN) {
       if (millis() - model.stateStartTime > MSG_DISPLAY_MS) { 
          model.currentState = STATE_MENU;
          lastInputTime = millis();
       }
       return;
    }

    if (model.currentState == STATE_PLAYING) {
      if (xVal < JOYSTICK_LEFT_THRESHOLD && model.player.x > 0) model.player.x -= MOVE_SPEED;
      if (xVal > JOYSTICK_RIGHT_THRESHOLD && model.player.x < LEVEL_LENGTH - 2) model.player.x += MOVE_SPEED;

      if (yVal < JOYSTICK_JUMP_THRESHOLD) {
        int px = (int)(model.player.x + 0.5);
        if (model.player.y == 0 || model.level.getTile(px, 0) != TILE_PLATFORM) {
          model.playerJump();
        }
      }
    }
  }
};

GameModel model;
LCDRenderer lcdRenderer;
GameController controller(model, &lcdRenderer);

void setup() {
  pinMode(PIN_JOY_X, INPUT);
  pinMode(PIN_JOY_Y, INPUT);
  pinMode(PIN_BTN, INPUT_PULLUP);
  controller.setup();
}

void loop() {
  controller.loop();
}