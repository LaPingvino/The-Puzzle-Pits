/* gfx.c
** 10.21.95 By Abe Pralle
** graphics routines for PuzzlePits
*/

#include "oldsound.h"
#include <dos.h>
#include "file.h"
#include "gfx.h"
#include <conio.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <time.h>
#include "tiledefs.h"
#include <dir.h>

int  oldmode;
char far *logical;
char far *physical=MK_FP(0xa000,0);
char far *shapedata1,far *shapedata2,far *shapedata3;  //memory to hold shapes
char far *fontdata;
char far *shapes[MAXSHAPE];  //pointers to start of shape in mem
char far *fontletter[256];
char drcolor=0;
WORD txtcursx=0,txtcursy=0;
WORD mouseavailable,mx,my,moffx,moffy,mouseimage,button1,button2;
unsigned char inkey;
unsigned char far mousebg[256];
char keytable[256];
char KeyMapLower[]={"..1234567890-=..qwertyuiop[]..asdfghjkl;'`.\\zxcvbnm,./... "};
char KeyMapUpper[]={"..!@#$%^&*()_+..QWERTYUIOP{}..ASDFGHJKL:\"~.|ZXCVBNM<>?... "};

volatile unsigned int far *bios_ticks=(unsigned int far *) MK_FP(0x40,0x6c);

env_BLASTER blaster;
SOUND       sounds[10];
int         blasteravailable;

extern void far setmode(int mode);
extern void far cls(char far *screen);
extern void far screenswap(char far *screen);
extern void far setpalette(char far *colorlist);
extern WORD far initmouse(void);
extern WORD far readmbutton(void);
extern void far relpos(WORD far *x,WORD far *y);
extern void far setkb(void);
extern void far resetkb(void);


/*
void main(void)
{
  int x,y,frame;
  FILE *fp;
  gfxInit();
  LoadShapes("tiles.bmp",0);
  //copymousebg(physical);
  //drawmouse(physical);
  Clear(LOGIC);
  SetPalette(1);
  SetColor(1);
  //ScreenSwap();
  while(!keytable[CRKEY]) CheckMouse();
  cleanExit(0);
}
*/


void blitletter(unsigned char far *tile,char far *screen)
{
  int deltax,width,height;
  register int i;
  deltax=320 - *tile;
  width=*tile;
  tile+=1;
  height=*tile;
  tile+=1;
  screen+=txtcursy*320+txtcursx;
  txtcursx+=width;
bmyloop:
  for(i=0;i<width;i++){
    if(*tile) *screen=drcolor;
    screen+=1;
    tile+=1;
  }
  screen+=deltax;
  height--;
  if(height>0) goto bmyloop;
}


void text(WORD x,WORD y,CHAR *str,WORD len)
{
  txtcursx=x;
  txtcursy=y;
  while(len){
    blitletter(fontletter[*str],logical);
    str+=1;
    len-=1;
  }
}

void Txt(WORD x,WORD y,CHAR *str,WORD len)
{
  text(x,y-6,str,len);  // 6 pixel correction from Amiga's baseline
}

WORD TxtLength(CHAR *str,WORD len)
{
  WORD total=0;
  while(len>0){
    total += *fontletter[*str];
    str+=1;
    len--;
  }
  return total;
}

