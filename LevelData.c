#include "Graphics/levelEasy.h"
#include "Graphics/levelMedium.h"
#include "Graphics/level1.h"
#include "Graphics/Colors.h"

const unsigned char* LEVELS[] = {levelEasy, levelMedium, level1};

const unsigned char PALETTE[]={
  // Top is background
  BLACK,DARK_GRAY,PINK,GRAY,BLACK,LIGHT_PINK,PINK,LIGHT_PINK,BLACK,DARK_RED,RED,LIGHT_RED,BLACK,DARK_GREEN,LIGHT_CYAN,DARK_PURPLE,
  BLACK,DARK_PURPLE,PINK,LIGHT_PINK,BLACK,LIGHT_PINK,PINK,LIGHT_PINK,BLACK,DARK_RED,RED,LIGHT_RED,BLACK,DARK_GREEN,LIGHT_CYAN,DARK_PURPLE
};

const unsigned char PALETTE_HIDDEN[]={
  BLACK,BLACK,PINK,BLACK,BLACK,LIGHT_PINK,PINK,LIGHT_PINK,BLACK,DARK_GREEN,LIGHT_CYAN,DARK_PURPLE,BLACK,DARK_RED,RED,LIGHT_RED,
  BLACK,DARK_PURPLE,PINK,LIGHT_PINK,BLACK,LIGHT_PINK,PINK,LIGHT_PINK,BLACK,DARK_RED,RED,LIGHT_RED,BLACK,DARK_GREEN,LIGHT_CYAN,DARK_PURPLE
};

void loadPalette (void) {
	PPU_ADDRESS = 0x3f;
	PPU_ADDRESS = 0x00;
	PPU_ADDRESS = 0x00;
	for( index = 0; index < sizeof(PALETTE); ++index ){
		PPU_DATA = PALETTE[index];
	}
}

void loadHiddenPalette (void) {
	PPU_ADDRESS = 0x3f;
	PPU_ADDRESS = 0x00;
	PPU_ADDRESS = 0x00;
	for( index = 0; index < sizeof(PALETTE); ++index ){
		PPU_DATA = PALETTE_HIDDEN[index];
	}
}

void loadLevel(void) {
	PPU_ADDRESS = 0x20; // address of nametable #0 = 0x2000
	PPU_ADDRESS = 0x00;
	UnRLE(LEVELS[levelNum]);	// uncompresses our data
}
/*

*/
