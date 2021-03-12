#include "DEFINE.c"
#include "LevelData.c"

// Physics
#define GRAVITY 1
#define JUMP_VELOCITY -8
#define VELOCITY_FACTOR 4 //No decimals so only use 1/VELOCITY_FACTOR'th of the velocity
#define MAX_JUMP_COUNT 10 //Number of frames you can hold jump and keep traveling up
#define MAX_VELOCITY_X 1
#define MAX_VELOCITY_WITH_B_X 2

#pragma bss-name (push, "OAM")
unsigned char SPRITES[256];
#pragma bss-name (pop)

#pragma bss-name (push, "ZEROPAGE")
unsigned char timer;
unsigned char isHidden;
unsigned char checkCollision;
unsigned char isWalking;
unsigned char walkingDirection;
unsigned char mainCharState;
unsigned char numCandles;
unsigned char palletteToUpdate;
unsigned char palletteNum;
unsigned char prevPalletteToUpdate;
unsigned char palletteToUpdate2;
unsigned char palletteNum2;
unsigned char prevPalletteToUpdate2;
unsigned char palletteToUpdate3;
unsigned char palletteNum3;
unsigned char prevPalletteToUpdate3;

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
#define WALKING_NUM_STEPS 8 //

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
  levelNum = 0;
  startX = 1;
  startY = 0;
  isHidden = 0;
  palletteToUpdate = 0;
  palletteNum = 0;
  prevPalletteToUpdate = 0;
  palletteToUpdate2 = 0;
  palletteNum2 = 0;
  prevPalletteToUpdate2 = 0;
  palletteToUpdate3 = 0;
  palletteNum3 = 0;
  prevPalletteToUpdate3 = 0;

  loadPalette();
	resetScroll();

	Reset_Music(); // note, this is famitone init, and I added the music data address. see famitone2.s
	Play_Music(song); // song = 0

	Wait_Vblank();
	allOn(); // turn on screen
	while (1){ // infinite loop
    while (NMI_flag == 0); // wait till NMI
    every_frame();

		Get_Input();

    if (gameState == GAME_STATE_LOADING) {
			allOff();

			loadLevel();

      loadCollisionFromNametables();

      hiddenModeOff();
      mainCharState = MAIN_CHAR_ALIVE;

      newX = startX;
      newY = startY;

      updateSprites();
			Wait_Vblank();
			allOn();
			resetScroll();
      gameState = GAME_STATE_LOADED_WAITING;
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
        //In game
        newX = SPRITES[MAIN_CHAR_SPRITE_INDEX + 3];
        newY = SPRITES[MAIN_CHAR_SPRITE_INDEX];

        if((joypad1 & A_BUTTON) != 0 && (joypad1old & A_BUTTON) == 0) {
          //changePallette();
          hiddenModeOff();
        }
        else if((joypad1old & A_BUTTON) != 0 && (joypad1 & A_BUTTON) == 0) {
          hiddenModeOn();
        }

        move();
        checkBackgroundCollision();

        updateSprites();

        //TODO probably much faster to just do left/right shifts
        //TODO store whole palette in mem, only change the bytes we want and change old ones back
        prevPalletteToUpdate = palletteToUpdate;
        prevPalletteToUpdate2 = palletteToUpdate2;
        prevPalletteToUpdate3 = palletteToUpdate3;

        temp1 = newX + 8;
        temp2 = newY + 8;

        if(walkingDirection == LEFT) {
          temp1 = temp1 - 1;
        }
        else if(walkingDirection == UP) {
          temp2 = temp2 - 1;
        }

        temp3 = temp1/NUM_PIXELS_X_IN_PALLETTE_BYTE;
        palletteToUpdate = temp2/NUM_PIXELS_X_IN_PALLETTE_BYTE;
        palletteToUpdate = palletteToUpdate*NUM_PALLETTES_ACROSS_IN_BYTE;
        palletteToUpdate = palletteToUpdate + temp3;
        temp3 = temp1 % NUM_PIXELS_X_IN_PALLETTE_BYTE;
        temp3 = temp3/16;

        temp4 = temp2 % NUM_PIXELS_X_IN_PALLETTE_BYTE;
        temp4 = temp4/16;
        temp4 = temp4*2;

        if(temp3 == 0) {
          //Left side
          palletteToUpdate2 = palletteToUpdate - 1;
          palletteNum2 = 1;
          palletteNum3 = 0;
        }
        else {
          //Right side
          palletteToUpdate2 = palletteToUpdate + 1;
          palletteNum2 = 0;
          palletteNum3 = 1;
        }

        if(temp4 == 0) {
          //top side
          palletteToUpdate3 = palletteToUpdate - 8;
          palletteNum3 = palletteNum3 + 2;
        }
        else {
          //bottom side
          palletteToUpdate3 = palletteToUpdate + 8;
          palletteNum2 = palletteNum2 + 2;
        }

        temp2 = palletteNum2*2;
        palletteNum2 = 0x03 << temp2;
        palletteNum2 = palletteNum2 | 0xAA;
        temp2 = palletteNum3*2;
        palletteNum3 = 0x03 << temp2;
        palletteNum3 = palletteNum3 | 0xAA;

        temp3 = temp3 + temp4;
        temp3 = 2*temp3;
        palletteNum = 0xC0;
        palletteNum = palletteNum >> temp3;
        /*
        0 -> 11111100
        1 -> 11110011
        2 -> 11001111
        3 -> 00111111
         */
        palletteNum = ~palletteNum;


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
        prevPalletteToUpdate = palletteToUpdate;
        prevPalletteToUpdate2 = palletteToUpdate2;
        prevPalletteToUpdate3 = palletteToUpdate3;
        palletteNum = 0;
        mainCharState = MAIN_CHAR_ALIVE;
        updateSprites(); //Might be a cleaner way to reset the level
        isHidden = 0;
        allOff();
        loadPalette();
        Wait_Vblank();
        allOn();
        gameState = GAME_STATE_LOADED_WAITING;
      }
    }
    else if (gameState == GAME_STATE_LEVEL_COMPLETE) {
      levelNum += 1;
      levelNum = levelNum % 3; //Currently only 3
      gameState = GAME_STATE_LOADING;
      isHidden = 0;
    }

    Music_Update();

    NMI_flag = 0;
  }
}

