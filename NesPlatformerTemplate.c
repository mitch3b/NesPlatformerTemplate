#include "DEFINE.c"
#include "LevelData.c"

typedef struct {
  unsigned char x;
  unsigned char y;
  unsigned char picked_up;
} candle_struct;

#define MAX_CANDLES 4
candle_struct candles[MAX_CANDLES];
unsigned char candlesLeft;

#define ENEMY_SPEED 2
#define END_ENEMY_SPEED 1
#define MOVE_DOWN  0
#define MOVE_LEFT  1
#define MOVE_UP    2
#define MOVE_RIGHT 3
#define END_ENEMY_SPEED 1
#define END_ENEMY_SPEED 1
#define END_ENEMY_SPEED 1
typedef struct {
  unsigned char startX;
  unsigned char startY;
  unsigned char isHoriz;
  unsigned char x;
  unsigned char y;
  unsigned char isMoving;
} crusher_enemy_struct;

#define MAX_ENEMIES 10
crusher_enemy_struct enemies[MAX_ENEMIES];
unsigned char numEnemies;

// Blocks
#define BLOCK_ID_SOLID 0x01
#define BLOCK_ID_OPEN  0x02
#define BLOCK_ID_DEATH_SOLID 0x04
#define BLOCK_ID_DEATH 0x24
#define BLOCK_ID_START 0x06
#define BLOCK_ID_END   0x08
#define BLOCK_CANDLE   0x0A
#define BLOCK_ENEMY_HORIZONTAL 0x0B
#define BLOCK_ENEMY_VERTICAL    0x0C
#define END_ENEMY 0x0D

#pragma bss-name (push, "OAM")
unsigned char SPRITES[256];
#pragma bss-name (pop)

#pragma bss-name (push, "ZEROPAGE")
unsigned char timer;
unsigned char isHidden;
unsigned char avoidMovementBuffer;
unsigned char checkCollision;
unsigned char isWalking;
unsigned char walkingDirection;
unsigned char mainCharState;
unsigned char paletteToUpdate;
unsigned char paletteNum;
unsigned char prevPaletteToUpdate;
unsigned char paletteToUpdate2;
unsigned char paletteNum2;
unsigned char prevPaletteToUpdate2;
unsigned char paletteToUpdate3;
unsigned char paletteNum3;
unsigned char prevPaletteToUpdate3;

#define GAME_STATE_LOADING 0
#define GAME_STATE_LOADED_WAITING 1
#define GAME_STATE_PLAYING_LEVEL 2
#define GAME_STATE_LEVEL_COMPLETE 3

#define DEAD_FOR_THIS_MANY_FRAMES 30

#define MAIN_CHAR_ALIVE 0
#define MAIN_CHAR_WILL_DIE 1 //To finish movement animation before dying
#define MAIN_CHAR_DYING 2
#define MAIN_CHAR_DEAD 3

#define WALKING_SPEED 1     // Num steps per frame
#define WALKING_NUM_STEPS 16 //

unsigned char startX;
unsigned char startY;

#pragma bss-name (pop)
#pragma bss-name (push, "BSS")

// 32 x 30
// TODO should use a bit per block instead of byte, but this is a lot easier at the moment
unsigned char collision[240];

