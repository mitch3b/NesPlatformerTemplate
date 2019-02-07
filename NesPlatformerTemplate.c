#include "DEFINE.c"
#include "LevelData.c"

void main (void) {
  allOff(); // turn off screen
	song = 0;
  gameState = 1;
  levelNum = 0;

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
      initSprites();
			Wait_Vblank();
			allOn();
			resetScroll();
      gameState = 2;
    }
    else if (gameState == 2) {
      updateSprites();
    }

    Music_Update();

    NMI_flag = 0;
  }
}

void initSprites(void) {
  SPRITES[MAIN_CHAR_SPRITE_INDEX]     = 0xA0; //Y
  SPRITES[MAIN_CHAR_SPRITE_INDEX + 1] = MAIN_CHAR_FIRST_SPRITE; //sprite
  SPRITES[MAIN_CHAR_SPRITE_INDEX + 2] = 0x00; //attribute
  SPRITES[MAIN_CHAR_SPRITE_INDEX + 3] = 0x30; //X
}

void updateSprites(void) {
  if((joypad1 & UP) != 0) {
      SPRITES[MAIN_CHAR_SPRITE_INDEX] -= 1;
  }
  else if((joypad1 & DOWN) != 0) {
      SPRITES[MAIN_CHAR_SPRITE_INDEX] += 1;
  }

  //SPRITES[MAIN_CHAR_SPRITE_INDEX + 1] = MAIN_CHAR_FIRST_SPRITE; //sprite

  if((joypad1 & LEFT) != 0) {
      SPRITES[MAIN_CHAR_SPRITE_INDEX + 3] -= 2; //X
      SPRITES[MAIN_CHAR_SPRITE_INDEX + 2] = 0x00; //attribute
  }
  else if((joypad1 & RIGHT) != 0) {
      SPRITES[MAIN_CHAR_SPRITE_INDEX + 3] += 2; //X
      SPRITES[MAIN_CHAR_SPRITE_INDEX + 2] = 0x40; //attribute
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
