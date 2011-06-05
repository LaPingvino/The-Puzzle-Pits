/* Misc defines */

#include <dos.h>

#define _ibmpits_ 1
#define CRKEY 28
#define UPKEY 72
#define DNKEY 80
#define RTKEY 77
#define LFKEY 75
#define SPKEY 58
#define F1KEY 59
#define F4KEY 62

typedef unsigned char far CodePtr;

#define NULLADDR (CodePtr *) MK_FP(0,0)
#define LEVELPATH "levels\\"

#define strnicmp _fstrnicmp
#define strcpy  _fstrcpy
#define strcat  _fstrcat

#define SND_CRACK     0
#define SND_GROAN     1
#define SND_MAGIC     2
#define SND_PLATEOFF  3
#define SND_PLATEON   4
#define SND_SCRAPE    5
#define SND_WATER     6
#define SND_LAVA      7
#define SND_SPLINTER  8
#define SND_GLASS     9

#define Max(x,y) ((x)>(y)?(x):(y))
#define Min(x,y) ((x)<(y)?(x):(y))

#define RET_OK 0
#define RET_WARN 5
#define RET_ERROR 10
#define RET_FAIL 20

#define FRAMEDELAY  9
#define MAXSERIES 8   //change to 8 for msdos
#define NORTH 0
#define EAST  4
#define SOUTH 8
#define WEST  12
#define NORTHPUSH 0
#define EASTPUSH  1
#define SOUTHPUSH 2
#define WESTPUSH  3

#define MAXCODEMEMORY 32768
#define MAXPITCODE 100

struct PitCode{
  char name[16];
  unsigned char far *codeptr;
  WORD codesize;
  WORD next;
  WORD prev;
};

struct GameVars
{
  WORD guyx,guyy;
  WORD facing;
  WORD walkframe;
  ULONG pushing;
  ULONG lasttimewalked;
  WORD autowalk;
  WORD redcount,greencount,bluecount;
  WORD totalred,totalgreen,totalblue;
  WORD redgemon,greengemon,bluegemon,yellowgemon;
  WORD redgems,greengems,bluegems,yellowgems,redkeys,greenkeys,bluekeys;
  WORD moves;
  WORD xoff,yoff;
  WORD curtile;
  WORD lastx,lasty;
  WORD tilex,tiley;    //menu position of current tile
  char level[MAXSERIES+1];
  char title[100+1];
  char password[21];
  WORD saved;
};

extern WORD  mx,my,button1,button2;
extern unsigned char inkey;

/*
extern unsigned char far code[30000];
extern struct PitCode pitcodedef[MAXPITCODE];
extern UWORD totalcode;

extern struct GameVars gamevars;
extern WORD  editxc, edityc;
extern WORD  under[23][23],over[23][23],pitcode[23][23];
extern WORD  curpointer;
extern WORD  tilemenu[10];
extern WORD  keyqual;
extern char  NumberString[];
extern char  KeyMapLower[];
extern char  KeyMapUpper[];
*/

/* Pitcode */
#define CMD_ENDIF          1
#define CMD_NEXT           2
#define CMD_ELSE           3
#define CMD_SWITCHOFF      4
#define CMD_SWITCHON       5
#define CMD_IFEQ           6
#define CMD_IFLT           7
#define CMD_IFGT           8
#define CMD_IFLE           9
#define CMD_IFGE          10
#define CMD_IFNE          11
#define CMD_IFINPUT       12
#define CMD_FORLOOP       13
#define CMD_REDRAW        14
#define CMD_APPEAR        15
#define CMD_STOP          16
#define CMD_WIN           17
#define CMD_TARGET        18
#define CMD_REDGEMOFF     19
#define CMD_GREENGEMOFF   20
#define CMD_BLUEGEMOFF    21
#define CMD_POPUP         22
#define CMD_LET           23
#define CMD_ADD           24
#define CMD_SUB           25
#define CMD_MUL           26
#define CMD_DIV           27
#define CMD_SWAPOVER      28
#define CMD_SWAPUNDER     29
#define CMD_GOTO          30
#define CMD_CHANGEMAP     31
#define CMD_PRINT         32
#define CMD_INPUT         33