void main (void) {
  allOff(); // turn off screen
	song = 0;
  gameState = GAME_STATE_LOADING;
  levelNum = 4;
  startX = 1;
  startY = 0;
  isHidden = 0;

  paletteToUpdate = 0;
  paletteNum = 0;
  prevPaletteToUpdate = 0;
  paletteToUpdate2 = 0;
  paletteNum2 = 0;
  prevPaletteToUpdate2 = 0;
  paletteToUpdate3 = 0;
  paletteNum3 = 0;
  prevPaletteToUpdate3 = 0;

  loadPalette();
	resetScroll();

	Reset_Music(); // note, this is famitone init, and I added the music data address. see famitone2.s
	//Play_Music(song); // song = 0

	Wait_Vblank();
	allOn(); // turn on screen
	while (1){ // infinite loop
    while (NMI_flag == 0); // wait till NMI
    every_frame();

		Get_Input();

    if (gameState == GAME_STATE_LOADING) {
			allOff();

      _Hide_Sprites();
			loadLevel();

      loadCollisionFromNametables();

      candlesLeft = candleCount;
      drawCandles();

      for(temp1 = 0 ; temp1 < numEnemies ; temp1++) {
        enemies[temp1].x = enemies[temp1].startX;
        enemies[temp1].y = enemies[temp1].startY;
      }

      drawEnemies();

      hiddenModeOn();
      isWalking = 0;
      mainCharState = MAIN_CHAR_ALIVE;
      avoidMovementBuffer = 0;

      newX = startX;
      newY = startY;

      updateSprites();
			Wait_Vblank();
			allOn();
			resetScroll();
      gameState = GAME_STATE_PLAYING_LEVEL;//GAME_STATE_LOADED_WAITING;
    }
    else if (gameState == GAME_STATE_LOADED_WAITING) {
      if(isHidden == 0 &&
         (
           ((joypad1 & UP) != 0 && (joypad1old & UP) == 0) ||
           ((joypad1 & DOWN) != 0 && (joypad1old & DOWN) == 0) ||
           ((joypad1 & RIGHT) != 0 && (joypad1old & RIGHT) == 0) ||
           ((joypad1 & LEFT) != 0 && (joypad1old & LEFT) == 0)
         )) {

          hiddenModeOn();
          gameState = GAME_STATE_PLAYING_LEVEL;
      }
    }
    else if (gameState == GAME_STATE_PLAYING_LEVEL) {
      if(mainCharState == MAIN_CHAR_ALIVE || mainCharState == MAIN_CHAR_WILL_DIE) {
        if((joypad1old & UP) == 0 &&
           (joypad1old & DOWN) == 0 &&
           (joypad1old & RIGHT) == 0 &&
           (joypad1old & LEFT) == 0) {
          avoidMovementBuffer = 1;
        }

        //In game
        newX = SPRITES[MAIN_CHAR_SPRITE_INDEX + 3];
        newY = SPRITES[MAIN_CHAR_SPRITE_INDEX];

        if((joypad1 & A_BUTTON) != 0 && (joypad1old & A_BUTTON) == 0) {
          //changePalette();
          hiddenModeOff();
        }
        else if((joypad1old & A_BUTTON) != 0 && (joypad1 & A_BUTTON) == 0) {
          hiddenModeOn();
        }

        if(avoidMovementBuffer > 0) {
          move();
        }

        checkBackgroundCollision();
        checkCandleCollision();
        checkEnemyCollision();

        updateEnemies();

        updateSprites();
        drawEnemies();

        //TODO probably much faster to just do left/right shifts
        //TODO store whole palette in mem, only change the bytes we want and change old ones back
        prevPaletteToUpdate = paletteToUpdate;
        prevPaletteToUpdate2 = paletteToUpdate2;
        prevPaletteToUpdate3 = paletteToUpdate3;

        temp1 = newX + 8;
        temp2 = newY + 8;

        if(walkingDirection == LEFT) {
          temp1 = temp1 - 1;
        }
        else if(walkingDirection == UP) {
          temp2 = temp2 - 1;
        }

        temp3 = temp1 >> NUM_PIXELS_X_IN_PALETTE_BYTE_SHIFT;
        paletteToUpdate = temp2 >> NUM_PIXELS_X_IN_PALETTE_BYTE_SHIFT;
        paletteToUpdate = paletteToUpdate << NUM_PALETTES_ACROSS_IN_BYTE_SHIFT;
        paletteToUpdate = paletteToUpdate + temp3;
        temp3 = temp1 % NUM_PIXELS_X_IN_PALETTE_BYTE;
        temp3 = temp3 >> 4;

        temp4 = temp2 % NUM_PIXELS_X_IN_PALETTE_BYTE;
        temp4 = temp4 >> 4;
        temp4 = temp4*2;

        if(temp3 == 0) {
          //Left side
          paletteToUpdate2 = paletteToUpdate - 1;
          paletteNum2 = 1;
          paletteNum3 = 0;
        }
        else {
          //Right side
          paletteToUpdate2 = paletteToUpdate + 1;
          paletteNum2 = 0;
          paletteNum3 = 1;
        }

        if(temp4 == 0) {
          //top side
          paletteToUpdate3 = paletteToUpdate - 8;
          paletteNum3 = paletteNum3 + 2;
        }
        else {
          //bottom side
          paletteToUpdate3 = paletteToUpdate + 8;
          paletteNum2 = paletteNum2 + 2;
        }

        temp2 = paletteNum2*2;
        paletteNum2 = 0x03 << temp2;
        //paletteNum2 = paletteNum2 | 0xFF;
        temp2 = paletteNum3*2;
        paletteNum3 = 0x03 << temp2;
        //paletteNum3 = paletteNum3 | 0xFF;

        temp3 = temp3 + temp4;
        temp3 = 2*temp3;
        paletteNum = 0xC0; // 11000000
        paletteNum = paletteNum >> temp3;
        /*
        0 -> 11111100
        1 -> 11110011
        2 -> 11001111
        3 -> 00111111
         */
        paletteNum = ~paletteNum;
      }
      else if(mainCharState == MAIN_CHAR_DYING) {
        //Animation
        timer--;

        if(timer == 0) {
          mainCharState = MAIN_CHAR_DEAD;
        }
      }
      else if (mainCharState == MAIN_CHAR_DEAD) {
        newX = startX;
        newY = startY;
        prevPaletteToUpdate = paletteToUpdate;
        prevPaletteToUpdate2 = paletteToUpdate2;
        prevPaletteToUpdate3 = paletteToUpdate3;
        paletteNum = 0;
        avoidMovementBuffer = 0;
        isWalking = 0;

        mainCharState = MAIN_CHAR_ALIVE;
        updateSprites(); //Might be a cleaner way to reset the level
        isHidden = 1;

        for(temp1 = 0 ; temp1 < numEnemies ; temp1++) {
          enemies[temp1].x = enemies[temp1].startX;
          enemies[temp1].y = enemies[temp1].startY;
          enemies[temp1].isMoving = 0;
        }

        drawEnemies();

        // Reload candle (TODO better way to do this)
        candlesLeft = candleCount;

        for(temp2 = 0 ; temp2 < candleCount ; temp2++) {
          candles[temp2].picked_up = 0;
        }

        drawCandles();

        gameState = GAME_STATE_PLAYING_LEVEL;//GAME_STATE_LOADED_WAITING;
      }
    }
    else if (gameState == GAME_STATE_LEVEL_COMPLETE) {
      levelNum += 1;
      levelNum = levelNum % NUM_LEVELS;
      gameState = GAME_STATE_LOADING;
    }

    Music_Update();

    NMI_flag = 0;
  }
}

