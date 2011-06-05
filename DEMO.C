#include <stdio.h>
#include <string.h>
#include "file.h"
#include "tiledefs.h"
#include "gfx.h"

typedef struct {
  WORD crack,groan,magic,plateoff,plateon,scrape,water,lava,splinter,glass;
} SFX;


#define abs(x) ((x)<0?-(x):(x))
#define PUSHDELAY 25

SFX sfx;

struct GameVars gamevars;
WORD  editxc=0, edityc=0;
WORD  under[23][23],over[23][23],pitcode[23][23];
WORD  tilemenu[10];
char  NumberString[]={"1234567890"};
WORD  letgo1;  //user: let go of mouse button #1!
WORD  copysidebar=0;
WORD  redgems,greengems,bluegems,yellowgems,redkeys,greenkeys,bluekeys;
extern unsigned char keytable[256];

struct PitCode  pitcodedef[MAXPITCODE];
#ifdef _amigapits_
__far unsigned char code[MAXCODEMEMORY];
#endif
#ifdef _ibmpits_
unsigned char far code[32768];
#endif
UWORD totalcode;

#define COMPILEGAME 1
WORD  walkable[100+1];
WORD  path[23][23],lastpushed[23][23],arrows[442],plates[442];
WORD  lastpathx,lastpathy;
WORD  pcmaster,pcintro,pcredg,pcgreeng,pcblueg;
WORD  regs[NUMREGISTERS];
CHAR  inline[80];
WORD  levelnotdone,levelrepeat,levelwon;
WORD  mouseinput=1;

WORD  firsttime,juststartedgame;

WORD  GetRegVal(CodePtr **pos);
void  SetRegVal(CodePtr **pos, WORD newvalue);
void  RunPitCode(WORD index,WORD x,WORD y,WORD onoff);
void  CheckGameTileLogic(WORD x,WORD y);
void  PrintText(CodePtr *pos,WORD len,WORD input);
void  StoneText(WORD x,WORD y,CHAR *str,WORD n,WORD c);
void  ShadowText(WORD x,WORD y,CHAR *str,WORD n,WORD c);
void  SetupGame(void);
void  DrawItemBox(WORD x,WORD y,WORD image,WORD quantity,WORD fnkey);
void  UpdateItem(WORD image);
void  DrawSideBar(void);
WORD  GameLoadLevel(void);
WORD  ProcessLevel(void);
void  MagicEye(void);
void  Gdrall(void);
void  CheckUnderGuy(void);
void  CheckStuff(void);
void  PathFind(WORD xdest,WORD ydest);
void  CheckAutoWalk(void);
void  MoveObject(WORD sx,WORD sy,WORD dx,WORD dy);
void  MoveGuy(WORD dx,WORD dy);
void  CheckGuy();
void  IncrementLevel(void);
void  CheckLevelDone(void);
void  GameMainLoop(void);

main()
{
  WORD x;
  char firstlvl[]={'D'-6,'e'-8,'m'-10,'o'-12,'0'-14,'1'-16,0};

  if(firstlvl[0]!='D'){
    for(x=0;firstlvl[x]!=0;x++) firstlvl[x]+=6+2*x;  /* Decrypt */
  }
  if(cryptkey[0]!='M'){
    for(x=0;cryptkey[x]!=0;x++) cryptkey[x]+=4*x+4;
  }

  gfxInit();
  ChangeMouse(BLANKPOINTER);
  LoadShapes("gametiles",0);

  LoadScreen(0);
  SetPalette(2);
  FadeIn();
  Pause(120);
  Clear(LOGIC);
  FadeIn();

  inkey=0xff;
  ChangeMouse(BLANKPOINTER);
  strcpy(gamevars.level,firstlvl);
  SetPalette(1);
  GameMainLoop();
  Clear(LOGIC);
  FadeIn();
  FreeShapes();
  cleanExit(RET_OK);
  return 0;
}

void ReadRun(FILE *fp,WORD *data)
{
  UBYTE n;
  WORD i,tile;

  for(i=0;i<24;i++) data[i]=0;
  while(i<505){
    n=ReadUByte(fp);
    tile=(WORD) ReadUByte(fp);
    while(n>0){
      data[i++]=tile;
      if((i+1)%23==0){
        data[i++]=0;
        data[i++]=0;
      }
      n--;
    }
  }
}

WORD GetRegVal(CodePtr **pos){
  WORD cmd,x,y;

  cmd=**pos;
  *pos+=1;
  if(cmd<=200){
    if(cmd<=100){
      return cmd;
    }else{
      return (WORD) (cmd-201);
    }
  }else{
    if(cmd<=REG_HECTOMOVES){
      return regs[cmd-201];
    }else{
      switch(cmd){
        case REG_WALKABLE:
          x=GetRegVal(pos);
          return walkable[Max(x,0)];
        case REG_DELTA:
          x=GetRegVal(pos);
          y=GetRegVal(pos);
          return (WORD) (x+y);
        case REG_UNDER:
          x=GetRegVal(pos);
          x=Min(Max(x,1),21);
          y=GetRegVal(pos);
          y=Min(Max(y,1),21);
          return under[x][y];
        case REG_OVER:
          x=GetRegVal(pos);
          x=Min(Max(x,1),21);
          y=GetRegVal(pos);
          y=Min(Max(y,1),21);
          if(x==regs[REG_XGUY-201] && y==regs[REG_YGUY-201] && (gamevars.greengemon==0 || pcgreeng>=0)) return 100;
          return over[x][y];
        case REG_LASTPUSHED:
          x=GetRegVal(pos);
          x=Min(Max(x,1),21);
          y=GetRegVal(pos);
          y=Min(Max(y,1),21);
          return lastpushed[x][y];
        case REG_REDGEMON:
          return gamevars.redgemon;
        case REG_GREENGEMON:
          return gamevars.greengemon;
        case REG_BLUEGEMON:
          return gamevars.bluegemon;
      }
    }
  }
  return 0;
}

void  SetRegVal(CodePtr **pos, WORD newvalue)
{
  WORD cmd,x,y;

  cmd=**pos;
  *pos+=1;
  if(cmd>200){
    if(cmd<=REG_HECTOMOVES){
      regs[cmd-201]=newvalue;
      if(cmd>=REG_REDGEMS && cmd<=REG_REDKEYS){
        UpdateItem(cmd-REG_REDGEMS+REDGEM);
        copysidebar=1;
      }
    }else{
      switch(cmd){
        case REG_WALKABLE:
          x=GetRegVal(pos);
          walkable[Max(x,0)]=newvalue;
          break;
        case REG_DELTA:
          x=GetRegVal(pos);
          y=GetRegVal(pos);
          //do nothing else (can't set delta!)
          break;
        case REG_UNDER:
          x=GetRegVal(pos);
          x=Min(Max(x,1),21);
          y=GetRegVal(pos);
          y=Min(Max(y,1),21);
          under[x][y]=Min(Max(newvalue,0),100);
          lastpushed[x][y]=-1;
          CheckGameTileLogic(x,y);
          if(newvalue>=60 && newvalue<=63){
            x=y*21+x;
            y=0;
            while(arrows[y]){
              if(arrows[y]==x) break;
              y++;
            }
            if(!arrows[y]){
              arrows[y++]=x;
              arrows[y]=0;
            }
          }
          break;
        case REG_OVER:
          x=GetRegVal(pos);
          x=Min(Max(x,1),21);
          y=GetRegVal(pos);
          y=Min(Max(y,1),21);
          over[x][y]=Min(Max(newvalue,0),100);
          lastpushed[x][y]=-1;
          CheckGameTileLogic(x,y);
          break;
        case REG_LASTPUSHED:
          x=GetRegVal(pos);
          x=Min(Max(x,1),21);
          y=GetRegVal(pos);
          y=Min(Max(y,1),21);
          lastpushed[x][y]=newvalue;
          break;
      }
    }
  }
}