void SetPalette(WORD n)
{
  static char pal1[]={
    0,0,0, 63,63,63, 54,54,54, 50,50,50, 42,42,42, 37,37,37,
    25,25,25, 63,0,0, 54,0,0, 46,0,0, 37,0,0, 63,63,0,
    63,42,0, 0,63,63, 29,46,63, 33,0,33, 33,63,33, 0,63,0,
    0,46,0, 0,33,0, 0,33,63, 0,25,58, 0,16,50, 0,8,46,
    0,4,37, 0,0,33, 63,37,33, 46,21,21, 33,16,0, 29,12,0,
    25,8,0, 21,4,0, 0,0,0, 31,31,31, 27,27,27, 25,25,25,
    21,21,21, 18,18,18, 12,12,12, 31,0,0, 27,0,0, 23,0,0,
    18,0,0, 31,31,0, 31,21,0, 0,31,31, 14,23,31, 16,0,16,
    16,31,16, 0,31,0, 0,23,0, 0,16,0, 0,16,31, 0,12,29,
    0,8,25, 0,4,23, 0,2,18, 0,0,16, 31,18,16, 23,10,10,
    16,8,0, 14,6,0, 12,4,0, 10,2,0};

  static char pal2[]={
    0,0,0, 63,63,63, 54,54,54, 46,46,46, 33,33,33, 25,25,25,
    16,16,16, 63,33,0, 63,42,0, 63,50,0, 63,58,0, 63,63,0,
    63,63,12, 63,63,29, 63,63,25, 63,63,63, 0,33,63, 0,29,63,
    0,25,63, 0,25,58, 0,21,58, 0,16,54, 0,16,54, 0,12,50,
    0,12,50, 0,8,46, 0,4,46, 0,4,42, 0,4,0, 0,0,37,
    0,0,37, 0,0,33, 0,0,0, 31,31,31, 27,27,27, 23,23,23,
    16,16,16, 12,12,12, 8,8,8, 31,16,0, 31,21,0, 31,25,0,
    31,29,0, 31,31,0, 31,31,6, 31,31,14, 31,31,12, 31,31,31,
    0,16,31, 0,14,31, 0,12,31, 0,12,29, 0,10,29, 0,8,27,
    0,8,27, 0,6,25, 0,6,25, 0,4,23, 0,2,23, 0,2,21,
    0,2,0, 0,0,18, 0,0,18, 0,0,16};

  static char pal3[]={
    0,0,0, 63,63,63, 54,54,54, 0,33,63, 0,29,63, 0,25,63,
    0,25,58, 63,0,0, 54,0,0, 46,0,0, 37,0,0, 63,63,0,
    63,54,0, 63,46,0, 63,37,0, 37,37,37, 0,63,0, 0,54,0,
    0,46,0, 0,37,0, 0,21,58, 0,16,54, 0,16,54, 0,12,50,
    0,12,50, 0,8,46, 0,4,46, 0,4,42, 0,0,42, 0,0,37,
    0,0,37, 0,0,33, 0,0,0, 31,31,31, 27,27,27, 0,16,31,
    0,14,31, 0,12,31, 0,12,29, 31,0,0, 27,0,0, 23,0,0,
    18,0,0, 31,31,0, 31,27,0, 31,23,0, 31,18,0, 18,18,18,
    0,31,0, 0,27,0, 0,23,0, 0,18,0, 0,10,29, 0,8,27,
    0,8,27, 0,6,25, 0,6,25, 0,4,23, 0,2,23, 0,2,21,
    0,0,21, 0,0,18, 0,0,18, 0,0,16};

  switch(n){
    case 1: setpalette(pal1); break;
    case 2: setpalette(pal2); break;
    case 3: setpalette(pal3); break;
  }
}

void SetColor(WORD n)
{
  drcolor=n;
}

void blit(int x,int y,unsigned char far *tile,char far *screen)
{
  int deltax,width,height;
  register int i;
  deltax=320 - *tile;
  width=*tile;
  tile+=1;
  height=*tile;
  tile+=1;
  screen+=y*320+x;
bmyloop:
  for(i=0;i<width;i++){
    *screen=*tile;
    screen+=1;
    tile+=1;
  }
  screen+=deltax;
  height-=1;
  if(height>0) goto bmyloop;
}

void blitmask(int x,int y,unsigned char far *tile,char far *screen)
{
  int deltax,width,height;
  register int i;
  deltax=320 - *tile;
  width=*tile;
  tile+=1;
  height=*tile;
  tile+=1;
  screen+=y*320+x;
bmyloop:
  for(i=0;i<width;i++){
    if(*tile) *screen=*tile;
    screen+=1;
    tile+=1;
  }
  screen+=deltax;
  height-=1;
  if(height>0) goto bmyloop;
}

void littleblitmask(int x,int y,unsigned char far *tile,char far *screen)
{
  int deltax,width,height;
  register int i;
  deltax=(320 - *tile)+16;
  width=*tile;
  tile+=1;
  height=*tile;
  tile+=1;
  screen+=y*320+x;
bmyloop:
  for(i=0;i<width;i+=3){
    if(*tile) *screen=*tile;
    screen+=1;
    tile+=3;
  }
  tile+=48;
  screen+=deltax;
  height-=3;
  if(height>0) goto bmyloop;
}