void drawCandles(void) {
  temp1 = CANDLE_SPRITE_INDEX;
  for(temp2 = 0 ; temp2 < candleCount ; temp2++) {
    SPRITES[temp1++] = candles[temp2].y + 4;//candles[temp2].y; //Y
    SPRITES[temp1++] = 0x10; //sprite
    SPRITES[temp1++] = 0x02; //attribute (flip vert, flip horiz, priority, 3x unused, 2x palette)
    SPRITES[temp1++] = candles[temp2].x + 4;//candles[temp2].x; //X
  }
}

void updateEnemies(void) {
  for(temp5 = 0; temp5 < numEnemies ; temp5++){
    if(enemies[temp5].isHoriz == 2 && candlesLeft == 0) {
      //Try moving right at him
      if(enemies[temp5].x % NUM_PIXELS_IN_TILE == 0 && enemies[temp5].y % NUM_PIXELS_IN_TILE == 0) {
        if(enemies[temp5].x == newX) {
          if(enemies[temp5].y > newY) {
            tempArray[0] = MOVE_UP;
            //TODO these two should oscillate or randomize
            tempArray[1] = MOVE_LEFT;
            tempArray[2] = MOVE_RIGHT;
            tempArray[3] = MOVE_DOWN;
          }
          else {
            tempArray[0] = MOVE_DOWN;
            //TODO these two should oscillate or randomize
            tempArray[1] = MOVE_LEFT;
            tempArray[2] = MOVE_RIGHT;
            tempArray[3] = MOVE_UP;
          }
        }
        else if(enemies[temp5].y == newY) {
          if(enemies[temp5].x > newX) {
            tempArray[0] = MOVE_LEFT;
            //TODO these two should oscillate or randomize
            tempArray[1] = MOVE_DOWN;
            tempArray[2] = MOVE_UP;
            tempArray[3] = MOVE_RIGHT;
          }
          else {
            tempArray[0] = MOVE_RIGHT;
            //TODO these two should oscillate or randomize
            tempArray[1] = MOVE_DOWN;
            tempArray[2] = MOVE_UP;
            tempArray[3] = MOVE_LEFT;
          }
        }
        else {
          // Move to whichever direction is furthest but direct
          temp2 = enemies[temp5].x > newX;
          temp3 = enemies[temp5].y > newY;

          if(temp2 && temp3) {
            if(enemies[temp5].x - newX > enemies[temp5].y - newY) {
              tempArray[0] = MOVE_UP;
              tempArray[1] = MOVE_LEFT;
              //TODO these two should oscillate or randomize
              tempArray[2] = MOVE_DOWN;
              tempArray[3] = MOVE_RIGHT;
            }
            else {
              tempArray[0] = MOVE_LEFT;
              tempArray[1] = MOVE_UP;
              //TODO these two should oscillate or randomize
              tempArray[2] = MOVE_DOWN;
              tempArray[3] = MOVE_RIGHT;
            }
          }
          else if(temp2 && !temp3) {
            if(enemies[temp5].x - newX > newY - enemies[temp5].y) {
              tempArray[0] = MOVE_DOWN;
              tempArray[1] = MOVE_LEFT;
              //TODO these two should oscillate or randomize
              tempArray[2] = MOVE_UP;
              tempArray[3] = MOVE_RIGHT;
            }
            else {
              tempArray[0] = MOVE_LEFT;
              tempArray[1] = MOVE_DOWN;
              //TODO these two should oscillate or randomize
              tempArray[2] = MOVE_UP;
              tempArray[3] = MOVE_RIGHT;
            }
          }
          else if(!temp2 && temp3) {
            if(newX - enemies[temp5].x > enemies[temp5].y - newY) {
              tempArray[0] = MOVE_UP;
              tempArray[1] = MOVE_RIGHT;
              //TODO these two should oscillate or randomize
              tempArray[2] = MOVE_DOWN;
              tempArray[3] = MOVE_LEFT;
            }
            else {
              tempArray[0] = MOVE_RIGHT;
              tempArray[1] = MOVE_UP;
              //TODO these two should oscillate or randomize
              tempArray[2] = MOVE_DOWN;
              tempArray[3] = MOVE_LEFT;
            }
          }
          else {
            if(newX - enemies[temp5].x > newY - enemies[temp5].y) {
              tempArray[0] = MOVE_DOWN;
              tempArray[1] = MOVE_RIGHT;
              //TODO these two should oscillate or randomize
              tempArray[2] = MOVE_UP;
              tempArray[3] = MOVE_LEFT;
            }
            else {
              tempArray[0] = MOVE_RIGHT;
              tempArray[1] = MOVE_DOWN;
              //TODO these two should oscillate or randomize
              tempArray[2] = MOVE_UP;
              tempArray[3] = MOVE_LEFT;
            }
          }
        }
      }

      for(temp1 = 0 ; temp1 < 4 ; temp1++) {
        if(enemies[temp5].isMoving == MOVE_DOWN) {
          temp2 = enemies[temp5].x;
          temp3 = enemies[temp5].y + END_ENEMY_SPEED;

          //temp4 points to bottom left
          temp4 = temp3 + 15;
          temp6 = temp2;
        }
        else if(enemies[temp5].isMoving == MOVE_LEFT) {
          temp2 = enemies[temp5].x - END_ENEMY_SPEED;
          temp3 = enemies[temp5].y;

          //temp4 points to top left
          temp4 = temp3;
          temp6 = temp2;
        }
        else if(enemies[temp5].isMoving == MOVE_UP) {
          temp2 = enemies[temp5].x;
          temp3 = enemies[temp5].y - END_ENEMY_SPEED;

          //temp4 points to top left
          temp4 = temp3;
          temp6 = temp2;
        }
        else if(enemies[temp5].isMoving == MOVE_RIGHT) {
          temp2 = enemies[temp5].x + END_ENEMY_SPEED;
          temp3 = enemies[temp5].y;

          //temp4 points to top right
          temp4 = temp3;
          temp6 = temp2 + 15;
        }

        //temp4 is y, temp6 is x. but block in temp4
        temp4 = temp4/NUM_PIXELS_IN_TILE;
        temp4 = temp4*NUM_TILES_X;
        temp6 = temp6/NUM_PIXELS_IN_TILE;
        temp4 = temp4 + temp6;

        if(collision[temp4] == BLOCK_ID_SOLID || collision[temp4] == BLOCK_ID_DEATH_SOLID) {
          //Try another direction
          enemies[temp5].isMoving = tempArray[temp1];
        }
        else {
          //Move
          enemies[temp5].x = temp2;
          enemies[temp5].y = temp3;
          break;
        }
      }
      continue;
    }

    if(enemies[temp5].isMoving > 0) {
      if(enemies[temp5].isHoriz == 0) {
        if(enemies[temp5].isMoving == 1) {
          // Down
          temp1 = enemies[temp5].y + ENEMY_SPEED;

          //temp3 points to bottom left
          temp4 = enemies[temp5].x;
          temp4 = temp4 >> 4;
          temp2 = (temp1 + 15) >> 4;
          temp3 = temp2*16 + temp4;
        }
        else {
          // up
          temp1 = enemies[temp5].y - ENEMY_SPEED;

          //temp3 points to top left
          temp4 = enemies[temp5].x >> 4;
          temp2 = temp1 >> 4;
          temp3 = temp2*16 + temp4;
        }
      }
      else if(enemies[temp5].isHoriz == 1) {
        if(enemies[temp5].isMoving == 1) {
          temp1 = enemies[temp5].x + ENEMY_SPEED;

          //temp3 points to top right
          temp4 = temp1 + 15;
          temp4 = temp4 >> 4;
          temp2 = enemies[temp5].y >> 4;
          temp3 = temp2*16 + temp4;
        }
        else {
          temp1 = enemies[temp5].x - ENEMY_SPEED;

          //temp3 points to top left
          temp4 = temp1 >> 4;
          temp2 = enemies[temp5].y >> 4;
          temp3 = temp2*16 + temp4;
        }
      }

      if(collision[temp3] == BLOCK_ID_SOLID || collision[temp3] == BLOCK_ID_DEATH_SOLID) {
        enemies[temp5].isMoving = 0;
      }
      else {
        if(enemies[temp5].isHoriz == 0) {
          enemies[temp5].y = temp1;
        }
        else if(enemies[temp5].isHoriz == 1) {
          enemies[temp5].x = temp1;
        }
      }
    }
    else {
      if(enemies[temp5].isHoriz == 1) {
        if(newY == enemies[temp5].y) {
          if(newX > enemies[temp5].x){
            enemies[temp5].isMoving = 1;
          }
          else {
            enemies[temp5].isMoving = 2;
          }
        }
      }
      else if(enemies[temp5].isHoriz == 0) {
        if(newX == enemies[temp5].x) {
          if(newY > enemies[temp5].y){
            enemies[temp5].isMoving = 1;
          }
          else {
            enemies[temp5].isMoving = 2;
          }
        }
      }
    }
  }
}