void RunPitCode(WORD index,WORD x,WORD y,WORD onoff){
  CodePtr *pos,*oldpos,*destpos;
  WORD cmd,a,b,c,stkptr,notdone,x1,x2,y1,y2,z;
  WORD overtile;
  CodePtr *loopstack[32];

  if(index==-1 || index==0xff) return;
  pos=pitcodedef[index].codeptr;
  if(pos==NULLADDR) return;

  stkptr=0;
  notdone=1;
  regs[REG_XPOS-201]=x;
  regs[REG_YPOS-201]=y;

  while(notdone){
    cmd=(CodePtr) *pos;
    pos+=1;
    switch(cmd){
      case 0:
        notdone=0;
        break;
      case CMD_ENDIF:
        break;
      case CMD_NEXT:
        pos=loopstack[--stkptr];
        oldpos=pos;
        a=GetRegVal(&pos);
        b=GetRegVal(&pos);
        c=GetRegVal(&pos);
        if(c>b){   //loop forward
          if(a<c){
            loopstack[stkptr++]=oldpos;
            SetRegVal(&oldpos,a+1);
            pos+=2;
          }else{
            pos+=((pos[0])<<8)+pos[1];
          }
        }else{     //loop reverse
          if(a>c){
            loopstack[stkptr++]=oldpos;
            SetRegVal(&oldpos,a-1);
            pos+=2;
          }else{
            pos+=((pos[0])<<8)+pos[1];
          }
        }
        break;
      case CMD_ELSE:
        pos+=((pos[0])<<8)+pos[1];
        break;
      case CMD_SWITCHOFF:
        if(onoff==1) return;
        break;
      case CMD_SWITCHON:
        if(onoff==1){
          pos+=2;
        }else{
          pos+=((pos[0])<<8)+pos[1];
        }
        break;
      case CMD_IFEQ:
        a=GetRegVal(&pos);
        b=GetRegVal(&pos);
        if(a!=b){
          pos+=((pos[0])<<8)+pos[1];
          if(*pos==CMD_ELSE) pos+=3;
        }else{
          pos+=2;
        }
        break;
      case CMD_IFLT:
        a=GetRegVal(&pos);
        b=GetRegVal(&pos);
        if(a>=b){
          pos+=((pos[0])<<8)+pos[1];
          if(*pos==CMD_ELSE) pos+=3;
        }else{
          pos+=2;
        }
        break;
      case CMD_IFGT:
        a=GetRegVal(&pos);
        b=GetRegVal(&pos);
        if(a<=b){
          pos+=((pos[0])<<8)+pos[1];
          if(*pos==CMD_ELSE) pos+=3;
        }else{
          pos+=2;
        }
        break;
      case CMD_IFLE:
        a=GetRegVal(&pos);
        b=GetRegVal(&pos);
        if(a>b){
          pos+=((pos[0])<<8)+pos[1];
          if(*pos==CMD_ELSE) pos+=3;
        }else{
          pos+=2;
        }
        break;
      case CMD_IFGE:
        a=GetRegVal(&pos);
        b=GetRegVal(&pos);
        if(a<b){
          pos+=((pos[0])<<8)+pos[1];
          if(*pos==CMD_ELSE) pos+=3;
        }else{
          pos+=2;
        }
        break;
      case CMD_IFNE:
        a=GetRegVal(&pos);
        b=GetRegVal(&pos);
        if(a==b){
          pos+=((pos[0])<<8)+pos[1];
          if(*pos==CMD_ELSE) pos+=3;
        }else{
          pos+=2;
        }
        break;
      case CMD_IFINPUT:
        a=((pos[0])<<8)+pos[1];
        pos+=2;

        if(strnicmp(&pos[2],inline,a)!=0){
          pos+=((pos[0])<<8)+pos[1];
          if(*pos==CMD_ELSE) pos+=3;
        }else{
          pos+=2+a;
        }
        break;
      case CMD_FORLOOP:
        loopstack[stkptr++]=pos;
        oldpos=pos;
        a=GetRegVal(&pos);
        b=GetRegVal(&pos);
        c=GetRegVal(&pos);
        SetRegVal(&oldpos,b);  //set loop counter to first index
        pos+=2;                //first cmd inside loop!
        break;
      case CMD_REDRAW:
        Gdrall();
        ScreenSwap();
        if(copysidebar) {ScreenCopySidebar(); copysidebar=0;}
        break;
      case CMD_APPEAR:
        sfx.magic=1;
        Gdrall();
        FadeIn();
        break;
      case CMD_STOP:
        notdone=0;
        break;
      case CMD_WIN:
        levelnotdone=0;
        levelwon=1;
        notdone=0;
        break;
      case CMD_TARGET:
        while(button1 || keytable[F1KEY]) CheckMouse();
        ChangeMouse(CROSSHAIRS);
        if(!mouseinput) SetMousePos(108,84);
        do{
          Gdrall();
          ScreenSwap();
          if(copysidebar) {ScreenCopySidebar(); copysidebar=0;}
          CheckMouse();
          if(inkey==28) SetMousePos((mx/24)*24+12,(my/24)*24-24+12);
          if(inkey==29) SetMousePos((mx/24)*24+12,(my/24)*24+24+12);
          if(inkey==30) SetMousePos((mx/24)*24+24+12,(my/24)*24+12);
          if(inkey==31) SetMousePos((mx/24)*24-24+12,(my/24)*24+12);
          inkey=0xff;
        }while((!button1 && !keytable[CRKEY] && !keytable[F1KEY]) ||
                mx>=216 || my>=168);
        ChangeMouse(BUSYPOINTER);
        x1=mx/24-4+regs[REG_XGUY-201];
        y1=my/24-3+regs[REG_YGUY-201];
        if(x1<1 || x1>21 || y1<1 || y1>21){
          x1=0;
          y1=0;
        }
        regs[REG_XPOS-201]=x1;
        regs[REG_YPOS-201]=y1;
        while(button1 || keytable[CRKEY] || keytable[F1KEY]) CheckMouse();
        inkey=0xff;
        break;
      case CMD_REDGEMOFF:
        if(gamevars.redgemon){
          gamevars.redgemon=0;
          RunPitCode(pcredg,0,0,0);
          regs[REG_REDGEMS-201]--;
          UpdateItem(REDGEM);
          copysidebar=1;
        }
        break;
      case CMD_GREENGEMOFF:
        if(gamevars.greengemon){
          gamevars.greengemon=0;
          RunPitCode(pcgreeng,0,0,0);
          regs[REG_GREENGEMS-201]--;
          UpdateItem(GREENGEM);
          copysidebar=1;
        }
        break;
      case CMD_BLUEGEMOFF:
        if(gamevars.bluegemon){
          gamevars.bluegemon=0;
          RunPitCode(pcblueg,0,0,0);
          regs[REG_BLUEGEMS-201]--;
          UpdateItem(BLUEGEM);
          copysidebar=1;
        }
        break;
      case CMD_POPUP:
        under[x][y]=OFFPLATE;
        sfx.plateoff=1;
        break;
      case CMD_LET:
        oldpos=pos;
        a=GetRegVal(&pos);
        b=GetRegVal(&pos);
        SetRegVal(&oldpos,b);
        break;
      case CMD_ADD:
        oldpos=pos;
        a=GetRegVal(&pos);
        b=GetRegVal(&pos);
        SetRegVal(&oldpos,a+b);
        break;
      case CMD_SUB:
        oldpos=pos;
        a=GetRegVal(&pos);
        b=GetRegVal(&pos);
        SetRegVal(&oldpos,a-b);
        break;
      case CMD_MUL:
        oldpos=pos;
        a=GetRegVal(&pos);
        b=GetRegVal(&pos);
        SetRegVal(&oldpos,a*b);
        break;
      case CMD_DIV:
        oldpos=pos;
        a=GetRegVal(&pos);
        b=GetRegVal(&pos);
        if(b!=0){
          SetRegVal(&oldpos,a/b);
        }else{
          SetRegVal(&oldpos,0);
        }
        break;
      case CMD_SWAPOVER:
        x1=GetRegVal(&pos);
        y1=GetRegVal(&pos);
        x2=GetRegVal(&pos);
        y2=GetRegVal(&pos);
        if(x1>=1 && y1>=1 && x1<=21 && y1<=21 &&
           x2>=1 && y2>=1 && x2<=21 && y2<=21){
          a=over[x1][y1];
          over[x1][y1]=over[x2][y2];
          over[x2][y2]=a;
          a=lastpushed[x1][y1];
          lastpushed[x1][y1]=lastpushed[x2][y2];
          lastpushed[x2][y2]=a;
          if(x1==regs[REG_XGUY-201] && y1==regs[REG_YGUY-201]){
            regs[REG_XGUY-201]=x2;
            regs[REG_YGUY-201]=y2;
          }else{
            if(x2==regs[REG_XGUY-201] && y2==regs[REG_YGUY-201]){
              regs[REG_XGUY-201]=x1;
              regs[REG_YGUY-201]=y1;
            }
          }
          CheckGameTileLogic(x1,y1);
          CheckGameTileLogic(x2,y2);
        }
        break;
      case CMD_SWAPUNDER:
        x1=GetRegVal(&pos);
        y1=GetRegVal(&pos);
        x2=GetRegVal(&pos);
        y2=GetRegVal(&pos);
        if(x1>=1 && y1>=1 && x1<=21 && y1<=21 &&
           x2>=1 && y2>=1 && x2<=21 && y2<=21){
          a=under[x1][y1];
          under[x1][y1]=under[x2][y2];
          under[x2][y2]=a;
          a=lastpushed[x1][y1];
          lastpushed[x1][y1]=lastpushed[x2][y2];
          lastpushed[x2][y2]=a;
          CheckGameTileLogic(x1,y1);
          CheckGameTileLogic(x2,y2);
        }
        break;
      case CMD_GOTO:
        x1=GetRegVal(&pos);
        y1=GetRegVal(&pos);
        x2=GetRegVal(&pos);
        y2=GetRegVal(&pos);
        if(x1>=1 && y1>=1 && x1<=21 && y1<=21){
          a=pitcode[x1][y1];
          if(a<0xff){
            pos=pitcodedef[a].codeptr;
            regs[REG_XPOS-201]=x2;
            regs[REG_YPOS-201]=y2;
            if(pos==0) notdone=0;
          }else{
            notdone=0;
          }
        }
        break;
      case CMD_CHANGEMAP:
        a=((pos[0])<<8)+pos[1];
        pos+=2;
        destpos=&pos[a];
        while(pos<destpos){
          x1=GetRegVal(&pos);
          y1=GetRegVal(&pos);
          z=GetRegVal(&pos);
          overtile=over[x1][y1];
          if(z>=0 && z<=100){
            if(z<53 || z==100){   //new overtile
              switch(overtile){
                case BLUEBARREL:  gamevars.bluecount--;  break;
                case REDCRATE:    gamevars.redcount--;   break;
                case GREENCHEST:  gamevars.greencount--; break;
              }
              if(z==BLUEBARREL || z==REDCRATE || z==GREENCHEST){
                z-=4; break;
              }
              over[x1][y1]=z;
            }else{
              under[x1][y1]=z;
              if(z>=60 && z<=63){
                x2=y1*21+x1;
                y2=0;
                while(arrows[y2]){
                  if(arrows[y2]==x2) break;
                  y2++;
                }
                if(!arrows[y2]){
                  arrows[y2++]=x2;
                  arrows[y2]=0;
                }
              }
            }
            lastpushed[x1][y1]=-1;
            CheckGameTileLogic(x1,y1);
          }else{
            switch(z){
              case CLEAR_ALL:  //clear under & over
                under[x1][y1]=EMPTYSPACE;
                CheckGameTileLogic(x1,y1);
                over[x1][y1]=EMPTYSPACE;
                break;
              case CLEAR_OVER:  //clear over
                over[x1][y1]=EMPTYSPACE;
                CheckGameTileLogic(x1,y1);
                break;
              case CLEAR_UNDER:  //clear under
                under[x1][y1]=EMPTYSPACE;
                CheckGameTileLogic(x1,y1);
                break;
            }
          }
        }
        break;
      case CMD_PRINT:
        a=((pos[0])<<8)+pos[1];
        pos+=2;
        PrintText(pos,a,0);
        pos+=a;
        break;
      case CMD_INPUT:
        a=((pos[0])<<8)+pos[1];
        pos+=2;
        PrintText(pos,a,1);
        pos+=a;
        break;
    }
  }
}