void Blit16(WORD bmap,WORD x,WORD y,WORD image)
{
  if(bmap==LOGIC){
    blit(x<<4,y<<4,shapes[image],logical);
  }else{
    blit(x<<4,y<<4,shapes[image],physical);
  }
}

void BlitMask16(WORD bmap,WORD x,WORD y,WORD image)
{
  if(bmap==LOGIC){
    blitmask(x<<4,y<<4,shapes[image],logical);
  }else{
    blitmask(x<<4,y<<4,shapes[image],physical);
  }
}

void Blit24(WORD bmap,WORD x,WORD y,WORD image)
{
  if(bmap==LOGIC){
    blit(x*24,y*24,shapes[image],logical);
  }else{
    blit(x*24,y*24,shapes[image],physical);
  }
}

void BlitMask24(WORD bmap,WORD x,WORD y,WORD image)
{
  if(bmap==LOGIC){
    blitmask(x*24,y*24,shapes[image],logical);
  }else{
    blitmask(x*24,y*24,shapes[image],physical);
  }
}

void Blit(WORD bmap,WORD x,WORD y,WORD image)
{
  if(bmap==LOGIC){
    blit(x,y,shapes[image],logical);
  }else{
    blit(x,y,shapes[image],physical);
  }
}

void BlitMask(WORD bmap,WORD x,WORD y,WORD image)
{
  if(bmap==LOGIC){
    blitmask(x,y,shapes[image],logical);
  }else{
    blitmask(x,y,shapes[image],physical);
  }
}

void LittleTile(WORD bmap,WORD x,WORD y,WORD image)
{
  if(bmap==LOGIC){  
    littleblitmask(x<<3,y<<3,shapes[image],logical);
  }else{
    littleblitmask(x<<3,y<<3,shapes[image],physical);
  }
}

void OwnBlit(void)
{
  // does nothing
}

void DisownBlit(void)
{
  //does nothing
}


void Plot(WORD x,WORD y,BYTE c,char far *vscreen)
{
  asm {
    push es
    mov  ax,[y]
    mov  bx,320
    mul  bx
    add  ax,[x]
    les  di,[vscreen]
    mov  di,ax
    mov  al,c
    stosb
    pop  es
  }
}

void GetLevelName(char *lname,WORD firsttime)
{ /* returns each successive *.pit file in the levels\ directory */
  static struct ffblk myffblk;
  WORD i;

  if(firsttime==0){
    chdir("levels");
    if(!findfirst("*.pit",&myffblk,0)){
      strcpy(lname,myffblk.ff_name);
    }else{
      lname[0]=0;
    }
    chdir("..");
  }else{
    if(!findnext(&myffblk)){
      strcpy(lname,myffblk.ff_name);
    }else{
      lname[0]=0;
    }
  }
  if(lname[0]!=0){
    for(i=0;lname[i]!='.' && lname[i]!=0;i++);
    lname[i]=0;
  }
}

ULONG Timer(void)
{
  static ULONG time;
  time=(ULONG) *bios_ticks;
  return (time*33)/10;
}

void WasteTime(void)
{
  static ULONG targettime=0;
  while(Timer()<targettime && targettime-Timer()<=FRAMEDELAY){
    //if(targettime-Timer()>3){
    CheckMouse();
    //}
  }
  targettime=Timer()+FRAMEDELAY;
  if(targettime>216262) targettime=0;
}

void partialerasemouse(char far *screen,WORD newmx,WORD newmy)
{     /* erase parts of old mouse not covered by new mouse */
  WORD x1,x2,y1,y2,newx1,newy1,newx2,newy2,sx,sy;
  char far *dest,far *src,far *newmouse;

    /* erase old image */
  dest=screen;
  src=mousebg;
  x1=(mx-moffx);
  x2=x1+15;
  y1=my-moffy;
  y2=y1+15;
  newx1=newmx-moffx;
  newx2=newx1+15;
  newy1=newmy-moffy;
  newy2=newy1+15;
  sy=y1;
  dest+=y1*320+x1;
  newmouse=shapes[mouseimage+276]+2;   //skip width & height bytes
  newmouse+=((y1-newy1)<<4)+(x1-newx1);
  while(sy<=y2){
    sx=x1;
    while(sx<=x2){
      if(sx>=newx1 && sx<=newx2 && sy>=newy1 && sy<=newy2){
        if(*newmouse==0){  
          *dest=*src;
        }
      }else{
        *dest=*src;
      }
      newmouse+=1;
      dest+=1;
      src+=1;
      sx++;
    }
    dest+=304;
    sy++;
  }
}

