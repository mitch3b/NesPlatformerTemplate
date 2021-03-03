#include "Graphics/level1.h"
#include "Graphics/Colors.h"

const unsigned char* LEVELS[] = {level1};

const unsigned char PALETTE[]={
  BLACK, LIGHT_LIME_GREEN, DARK_GREEN, DARK_ORANGE, BLACK, RED, DARK_FUCHSIA, DARK_FUCHSIA, BLACK, DARK_RED, RED, LIGHT_RED, BLACK, DARK_GREEN, LIGHT_CYAN, DARK_PURPLE,
  BLACK, LIGHT_LIME_GREEN, DARK_GREEN, DARK_ORANGE, BLACK, RED, DARK_FUCHSIA, DARK_FUCHSIA, BLACK, DARK_RED, RED, LIGHT_RED, BLACK, DARK_GREEN, LIGHT_CYAN, DARK_PURPLE
};

void loadPalette (void) {
	PPU_ADDRESS = 0x3f;
	PPU_ADDRESS = 0x00;
	PPU_ADDRESS = 0x00;
	for( index = 0; index < sizeof(PALETTE); ++index ){
		PPU_DATA = PALETTE[index];
	}
}

void loadLevel(void) {
	PPU_ADDRESS = 0x20; // address of nametable #0 = 0x2000
	PPU_ADDRESS = 0x00;
	UnRLE(LEVELS[levelNum]);	// uncompresses our data

  //loadCollisionFromNametables();
}
/*
//Would be better to do in asm (like in UnCollision) but haven't figured out a good way yet
void loadCollisionFromNametables(void)
{
  PPU_ADDRESS = 0x20; // address of nametable #0
  PPU_ADDRESS = 0x00;

  //First read is always invalid
  tempInt = *((unsigned char*)0x2007);

  for(tempInt = 0 ; tempInt < 960 ; tempInt++) {
    collision[tempInt] = tempInt;//(*((unsigned char*)0x2007) == 0x00) ? 0x00 : 0x01;
  }
}
*/
