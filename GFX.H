/* gfx.h
   10.21.95 by Abe Pralle
*/


#define MAXSHAPE 300
#define LOGIC 0
#define PHYSIC 1

void Txt(WORD x,WORD y,CHAR *str,WORD len);
WORD TxtLength(CHAR *str,WORD len);
void SetPalette(WORD n);
void SetColor(WORD n);

void Blit16(WORD bmap,WORD x,WORD y,WORD image);
void BlitMask16(WORD bmap,WORD x,WORD y,WORD image);
void Blit24(WORD bmap,WORD x,WORD y,WORD image);
void BlitMask24(WORD bmap,WORD x,WORD y,WORD image);
void Blit(WORD bmap,WORD x,WORD y,WORD image);
void BlitMask(WORD bmap,WORD x,WORD y,WORD image);
void LittleTile(WORD bmap,WORD x,WORD y,WORD image);
void OwnBlit(void);
void DisownBlit(void);

void GetLevelName(char *lname,WORD firsttime);
ULONG Timer(void);
void WasteTime(void);
void SetMousePos(WORD x,WORD y);
void CheckMouse(void);
void ChangeMouse(WORD n);
void Frame(WORD x,WORD y,WORD width,WORD height,WORD c1,WORD c2);
void FilledFrame(WORD x,WORD y,WORD width,WORD height,WORD c1,WORD c2,WORD c3);
void Boxf(WORD x,WORD y,WORD width,WORD height,WORD c1);
void Pause(LONG n);
void LoadScreen(LONG n);
void FadeIn(void);
void ScreenCopySidebar(void);
void LoadSprites(void);
void FreeSprites(void);

void Plot(WORD x,WORD y,BYTE c,char far *vscreen);

void ScreenSwap(void);
void ScreenCopy(void);
void clearscreen(char far *vscreen);
void Clear(WORD n);
void FreeShapes(void);
void LoadShapes(char *filename,WORD pos);

void PlaySound(int n);

void gfxInit(void);
void cleanExit(WORD retval);