#define R0               201
#define R9               210
#define REG_XGUY         211
#define REG_YGUY         212
#define REG_LASTX        213
#define REG_LASTY        214
#define REG_FACING       215
#define REG_REDGEMS      216
#define REG_GREENGEMS    217
#define REG_BLUEGEMS     218
#define REG_YELLOWGEMS   219
#define REG_REDKEYS      220
#define REG_GREENKEYS    221
#define REG_BLUEKEYS     222
#define REG_INPUTVALUE   223
#define REG_XPOS         224
#define REG_YPOS         225
#define REG_MOVES        226
#define REG_HECTOMOVES   227
#define REG_WALKABLE     228
#define REG_DELTA        229
#define REG_UNDER        230
#define REG_OVER         231
#define REG_LASTPUSHED   232
#define REG_REDGEMON     233
#define REG_GREENGEMON   234
#define REG_BLUEGEMON    235
#define REG_LAST         235
#define NUMREGISTERS      35

/* sprites */
#define STDPOINTER                 0
#define CROSSHAIRS                 1
#define BRACKETCROSSHAIRS          2
#define UPARROWPTR                 3
#define RIGHTARROWPTR              4
#define DOWNARROWPTR               5
#define LEFTARROWPTR               6
#define BUSYPOINTER                7
#define BLANKPOINTER               8

/* logical tiles */
#define EMPTYSPACE                 0
#define WALL                       1
#define TEXTWALL                   5
#define PILLAR                     9
#define OFFSWITCH                 13
#define ONSWITCH                  17
#define WATERBARREL               21
#define HOLEBARREL                25
#define BARREL                    29
#define BLUEBARREL                33
#define CRATE                     37
#define REDCRATE                  41
#define CHEST                     45
#define GREENCHEST                49

#define FIRST_UNDERTILE           53
#define FLOOR                     53
#define REDTARGET                 54
#define GREENTARGET               55
#define BLUETARGET                56
#define MARKEDTILE                57
#define OFFPLATE                  58
#define ONPLATE                   59
#define UPARROW                   60
#define RIGHTARROW                61
#define DOWNARROW                 62
#define LEFTARROW                 63
#define REDGEM                    64
#define GREENGEM                  65
#define BLUEGEM                   66
#define YELLOWGEM                 67
#define REDKEY                    68
#define GREENKEY                  69
#define BLUEKEY                   70
#define GLASS                     71
#define REDLOCK                   76
#define GREENLOCK                 77
#define BLUELOCK                  78
#define WATERCRATE                79
#define HOLECRATE                 80
#define WOOD                      81
#define GRASS                     82
#define HOLE                      83
#define WATER                     84
#define LAVA                      88
#define LAVADEBRIS1               92
#define LAVADEBRIS2               96
#define LAST_UNDERTILE            96

#define GUY                      100
#define CLEAR_ALL                -3
#define CLEAR_OVER               -2
#define CLEAR_UNDER              -1

/* game tiles */
#define GUYMAGIC                 148
#define GUYPUSH                  196
#define GUYMORPH                 228
#define REDGEMON                 252
#define GREENGEMON               253
#define BLUEGEMON                254
#define YELLOWGEMON              255
#define YIELD                    256
#define YIELDPRESSED             257
#define STOP                     258
#define STOPPRESSED              259
#define EYEFRAME1                260
#define EYEFRAME2                261
#define EYEFRAME3                262
#define EYEFRAME4                263
#define OPENEYE                  264
#define F1SHAPE                  265
#define F2SHAPE                  266
#define F3SHAPE                  267
#define F4SHAPE                  268
#define F5SHAPE                  269
#define F6SHAPE                  270
#define F7SHAPE                  271
#define F8SHAPE                  272
#define F9SHAPE                  273
#define F10SHAPE                 274
#define TITLETEXTBG              275
