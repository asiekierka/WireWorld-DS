#include "nds.h"
#include <nds/arm9/sound.h>		// sound functions
#include <nds/timers.h>		// timer functions
#include "tileedit.h"
#include <stdio.h>
#include <string.h>
#include <fat.h>
#include "wireworldTS.h"
#include "wireworld_controls.h"
#include "DSding_raw.h"
#include "DSload_raw.h"
#include "DSkill_raw.h"
#include "DSnew_raw.h"
#define DLDI_BUILD
#define wm_x 128
#define wm_y 128
#define wm_size2 4096
#define wm_size 16384
char maparr[wm_x*wm_y];
char maparr_bk[wm_x*wm_y];
char maparr_sav[wm_x*wm_y];

#ifdef __BLOCKSDS__
#define iprintf printf
#define memcpy32(a,b,c) memcpy(a,b,c*4)
#define memset32(a,b,c) memset(a,b,c*4)
#else
#include "tonc_memfunc.h"
#endif

#define bg0map ((u16*)BG_TILE_RAM_SUB(0))
char charcol;
int dospd,issavst,ison,arrx,arry,edwx,edwy,refscr;
int FATresult,oldtx,oldty;
FILE *testfile;
touchPosition tp;
/* edwx and edwy are the 32x20 editor window's position */
#define TILE_FLIP_X       (1<<10)
#define TILE_FLIP_Y       (2<<10)
#define TILE_FLIP_XY      (3<<10)
#define StylusInBox(x,y,w,h,tp) (tp.x >= x && tp.x < x+w && tp.y >= y && tp.y < y+h)
#define POS2IDX(x, y)    ((x) + ((y)*32))
#define XY2FB(x,y)    ((x) + ((y)*(SCREEN_WIDTH)))
#define TIL_SPACE (RGB15(0,0,0))
#define TIL_TAIL (RGB15(10,10,31))
#define TIL_SPARK (RGB15(31,0,0))
#define TIL_WIRE (RGB15(31,25,9))
#define abs(x) ((x)>0?(x):-(x))
#define xy2m(x,y) ((x) + ((y)*128))


// playGenericSound(DSnew_raw, DSnew_raw_size);
/*	setGenericSound(	22050,
						127,
						64,
						1 ); */
static void playGenericSound(const void *data, u32 len)
{
	soundPlaySample(data, SoundFormat_16Bit, len, 22050, 127, 64, false, 0);
}

// eKid's profiling functions
inline void startProfile()
{
    // disable timers
    TIMER0_CR = 0;
    TIMER1_CR = 0;
 
    // reset counters
    TIMER0_DATA = 0;
    TIMER1_DATA = 0;
 
    // enable timers
    TIMER1_CR = TIMER_ENABLE | TIMER_CASCADE;
    TIMER0_CR = TIMER_ENABLE;
}
 
inline u32 endProfile()
{
    // read cycle count
    u32 cycles;
    cycles = (TIMER1_DATA<<16) + TIMER0_DATA;
 
    // disable timers
    TIMER0_CR = 0;
    TIMER0_CR = 0;
 
    // return cycle count
    return cycles;
}

inline void VBLwait(int vblm)
{
  for (int vblw=0; vblw<vblm; vblw++)
   swiWaitForVBlank();
}

inline void ShowDec(int wah)
{
 lcdSwap();
 BG_PALETTE_SUB[255] = RGB15(31,31,31);
 // consoleInitDefault((u16*)SCREEN_BASE_BLOCK_SUB(0), (u16*)CHAR_BASE_BLOCK_SUB(1), 16);
 consoleDemoInit();
 iprintf("%d cycles",wah);
 while(1)
 {
 swiWaitForVBlank();
 }
}

inline void ShowErr(char errshow[])
{
 lcdSwap();
 BG_PALETTE_SUB[255] = RGB15(31,31,31);
 consoleDemoInit();
 iprintf("ERROR! ");
 iprintf(errshow);
 while(1)
 {
 swiWaitForVBlank();
 }
}