void drawEnemies(void) {
  temp1 = ENEMY_SPRITE_INDEX;

  for(temp5 = 0; temp5 < numEnemies ; temp5++){
    if(enemies[temp5].isHoriz == 0) {
      temp2 = 0x21;
      if(enemies[temp5].x == newX || enemies[temp5].y == newY || enemies[temp5].isMoving > 0) {
        temp2 = 0x20;
      }
      temp3 = 0x30;
    }
    else if(enemies[temp5].isHoriz == 1) {
      temp2 = 0x21;
      if(enemies[temp5].x == newX || enemies[temp5].y == newY || enemies[temp5].isMoving > 0 ) {
        temp2 = 0x31;
      }
      temp3 = 0x30;
    }
    else if(enemies[temp5].isHoriz == 2) {
      temp2 = 0x22;
      if(candlesLeft == 0) {
        temp2 = 0x23;
      }
      temp3 = 0x32;
    }

    SPRITES[temp1++] = enemies[temp5].y;
    SPRITES[temp1++] = temp2; //sprite
    SPRITES[temp1++] = 0x02; //attribute (flip vert, flip horiz, priority, 3x unused, 2x palette)
    SPRITES[temp1++] = enemies[temp5].x;

    SPRITES[temp1++] = enemies[temp5].y;
    SPRITES[temp1++] = temp2; //sprite
    SPRITES[temp1++] = 0x42; //attribute (flip vert, flip horiz, priority, 3x unused, 2x palette)
    SPRITES[temp1++] = enemies[temp5].x + 8;

    SPRITES[temp1++] = enemies[temp5].y + 8;
    SPRITES[temp1++] = temp3; //sprite
    SPRITES[temp1++] = 0x02; //attribute (flip vert, flip horiz, priority, 3x unused, 2x palette)
    SPRITES[temp1++] = enemies[temp5].x;//candles[temp2].x; //X

    SPRITES[temp1++] = enemies[temp5].y + 8;
    SPRITES[temp1++] = temp3; //sprite
    SPRITES[temp1++] = 0x42; //attribute (flip vert, flip horiz, priority, 3x unused, 2x palette)
    SPRITES[temp1++] = enemies[temp5].x + 8;//candles[temp2].x; //X
  }
}