void CheckGameTileLogic(WORD x,WORD y)
{
  WORD undertile;

  undertile=under[x][y];
  switch(over[x][y]){
    case BLUEBARREL:  over[x][y]=BARREL;
                      gamevars.bluecount--;
                      break;
    case REDCRATE:    over[x][y]=CRATE;
                      gamevars.redcount--;
                      break;
    case GREENCHEST:  over[x][y]=CHEST;
                      gamevars.greencount--;
                      break;
    case HOLEBARREL:
    case WATERBARREL: over[x][y]=BARREL;
                      break;
  }
  switch(over[x][y]){
    case BARREL:  switch(undertile){
                    case EMPTYSPACE:  over[x][y]=EMPTYSPACE;  break;
                    case WATER:       over[x][y]=WATERBARREL;
                                      sfx.water=1;
                                      break;
                    case LAVADEBRIS1:
                    case LAVADEBRIS2:
                    case LAVA:        over[x][y]=EMPTYSPACE;
                                      under[x][y]=LAVADEBRIS1;
                                      sfx.lava=1;
                                      break;
                    case BLUETARGET:  over[x][y]=BLUEBARREL;
                                      gamevars.bluecount++;
                                      break;
                    case HOLE:        over[x][y]=HOLEBARREL;
                                      sfx.splinter=1;
                                      break;
                    case WOOD:        over[x][y]=HOLEBARREL;
                                      sfx.splinter=1;
                                      break;
                  } break;
    case CRATE:   switch(undertile){
                    case EMPTYSPACE:  over[x][y]=EMPTYSPACE;  break;
                    case WATER:       over[x][y]=EMPTYSPACE;
                                      sfx.water=1;
                                      under[x][y]=WATERCRATE; break;
                    case LAVADEBRIS1:
                    case LAVADEBRIS2:
                    case LAVA:        over[x][y]=EMPTYSPACE;
                                      under[x][y]=LAVADEBRIS1;
                                      sfx.lava=1;
                                      break;
                    case REDTARGET:   over[x][y]=REDCRATE;
                                      gamevars.redcount++;
                                      break;
                    case HOLE:
                    case WOOD:        over[x][y]=EMPTYSPACE;
                                      under[x][y]=HOLE;
                                      sfx.splinter=1;
                                      break;
                  } break;
    case CHEST:   switch(undertile){
                    case EMPTYSPACE:  over[x][y]=EMPTYSPACE;  break;
                    case GREENTARGET: over[x][y]=GREENCHEST;
                                      gamevars.greencount++;
                                      break;
                    case LAVA:        under[x][y]=GLASS;
                                      sfx.crack=1;
                    case HOLE:        over[x][y]=EMPTYSPACE;  break;
                  } break;
  }
}

void PrintText(CodePtr *pos,WORD len,WORD input)
{
  WORD tlen,i,s,y,backup,x,oldx,oldy;
  char scanbuffer[80];
  i=0;
  oldx=mx; oldy=my;  //old mouse position
  while(i<len){
    FilledFrame(0,0,216,168,3,6,5);
    y=8;
    while(y<164 && i<len){
      tlen=4;
      backup=0;
      s=i;
      while(tlen<205 && pos[i]!=10 && i<len){
        tlen+=TxtLength(&pos[i],1);
        if(pos[i]==' ') backup=1;
        i++;
      }
      if(tlen>=205 && backup){
        while(pos[i]!=' '){
          i--;
        }
      }
      if(i>s) StoneText(4,y,&pos[s],i-s,0);
      y+=11;
      while(pos[i]==' ') i++;
      if(pos[i]==10) i++;
    }
    if(juststartedgame){
      juststartedgame=0;
      FadeIn();
    }else{
      ScreenSwap();
      ScreenCopy();
      if(copysidebar){ScreenCopySidebar(); copysidebar=0;}
    }
    SetMousePos(216,168);
    ChangeMouse(BUSYPOINTER);
    if(i>=len && input){  //get user input
      x=0;
      inkey=0xff;
      while(inkey!=13){
        Boxf(3,y-7,200,10,5);
        StoneText(4,y,inline,x,0);
        StoneText(4+TxtLength(inline,x),y,"_",1,0);
        ScreenSwap();
        inkey=0xff;
        do{
          CheckMouse();
        }while(inkey==0xff);
        if(inkey>=' ' && inkey<=126 && x<16){
          inline[x++]=inkey;
        }
        if(inkey==8 && x>0) x--;   //backspace
      }
      inline[x]=0;  //null terminate string
      strcpy(scanbuffer,inline);
      sscanf(scanbuffer,"%d",&regs[REG_INPUTVALUE-201]);
      //SetMousePos(oldx,oldy);  //Restore mouse as favor to keyboard users
    }else{
      inkey=0xff;
      while(button1==1 || keytable[CRKEY] || keytable[SPKEY]) CheckMouse();  //let go, dammit!
      while(button1==0 && !keytable[CRKEY] && !keytable[SPKEY]){
        CheckMouse();  //okay, you can click now
      }
      while(button1==1 || keytable[CRKEY] || keytable[SPKEY]) CheckMouse();  //let go again
    }
    if(!mouseinput){
      ChangeMouse(BLANKPOINTER);
      SetMousePos(108,84);
    }
    ScreenCopy();
    inkey=0xff;
  }
}

void StoneText(WORD x,WORD y,CHAR *str,WORD n,WORD c)
{
  SetColor(3);
  Txt(x-1,y,str,n);
  Txt(x,y+1,str,n);
  SetColor(6);
  Txt(x+1,y,str,n);
  Txt(x,y-1,str,n);
  SetColor(c);
  Txt(x,y,str,n);
}

void ShadowText(WORD x,WORD y,CHAR *str,WORD n,WORD c)
{
  SetColor(0);
  Txt(x+1,y+1,str,n);
  SetColor(c);
  Txt(x,y,str,n);
}

void SetupGame(void)
{
  DrawSideBar();
}

void  DrawItemBox(WORD x,WORD y,WORD image,WORD quantity,WORD fnkey)
{
  static char quanstring[6];
  static tlen;

  if(quantity==0){
    Boxf(x-6,y,32,36,5);
  }else{
    Boxf(x,y+27,26,10,5);  //Clear old text
    OwnBlit();
    Blit(LOGIC,x-6,y+1,fnkey);
    BlitMask(LOGIC,x+1,y+1,image);
    DisownBlit();
    Frame(x,y,26,26,3,6);
    if(quantity!=-1){
      sprintf(quanstring,"%d",quantity);
      tlen=TxtLength(quanstring,strlen(quanstring));
      StoneText(x+((26-tlen)>>1),y+34,quanstring,strlen(quanstring),0);
    }
  }
}

void UpdateItem(WORD image)
{
  switch(image){
    case REDGEM:
      if(gamevars.redgemon) DrawItemBox(223,89,REDGEMON,regs[REG_REDGEMS-201],F1SHAPE);
      else DrawItemBox(223,89,REDGEM,regs[REG_REDGEMS-201],F1SHAPE);
      break;
    case GREENGEM:
      if(gamevars.greengemon) DrawItemBox(255,89,GREENGEMON,regs[REG_GREENGEMS-201],F2SHAPE);
      else DrawItemBox(255,89,GREENGEM,regs[REG_GREENGEMS-201],F2SHAPE);
      break;
    case BLUEGEM:
      if(gamevars.bluegemon) DrawItemBox(287,89,BLUEGEMON,regs[REG_BLUEGEMS-201],F3SHAPE);
      else DrawItemBox(287,89,BLUEGEM,regs[REG_BLUEGEMS-201],F3SHAPE);
      break;
    case YELLOWGEM:
      if(gamevars.yellowgemon) DrawItemBox(287,51,YELLOWGEMON,regs[REG_YELLOWGEMS-201],F4SHAPE);
      else DrawItemBox(287,51,YELLOWGEM,regs[REG_YELLOWGEMS-201],F4SHAPE);
      break;
    case REDKEY:
      DrawItemBox(223,127,REDKEY,regs[REG_REDKEYS-201],F5SHAPE);
      break;
    case GREENKEY:
      DrawItemBox(255,127,GREENKEY,regs[REG_GREENKEYS-201],F6SHAPE);
      break;
    case BLUEKEY:
      DrawItemBox(287,127,BLUEKEY,regs[REG_BLUEKEYS-201],F7SHAPE);
      break;
  }
}

void DrawSideBar(void)
{
  FilledFrame(216,0,104,168,3,6,5);
  DrawItemBox(223,13,YIELD,-1,F9SHAPE);
  DrawItemBox(287,13,STOP,-1,F10SHAPE);
  if(gamevars.yellowgemon) DrawItemBox(287,51,YELLOWGEMON,regs[REG_YELLOWGEMS-201],F4SHAPE);
  else DrawItemBox(287,51,YELLOWGEM,regs[REG_YELLOWGEMS-201],F4SHAPE);
  if(gamevars.redgemon) DrawItemBox(223,89,REDGEMON,regs[REG_REDGEMS-201],F1SHAPE);
  else DrawItemBox(223,89,REDGEM,regs[REG_REDGEMS-201],F1SHAPE);
  if(gamevars.greengemon) DrawItemBox(255,89,GREENGEMON,regs[REG_GREENGEMS-201],F2SHAPE);
  else DrawItemBox(255,89,GREENGEM,regs[REG_GREENGEMS-201],F2SHAPE);
  if(gamevars.bluegemon) DrawItemBox(287,89,BLUEGEMON,regs[REG_BLUEGEMS-201],F3SHAPE);
  else DrawItemBox(287,89,BLUEGEM,regs[REG_BLUEGEMS-201],F3SHAPE);
  DrawItemBox(223,127,REDKEY,regs[REG_REDKEYS-201],F5SHAPE);
  DrawItemBox(255,127,GREENKEY,regs[REG_GREENKEYS-201],F6SHAPE);
  DrawItemBox(287,127,BLUEKEY,regs[REG_BLUEKEYS-201],F7SHAPE);
}