void erasemouse(char far *screen)
{
  WORD i,j;
  char far *dest,far *src;

    /* erase old image */
  dest=screen+(my-moffy)*320+mx-moffx;
  src=mousebg;
  for(i=0;i<16;i++){
    for(j=0;j<16;j++){
      *dest=*src;
      dest+=1;
      src+=1;
    }
    dest+=304;
  }
}

void copymousebg(char far *screen)
{
  WORD i,j;
  char far *dest,far *src;

    /* save bg of new image */
  dest=mousebg;
  src=screen+(my-moffy)*320+mx-moffx;
  for(i=0;i<16;i++){
    for(j=0;j<16;j++){
      *dest=*src;
      dest+=1;
      src+=1;
    }
    src+=304;
  }
}

void drawmouse(char far *screen)
{
  WORD i,j;
  char far *dest,far *src;

    /* put new image on screen */
  /*
  dest=screen+(my-moffy)*320+mx-moffx;
  src=shapes[mouseimage+276]+2;   //skip width & height bytes
  for(i=0;i<16;i++){
    for(j=0;j<16;j++){
      *dest=*src;
      dest+=1;
      src+=1;
    }
    dest+=304;
  }
  */
  blitmask(mx-moffx,my-moffy,shapes[mouseimage+276],screen);
}

void SetMousePos(WORD x,WORD y)
{
  if(x<16) x=16;
  if(x>303) x=303;
  if(y<16) y=16;
  if(y>183) y=183;
  erasemouse(physical);
  mx=x;
  my=y;
  copymousebg(physical);
  drawmouse(physical);
}
 
void bgshiftleft(int n)
{
  unsigned char far *src, far *dest;
  int x,y,delta;

  if(n==0) return;
  src=mousebg+n;
  dest=mousebg;
  delta=n;
  for(y=0;y<16;y++){
    for(x=n;x<16;x++){
      *dest=*src;
      dest+=1;
      src+=1;
    }
    dest+=delta;
    src+=delta;
  }
}

void bgshiftright(int n)
{
  unsigned char far *src, far *dest;
  int x,y,delta;

  if(n==0) return;
  src=mousebg+255-n;
  dest=mousebg+255;
  delta=n;
  for(y=0;y<16;y++){
    for(x=n;x<16;x++){
      *dest=*src;
      dest-=1;
      src-=1;
    }
    dest-=delta;
    src-=delta;
  }
}

void bgshiftup(int n)
{
  unsigned char far *src, far *dest;
  int x,y;

  if(n==0) return;
  src=mousebg+(n<<4);
  dest=mousebg;
  for(y=n;y<16;y++){
    for(x=0;x<16;x++){
      *dest=*src;
      dest+=1;
      src+=1;
    }
  }
}

void bgshiftdown(int n)
{
  unsigned char far *src, far *dest;
  int x,y;

  if(n==0) return;
  src=mousebg+255-(n<<4);
  dest=mousebg+255;
  for(y=n;y<16;y++){
    for(x=0;x<16;x++){
      *dest=*src;
      dest-=1;
      src-=1;
    }
  }
}

void bgcopyleft(int n)
{
  unsigned char far *src, far *dest;
  int x,y,delta;

  if(n==0) return;
  n=16-n;
  delta=n;
  dest=mousebg+n;
  src=physical+(my-moffy)*320+(mx-moffx)+n;
  for(y=0;y<16;y++){
    for(x=n;x<16;x++){
      *dest=*src;
      dest+=1;
      src+=1;
    }
    src+=304+delta;
    dest+=delta;
  }
}