void every_frame(void) {
  OAM_ADDRESS = 0;
  OAM_DMA = 2; // push all the sprite data from the ram at 200-2ff to the sprite me

  allOff();

  //Clear prev palette changes
  PPU_ADDRESS = 0x23;
  PPU_ADDRESS = 0xc0 + prevPaletteToUpdate;
  PPU_DATA = 0;
  PPU_ADDRESS = 0x23;
  PPU_ADDRESS = 0xc0 + prevPaletteToUpdate2;
  PPU_DATA = 0;
  PPU_ADDRESS = 0x23;
  PPU_ADDRESS = 0xc0 + prevPaletteToUpdate3;
  PPU_DATA = 0;

  PPU_ADDRESS = 0x23;
  PPU_ADDRESS = 0xc0 + paletteToUpdate;

  PPU_DATA = paletteNum;

  PPU_ADDRESS = 0x23;
  PPU_ADDRESS = 0xc0 + paletteToUpdate2;

  PPU_DATA = paletteNum2;

  PPU_ADDRESS = 0x23;
  PPU_ADDRESS = 0xc0 + paletteToUpdate3;

  PPU_DATA = paletteNum3;
  allOn();

  SCROLL = 0;
  SCROLL = 0xff;
}

void hiddenModeOff() {
  isHidden = 0;
  allOff();
  loadPalette();
  allOn();
  resetScroll();
  Wait_Vblank();
}