WORD GameLoadLevel(void)
{
  FILE *fp;
  char fname[40]=LEVELPATH;
  static char key[]={'A'-5,'m'-10,'a'-15,'n'-20,'d'-25,'a'-30,0};
  WORD numpitcode,magic,i,keypos;

  if(key[0]!='A'){
    for(i=0;key[i]!=0;i++) key[i]+=(i+1)*5;
  }
  strcat(fname,gamevars.level);
  strcat(fname,".pit");
  if(!Exists(fname)) return 0;
  fp=ReadFile(fname);

  magic=ReadWord(fp);  //Magic Number
  if(magic!=0x0895){
    CloseFile(fp);
    return 0;
  }
  ReadString(fp,gamevars.password);
  ReadString(fp,gamevars.title);
  if(strcmp(gamevars.title,"Unstable Ground")!=0 && strcmp(gamevars.title,"Remote Control")!=0){
    CloseFile(fp);
    return 0;
  }
  ReadRun(fp,(WORD *) under);
  ReadRun(fp,(WORD *) over);
  numpitcode=ReadWord(fp);
  pcmaster=-1;
  pcintro=-1;
  pcredg=-1;
  pcgreeng=-1;
  pcblueg=-1;
  if(numpitcode){
    ReadRun(fp,(WORD *) pitcode);
    while(numpitcode){
      i=ReadWord(fp);
      ReadString(fp,pitcodedef[i].name);
      if(stricmp(pitcodedef[i].name,"Intro")==0) pcintro=i;
      else if(stricmp(pitcodedef[i].name,"Master")==0) pcmaster=i;
      else if(stricmp(pitcodedef[i].name,"RedGem")==0) pcredg=i;
      else if(stricmp(pitcodedef[i].name,"GreenGem")==0) pcgreeng=i;
      else if(stricmp(pitcodedef[i].name,"BlueGem")==0) pcblueg=i;
      pitcodedef[i].codesize=ReadWord(fp);
      pitcodedef[i].codeptr=ReadWord(fp)+code;
      if(pitcodedef[i].codesize==0){
        pitcodedef[i].codeptr=NULLADDR;
      }
      numpitcode--;
    }
    totalcode=ReadWord(fp);
    ReadMem(fp,code,totalcode);
    for(i=0,keypos=0;i<totalcode;i++){
      code[i]^=key[keypos++];
      if(key[keypos]==0) keypos=0;
    }
    for(i=0;i<10;i++){
      regs[i]=0;
    }
 }

  CloseFile(fp);

  FilledFrame(0,168,320,32,3,6,5);
  StoneText(5,179,gamevars.level,strlen(gamevars.level),0);
  StoneText(5,193,gamevars.title,strlen(gamevars.title),0);
  DrawSideBar();
  copysidebar=1;
  ProcessLevel();
  return 1;
}

WORD ProcessLevel(void)  /* Sets up internal arrays according to level */
{
  WORD x,y,arrowptr;
  gamevars.redcount=0;
  gamevars.greencount=0;
  gamevars.bluecount=0;
  gamevars.totalred=0;
  gamevars.totalgreen=0;
  gamevars.totalblue=0;
  regs[REG_XGUY-201]=0;
  arrowptr=0;
  for(y=1;y<=21;y++){
    for(x=1;x<=21;x++){
      lastpushed[x][y]=-1;
      switch(under[x][y]){
        case GLASS:
        case UPARROW:
        case RIGHTARROW:
        case DOWNARROW:
        case LEFTARROW:
          arrows[arrowptr++]=y*21+x;
          break;
        case REDTARGET:
          gamevars.totalred++;
          break;
        case GREENTARGET:
          gamevars.totalgreen++;
          break;
        case BLUETARGET:
          gamevars.totalblue++;
          break;
      }
      switch(over[x][y]){
        case GUY:
          regs[REG_XGUY-201]=x;
          regs[REG_YGUY-201]=y;
          regs[REG_LASTX-201]=x;
          regs[REG_LASTY-201]=y;
          over[x][y]=EMPTYSPACE;
          break;
        case REDCRATE:
          gamevars.redcount++;
          break;
        case GREENCHEST:
          gamevars.greencount++;
          break;
        case BLUEBARREL:
          gamevars.bluecount++;
          break;
      }
    }
  }
  arrows[arrowptr]=0;
  if(regs[REG_XGUY-201]==0){
    regs[REG_XGUY-201]=1;
    regs[REG_YGUY-201]=1;
  }

  /* Restore walkable array (may have been changed last level) */
  for(x=0;x<=100;x++) walkable[x]=0;
  for(x=FIRST_UNDERTILE;x<=GRASS;x++) walkable[x]=1;

  regs[REG_FACING-201]=NORTH;
  gamevars.walkframe=0;
  gamevars.pushing=0;
  gamevars.lasttimewalked=0;
  gamevars.autowalk=0;
  gamevars.moves=0;
  regs[REG_MOVES-201]=0;
  regs[REG_HECTOMOVES-201]=0;
  if(gamevars.greengemon>0) regs[REG_GREENGEMS-201]--;
  if(gamevars.redgemon==1) regs[REG_REDGEMS-201]--;
  if(gamevars.bluegemon==1) regs[REG_BLUEGEMS-201]--;
  gamevars.redgemon=0;
  gamevars.greengemon=0;
  gamevars.bluegemon=0;
  gamevars.yellowgemon=0;
  UpdateItem(REDGEM);
  UpdateItem(BLUEGEM);
  UpdateItem(GREENGEM);
  copysidebar=1;
  sfx.crack=0;
  sfx.groan=0;
  sfx.magic=0;
  sfx.plateoff=0;
  sfx.plateon=0;
  sfx.scrape=0;
  sfx.water=0;
  sfx.lava=0;

  return 1;
}

void MagicEye(void)
{
  WORD underguy,walkframe,guyframe,magic;
  static WORD walkframemap[]={0,16,0,32};
  static WORD pushframemap[]={0,16,0,16};
  REGISTER WORD tile;
  WORD x,y,i;

  underguy=over[regs[REG_XGUY-201]][regs[REG_YGUY-201]];
  magic=gamevars.redgemon+gamevars.greengemon+gamevars.bluegemon;
  if(gamevars.pushing==0 || magic){
    walkframe=walkframemap[gamevars.walkframe];
    if(magic){
      guyframe=GUYMAGIC;
    }else{
      guyframe=GUY;
    }
    guyframe+=regs[REG_FACING-201]+walkframe;
  }else{
    walkframe=pushframemap[gamevars.walkframe];
    guyframe=GUYPUSH+regs[REG_FACING-201]+walkframe;
  }
  if(gamevars.greengemon>1){
    guyframe=GUYMORPH+20;
  }
  if(gamevars.greengemon<7){
    over[regs[REG_XGUY-201]][regs[REG_YGUY-201]]=guyframe;
  }

  PlaySound(SND_MAGIC);
  i=0;
  do{
    y=i/22;
    x=i%22;
    if(y>0){
      tile=under[x][y];
      if(tile) LittleTile(PHYSIC,x+2,y-1,tile);
      tile=over[x][y];     if(tile) LittleTile(PHYSIC,x+2,y-1,tile+3);
      tile=over[x+1][y];   if(tile) LittleTile(PHYSIC,x+2,y-1,tile+2);
      tile=over[x][y+1];   if(tile) LittleTile(PHYSIC,x+2,y-1,tile+1);
      tile=over[x+1][y+1]; if(tile) LittleTile(PHYSIC,x+2,y-1,tile);
    }
    i+=59;
    if(i>=484) i-=484;
  }while(i!=0);
  if(gamevars.greengemon==7){
    x=regs[REG_XGUY-201]+1;
    y=regs[REG_YGUY-201]-2;
    LittleTile(PHYSIC,x,y,guyframe);
    LittleTile(PHYSIC,x+1,y,guyframe+1);
    LittleTile(PHYSIC,x,y+1,guyframe+2);
    LittleTile(PHYSIC,x+1,y+1,guyframe+3);
  }
  if(gamevars.greengemon<7){
    over[regs[REG_XGUY-201]][regs[REG_YGUY-201]]=underguy;
  }
  while(button1==1 || keytable[F4KEY]) CheckMouse();
  while(button1==0 && !keytable[SPKEY] && !keytable[CRKEY]
         && !keytable[F4KEY]) CheckMouse();
  while(button1==1 || keytable[SPKEY] || keytable[CRKEY]
        || keytable[F4KEY]) CheckMouse();
  inkey=0xff;
}