void bgcopyright(int n)
{
  unsigned char far *src, far *dest;
  int x,y,delta;

  if(n==0) return;
  n=16-n;
  delta=n;
  dest=mousebg;
  src=physical+(my-moffy)*320+(mx-moffx);
  for(y=0;y<16;y++){
    for(x=n;x<16;x++){
      *dest=*src;
      dest+=1;
      src+=1;
    }
    src+=304+delta;
    dest+=delta;
  }
}

void bgcopyup(int n)
{
  unsigned char far *src, far *dest;
  int x,y;

  if(n==0) return;
  n=16-n;
  dest=mousebg+(n<<4);
  src=physical+((my-moffy)+n)*320+(mx-moffx);
  for(y=n;y<16;y++){
    for(x=0;x<16;x++){
      *dest=*src;
      dest+=1;
      src+=1;
    }
    src+=304;
  }
}

void bgcopydown(int n)
{
  unsigned char far *src, far *dest;
  int x,y;

  if(n==0) return;
  n=16-n;
  dest=mousebg;
  src=physical+(my-moffy)*320+(mx-moffx);
  for(y=n;y<16;y++){
    for(x=0;x<16;x++){
      *dest=*src;
      dest+=1;
      src+=1;
    }
    src+=304;
  }
}

void CheckMouse(void)
{
  WORD x,y,mb,x1,x2,y1,y2,dx,dy,i,j;
  char newkey;
  unsigned char far *dest,far *src;

  if(mouseavailable){
    mb=readmbutton();
    button1=mb & 1;
    button2=(mb>>1) & 1;
    relpos(&x,&y);     //get new mouse pos
    x+=mx;
    y+=my;          //let x and y be new mouse coords for now
    if(x<16) x=16;
    if(x>303) x=303;
    if(y<16) y=16;
    if(y>183) y=183;
    if(x!=mx || y!=my){
      partialerasemouse(physical,x,y);
      if(x<mx){   //shift old mouse background 
        if(y<my){     // shift down-right
          dx=mx-x;
          dy=my-y;
          mx=x;
          my=y;
          if(dx<16 && dy<16){
            bgshiftdown(dy);
            bgshiftright(dx);
            bgcopydown(dy);
            bgcopyright(dx);
          }else{
            copymousebg(physical);
          }
        }else{      //shift up-right
          dx=mx-x;
          dy=y-my;
          mx=x;
          my=y;
          if(dx<16 && dy<16){
            bgshiftup(dy);
            bgshiftright(dx);
            bgcopyup(dy);
            bgcopyright(dx);
          }else{
            copymousebg(physical);
          }
        }
      }else{
        if(y<my){       //shift down-left
          dx=x-mx;       
          dy=my-y;
          mx=x;
          my=y;
          if(dx<16 && dy<16){
            bgshiftdown(dy);
            bgshiftleft(dx);
            bgcopydown(dy);
            bgcopyleft(dx);
          }else{
            copymousebg(physical);
          }
        }else{       //shift up-left
          dx=x-mx;
          dy=y-my;
          my=y;
          mx=x;
          if(dx<16 && dy<16){
            bgshiftup(dy);
            bgshiftleft(dx);
            bgcopyup(dy);
            bgcopyleft(dx);
          }else{
            copymousebg(physical);
          }
        }
      }
      //copymousebg(physical);
      drawmouse(physical);
    }
  }
    /* now check keyboard */
  newkey=*keytable;
  if(newkey>0 && newkey<83){   
    *keytable=0;
    switch(newkey){
      case 14: inkey=8; break;   //backspace
      case 28: inkey=13; break;  //enter
      case 1:  inkey=27; break;  //escape
      case 83: inkey=127; break; //delete
      case 72: inkey=28; break;  //up arrow
      case 80: inkey=29; break;  //down arrow
      case 77: inkey=30; break;  //right arrow
      case 75: inkey=31; break;  //left arrow
      default:
        if(newkey>=59 && newkey<=68){  //function keys
          inkey=newkey+70;
        }else{
          if(keytable[42] || keytable[54]){
            inkey=KeyMapUpper[newkey];
          }else{
            inkey=KeyMapLower[newkey];
          }
          if(inkey=='.' && newkey!=52) inkey=0xff;
        }
    }
  }
}
     