void hiddenModeOn() {
  isHidden = 1;
  allOff();
  loadHiddenPalette();
  allOn();
  resetScroll();
  Wait_Vblank();
}

void changePalette() {
  temp1 = newX % 16;
  temp2 = newY % 16;
  temp3 = temp2*8;
  temp3 = temp3 + temp1*2;
  temp3 = 0xc0;

  allOff();
  PPU_ADDRESS = 0x23;
  PPU_ADDRESS = 0xc0;
  PPU_ADDRESS = 0x00;

  PPU_DATA = 0x30;
  Wait_Vblank();
  allOn();
}

//Would be better to do in asm (like in UnCollision) but haven't figured out a good way yet
void loadCollisionFromNametables(void)
{
  PPU_ADDRESS = 0x20; // address of nametable #0
  PPU_ADDRESS = 0x00;

  //First read is always invalid
  tempInt = *((unsigned char*)0x2007);
  candleCount = 0;
  numEnemies = 0;

  for(tempInt = 0 ; tempInt < 240 ; tempInt++) {
    //Top left of 2x2 square
    temp1 = *((unsigned char*)0x2007);

    //Only pull top left corner block
    collision[tempInt] = temp1;

    if(temp1 == BLOCK_ID_START) {
      startX = 16*(tempInt % 16);
      startY = 16*(tempInt/16);
    }
    else if(temp1 == BLOCK_CANDLE) {
      candles[candleCount].x = 16*(tempInt % 16);
      candles[candleCount].y = 16*(tempInt/16);
      candles[candleCount].picked_up = 0;
      candleCount++;
    }
    else if(temp1 == BLOCK_ENEMY_HORIZONTAL) {
      enemies[numEnemies].startX = 16*(tempInt % 16);
      enemies[numEnemies].startY = 16*(tempInt/16);
      enemies[numEnemies].isHoriz = 1;
      enemies[numEnemies].isMoving = 0;
      enemies[numEnemies].x = enemies[numEnemies].startX;
      enemies[numEnemies].y = enemies[numEnemies].startY;
      numEnemies++;
    }
    else if(temp1 == BLOCK_ENEMY_VERTICAL) {
      enemies[numEnemies].startX = 16*(tempInt % 16);
      enemies[numEnemies].startY = 16*(tempInt/16);
      enemies[numEnemies].isHoriz = 0;
      enemies[numEnemies].isMoving = 0;
      enemies[numEnemies].x = enemies[numEnemies].startX;
      enemies[numEnemies].y = enemies[numEnemies].startY;
      numEnemies++;
    }
    else if(temp1 == END_ENEMY) {
      enemies[numEnemies].startX = 16*(tempInt % 16);
      enemies[numEnemies].startY = 16*(tempInt/16);
      enemies[numEnemies].isHoriz = 2;
      enemies[numEnemies].x = enemies[numEnemies].startX;
      enemies[numEnemies].y = enemies[numEnemies].startY;
      numEnemies++;
    }

    //Burn the right side of 2x2
    temp1 = *((unsigned char*)0x2007);

    if((tempInt % 16) == 15) {
      //skip every other row
      for(temp2 = 0; temp2 < 32 ; temp2++) {
        temp1 = *((unsigned char*)0x2007);
      }
    }
  }
}