void Gdrall(void)
{
  static WORD underguy,x,y,xd,yd,frame=0,walkframe,guyframe,magic;
  static WORD glassflash=0,flashx,flashy,flashon=0;
  static WORD walkframemap[]={0,16,0,32};
  static WORD pushframemap[]={0,16,0,16};
  static ULONG lastdrew=0;
  static char movestring[16];
  REGISTER WORD tile;

  flashx=glassflash%9;
  flashy=glassflash/9;
  if(under[flashx+regs[REG_XGUY-201]-4][flashy+regs[REG_YGUY-201]-3]!=GLASS){
    glassflash+=17;
    glassflash%=63;
  }
  magic=gamevars.redgemon+gamevars.greengemon+gamevars.bluegemon;
  underguy=over[regs[REG_XGUY-201]][regs[REG_YGUY-201]];
  if(gamevars.pushing==0 || magic){
    walkframe=walkframemap[gamevars.walkframe];
    if(magic){
      guyframe=GUYMAGIC;
    }else{
      guyframe=GUY;
    }
    guyframe+=regs[REG_FACING-201]+walkframe;
  }else{
    walkframe=pushframemap[gamevars.walkframe];
    guyframe=GUYPUSH+regs[REG_FACING-201]+walkframe;
  }
  if(gamevars.greengemon>1){
    guyframe=GUYMORPH+(gamevars.greengemon-2)*4;
  }
  if(gamevars.greengemon<7){
    over[regs[REG_XGUY-201]][regs[REG_YGUY-201]]=guyframe;
  }
  yd=regs[REG_YGUY-201]-3;
  OwnBlit();

  //while(Timer()-lastdrew<FRAMEDELAY);
  WasteTime();  //make sure the delay between frames has been long enough
  for(y=0; y<7; y++){
    xd=regs[REG_XGUY-201]-4;
    for(x=0; x<9; x++){
      if(xd>=0 && xd<=21 && yd>=0 && yd<=21){
        tile=under[xd][yd];
        switch(tile){
          case WATER:
          case LAVADEBRIS1:
          case LAVADEBRIS2:
          case LAVA:  tile+=frame; break;
          case GLASS:
              if(x==flashx && y==flashy){
                if(flashon) tile+=frame+1;
                else if(frame==0){flashon=1; tile++;}
              }
              break;
        }
        Blit24(LOGIC,x,y,tile);
        tile=over[xd][yd];
        if(tile) BlitMask24(LOGIC,x,y,tile+3);
        tile=over[xd+1][yd];
        if(tile) BlitMask24(LOGIC,x,y,tile+2);
        tile=over[xd][yd+1];
        if(tile) BlitMask24(LOGIC,x,y,tile+1);
        tile=over[xd+1][yd+1];
        if(tile) BlitMask24(LOGIC,x,y,tile);
        switch(under[xd][yd]){
          case LAVADEBRIS2:
            if(frame==1) under[xd][yd]=LAVA; break;
          case LAVADEBRIS1:
            if(frame==1) under[xd][yd]=LAVADEBRIS2; break;
          case GLASS:
            if(x==flashx && y==flashy && flashon && frame==1){
              flashon=0;
              glassflash+=17;
              glassflash%=63;
            }
            break;
        }
      }else{
        Blit24(LOGIC,x,y,EMPTYSPACE);
      }
      xd++;
      frame+=1;
      frame%=4;
    } /* end x loop */
    yd++;
    #ifdef _ibmpits_
      CheckMouse();
    #endif
  }
  if(gamevars.pushing && !magic){
    switch(regs[REG_FACING-201]){
      case EAST:  BlitMask24(LOGIC,4,2,guyframe+1);
                  BlitMask24(LOGIC,4,3,guyframe+3);
                  xd=regs[REG_XGUY-201];
                  yd=regs[REG_YGUY-201];
                  tile=over[xd][yd+1];
                  if(tile) BlitMask24(LOGIC,4,3,tile+1);
                  tile=over[xd+1][yd+1];
                  if(tile) BlitMask24(LOGIC,4,3,tile);
                  break;
      case SOUTH: BlitMask24(LOGIC,3,3,guyframe+2);
                  BlitMask24(LOGIC,4,3,guyframe+3);
                  xd=regs[REG_XGUY-201];
                  yd=regs[REG_YGUY-201];
                  tile=over[xd+1][yd];
                  if(tile) BlitMask24(LOGIC,4,3,tile+2);
                  tile=over[xd+1][yd+1];
                  if(tile) BlitMask24(LOGIC,4,3,tile);
                  break;
    }
  }
  if(gamevars.greengemon==7){
    BlitMask24(LOGIC,3,2,guyframe);
    BlitMask24(LOGIC,4,2,guyframe+1);
    BlitMask24(LOGIC,3,3,guyframe+2);
    BlitMask24(LOGIC,4,3,guyframe+3);
  }
  DisownBlit();
  lastdrew=Timer();
  if(gamevars.greengemon<7){
    over[regs[REG_XGUY-201]][regs[REG_YGUY-201]]=underguy;
  }

  /* Update moves display */
  Boxf(216,172,103,10,5);
  sprintf(movestring,"Moves:  %d",gamevars.moves);
  StoneText(216+1,173+6,movestring,strlen(movestring),0);
    /* play sfx */
  if(sfx.crack){ sfx.crack=0; PlaySound(SND_CRACK);}
  if(sfx.groan){ sfx.groan=0;}
  if(sfx.magic){ sfx.magic=0; PlaySound(SND_MAGIC);}
  if(sfx.plateoff){ sfx.plateoff=0;  PlaySound(SND_PLATEOFF);}
  if(sfx.plateon){  sfx.plateon=0;   PlaySound(SND_PLATEON);}
  if(sfx.scrape){sfx.scrape=0; PlaySound(SND_SCRAPE);}
  if(sfx.water){sfx.water=0; PlaySound(SND_WATER);}
  if(sfx.lava){sfx.lava=0; PlaySound(SND_LAVA);}
  if(sfx.splinter){sfx.splinter=0; PlaySound(SND_SPLINTER);}
  if(sfx.glass){sfx.glass=0; PlaySound(SND_GLASS);}
}

void CheckUnderGuy(void)
{
  /* Check under guy */
  if(gamevars.greengemon<7){
  switch(under[regs[REG_XGUY-201]][regs[REG_YGUY-201]]){
    case REDGEM:
      regs[REG_REDGEMS-201]++;
      under[regs[REG_XGUY-201]][regs[REG_YGUY-201]]=FLOOR;
      UpdateItem(REDGEM);
      copysidebar=1;
      break;
    case GREENGEM:
      regs[REG_GREENGEMS-201]++;
      under[regs[REG_XGUY-201]][regs[REG_YGUY-201]]=FLOOR;
      UpdateItem(GREENGEM);
      copysidebar=1;
      break;
    case BLUEGEM:
      regs[REG_BLUEGEMS-201]++;
      under[regs[REG_XGUY-201]][regs[REG_YGUY-201]]=FLOOR;
      UpdateItem(BLUEGEM);
      copysidebar=1;
      break;
    case YELLOWGEM:
      regs[REG_YELLOWGEMS-201]=-1;
      under[regs[REG_XGUY-201]][regs[REG_YGUY-201]]=FLOOR;
      UpdateItem(YELLOWGEM);
      copysidebar=1;
      break;
    case REDKEY:
      regs[REG_REDKEYS-201]++;
      under[regs[REG_XGUY-201]][regs[REG_YGUY-201]]=FLOOR;
      UpdateItem(REDKEY);
      copysidebar=1;
      break;
    case GREENKEY:
      regs[REG_GREENKEYS-201]++;
      under[regs[REG_XGUY-201]][regs[REG_YGUY-201]]=FLOOR;
      UpdateItem(GREENKEY);
      copysidebar=1;
      break;
    case BLUEKEY:
      regs[REG_BLUEKEYS-201]++;
      under[regs[REG_XGUY-201]][regs[REG_YGUY-201]]=FLOOR;
      UpdateItem(BLUEKEY);
      copysidebar=1;
      break;
  }
  }
}