void ChangeMouse(WORD n)
{
  if(mouseimage!=n){
    erasemouse(physical);
    switch(n){
      case STDPOINTER:
        moffx=0;
        moffy=0;
        break;
      case CROSSHAIRS:
        moffx=7;
        moffy=7;
        break;
      case BRACKETCROSSHAIRS:
        moffx=7;
        moffy=7;
        break;
      case UPARROWPTR:
        moffx=7;
        moffy=0;
        break;
      case RIGHTARROWPTR:
        moffx=15;
        moffy=7;
        break;
      case DOWNARROWPTR:
        moffx=7;
        moffy=15;
        break;
      case LEFTARROWPTR:
        moffx=0;
        moffy=7;
        break;
      case BUSYPOINTER:
        moffx=7;
        moffy=7;
        break;
      case BLANKPOINTER:
        moffx=7;
        moffy=7;
        break;
    }
    copymousebg(physical);
    mouseimage=n;
    drawmouse(physical);
  }
}

void Frame(WORD x,WORD y,WORD width,WORD height,WORD c1,WORD c2)
{
  WORD i;
  WORD x2=x+width-1,y2=y+height-1;
  char far *screen=logical;
  screen+=y*320+x;
  for(i=x;i<x2;i++){
    *screen=c1;
    screen+=1;
  }
  for(i=y;i<y2;i++){
    *screen=c1;
    screen+=320;
  }
  for(i=x2;i>x;i--){
    *screen=c2;
    screen-=1;
  }
  for(i=y2;i>=y;i--){
    *screen=c2;
    screen-=320;
  }
}

void FilledFrame(WORD x,WORD y,WORD width,WORD height,WORD c1,WORD c2,WORD c3)
{
  Boxf(x+1,y+1,width-1,height-1,c3);
  Frame(x,y,width,height,c1,c2);
}

void Boxf(WORD x,WORD y,WORD width,WORD height,WORD c1)
{
  WORD deltax=320-width;
  WORD i,j,y2,x2;
  char far *screen=logical;
  screen+=y*320+x;
  x2=x+width-1;
  y2=y+height-1;
  for(i=y;i<=y2;i++){
    for(j=x;j<=x2;j++){
      *screen=c1;
      screen+=1;
    }
    screen+=deltax;
  }
}

void Pause(LONG n)
{
  ULONG targettime;
  targettime=Timer()+n;
  while(Timer()<targettime);
}

void LoadScreen(LONG n)
{
  FILE *fp;
  switch(n){
    case 0: fp=ReadFile("logo.dat"); break;
    case 1: fp=ReadFile("title.dat"); break;
  }
  ReadMem(fp,logical,64000);
  CloseFile(fp);
}

void FadeIn(void)
{
  WORD i,y,n;
  char far *src;             
  char far *dest;
  ULONG curtime;
  copymousebg(logical);
  drawmouse(logical);
  n=0;
  for(i=0;i!=320;i+=17){
    if(i>320){i-=320;}
    if(i<320){
      src=logical+i;
      dest=physical+i;
      curtime=Timer();
      for(y=0;y<200;y++){
        *dest=*src;
        dest+=320;
        src+=320;
      }
      n++;
      if(n==32) while(curtime==Timer());
      n%=32;
    }
  }
  erasemouse(logical);
}

void ScreenCopySidebar(void)
{
  //This function does nothing;
}

void LoadSprites(void)
{
  //This function does nothing.
}

void FreeSprites(void)
{
  //This function does nothing
}

void ScreenSwap(void)
{ 
  copymousebg(logical);
  drawmouse(logical);
  screenswap(logical);
  erasemouse(logical);
}

void ScreenCopy(void)
{
  //This function does nothing.
}

void clearscreen(char far *vscreen)
{
  asm{
    push es
    push di
    les  di,[vscreen]
    mov  cx,32000
    xor  ax,ax
    rep  movsw
    pop  di
    pop  es
  }
}

void Clear(WORD n)
{
  if(n==LOGIC){
    //clearscreen(screen);
    cls(logical);
  }
}

void FreeShapes(void)
{
  //this function does nothing special (holdover from Amiga version)
}

