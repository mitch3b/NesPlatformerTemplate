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
signed char yVelocity;
signed char jumpCount;
signed char isFalling;

//POWERUPS
unsigned char powerUp;
unsigned char powerUpState;

#pragma bss-name (pop)
#pragma bss-name (push, "BSS")

// 32 x 30
// TODO should use a bit per block instead of byte, but this is a lot easier at the moment
unsigned char collision[960];

void main (void) {
  allOff(); // turn off screen
	song = 0;
  gameState = 1;
  levelNum = 0;
  powerUp = 1;
  powerUpState = 0;

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

    if (gameState == 1) {
			allOff();

			loadLevel();
      for(index = 0 ; index < 255 ; index++) {
        collision[index] = index;
      }
      //UnCollision();
      loadCollisionFromNametables();

      initSprites();
			Wait_Vblank();
			allOn();
			resetScroll();
      gameState = 2;
    }
    else if (gameState == 2) {
      //In game
      newX = SPRITES[MAIN_CHAR_SPRITE_INDEX + 3];
      newY = SPRITES[MAIN_CHAR_SPRITE_INDEX];
      applyX();
      applyY();
      updateSprites();
    }

    Music_Update();

    NMI_flag = 0;
  }
}

//Would be better to do in asm (like in UnCollision) but haven't figured out a good way yet
void loadCollisionFromNametables(void)
{
  PPU_ADDRESS = 0x20; // address of nametable #0
  PPU_ADDRESS = 0x00;

  //First read is always invalid
  tempInt = *((unsigned char*)0x2007);

  for(tempInt = 0 ; tempInt < 960 ; tempInt++) {
    collision[tempInt] = (*((unsigned char*)0x2007) == 0x00) ? 0x00 : 0x01;
  }
}

void applyX(void) {
  temp1 = ((joypad1 & B_BUTTON) != 0) ? MAX_VELOCITY_WITH_B_X : MAX_VELOCITY_X;
  if((joypad1 & LEFT) != 0) {
    SPRITES[MAIN_CHAR_SPRITE_INDEX + 2] = 0x00; //attribute
      //SPRITES[MAIN_CHAR_SPRITE_INDEX + 1] = MAIN_CHAR_FIRST_SPRITE; //sprite
    newX = SPRITES[MAIN_CHAR_SPRITE_INDEX + 3] - temp1;
  }
  else if((joypad1 & RIGHT) != 0) {
    SPRITES[MAIN_CHAR_SPRITE_INDEX + 2] = 0x40; //attribute
      //SPRITES[MAIN_CHAR_SPRITE_INDEX + 1] = MAIN_CHAR_FIRST_SPRITE; //sprite
    newX = SPRITES[MAIN_CHAR_SPRITE_INDEX + 3] + temp1;
  }

  //Test X collision
  putCharInBackgroundVars();
  if(isBackgroundCollision() == 0) {
    SPRITES[MAIN_CHAR_SPRITE_INDEX + 3] = newX;
  }
  else {
    newX = SPRITES[MAIN_CHAR_SPRITE_INDEX + 3];
  }
}

void applyY(void) {

  // TODO just checking against 0 allows a double jump at the peak
  if(yVelocity >=0 && yVelocity < VELOCITY_FACTOR && isFalling == 0 && (joypad1 & A_BUTTON) != 0 && (joypad1old & A_BUTTON) == 0) {
    yVelocity = JUMP_VELOCITY;
    jumpCount = MAX_JUMP_COUNT;
  }
  else if((joypad1 & A_BUTTON) != 0 && jumpCount != 0) {
    --jumpCount;
    //yVelocity should still be JUMP_VELOCITY
  }
  else {
    yVelocity = yVelocity + GRAVITY;
  }

  tempSigned = yVelocity/VELOCITY_FACTOR;
  newY = SPRITES[MAIN_CHAR_SPRITE_INDEX] + tempSigned;

  //Test Y collision
  putCharInBackgroundVars();
  if(isBackgroundCollision() == 0) {
    //Because of subpixels, want to make sure we're actually moving
    if(SPRITES[MAIN_CHAR_SPRITE_INDEX] != newY) {
      SPRITES[MAIN_CHAR_SPRITE_INDEX] = newY;
      isFalling = 1;
    }
  }
  else {
    isFalling = 0;
    //Round up to the block above this
    newY = newY - (newY % 8);
    if(yVelocity > 0) {
      //Falling so round up a block
      newY = newY - 8;
    }
    else {
      //Moving up so round down a block
      newY = newY + 8;
    }

    yVelocity = 0;
  }
}

void putCharInBackgroundVars(void) {
  collisionX = newX;
  collisionY = newY;
  collisionWidth = CHARACTER_WIDTH;
  collisionHeight = CHARACTER_HEIGHT;
}

char isBackgroundCollision(void) {
  temp1 = newX >> 3;
  temp2 = newY >> 3;
  tempInt = 32*temp2 + temp1;

  if(collision[tempInt] == 0) {
    if((newX % 8 != 0) && collision[tempInt + 1] != 0) {
      return 1;
    }

    if((newY % 8 != 0) && collision[tempInt + 32] != 0) {
      return 1;
    }

    if((newX % 8 != 0) && (newY % 8 != 0) && collision[tempInt + 33] != 0) {
      return 1;
    }
  }
  else {
    return 1;
  }

  return 0;
}

void initSprites(void) {
  SPRITES[MAIN_CHAR_SPRITE_INDEX]     = 0x08; //Y
  SPRITES[MAIN_CHAR_SPRITE_INDEX + 1] = MAIN_CHAR_FIRST_SPRITE; //sprite
  SPRITES[MAIN_CHAR_SPRITE_INDEX + 2] = 0x00; //attribute
  SPRITES[MAIN_CHAR_SPRITE_INDEX + 3] = 0x30; //X
}

void updateSprites(void) {
  //TODO not sure this is the best place...
  if(powerUp == 1) {
    temp1 = SPRITES[MAIN_CHAR_SPRITE_INDEX];
    SPRITES[POWERUP_SPRITE_INDEX] = temp1;

    //if(SPRITES[MAIN_CHAR_SPRITE_INDEX + 2] == 0x00) {
      //Facing left
      SPRITES[POWERUP_SPRITE_INDEX + 1] = 1;
      SPRITES[POWERUP_SPRITE_INDEX + 2] = SPRITES[MAIN_CHAR_SPRITE_INDEX + 2];
      SPRITES[POWERUP_SPRITE_INDEX + 3] = SPRITES[MAIN_CHAR_SPRITE_INDEX + 3] + 7;
    //}
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
	PPU_CTRL = 0x80; // This will turn NMI on, make sure this matches the one in the NMI loop
	PPU_MASK = 0x1e; // enable all rendering
}

void resetScroll (void) {
	PPU_ADDRESS = 0;
	PPU_ADDRESS = 0;
	SCROLL = 0;
	SCROLL = 0xff;
}