void CheckStuff(void)
{
  WORD x,y,undertile,overtile,arrowptr,plateptr,isglass;

  regs[REG_MOVES-201]=gamevars.moves % 100;
  regs[REG_HECTOMOVES-201]=gamevars.moves/100;

  CheckUnderGuy();

  RunPitCode(pcmaster,0,0,1);
  if(pcredg>=0 && gamevars.redgemon) RunPitCode(pcredg,0,0,1);
  if(pcgreeng>=0 && gamevars.greengemon) RunPitCode(pcgreeng,0,0,1);
  if(pcblueg>=0 && gamevars.bluegemon) RunPitCode(pcblueg,0,0,1);

  /* do floor arrows & glass before checking to alter objects*/
  arrowptr=0;
  while((x=arrows[arrowptr++])>0){
    y=x/21;
    x%=21;
    if(over[x][y]>=BARREL){
      undertile=under[x][y];
      isglass=0;
      if(undertile==GLASS){
        if(lastpushed[x][y]>=NORTHPUSH){
          undertile=UPARROW+lastpushed[x][y];
          isglass=1;
        }
      }
      switch(undertile){
        case UPARROW:
          if(x==regs[REG_XGUY-201] && y-1==regs[REG_YGUY-201] && gamevars.greengemon<7) overtile=GUY;
          else overtile=over[x][y-1];
          if((walkable[overtile] || overtile==EMPTYSPACE) && under[x][y-1]!=GRASS){
            MoveObject(x,y,x,y-1);
            if(isglass) sfx.glass=1; else sfx.magic=1;
          }
          else lastpushed[x][y]=-1;
          break;
        case RIGHTARROW:
          if(x+1==regs[REG_XGUY-201] && y==regs[REG_YGUY-201] && gamevars.greengemon<7) overtile=GUY;
          else overtile=over[x+1][y];
          if((walkable[overtile] || overtile==EMPTYSPACE) && under[x+1][y]!=GRASS){
            MoveObject(x,y,x+1,y);
            if(isglass) sfx.glass=1; else sfx.magic=1;
          }
          else lastpushed[x][y]=-1;
          break;
        case DOWNARROW:
          if(x==regs[REG_XGUY-201] && y+1==regs[REG_YGUY-201] && gamevars.greengemon<7) overtile=GUY;
          else overtile=over[x][y+1];
          if((walkable[overtile] || overtile==EMPTYSPACE) && under[x][y+1]!=GRASS){
            MoveObject(x,y,x,y+1);
            if(isglass) sfx.glass=1; else sfx.magic=1;
          }
          else lastpushed[x][y]=-1;
          break;
        case LEFTARROW:
          if(x-1==regs[REG_XGUY-201] && y==regs[REG_YGUY-201] && gamevars.greengemon<7) overtile=GUY;
          else overtile=over[x-1][y];
          if((walkable[overtile] || overtile==EMPTYSPACE) && under[x-1][y]!=GRASS){
            MoveObject(x,y,x-1,y);
            if(isglass) sfx.glass=1; else sfx.magic=1;
          }
          else lastpushed[x][y]=-1;
          break;
      }
    }
  }
  arrowptr=0;
  plateptr=0;

  gamevars.totalred=0;
  gamevars.totalgreen=0;
  gamevars.totalblue=0;
  for(y=0;y<=22;y++){
    for(x=0;x<=22;x++){
      undertile=under[x][y];
      overtile=over[x][y];
      switch(undertile){
        case GLASS:
        case UPARROW:
        case RIGHTARROW:
        case DOWNARROW:
        case LEFTARROW:
          arrows[arrowptr++]=y*21+x;
          break;
        case REDTARGET:
          gamevars.totalred++;
          break;
        case GREENTARGET:
          gamevars.totalgreen++;
          break;
        case BLUETARGET:
          gamevars.totalblue++;
          break;
      }
      if(overtile==REDCRATE && undertile!=REDTARGET){
        over[x][y]=CRATE; gamevars.redcount--;
      }else{
        if(overtile==GREENCHEST && undertile!=GREENTARGET){
          over[x][y]=CHEST; gamevars.greencount--;
        }else{
          if(overtile==BLUEBARREL && undertile!=BLUETARGET){
            over[x][y]=BARREL; gamevars.bluecount--;
          }
        }
      }
      if(x==regs[REG_XGUY-201] && y==regs[REG_YGUY-201]) overtile=GUY;
      else overtile=over[x][y];
      switch(overtile){
        case EMPTYSPACE:  if(undertile==ONPLATE){
                            plates[plateptr++]=(y+21)*21+x;
                          }
                          break;
        case GUY:     if(undertile==OFFPLATE && gamevars.greengemon<7){
                        under[x][y]=ONPLATE;
                        sfx.plateon=1;
                        plates[plateptr++]=y*21+x;
                      }
                      break;
        case BARREL:  switch(undertile){
                        case EMPTYSPACE:  over[x][y]=EMPTYSPACE;  break;
                        case WATER:       over[x][y]=WATERBARREL;
                                          sfx.water=1;
                                          break;
                        case LAVADEBRIS1:
                        case LAVADEBRIS2:
                        case LAVA:        over[x][y]=EMPTYSPACE;
                                          under[x][y]=LAVADEBRIS1;
                                          sfx.lava=1;
                                          break;
                        case BLUETARGET:  over[x][y]=BLUEBARREL;
                                          gamevars.bluecount++;
                                          break;
                        case HOLE:        over[x][y]=HOLEBARREL;
                                          sfx.splinter=1;
                                          break;
                        case WOOD:        over[x][y]=HOLEBARREL;
                                          sfx.splinter=1; break;
                        case OFFPLATE:   under[x][y]=ONPLATE;
                                          plates[plateptr++]=y*21+x; break;
                      } break;
        case CRATE:   switch(undertile){
                        case EMPTYSPACE:  over[x][y]=EMPTYSPACE;  break;
                        case WATER:       over[x][y]=EMPTYSPACE;
                                          sfx.water=1;
                                          under[x][y]=WATERCRATE; break;
                        case LAVADEBRIS1:
                        case LAVADEBRIS2:
                        case LAVA:        over[x][y]=EMPTYSPACE;
                                          under[x][y]=LAVADEBRIS1;
                                          sfx.lava=1;
                                          break;
                        case REDTARGET:   over[x][y]=REDCRATE;
                                          gamevars.redcount++;
                                          break;
                        case HOLE:
                        case WOOD:        over[x][y]=EMPTYSPACE;
                                          under[x][y]=HOLE;
                                          sfx.splinter=1;
                                          break;
                        case OFFPLATE:   under[x][y]=ONPLATE;
                                          plates[plateptr++]=y*21+x; break;
                      } break;
        case CHEST:   switch(undertile){
                        case EMPTYSPACE:  over[x][y]=EMPTYSPACE;  break;
                        case GREENTARGET: over[x][y]=GREENCHEST;
                                          gamevars.greencount++;
                                          break;
                        case LAVA:        under[x][y]=GLASS;
                                          sfx.crack=1;
                        case HOLE:        over[x][y]=EMPTYSPACE;
                                          break;
                        case OFFPLATE:   under[x][y]=ONPLATE;
                                          plates[plateptr++]=y*21+x; break;
                      } break;
      }
    }
  }
  arrows[arrowptr]=0;

    /* Now check all located plates */
  while(plateptr>0){
    plateptr--;
    x=plates[plateptr];
    y=x/21;
    x%=21;
    if(y<=21){  //Call pitcode with "ON"
      sfx.plateon=1;
      RunPitCode(pitcode[x][y],x,y,1);
    }else{
      y-=21;
      RunPitCode(pitcode[x][y],x,y,0);  //"OFF"
    }
  }

  RunPitCode(pcmaster,0,0,0);
}

void PathFind(WORD xdest,WORD ydest)
{
  static WORD pathq1[400],pathq2[400];
  WORD *readq,*writeq,readpos,writepos,qsize,x,y;

  lastpathx=xdest;
  lastpathy=ydest;
  for(y=0;y<=22;y++){
    for(x=0;x<=22;x++){
      path[x][y]=-1;
    }
  }
  gamevars.autowalk=0;
  readq=pathq1;
  writeq=pathq2;
  qsize=0;
  x=ydest*23+xdest;
  path[xdest][ydest+1]=NORTH;
  pathq1[qsize++]=x+23;
  path[xdest-1][ydest]=EAST;
  pathq1[qsize++]=x-1;
  path[xdest][ydest-1]=SOUTH;
  pathq1[qsize++]=x-23;
  path[xdest+1][ydest]=WEST;
  pathq1[qsize++]=x+1;
  while(qsize>0){
    readpos=0;
    writepos=0;
    while(readpos<qsize){
      x=readq[readpos]%23;
      y=readq[readpos]/23;
      if(x>=1 && x<=21 && y>=1 && y<=21){
        if(x==regs[REG_XGUY-201] && y==regs[REG_YGUY-201]){
          readpos=qsize;
          writepos=0;
          gamevars.autowalk=1;
          path[xdest][ydest]=-1;
        }else{
          if(((walkable[over[x][y]] || over[x][y]==0) && walkable[under[x][y]])||gamevars.greengemon==7){
            if(path[x][y+1]==-1){ path[x][y+1]=NORTH; writeq[writepos++]=(y+1)*23+x;}
            if(path[x-1][y]==-1){ path[x-1][y]=EAST;  writeq[writepos++]=y*23+x-1;}
            if(path[x][y-1]==-1){ path[x][y-1]=SOUTH; writeq[writepos++]=(y-1)*23+x;}
            if(path[x+1][y]==-1){ path[x+1][y]=WEST;  writeq[writepos++]=y*23+x+1;}
          }
        }
      }
      readpos++;
    }
    qsize=writepos;
    if(readq==pathq1){
      readq=pathq2;
      writeq=pathq1;
    }else{
      readq=pathq1;
      writeq=pathq2;
    }
  }
}

void CheckAutoWalk(void)
{
  WORD facing=regs[REG_FACING-201];
  if(gamevars.autowalk){
    switch(path[regs[REG_XGUY-201]][regs[REG_YGUY-201]]){
      case NORTH:
        if(facing==SOUTH){
          regs[REG_FACING-201]=WEST;
        }else{
          regs[REG_FACING-201]=NORTH;
          MoveGuy(0,-1);
        }
        break;
      case EAST:
        if(facing==WEST){
          regs[REG_FACING-201]=NORTH;
        }else{
          regs[REG_FACING-201]=EAST;
          MoveGuy(1,0);
        }
        break;
      case SOUTH:
        if(facing==NORTH){
          regs[REG_FACING-201]=EAST;
        }else{
          regs[REG_FACING-201]=SOUTH;
          MoveGuy(0,1);
        }
        break;
      case WEST:
        if(facing==EAST){
          regs[REG_FACING-201]=SOUTH;
        }else{
          regs[REG_FACING-201]=WEST;
          MoveGuy(-1,0);
        }
        break;
      default:     gamevars.autowalk=0;
    }
  }
}

void MoveObject(WORD sx,WORD sy,WORD dx,WORD dy)
{
  WORD searchfor,i;
  over[dx][dy]=over[sx][sy];
  over[sx][sy]=EMPTYSPACE;
  CheckGameTileLogic(sx,sy);
  CheckGameTileLogic(dx,dy);
  if(sy!=dy){
    if(dy>sy) lastpushed[dx][dy]=SOUTHPUSH;
    else lastpushed[dx][dy]=NORTHPUSH;
  }else{
    if(dx>sx) lastpushed[dx][dy]=EASTPUSH;
    else lastpushed[dx][dy]=WESTPUSH;
  }
  lastpushed[sx][sy]=-1;

  /* erase destination from list of arrows & glass */
  searchfor=dy*21+dx;
  i=0;
  while(arrows[i]!=0){
    if(arrows[i]==searchfor){
      arrows[i]=1;   //outside map
      break;
    }
    i++;
  }
}

void Leaving(WORD dx,WORD dy)
{
  WORD x,y,xd,yd,undertile;

  regs[REG_LASTX-201] = regs[REG_XGUY-201];
  regs[REG_LASTY-201] = regs[REG_YGUY-201];
  if(gamevars.redgemon && pcredg==-1){
    x=regs[REG_XGUY-201];
    y=regs[REG_YGUY-201];
    undertile=under[x][y];
    if(undertile==FLOOR || (undertile>=UPARROW && undertile<=LEFTARROW)){
      if(dx>0) under[x][y]=RIGHTARROW;
      else if(dx<0) under[x][y]=LEFTARROW;
      else if(dy>0) under[x][y]=DOWNARROW;
      else under[x][y]=UPARROW;
    }
  }
  if(gamevars.bluegemon && pcblueg==-1){
    x=regs[REG_XGUY-201];
    y=regs[REG_YGUY-201];
    xd=x-dx;
    yd=y-dy;
    if(over[xd][yd]>=WATERBARREL){
      over[x][y]=over[xd][yd];
      over[xd][yd]=EMPTYSPACE;
      if(x>xd) lastpushed[x][y]=EAST;
      else if(x<xd) lastpushed[x][y]=WEST;
      else if(y>yd) lastpushed[x][y]=SOUTH;
      else if(y<yd) lastpushed[x][y]=NORTH;
      CheckGameTileLogic(xd,yd);
      CheckGameTileLogic(x,y);
    }
    gamevars.bluegemon=0;
    regs[REG_BLUEGEMS-201]--;
    UpdateItem(BLUEGEM);
    copysidebar=1;
  }
}