void LoadShapes(char *filename,WORD pos)
{
  UWORD width,height,i;
  unsigned char far *datpos;
  FILE *fp;

  filename[0]=0;

  fp=ReadFile("tiles1.dat");
  ReadMem(fp,shapedata1,FileSize(fp));
  CloseFile(fp);

  datpos=shapedata1;
  do{
    shapes[pos++]=datpos;
    width=datpos[0];
    height=datpos[1];
    datpos+=width*height+2;
  }while(width!=0);
  pos--;

  fp=ReadFile("tiles2.dat");
  ReadMem(fp,shapedata2,FileSize(fp));
  CloseFile(fp);

  datpos=shapedata2;
  do{
    shapes[pos++]=datpos;
    width=datpos[0];
    height=datpos[1];
    datpos+=width*height+2;
  }while(width!=0);
  pos--;

  fp=ReadFile("tiles3.dat");
  ReadMem(fp,shapedata3,FileSize(fp));
  CloseFile(fp);

  datpos=shapedata3;
  do{
    shapes[pos++]=datpos;
    width=datpos[0];
    height=datpos[1];
    /*
    if(pos>=277 & pos<=285){   //remap pointers to green
      datpos+=2;       //skip size bytes
      i=width*height;
      while(i){
        if(*datpos==28) *datpos=0;
        else *datpos-=12;
        datpos+=1;
        i--;
      }
    }else{
    */
    datpos+=width*height+2;
    //}
  }while(width!=0);

  fp=ReadFile("plasma1.fnt");
  ReadMem(fp,fontdata,FileSize(fp));
  CloseFile(fp);

  pos=32;
  datpos=fontdata;
  do{
    fontletter[pos++]=datpos;
    width=datpos[0];
    height=datpos[1];
    datpos+=width*height+2;
  }while(width!=0);
}

void PlaySound(int n)
{
  if(blasteravailable){
    if(sounds[n].samples!=0){
      dig_Play(&sounds[n]);
    }
  }
}

void gfxInit(void)
{
  int x;
  for(x=0;x<=255;x++) keytable[x]=0;
  oldmode=*(int far *)MK_FP(0x40,0x49);  //3  

  /*set mode to 13h (VGA 320x200)*/       
  setmode(0x13);  
  logical=(char far *) farmalloc(64000);
  shapedata1=(char far *) farmalloc(60000);
  shapedata2=(char far *) farmalloc(60000);
  shapedata3=(char far *) farmalloc(50000);
  fontdata=(char far *) farmalloc(5000);
  if(!logical || !shapedata1 || !shapedata2 || !shapedata3 || !fontdata) 
    cleanExit(1);
  mx=108;
  my=84;
  moffx=7;
  moffy=7;
  mouseimage=BLANKPOINTER;
  mouseavailable=initmouse();
  copymousebg(physical);
  setkb();
  blasteravailable=env_GetBlaster(&blaster);
  if(blasteravailable){
    blasteravailable=dig_Init(&blaster);
    if(blasteravailable){
      for(x=0;x<=9;x++) sounds[x].samples=0;
      load_Wave("crack.wav",&sounds[0]);
      //load_Wave("groan.wav",&sounds[1]);
      load_Wave("magic.wav",&sounds[2]);
      load_Wave("plateoff.wav",&sounds[3]);
      load_Wave("plateon.wav",&sounds[4]);
      load_Wave("scrape.wav",&sounds[5]);
      load_Wave("water.wav",&sounds[6]);
      load_Wave("lava.wav",&sounds[7]);
      load_Wave("splinter.wav",&sounds[8]);
      load_Wave("glass.wav",&sounds[9]);
    }
  }
}                                 

void cleanExit(WORD retval)
{
  int x;
  if(blasteravailable){
    dig_Kill();
    for(x=0;x<=7;x++){
      if(sounds[x].samples!=0){
        farfree(sounds[x].samples);
      }
    }
  }
  resetkb();
  if(fontdata) farfree(fontdata);
  if(shapedata3) farfree(shapedata3);
  if(shapedata2) farfree(shapedata2);
  if(shapedata1) farfree(shapedata1);
  if(logical) farfree(logical);
  setmode(oldmode);
  exit(retval);
}