void move(void) {
  if(isWalking == 0) {
    if((joypad1 & LEFT) != 0) {
      isWalking = WALKING_NUM_STEPS;
      walkingDirection = LEFT;
      checkCollision = 1;
    }
    else if((joypad1 & RIGHT) != 0) {
      isWalking = WALKING_NUM_STEPS;
      walkingDirection = RIGHT;
      checkCollision = 1;
    }
    if((joypad1 & UP) != 0) {
      isWalking = WALKING_NUM_STEPS;
      walkingDirection = UP;
      checkCollision = 1;
    }
    else if((joypad1 & DOWN) != 0) {
      isWalking = WALKING_NUM_STEPS;
      walkingDirection = DOWN;
      checkCollision = 1;
    }
  }

  if(isWalking > 0) {
    if(walkingDirection == LEFT) {
      newX -= WALKING_SPEED;
    }
    else if(walkingDirection == RIGHT) {
      newX += WALKING_SPEED;
    }
    if(walkingDirection == UP) {
      newY -= WALKING_SPEED;
    }
    else if(walkingDirection == DOWN) {
      newY += WALKING_SPEED;
    }

    isWalking -= 1;

    if(isWalking == 0 && mainCharState == MAIN_CHAR_WILL_DIE) {
        mainCharState = MAIN_CHAR_DYING;
        timer = DEAD_FOR_THIS_MANY_FRAMES;
    }

    //if(true) {
    temp1 = newX/4;
    //}
  }
}

void putCharInBackgroundVars(void) {
  collisionX = newX;
  collisionY = newY;
  collisionWidth = CHARACTER_WIDTH;
  collisionHeight = CHARACTER_HEIGHT;
}

void checkEnemyCollision(void) {
  // Only do this math once. Forces enemies to be the same size
  temp1 = newY + CHARACTER_HEIGHT;
  temp3 = newX + CHARACTER_WIDTH;
  temp4 = newY - CHARACTER_HEIGHT;
  temp5 = newX - CHARACTER_WIDTH;

  for(temp2 = 0 ; temp2 < numEnemies ; temp2++) {
   if(temp4 <= enemies[temp2].y && temp1 >= enemies[temp2].y &&
      temp5 <= enemies[temp2].x && temp3 >= enemies[temp2].x) {

      mainCharState = MAIN_CHAR_DYING;
      timer = DEAD_FOR_THIS_MANY_FRAMES;
   }
  }
}

void checkCandleCollision(void) {
 for(temp2 = 0 ; temp2 < candleCount ; temp2++) {
   if(candles[temp2].picked_up == 0) {
     if(newY == candles[temp2].y && newX == candles[temp2].x) {
       // Hide candle
       candles[temp2].picked_up = 1;
       candlesLeft--;

       temp1 = (temp2 << 2) + CANDLE_SPRITE_INDEX;
       SPRITES[temp1] = 0x00;
       SPRITES[temp1 + 3] = 0x00;
     }
   }
 }
}

void checkBackgroundCollision(void) {
  //Only checking background collisions at the start and end of movement
  if(checkCollision > 0) {

    if(walkingDirection == LEFT) {
      //temp3 points top left
      temp1 = newX >> 4;
      temp2 = newY >> 4;
      temp3 = temp2*16 + temp1;

      //temp4 points bottom left
      temp1 = newX >> 4;
      temp2 = (newY + 15) >> 4;
      temp4 = temp2*16 + temp1;
    }
    else if(walkingDirection == RIGHT) {
      //temp3 points top right
      temp1 = (newX + 15) >> 4;
      temp2 = newY >> 4;
      temp3 = temp2*16 + temp1;

      //temp4 points bottom right
      temp1 = (newX + 15) >> 4;
      temp2 = (newY + 15) >> 4;
      temp4 = temp2*16 + temp1;
    }
    if(walkingDirection == UP) {
      //temp3 points top left
      temp1 = newX >> 4;
      temp2 = newY >> 4;
      temp3 = temp2*16 + temp1;

      //temp4 points top right
      temp1 = (newX + 15) >> 4;
      temp2 = newY >> 4;
      temp4 = temp2*16 + temp1;
    }
    else if(walkingDirection == DOWN) {
      //temp3 points bottom left
      temp1 = newX >> 4;
      temp2 = (newY + 15) >> 4;
      temp3 = temp2*16 + temp1;

      //temp4 points bottom right
      temp1 = (newX + 15) >> 4;
      temp2 = (newY + 15) >> 4;
      temp4 = temp2*16 + temp1;
    }

    //TODO next, check for solid, then death? but probably need to check two corners for each direction
    temp1 = BLOCK_ID_OPEN;

    if(collision[temp3] == BLOCK_ID_SOLID ||
       collision[temp4] == BLOCK_ID_SOLID) {
         temp1 = BLOCK_ID_SOLID;
    }
    else if(collision[temp3] == BLOCK_ID_DEATH ||
            collision[temp4] == BLOCK_ID_DEATH ||
            collision[temp3] == BLOCK_ID_DEATH_SOLID ||
            collision[temp4] == BLOCK_ID_DEATH_SOLID) {
        temp1 = BLOCK_ID_DEATH;
    }
    else if(collision[temp3] == BLOCK_ID_END &&
            collision[temp4] == BLOCK_ID_END) {
        temp1 = BLOCK_ID_END;
    }
    else if(candleCount > 0 &&
             (collision[temp3] == END_ENEMY ||
              collision[temp4] == END_ENEMY)) {
      temp1 = BLOCK_ID_DEATH;
    }

    if(temp1 == BLOCK_ID_SOLID) {
      //Go back and don't move
      newX = SPRITES[MAIN_CHAR_SPRITE_INDEX + 3];
      newY = SPRITES[MAIN_CHAR_SPRITE_INDEX];
      isWalking = 0;
    }
    else if(temp1 == BLOCK_ID_DEATH) {
      mainCharState = MAIN_CHAR_WILL_DIE;
    }
    else if(temp1 == BLOCK_ID_END && candlesLeft == 0) {
      gameState = GAME_STATE_LEVEL_COMPLETE;
    }

    checkCollision = 0;
  }
}