void MoveGuy(WORD dx,WORD dy)
{
  static WORD overtile, undertile, xloc, yloc, xdest, ydest;
  if(dx==0 && dy==0){
    gamevars.walkframe=0;
    regs[REG_LASTX-201] = regs[REG_XGUY-201];
    regs[REG_LASTY-201] = regs[REG_YGUY-201];
    CheckStuff();
    gamevars.moves++;
  }else{
    xloc=regs[REG_XGUY-201]+dx;  //Space in front of guy
    yloc=regs[REG_YGUY-201]+dy;
    xdest=xloc+dx;          //Space where guy might push barrel to
    ydest=yloc+dy;
    if(xloc>=1 && xloc<=21 && yloc>=1 && yloc<=21){
      overtile=over[xloc][yloc];
      undertile=under[xloc][yloc];
      if(walkable[overtile] || overtile==0 || gamevars.greengemon==7){
        if(walkable[undertile] || gamevars.greengemon==7){
          if(gamevars.pushing){
            letgo1=0; gamevars.pushing=0;
          }else{
            Leaving(dx,dy);
            regs[REG_XGUY-201]+=dx;
            regs[REG_YGUY-201]+=dy;
            gamevars.walkframe++;
            gamevars.walkframe%=4;
            gamevars.lasttimewalked=Timer();
            gamevars.moves++;
            CheckStuff();
          }
        }else{
          gamevars.pushing=0;
          gamevars.autowalk=0;
        }
      }else{
        if(gamevars.autowalk){
          gamevars.autowalk=0;
          if(abs(lastpathx-regs[REG_XGUY-201])+abs(lastpathy-regs[REG_YGUY-201])>1){
            PathFind(lastpathx,lastpathy);
          }
        }else{
          if(overtile>=WATERBARREL){
            if(gamevars.pushing){
              if(Timer()>=gamevars.pushing){
                if(overtile>=BARREL && overtile<=GREENCHEST){
                  overtile=over[xdest][ydest];
                  if(((walkable[overtile] || overtile==0) &&
                       under[xdest][ydest]!=GRASS) ||
                       overtile>=CHEST){
                    if(overtile>=CHEST){
                      if(overtile==GREENCHEST) gamevars.greencount--;
                      under[xdest][ydest]=GLASS;  //SFX formglass
                      sfx.crack=1;
                    }
                    sfx.scrape=1;
                    MoveObject(xloc,yloc,xdest,ydest);
                    gamevars.moves++;
                    gamevars.pushing=Timer()+PUSHDELAY;
                    if(walkable[undertile]){
                      gamevars.walkframe++;
                      gamevars.walkframe%=4;
                      gamevars.lasttimewalked=Timer();
                      Leaving(dx,dy);
                      regs[REG_XGUY-201]=xloc;
                      regs[REG_YGUY-201]=yloc;
                    }
                    CheckStuff();
                  }else{
                    if(over[xloc][yloc]>=CHEST){
                      if(over[xloc][yloc]==GREENCHEST) gamevars.greencount--;
                      over[xloc][yloc]=EMPTYSPACE;
                      under[xloc][yloc]=GLASS;  //SFX formglass
                      sfx.crack=1;
                      gamevars.moves++;
                      if(walkable[undertile]){
                        gamevars.walkframe++;
                        gamevars.walkframe%=4;
                        gamevars.lasttimewalked=Timer();
                        Leaving(dx,dy);
                        regs[REG_XGUY-201]=xloc;
                        regs[REG_YGUY-201]=yloc;
                      }
                      CheckStuff();
                    }else{
                      //sfx.groan=1;
                      //SFX groan
                    }
                  }
                }else{
                  //sfx.groan=1;
                  //SFX groan
                }
              }
            }else{
              gamevars.pushing=Timer()+PUSHDELAY;
            }
          }else{
            gamevars.pushing=0;
            switch(overtile){
              case TEXTWALL:
                RunPitCode(pitcode[xloc][yloc],xloc,yloc,1);
                break;
              case OFFSWITCH:
                over[xloc][yloc]=ONSWITCH;
                sfx.plateon=1;
                RunPitCode(pitcode[xloc][yloc],xloc,yloc,1);
                letgo1=0;
                break;
              case ONSWITCH:
                over[xloc][yloc]=OFFSWITCH;
                sfx.plateoff=1;
                RunPitCode(pitcode[xloc][yloc],xloc,yloc,0);
                letgo1=0;
                break;
            }
          }
        }
      }
    }
  }
}