inline void WriteEditStSo(void)
{
if (dospd == 15)
{
 bg0map[POS2IDX(20,21)] = 23;
 bg0map[POS2IDX(21,21)] = 24;
 bg0map[POS2IDX(20,22)] = 23 | TILE_FLIP_Y;
 bg0map[POS2IDX(21,22)] = 24 | TILE_FLIP_Y;
}
else if (dospd == 5)
{
 bg0map[POS2IDX(20,21)] = 25;
 bg0map[POS2IDX(21,21)] = 26;
 bg0map[POS2IDX(20,22)] = 25 | TILE_FLIP_Y;
 bg0map[POS2IDX(21,22)] = 26 | TILE_FLIP_Y;
}

if (ison == 0)
{
 bg0map[POS2IDX(29,21)] = 10;
 bg0map[POS2IDX(30,21)] = 11;
 bg0map[POS2IDX(29,22)] = 12;
 bg0map[POS2IDX(30,22)] = 13;
}
else
{
 bg0map[POS2IDX(29,21)] = 5 | TILE_FLIP_XY;
 bg0map[POS2IDX(30,21)] = 5 | TILE_FLIP_Y;
 bg0map[POS2IDX(29,22)] = 5 | TILE_FLIP_X;
 bg0map[POS2IDX(30,22)] = 5;
}
}

inline void ClearMap(int plsnd)
{
 refscr = 1;
 memset32((void*)maparr, ' ', wm_size2);
 if (plsnd == 1)
 {
  playGenericSound(DSnew_raw, DSnew_raw_size);
  VBLwait(7);
 }
}

inline bool ReadWWMap(int plsnd)
{
    if (FATresult == 0) {
	ClearMap(0);
	return false;
	 // ShowErr("DLDI couldn't load.");
    } else if( (testfile = fopen("/wireworld.txt", "r")) == NULL) {
	ClearMap(0);
	return false;
	 // ShowErr("Can't open wireworld.txt.");
    } else {
      ClearMap(0);
	  u32 readp;
	  readp = 0;
	  while (feof(testfile) == false)
	   {
	    maparr[readp] = fgetc(testfile);
	    readp++;
	   }
 if (fclose(testfile) == EOF)
   ShowErr("Can't close wireworld.txt.");
 if (plsnd == 1)
 {
  playGenericSound(DSload_raw, DSload_raw_size);
  VBLwait(6);
 }
 return true;
 }
}

inline void SaveWWMap(void)
{
    if( (testfile = fopen("/wireworld.txt", "w")) == NULL)
	 ShowErr("Can't open wireworld.txt.");
    for (u32 wrp = 0; wrp < wm_size; wrp++)
	{
	  fputc(maparr[wrp],testfile);
	}
  if (fclose(testfile) == EOF)
   ShowErr("Can't close wireworld.txt.");
}

inline void drawBresLine(u32 x1, u32 y1, u32 x2, u32 y2, char drawchar)
{
 
	// Guarantees that all lines go from left to right
	if ( x2 < x1 )
	{
		x1 ^= x2; x2 ^= x1; x1 ^= x2;
		y1 ^= y2; y2 ^= y1; y1 ^= y2;
	}
 
	s32 dy,dx;
	dy = y2 - y1;
	dx = x2 - x1;
 
	// If the gradient is greater than one we have to flip the axes
	if ( abs(dy) < dx )
	{
		u32 xp,yp;
		s32 d;
		s32 add = 1;
 
		xp = x1;
		yp = y1;
 
		if(dy < 0)
		{
			dy = -dy;
			add =- 1;
		}
 
		d = 2*dy - dx;
 
		for(; xp<=x2; xp++)
		{
			if(d > 0)
			{
				yp += add;
				d -= 2 * dx;
			}
 
			maparr[xy2m(xp,yp)] = drawchar;
 
			d += 2 * dy;
		}
	}
	else
	{
		x1 ^= y1; y1 ^= x1; x1 ^= y1;
		x2 ^= y2; y2 ^= x2; x2 ^= y2;
 
		if ( x2 < x1 )
		{
		x1 ^= x2; x2 ^= x1; x1 ^= x2;
		y1 ^= y2; y2 ^= y1; y1 ^= y2;
		}
 
		u32 xp,yp;
		s32 d;
 
		dy = y2 - y1;
		dx = x2 - x1;
 
		s32 add = 1;
 
		if(dy < 0)
		{
			dy = -dy;
			add=-1;
		}
 
		xp = x1;
		yp = y1;
 
		d = 2 * dy - dx;
 
		for(xp=x1; xp<=x2; xp++) {
 
			if(d > 0)
			{
				yp += add;
				d -= 2 * dx;
			}
 
			maparr[xy2m(yp,xp)] = drawchar;
 
			d += 2 * dy;
		}
	}
}