void updateSprites(void) {
  temp1 = MAIN_CHAR_FIRST_SPRITE

  if(mainCharState != MAIN_CHAR_ALIVE) {
    temp1 = MAIN_CHAR_DEAD_FIRST_SPRITE;
  }

  SPRITES[MAIN_CHAR_SPRITE_INDEX]     = newY; //Y
  SPRITES[MAIN_CHAR_SPRITE_INDEX + 1] = temp1; //sprite
  SPRITES[MAIN_CHAR_SPRITE_INDEX + 2] = 0x00; //attribute
  SPRITES[MAIN_CHAR_SPRITE_INDEX + 3] = newX; //X

  temp1 += 1;
  SPRITES[MAIN_CHAR_SPRITE_INDEX + 4] = newY; //Y
  SPRITES[MAIN_CHAR_SPRITE_INDEX + 5] = temp1; //sprite
  SPRITES[MAIN_CHAR_SPRITE_INDEX + 6] = 0x00; //attribute
  SPRITES[MAIN_CHAR_SPRITE_INDEX + 7] = newX + 8; //X

  temp1 += 1;
  SPRITES[MAIN_CHAR_SPRITE_INDEX + 8] = newY + 8; //Y
  SPRITES[MAIN_CHAR_SPRITE_INDEX + 9] = temp1; //sprite
  SPRITES[MAIN_CHAR_SPRITE_INDEX + 10] = 0x00; //attribute
  SPRITES[MAIN_CHAR_SPRITE_INDEX + 11] = newX; //X

  temp1 += 1;
  SPRITES[MAIN_CHAR_SPRITE_INDEX + 12] = newY + 8; //Y
  SPRITES[MAIN_CHAR_SPRITE_INDEX + 13] = temp1; //sprite
  SPRITES[MAIN_CHAR_SPRITE_INDEX + 14] = 0x00; //attribute
  SPRITES[MAIN_CHAR_SPRITE_INDEX + 15] = newX + 8; //X

  temp1 = CANDLE_SPRITE_INDEX + 1;

  //TODO not sure why can't use candleCount here
  for(temp2 = 0 ; temp2 < candleCount ; temp2++) {
    if((Frame_Count % 10) < 5) {
      SPRITES[temp1] = 0x10; //sprite
    }
    else {
      SPRITES[temp1] = 0x11; //sprite
    }

    if((Frame_Count % 20) < 10) {
      SPRITES[temp1 + 1] = 0x02; //attribute (flip vert, flip horiz, priority, 3x unused, 2x palette)
    }
    else {
      SPRITES[temp1 + 1] = 0x42; //attribute (flip vert, flip horiz, priority, 3x unused, 2x palette)
    }

    temp1 += 4;
  }
}

/**
 *  DCBA98 76543210
 *  ---------------
 *  0HRRRR CCCCPTTT
 *  |||||| |||||+++- T: Fine Y offset, the row number within a tile
 *  |||||| ||||+---- P: Bit plane (0: "lower"; 1: "upper")
 *  |||||| ++++----- C: Tile column
 *  ||++++---------- R: Tile row
 *  |+-------------- H: Half of sprite table (0: "left"; 1: "right")
 *  +--------------- 0: Pattern table is at $0000-$1FFF
 */
void allOff (void) {
	PPU_CTRL = 0;
	PPU_MASK = 0;
}

void allOn (void) {
	PPU_CTRL = 0x88; // This will turn NMI on, make sure this matches the one in the NMI loop
	PPU_MASK = 0x1e; // enable all rendering
}

void resetScroll (void) {
	PPU_ADDRESS = 0;
	PPU_ADDRESS = 0;
	SCROLL = 0;
	SCROLL = 0xff;
}
