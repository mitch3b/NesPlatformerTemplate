// let's define some stuff
#define PPU_CTRL		*((unsigned char*)0x2000)
#define PPU_MASK		*((unsigned char*)0x2001)
#define PPU_STATUS		*((unsigned char*)0x2002)
#define OAM_ADDRESS		*((unsigned char*)0x2003)
#define SCROLL			*((unsigned char*)0x2005)
#define PPU_ADDRESS		*((unsigned char*)0x2006)
#define PPU_DATA		*((unsigned char*)0x2007)
#define OAM_DMA			*((unsigned char*)0x4014)

#define RIGHT		0x01
#define LEFT		0x02
#define DOWN		0x04
#define UP			0x08
#define START		0x10
#define SELECT		0x20
#define B_BUTTON	0x40
#define A_BUTTON	0x80

// Globals
// our startup code initialized all values to zero
#pragma bss-name(push, "ZEROPAGE")
unsigned char NMI_flag;
unsigned char Frame_Count;
unsigned char joypad1;
unsigned char joypad1old;
unsigned char joypad1test;
unsigned char joypad2;
unsigned char joypad2old;
unsigned char joypad2test;
unsigned char Horiz_scroll;
unsigned char Frame_Count;

unsigned char gameState;
unsigned char levelNum;

// temp vars
unsigned char index;

// got rid of Vert_scroll, now always 0
unsigned char Nametable;

// Music
unsigned char song;

//Main character
#define MAIN_CHAR_FIRST_SPRITE 0x00;

// OAM equals ram addresses 200-2ff
#pragma bss-name(push, "OAM")
unsigned char SPRITES[256]; //64 sprites * 4 bytes
#define MAIN_CHAR_SPRITE_INDEX	0x00

// Prototypes
void allOff(void);
void allOn(void);
void resetScroll (void);
void initSprites(void);
void updateSprites(void);

void initLevel(void);

void _Hide_Sprites(void);
void Reset_Music(void);
void __fastcall__ Play_Music(unsigned char song);
void Music_Update(void);
void __fastcall__ Play_Fx(unsigned char song);
void __fastcall__ memcpy (void* dest, const void* src, int count);
void Wait_Vblank(void);
void __fastcall__ UnRLE(const unsigned char *data);
void Get_Input(void);