void CheckGuy(void)
{
  static WORD xd,yd,deltax,deltay,newptr,newfacing,letgo2=1,onscreen,i,x,y;
  static WORD xtele,ytele,arrowkeys;
  deltax=mx/24-4;
  deltay=my/24-3;
  xd=deltax+regs[REG_XGUY-201];
  yd=deltay+regs[REG_YGUY-201];

  if(deltax!=0 || deltay!=0) mouseinput=1;

  /* handle keyboard input */
  if(keytable[UPKEY]){  //up arrow
    deltay=-1;
    deltax=0;
  }else{
    if(keytable[DNKEY]){  //down arrow
      deltay=1;
      deltax=0;
    }else{
      if(keytable[RTKEY]){  //right arrow
        deltay=0;
        deltax=1;
      }else{
        if(keytable[LFKEY]){  //left arrow
          deltay=0;
          deltax=-1;
        }else{
          if(keytable[CRKEY] || keytable[SPKEY]){  //return or space
            deltay=0;
            deltax=0;
          }
        }
      }
    }
  }
  arrowkeys=keytable[UPKEY]+keytable[DNKEY]+keytable[RTKEY]+keytable[LFKEY];

  /* adjust facing and mouse pointer */
  newfacing=regs[REG_FACING-201];
    if(deltax==0 && deltay==0){
      newptr=BUSYPOINTER;
    }else{
      if(abs(deltax)>abs(deltay)){
        if(deltax<0){
          if(newfacing==EAST && !arrowkeys && !button1) newfacing=SOUTH;
          else newfacing=WEST;
          newptr=LEFTARROWPTR;
        }else{
          if(newfacing==WEST && !arrowkeys && !button1) newfacing=NORTH;
          else newfacing=EAST;
          newptr=RIGHTARROWPTR;
        }
      }else{
        if(deltay<0){
          if(newfacing==SOUTH && !arrowkeys && !button1) newfacing=WEST;
          else newfacing=NORTH;
          newptr=UPARROWPTR;
        }else{
          if(newfacing==NORTH && !arrowkeys && !button1) newfacing=EAST;
          else newfacing=SOUTH;
          newptr=DOWNARROWPTR;
        }
      }
    }
    if(arrowkeys || keytable[CRKEY] || keytable[SPKEY] || !mouseinput){
      letgo1=1;
      mouseinput=0;
      newptr=BLANKPOINTER;
      ChangeMouse(BLANKPOINTER);
      SetMousePos(108,84);
    }
    if((mx<216 && my<168)||arrowkeys){
      ChangeMouse(newptr);
      onscreen=1;
    }else{
      mouseinput=1;
      ChangeMouse(STDPOINTER);
      onscreen=0;
    }
  if(gamevars.autowalk==0) regs[REG_FACING-201]=newfacing;

  /* Move? */
  if(deltax==0 && deltay==0 && (inkey<129 || inkey>138)){
    gamevars.pushing=0;
    if(button1==1 || keytable[CRKEY] || keytable[SPKEY]){
      gamevars.autowalk=0;
      MoveGuy(0,0);
    }else{
      CheckAutoWalk();
    }
  }else{
    if(button1==1 || (inkey>=129 && inkey<=138) || arrowkeys){
      if(onscreen && (inkey<129 || inkey>138)){
        if(letgo1){
          gamevars.autowalk=0;
          regs[REG_FACING-201]=newfacing;
          switch(regs[REG_FACING-201]){
            case NORTH:  MoveGuy( 0,-1); break;
            case EAST:   MoveGuy( 1, 0); break;
            case SOUTH:  MoveGuy( 0, 1); break;
            case WEST:   MoveGuy(-1, 0); break;
          }
        }
      }else{
        if((mx>=223 && mx<223+26 && my>=127 && my<127+26)||inkey==133){ //red key
          if(inkey==133) inkey=0xff;
          if(regs[REG_REDKEYS-201]>0){
            x=regs[REG_XGUY-201];
            y=regs[REG_YGUY-201];
            if(under[x][y]==REDLOCK){
              regs[REG_REDKEYS-201]--;
              ScreenCopy();
              UpdateItem(REDKEY);
              ScreenSwap();
              UpdateItem(REDKEY);
              under[x][y]=ONPLATE;
              RunPitCode(pitcode[x][y],x,y,1);
            }
          }
        }
        if((mx>=255 && mx<255+26 && my>=127 && my<127+26)||inkey==134){ //green key
          if(inkey==134) inkey=0xff;
          if(regs[REG_GREENKEYS-201]>0){
            x=regs[REG_XGUY-201];
            y=regs[REG_YGUY-201];
            if(under[x][y]==GREENLOCK){
              regs[REG_GREENKEYS-201]--;
              ScreenCopy();
              UpdateItem(GREENKEY);
              ScreenSwap();
              UpdateItem(GREENKEY);
              under[x][y]=ONPLATE;
              RunPitCode(pitcode[x][y],x,y,1);
            }
          }
        }
        if((mx>=287 && mx<287+26 && my>=127 && my<127+26)||inkey==135){ //blue key
          if(inkey==135) inkey=0xff;
          if(regs[REG_BLUEKEYS-201]>0){
            x=regs[REG_XGUY-201];
            y=regs[REG_YGUY-201];
            if(under[x][y]==BLUELOCK){
              regs[REG_BLUEKEYS-201]--;
              ScreenCopy();
              UpdateItem(BLUEKEY);
              ScreenSwap();
              UpdateItem(BLUEKEY);
              under[x][y]=ONPLATE;
              RunPitCode(pitcode[x][y],x,y,1);
            }
          }
        }
        if((mx>=223 && mx<223+26 && my>=89 && my<89+26 && letgo1)||inkey==129){ //red gem
          if(inkey==129) inkey=0xff;
          if(gamevars.redgemon==0){
            if(regs[REG_REDGEMS-201]>0){
              gamevars.redgemon=1;
              sfx.magic=1;
              ScreenCopy();
              UpdateItem(REDGEM);
              ScreenSwap();
              UpdateItem(REDGEM);
              RunPitCode(pcredg,0,0,1);
              letgo1=0;
            }
          }else{
            gamevars.redgemon=0;
            RunPitCode(pcredg,0,0,0);
            regs[REG_REDGEMS-201]--;
            ScreenCopy();
            UpdateItem(REDGEM);
            ScreenSwap();
            UpdateItem(REDGEM);
            letgo1=0;
          }
        }
        if((mx>=255 && mx<255+26 && my>=89 && my<89+26 && letgo1)||inkey==130){ //green gem
          if(inkey==130) inkey=0xff;
          if(gamevars.greengemon==0){
            if(regs[REG_GREENGEMS-201]>0){
              gamevars.greengemon=1;
              sfx.magic=1;
              ScreenCopy();
              UpdateItem(GREENGEM);
              ScreenSwap();
              UpdateItem(GREENGEM);
              RunPitCode(pcgreeng,0,0,1);
              if(pcgreeng==-1){
                if(regs[REG_FACING-201]!=SOUTH){
                  if(regs[REG_FACING-201]==NORTH){
                    regs[REG_FACING-201]=EAST;
                    Gdrall();
                    ScreenSwap();
                  }
                  regs[REG_FACING-201]=SOUTH;
                  Gdrall();
                  ScreenSwap();
                }
                xtele=regs[REG_XGUY-201];
                ytele=regs[REG_YGUY-201];
                for(i=2;i<=7;i++){
                  gamevars.greengemon=i;
                  Gdrall();
                  ScreenSwap();
                }
              }
              letgo1=0;
            }
          }else{
            x=regs[REG_XGUY-201];
            y=regs[REG_YGUY-201];
            if((over[x][y]==0
                || (over[xtele][ytele]==0 && (over[x][y]>=WATERBARREL || walkable[over[x][y]]))
                || pcgreeng>=0)
               && walkable[under[x][y]]){
              if(pcgreeng==-1){
                if(over[x][y]!=0 && !walkable[over[x][y]]){
                  over[xtele][ytele]=over[x][y];
                  over[x][y]=0;
                  CheckGameTileLogic(x,y);
                  CheckGameTileLogic(xtele,ytele);
                }
                regs[REG_FACING-201]=SOUTH;
                sfx.magic=1;
                for(i=6;i>=0;i--){
                  gamevars.greengemon=i;
                  if(i==0) CheckUnderGuy();
                  Gdrall();
                  ScreenSwap();
                }
                CheckStuff();
              }else{
                gamevars.greengemon=0;
              }
              RunPitCode(pcgreeng,0,0,0);
              regs[REG_GREENGEMS-201]--;
              ScreenCopy();
              UpdateItem(GREENGEM);
              ScreenSwap();
              UpdateItem(GREENGEM);
//              RunPitCode(pcmaster,0,0,1);
              letgo1=0;
            }
          }
        }
        if((mx>=287 && mx<287+26 && my>=89 && my<89+26 && letgo1)||inkey==131){ //blue gem
          if(inkey==131) inkey=0xff;
          if(gamevars.bluegemon==0){
            if(regs[REG_BLUEGEMS-201]>0){
              gamevars.bluegemon=1;
              sfx.magic=1;
              ScreenCopy();
              UpdateItem(BLUEGEM);
              ScreenSwap();
              UpdateItem(BLUEGEM);
              RunPitCode(pcblueg,0,0,1);
              letgo1=0;
            }
          }else{
            gamevars.bluegemon=0;
            RunPitCode(pcblueg,0,0,0);
            regs[REG_BLUEGEMS-201]--;
            ScreenCopy();
            UpdateItem(BLUEGEM);
            ScreenSwap();
            UpdateItem(BLUEGEM);
            letgo1=0;
          }
        }
        if((mx>=287 && mx<287+26 && my>=51 && my<51+26)||inkey==132){  //yellow gem
          if(inkey==132) inkey=0xff;
          if(regs[REG_YELLOWGEMS-201]==-1){
            gamevars.yellowgemon=1;
            ScreenCopy();
            UpdateItem(YELLOWGEM);
            ScreenSwap();
            MagicEye();
            gamevars.yellowgemon=0;
            UpdateItem(YELLOWGEM);
            FadeIn();
          }
        }
        if((mx>=223 && mx<223+26 && my>=13 && my<13+26 && letgo1)||inkey==137){  //yield
          if(inkey==137) inkey=0xff;
          ScreenCopy();
          OwnBlit();
          BlitMask(LOGIC,224,14,YIELDPRESSED);
          DisownBlit();
          ScreenSwap();
          OwnBlit();
          BlitMask(LOGIC,224,14,YIELD);
          DisownBlit();
          inkey='r';
          levelnotdone=0;
          letgo1=0;
        }
        if((mx>=287 && mx<287+26 && my>=13 && my<13+26 && letgo1)||inkey==138){  //stop
          if(inkey==138) inkey=0xff;
          ScreenCopy();
          OwnBlit();
          BlitMask(LOGIC,288,14,STOPPRESSED);
          DisownBlit();
          ScreenSwap();
          OwnBlit();
          BlitMask(LOGIC,288,14,STOP);
          DisownBlit();
          levelnotdone=0;
          levelrepeat=0;
          letgo1=0;
        }
      }
    }else{
      letgo1=1;
      gamevars.pushing=0;
      if(button2==1){
        if(onscreen){
          if(letgo2){
            letgo2=0;
            ChangeMouse(BUSYPOINTER);
            PathFind(xd,yd);
          }
        }
      }else{
        letgo2=1;
        if(Timer()-gamevars.lasttimewalked>=90) gamevars.walkframe=0;
      }
      CheckAutoWalk();
    }
  }
}

void IncrementLevel(void)
{
  WORD carry,x;
    /* Find last digit of current level */
  carry=1;
  x=strlen(gamevars.level)-1;
  while(x>=0 && carry==1){
    if(gamevars.level[x]>='0' && gamevars.level[x]<='9'){
      if(gamevars.level[x]=='9'){
        gamevars.level[x]='0';
      }else{
        gamevars.level[x]++;
        carry=0;
      }
    }else carry=0;
    x--;
  }
}

void CheckLevelDone(void)
{
  WORD x,count,total;
  static char ldone[]={"Level Complete!"};
  char pwd[120]={"Password:  "};
  count=gamevars.redcount+gamevars.greencount+gamevars.bluecount;
  total=gamevars.totalred+gamevars.totalgreen+gamevars.totalblue;
  if((count==total && total!=0) || (levelnotdone==0 && levelwon)){
    levelwon=0;
    redgems=regs[REG_REDGEMS-201];
    greengems=regs[REG_GREENGEMS-201];
    bluegems=regs[REG_BLUEGEMS-201];
    yellowgems=regs[REG_YELLOWGEMS-201];
    redkeys=regs[REG_REDKEYS-201];
    greenkeys=regs[REG_GREENKEYS-201];
    bluekeys=regs[REG_BLUEKEYS-201];
    ScreenCopy();
    levelnotdone=0;
    FilledFrame(0,0,216,168,3,6,5);
    x=TxtLength(ldone,15);
    StoneText((214-x)/2,85,ldone,15,0);
    strcat(pwd,gamevars.password);
    x=TxtLength(pwd,strlen(pwd));
    StoneText((214-x)/2,161,pwd,strlen(pwd),0);
    ScreenSwap();
    IncrementLevel();

    while(button1==1 || keytable[CRKEY] || keytable[SPKEY]) CheckMouse();
    while(button1==0 && !keytable[CRKEY] && !keytable[SPKEY]) CheckMouse();
    while(button1==1 || keytable[CRKEY] || keytable[SPKEY]) CheckMouse();
  }
}

void GameMainLoop(void)
{
  juststartedgame=1;
  gamevars.redgemon=0;
  gamevars.greengemon=0;
  gamevars.bluegemon=0;
  redgems=0; greengems=0; bluegems=0; yellowgems=0;
  redkeys=0; greenkeys=0; bluekeys=0;
  CheckMouse();
  SetupGame();
  levelrepeat=1;
  levelwon=0;
  firsttime=1;
  while(levelrepeat){
    regs[REG_REDGEMS-201]=redgems;
    regs[REG_GREENGEMS-201]=greengems;
    regs[REG_BLUEGEMS-201]=bluegems;
    regs[REG_YELLOWGEMS-201]=yellowgems;
    regs[REG_REDKEYS-201]=redkeys;
    regs[REG_GREENKEYS-201]=greenkeys;
    regs[REG_BLUEKEYS-201]=bluekeys;
    levelnotdone=1;
    if(GameLoadLevel()){
      if(firsttime){
        firsttime=0;
        RunPitCode(pcintro,0,0,1);
        juststartedgame=0;
      }
      RunPitCode(pcmaster,0,0,1);
      Gdrall();
      FadeIn();
      CheckUnderGuy();
      while(levelnotdone || levelwon){
        CheckGuy();
        if(!levelwon){
          Gdrall();
          ScreenSwap();
        }
        if(copysidebar){ScreenCopySidebar(); copysidebar=0;}
        CheckMouse();
        CheckLevelDone();
        while(inkey=='p'){  //Pause
          CheckMouse();
        }
        if(inkey=='q'){    //quit
          levelnotdone=0;
          levelrepeat=0;
        }
        firsttime=1;  //kinda kludgy - sets intro for next level
        if(inkey=='r'){    //restart
          gamevars.redgemon=0;
          gamevars.greengemon=0;
          gamevars.bluegemon=0;
          levelnotdone=0;
          inkey=0xff;
          if(gamevars.moves!=0) firsttime=0;
        }
      }
    }else{
      levelrepeat=0;
    }
  }
}