void every_frame(void) {
  OAM_ADDRESS = 0;
  OAM_DMA = 2; // push all the sprite data from the ram at 200-2ff to the sprite me

  allOff();

  //Clear prev pallette changes
  PPU_ADDRESS = 0x23;
  PPU_ADDRESS = 0xc0 + prevPalletteToUpdate;
  PPU_DATA = 0;
  PPU_ADDRESS = 0x23;
  PPU_ADDRESS = 0xc0 + prevPalletteToUpdate2;
  PPU_DATA = 0;
  PPU_ADDRESS = 0x23;
  PPU_ADDRESS = 0xc0 + prevPalletteToUpdate3;
  PPU_DATA = 0;

  PPU_ADDRESS = 0x23;
  PPU_ADDRESS = 0xc0 + palletteToUpdate;

  PPU_DATA = palletteNum;

  PPU_ADDRESS = 0x23;
  PPU_ADDRESS = 0xc0 + palletteToUpdate2;

  PPU_DATA = palletteNum2;

  PPU_ADDRESS = 0x23;
  PPU_ADDRESS = 0xc0 + palletteToUpdate3;

  PPU_DATA = palletteNum3;
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

void changePallette() {
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

#define BLOCK_ID_SOLID 0x01
#define BLOCK_ID_OPEN  0x02
#define BLOCK_ID_DEATH 0x04
#define BLOCK_ID_START 0x06
#define BLOCK_ID_END   0x08

//Would be better to do in asm (like in UnCollision) but haven't figured out a good way yet
void loadCollisionFromNametables(void)
{
  PPU_ADDRESS = 0x20; // address of nametable #0
  PPU_ADDRESS = 0x00;

  //First read is always invalid
  tempInt = *((unsigned char*)0x2007);

  for(tempInt = 0 ; tempInt < 240 ; tempInt++) {
    //Top left of 2x2 square
    temp1 = *((unsigned char*)0x2007);

    //Only pull top left corner block
    collision[tempInt] = temp1;

    if(temp1 == BLOCK_ID_START) {
      startX = 16*(tempInt % 16);
      startY = 16*(tempInt/16);
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

void checkBackgroundCollision(void) {
  //Only checking background collisions at the start and end of movement
  if(checkCollision > 0) {

    if(walkingDirection == LEFT) {
      //temp3 points top left
      temp1 = newX/16;
      temp2 = newY/16;
      temp3 = temp2*16 + temp1;

      //temp4 points bottom left
      temp1 = newX/16;
      temp2 = (newY + 15)/16;
      temp4 = temp2*16 + temp1;
    }
    else if(walkingDirection == RIGHT) {
      //temp3 points top right
      temp1 = (newX + 15)/16;
      temp2 = newY/16;
      temp3 = temp2*16 + temp1;

      //temp4 points bottom right
      temp1 = (newX + 15)/16;
      temp2 = (newY + 15)/16;
      temp4 = temp2*16 + temp1;
    }
    if(walkingDirection == UP) {
      //temp3 points top left
      temp1 = newX/16;
      temp2 = newY/16;
      temp3 = temp2*16 + temp1;

      //temp4 points top right
      temp1 = (newX + 15)/16;
      temp2 = newY/16;
      temp4 = temp2*16 + temp1;
    }
    else if(walkingDirection == DOWN) {
      //temp3 points bottom left
      temp1 = newX/16;
      temp2 = (newY + 15)/16;
      temp3 = temp2*16 + temp1;

      //temp4 points bottom right
      temp1 = (newX + 15)/16;
      temp2 = (newY + 15)/16;
      temp4 = temp2*16 + temp1;
    }

    //TODO next, check for solid, then death? but probably need to check two corners for each direction
    temp1 = BLOCK_ID_OPEN;

    if(collision[temp3] == BLOCK_ID_SOLID ||
       collision[temp4] == BLOCK_ID_SOLID) {
         temp1 = BLOCK_ID_SOLID;
    }
    else if(collision[temp3] == BLOCK_ID_DEATH ||
            collision[temp4] == BLOCK_ID_DEATH) {
        temp1 = BLOCK_ID_DEATH;
    }
    else if(collision[temp3] == BLOCK_ID_END &&
            collision[temp4] == BLOCK_ID_END) {
        temp1 = BLOCK_ID_END;
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
    else if(temp1 == BLOCK_ID_END) {
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