inline void KillElectrons(void)
{
 refscr = 1;
 for (int dp=0; dp<wm_size; dp++)
  {
    if (maparr[dp] == '#')
      maparr[dp] = '=';
    else if (maparr[dp] == '-')
      maparr[dp] = '=';
  }
  playGenericSound(DSkill_raw, DSkill_raw_size);
  VBLwait(7);
}


inline void GenWindow(void)
{
int mapSpot = (3 | (3 << 8) | (3 << 16) | (3 << 24));
memset((void*)bg0map, mapSpot, 1280);
mapSpot = 0;
char arrSpot = ' ';
for (int dy =0; dy < 20; dy++)
 for (int dx=0; dx < 32; dx++)
  {
   mapSpot = POS2IDX((dx),(dy));
   arrSpot = maparr[((dx+edwx) + ((dy+edwy)*(wm_x)))];
   if (arrSpot == ' ') bg0map[mapSpot] = 3;
   else if (arrSpot == '#') bg0map[mapSpot] = 1;
   else if (arrSpot == '=') bg0map[mapSpot] = 2;
   else if (arrSpot == '-') bg0map[mapSpot] = 0;

  }
/* Fix of rewriting the 640th tile */
 bg0map[640] = 4;
}

inline void WriteTop(void)
{
u32 arry, arrx;
  for (arry=0; arry <wm_y; arry++)
    for (arrx=0; arrx <wm_x; arrx+=4)
	{
	 u32 i = arrx+arry*128;
     ((u32*)BG_GFX)[(arrx>>2)+arry*64]= maparr[i] | 
                                    (maparr[i+1]<<8) |
                                    (maparr[i+2]<<16) |
                                    (maparr[i+3]<<24);
	}
}

inline void WriteMap(void)
{
 WriteTop();
 GenWindow();
}

inline void toggleOn(void)
{
      if (ison == 1)
	   {
	    if (issavst == 1)
		{
         memcpy32((void*)maparr, (void*)maparr_sav, wm_size2);
        }
	    ison = 0;
	   }
	   else
	   {
	    ison = 1;
	    if (issavst == 1)
		{
         memcpy32((void*)maparr_sav, (void*)maparr, wm_size2);
        }
	   }
  VBLwait(2);
}

inline void key_setSpeed(void)
{
	   if (dospd == 15)
		 dospd = 5;
       else dospd = 15;
	  WriteEditStSo();
  VBLwait(6);
}
inline void KeyCheck(void)
{
 uint16 tx,ty,tx2,ty2;
/* checking for zoom keys */
 scanKeys();
 uint16 keys_pressed = ~(REG_KEYINPUT);
 if(keys_pressed & KEY_UP)
  if (edwy > 0)
   edwy--;
 if(keys_pressed & KEY_DOWN)
  if (edwy < (wm_y-20))
   edwy++;
 if(keys_pressed & KEY_LEFT)
  if (edwx > 0)
   edwx--;
 if(keys_pressed & KEY_RIGHT)
  if (edwx < (wm_x-32))
   edwx++;
    if(keysDown() & KEY_A)
       toggleOn();
	if(keysDown() & KEY_X)
    {
	   if (issavst == 1)
		 issavst = 0;
       else issavst = 1;
	  swiWaitForVBlank();
	  swiWaitForVBlank();
	}
	if(keysDown() & KEY_Y)
     key_setSpeed();
    if(keysDown() & KEY_L)
	   ReadWWMap(1);
    if(keysDown() & KEY_R)
	   SaveWWMap();
/* checking for editor touch */
oldtx = tp.px;
oldty = tp.py;
touchRead(&tp);
if ((oldtx == 0) && (oldty == 0))
 {
  oldtx = tp.px;
  oldty = tp.py;
 }
     if ((tp.px >= 15 && tp.px < 31 && tp.py >= 167 && tp.py < 183) == true)
      charcol = ' ';
else if ((tp.px >= 39 && tp.px < 62-7 && tp.py >= 167 && tp.py < 183) == true)
      charcol = '=';
else if ((tp.px >= 63 && tp.px < 86-7 && tp.py >= 167 && tp.py < 183) == true)
      charcol = '#';
else if ((tp.px >= 87 && tp.px < 103 && tp.py >= 167 && tp.py < 183) == true)
      charcol = '-';
else if ((tp.px >= 159 && tp.px < 175 && tp.py >= 167 && tp.py < 183) == true)
      key_setSpeed();
else if ((tp.px >= 183 && tp.px < 199 && tp.py >= 167 && tp.py < 183) == true)
      ClearMap(1);
else if ((tp.px >= 207 && tp.px < 223 && tp.py >= 167 && tp.py < 183) == true)
      KillElectrons();
else if ((tp.px >= 231 && tp.px < 247 && tp.py >= 167 && tp.py < 183) == true
         && 
		 keysDown() & KEY_TOUCH)
toggleOn();
else if ((tp.px >= 2 && tp.px < 254 && tp.py >= 2 && tp.py < 160) == true)
   {
	tx = (tp.px >> 3) + edwx;
	ty = (tp.py >> 3) + edwy;
	tx2 = (oldtx >> 3) + edwx;
	ty2 = (oldty >> 3) + edwy;
	if ((tx == tx2) && (ty == ty2))
	 maparr[xy2m(tx,ty)] = charcol;
	else
	 {
	  drawBresLine(tx2,ty2,tx,ty,charcol);
	 }
	refscr = 1;
   }
}

