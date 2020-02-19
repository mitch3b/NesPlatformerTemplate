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

#define GAME_STATE_LOADING 0
#define GAME_STATE_PLAYING_LEVEL 1
#define GAME_STATE_LEVEL_COMPLETE 2

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

  loadPalette();
	resetScroll();

	Reset_Music(); // note, this is famitone init, and I added the music data address. see famitone2.s
	Play_Music(song); // song = 0

	Wait_Vblank();
	allOn(); // turn on screen
	while (1){ // infinite loop
    while (NMI_flag == 0); // wait till NMI

		//every_frame();	// moved this to the nmi code in reset.s for greater stability
		Get_Input();

    if (gameState == GAME_STATE_LOADING) {
			allOff();

			loadLevel();

      loadCollisionFromNametables();

      mainCharState = MAIN_CHAR_ALIVE;

      newX = startX;
      newY = startY;

      updateSprites();
			Wait_Vblank();
			allOn();
			resetScroll();
      gameState = GAME_STATE_PLAYING_LEVEL;
    }
    else if (gameState == GAME_STATE_PLAYING_LEVEL) {
      if(isHidden == 0 &&
         (
           ((joypad1 & UP) != 0 && (joypad1old & UP) == 0) ||
           ((joypad1 & DOWN) != 0 && (joypad1old & DOWN) == 0) ||
           ((joypad1 & RIGHT) != 0 && (joypad1old & RIGHT) == 0) ||
           ((joypad1 & LEFT) != 0 && (joypad1old & LEFT) == 0)
         )) {
          allOff();
          if(isHidden != 0) {
            loadPalette();
            isHidden = 0;
          }
          else {
            loadHiddenPalette();
            isHidden = 1;
          }

          Wait_Vblank();
    			allOn();
      }

      if(mainCharState == MAIN_CHAR_ALIVE || mainCharState == MAIN_CHAR_WILL_DIE) {
        //In game
        newX = SPRITES[MAIN_CHAR_SPRITE_INDEX + 3];
        newY = SPRITES[MAIN_CHAR_SPRITE_INDEX];
        move();
        checkBackgroundCollision();

        updateSprites();
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
        updateSprites(); //Might be a cleaner way to reset the level
        mainCharState = MAIN_CHAR_ALIVE;
        isHidden = 0;
        allOff();
        loadPalette();
        Wait_Vblank();
        allOn();
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
	PPU_CTRL = 0x80; // This will turn NMI on, make sure this matches the one in the NMI loop
	PPU_MASK = 0x1e; // enable all rendering
}

void resetScroll (void) {
	PPU_ADDRESS = 0;
	PPU_ADDRESS = 0;
	SCROLL = 0;
	SCROLL = 0xff;
}