inline void GenMake(void)
{
refscr = 1;
int temp = 0;
memcpy32((void*)maparr_bk, (void*)maparr, wm_size2);
for (int dp=0; dp<wm_size; dp++)
     if (maparr_bk[dp] == '#')
     maparr[dp] = '-';

  	else if (maparr_bk[dp] == '-') 
     maparr[dp] = '=';
	 
	else if (maparr_bk[dp] == '=')
	  {
	   temp = 0;
	   /* oh god i hate this part */
	   for (int ddy=-1; ddy<=1; ddy++)
	    for (int ddx=-1; ddx<=1; ddx++)
             if (maparr_bk[(dp+xy2m(ddx,ddy))] == '#') 
              temp++;

	   /* Ok, now count it */
	    if ((temp < 3) && (temp > 0))
		   maparr[dp] = '#';
	  }
}


inline int sethl(char zz)
{
 if (charcol == zz)
  return 13;
 else
  return 0;
}

inline void WriteEditorMenu(void)
{
u8 lol;
/* space */
lol = sethl(' ');
bg0map[POS2IDX(2,21)] = (7+lol) | TILE_FLIP_XY;
bg0map[POS2IDX(3,21)] = (7+lol) | TILE_FLIP_Y;
bg0map[POS2IDX(2,22)] = (7+lol) | TILE_FLIP_X;
bg0map[POS2IDX(3,22)] = 7+lol;
/* wire */
lol = sethl('=');
bg0map[POS2IDX(5,21)] = (9+lol) | TILE_FLIP_XY;
bg0map[POS2IDX(6,21)] = (9+lol) | TILE_FLIP_Y;
bg0map[POS2IDX(5,22)] = (9+lol) | TILE_FLIP_X;
bg0map[POS2IDX(6,22)] = 9+lol;
/* spark */
lol = sethl('#');
bg0map[POS2IDX(8,21)] = (8+lol) | TILE_FLIP_XY;
bg0map[POS2IDX(9,21)] = (8+lol) | TILE_FLIP_Y;
bg0map[POS2IDX(8,22)] = (8+lol) | TILE_FLIP_X;
bg0map[POS2IDX(9,22)] = 8+lol;
/* tail */
lol = sethl('-');
bg0map[POS2IDX(11,21)] = (6+lol) | TILE_FLIP_XY;
bg0map[POS2IDX(12,21)] = (6+lol) | TILE_FLIP_Y;
bg0map[POS2IDX(11,22)] = (6+lol) | TILE_FLIP_X;
bg0map[POS2IDX(12,22)] = 6+lol;
/* new */
 bg0map[POS2IDX(23,21)] = 14;
 bg0map[POS2IDX(24,21)] = 15;
 bg0map[POS2IDX(23,22)] = 14 | TILE_FLIP_Y;
 bg0map[POS2IDX(24,22)] = 14 | TILE_FLIP_XY;
/* electron clear */
 bg0map[POS2IDX(26,21)] = 16;
 bg0map[POS2IDX(27,21)] = 16 | TILE_FLIP_X;
 bg0map[POS2IDX(26,22)] = 17;
 bg0map[POS2IDX(27,22)] = 18;
/* start or stop, motion */
WriteEditStSo();
}

inline void loadMainData(void)
{
 dmaCopy(tileeditTiles, (void*)BG_TILE_RAM_SUB(1), tileeditTilesLen);
 dmaCopy(tileeditPal, BG_PALETTE_SUB, 32);
}

inline void loadTitleData(void)
{
 memcpy32((void*)BG_MAP_RAM_SUB(0), wireworldTSMap, (wireworldTSMapLen / 4));
 memcpy32((void*)BG_TILE_RAM_SUB(1), wireworldTSTiles, (wireworldTSTilesLen / 4));
 memcpy32(BG_PALETTE_SUB, wireworldTSPal, 8);
 memcpy32((void*)BG_MAP_RAM(2), wireworld_controlsMap, (wireworld_controlsMapLen/4));
 memcpy32((void*)BG_TILE_RAM(3), wireworld_controlsTiles, (wireworld_controlsTilesLen/4));
 memcpy32(BG_PALETTE, wireworld_controlsPal, 8);
}

inline void setScrPos(void)
{
 s32 ScrTmp = (((edwy + 20) - 64) / 2);
 if (ScrTmp < 0)
  REG_BG3Y = 0;
 else
  REG_BG3Y = (ScrTmp << 8);
}

int main(void)
{
	// irqs are nice
//	irqInit();
//	irqSet(IRQ_VBLANK, 0);
//	irqEnable(IRQ_VBLANK);
	videoSetMode(MODE_0_2D | DISPLAY_BG0_ACTIVE);
	videoSetModeSub(MODE_0_2D | DISPLAY_BG0_ACTIVE);
	vramSetBankA(VRAM_A_MAIN_BG);
	vramSetBankC(VRAM_C_SUB_BG); 
  	REG_BG0CNT_SUB = BG_32x32 | BG_COLOR_16 | BG_MAP_BASE(0) | BG_TILE_BASE(1);
 	REG_BG0CNT = BG_32x32 | BG_COLOR_16 | BG_MAP_BASE(2) | BG_TILE_BASE(3);
    lcdSwap();
    loadTitleData();
    soundEnable();
	bool wasSTART = false;
	dospd = 5;
	do
     {
	    scanKeys();
            touchRead(&tp);
	    if (keysDown() & KEY_START) wasSTART = true;
		else if ((keysDown() & KEY_TOUCH) && (tp.px > 3) && (tp.px < 254) && (tp.py < 190) && (tp.py > 3))
		 wasSTART = true;
        swiWaitForVBlank();
	 }
	while (wasSTART == false);
	playGenericSound(DSding_raw, DSding_raw_size);
	
    memset32((void*)VRAM_A, 0, 32768);
	REG_BG0CNT = 0;		
	videoSetMode(MODE_5_2D | DISPLAY_BG3_ACTIVE);	
    lcdSwap();
	loadMainData();
	
   #ifdef DLDI_BUILD
	FATresult = fatInitDefault();
	ReadWWMap(0);
   #endif
	REG_BG3CNT = BG_BMP8_256x256;
	BG_PALETTE[' '] = 0;
    BG_PALETTE['#'] = TIL_SPARK;
	BG_PALETTE['='] = TIL_WIRE;
	BG_PALETTE['-'] = TIL_TAIL;
	REG_BG3PA = 1 << 7;
	REG_BG3PD = 1 << 7;
	REG_BG3PB = 0;
	REG_BG3PC = 0;
	REG_BG3X = 0;
	REG_BG3Y = 0;
	for (int t=0; t<640; t++)
	 bg0map[t] = 3;
	for (int t=640; t<768; t++)
	 bg0map[t] = 4;
	refscr = 1;
while(1)
 {
 for (int t=0; t<dospd; t++)
  {
   swiWaitForVBlank();
   KeyCheck();
   
   if (refscr == 1)
   {
	 WriteEditorMenu();
     WriteMap();
	 setScrPos();
   }
  }
 if (ison == 1) GenMake();
 }
soundDisable();
return 0;
}
