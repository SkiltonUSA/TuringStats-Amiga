/* Native dashboard: OS font rasterization, RGB565 little-endian USB image. */
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <intuition/screens.h>
#include <proto/expansion.h>
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <libraries/configvars.h>
#include <exec/memory.h>
#include <stdlib.h>
#include "telemetry.h"
struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct ExpansionBase *ExpansionBase;
static struct BitMap textmap;
static struct RastPort textport;
static struct TextFont *font;
static UBYTE *frame;
static volatile UBYTE *regs;
static int log_pages;
#define WIDTH 320
#define HEIGHT 480
#define BG 0x0863
#define WHITE 0xffff
#define BLUE 0x4d9f
#define ORANGE 0xfd20
#define PAGE_SECONDS 10
static UWORD heading_colour=BLUE, value_colour=WHITE;
static unsigned page_number;
static int capture_pending;
static void pixel(unsigned x,unsigned y,UWORD c){unsigned at=(y*WIDTH+x)*2;frame[at]=c;frame[at+1]=c>>8;}
static void fill(unsigned x,unsigned y,unsigned w,unsigned h,UWORD c){unsigned a,b;for(b=y;b<y+h;b++)for(a=x;a<x+w;a++)pixel(a,b,c);}
static void label(unsigned y,const char*s,UWORD colour){unsigned x,v;size_t len=strlen(s);if(len>18)len=18;
 SetRast(&textport,0);SetAPen(&textport,1);SetDrMd(&textport,JAM1);Move(&textport,0,font->tf_Baseline);Text(&textport,(STRPTR)s,len);WaitBlit();
 for(v=0;v<8;v++)for(x=0;x<144;x++)if(textmap.Planes[0][v*textmap.BytesPerRow+x/8]&(0x80>>(x&7)))fill(16+x*2,y+v*2,2,2,colour);
}
static int region(APTR output,unsigned y,unsigned h){UBYTE cmd[6];ULONG at=y*WIDTH*2,remaining=h*WIDTH*2;
 command(cmd,197,0,y,WIDTH-1,y+h-1);if(!transfer(output,cmd,6))return 0;
 while(remaining){ULONG n=remaining>2560?2560:remaining;if(!transfer(output,frame+at,n)){printf("Region y=%u h=%u failed at byte %lu\n",y,h,(unsigned long)(at-y*WIDTH*2));return 0;}at+=n;remaining-=n;}return 1;
}
static void dashboard_close(void){telemetry_close();if(IntuitionBase)CloseLibrary((struct Library*)IntuitionBase);if(font)CloseFont(font);if(textmap.Planes[0])FreeRaster(textmap.Planes[0],144,8);if(frame)FreeVec(frame);if(ExpansionBase)CloseLibrary((struct Library*)ExpansionBase);if(GfxBase)CloseLibrary((struct Library*)GfxBase);}
static int dashboard_open(void){struct ConfigDev*card;struct TextAttr attr={(STRPTR)"topaz.font",8,0,0};
 GfxBase=(void*)OpenLibrary("graphics.library",37);if(!GfxBase)return 0;
 font=OpenFont(&attr);if(!font||font->tf_XSize!=8||font->tf_YSize!=8)return 0;
 InitBitMap(&textmap,1,144,8);textmap.Planes[0]=AllocRaster(144,8);if(!textmap.Planes[0])return 0;
 InitRastPort(&textport);textport.BitMap=&textmap;SetFont(&textport,font);
 frame=AllocVec(WIDTH*HEIGHT*2,MEMF_PUBLIC);if(!frame)return 0;
 ExpansionBase=(void*)OpenLibrary("expansion.library",37);
 if(ExpansionBase){card=FindConfigDev(NULL,0x6d6e,4);if(!card)card=FindConfigDev(NULL,0x6d6e,3);if(card&&card->cd_BoardSize>0xe1)regs=(volatile UBYTE*)card->cd_BoardAddr;}
 IntuitionBase=(void*)OpenLibrary("intuition.library",37);
 clock_read();cpu_start();return 1;
}
static void page_header(const char *title) {
 unsigned alternate=page_number++&1;
 heading_colour=alternate?ORANGE:BLUE;
 value_colour=alternate?BLUE:WHITE;
 if(log_pages)printf("Page %u: %s; %s\n",page_number,title,
                     alternate?"orange/blue":"blue/white");
 fill(0,0,WIDTH,HEIGHT,BG);fill(0,0,WIDTH,4,heading_colour);
 label(22,"A4000TX",value_colour);label(50,title,heading_colour);fill(16,80,288,1,0x39e7);
 label(446,TURING_VERSION_LABEL,heading_colour);capture_pending=1;
}
static void system_page(void) {
 page_header("SYSTEM MONITOR");
 label(94,"ZZ9000 TEMP",heading_colour);label(160,"CHIP RAM FREE",heading_colour);
 label(226,"FAST RAM FREE",heading_colour);label(292,"FAST RAM USED",heading_colour);
 fill(16,354,288,1,0x39e7);label(372,"68K CPU LOAD",heading_colour);
 label(398,"MEASURING",value_colour);
}
static void processor_page(void) {
 char text[48];page_header("PROCESSORS");
 label(94,(SysBase->AttnFlags&AFF_68060)?"68060 CLOCK":"68K CLOCK",heading_colour);
 if(cpu_mhz)snprintf(text,sizeof(text),"%lu MHz (EST)",(unsigned long)cpu_mhz);else strcpy(text,"UNAVAILABLE");
 label(120,text,value_colour);label(160,"68K CPU LOAD",heading_colour);
 label(226,"ZZ9000 DUAL A9",heading_colour);label(252,"666 MHz NOMINAL",value_colour);
 label(292,"ARM CORE 0 / 1",heading_colour);label(318,"LOAD UNAVAILABLE",value_colour);
 label(398,"NO ARM TELEMETRY",heading_colour);
}
static void pair(unsigned y,const char *title,const char *text) {
 label(y,title,heading_colour);label(y+24,text,value_colour);
}
static void os_page(void) {
 struct Library *lib;struct Screen *screen;char text[48];
 page_header("OS / WORKBENCH");
 if(rom_version)snprintf(text,sizeof(text),"%lu.%lu",rom_version&65535,rom_version>>16);
 else strcpy(text,"UNAVAILABLE");
 pair(94,"KICKSTART ROM",text);
 if(log_pages)printf("Kickstart ROM: %s\n",text);
 snprintf(text,sizeof(text),"%u.%u",SysBase->LibNode.lib_Version,SysBase->SoftVer);
 pair(160,"ACTIVE KICKSTART",text);
 lib=OpenLibrary("version.library",0);
 if(lib){snprintf(text,sizeof(text),"%u.%u",lib->lib_Version,lib->lib_Revision);CloseLibrary(lib);}
 else strcpy(text,"UNAVAILABLE");
 pair(226,"WORKBENCH",text);
 if(log_pages)printf("Workbench: %s\n",text);
 lib=OpenLibrary("workbench.library",0);
 if(lib){snprintf(text,sizeof(text),"%u.%u",lib->lib_Version,lib->lib_Revision);CloseLibrary(lib);}
 else strcpy(text,"UNAVAILABLE");
 pair(292,"WORKBENCH.LIBRARY",text);
 if(log_pages)printf("Workbench library: %s\n",text);
 strcpy(text,"UNAVAILABLE");
 screen=IntuitionBase?LockPubScreen((CONST_STRPTR)"Workbench"):NULL;
 if(screen){
  unsigned width=screen->Width,height=screen->Height;
  ULONG depth=GetBitMapAttr(screen->RastPort.BitMap,BMA_DEPTH);
  UnlockPubScreen(NULL,screen);
  snprintf(text,sizeof(text),"%ux%u %lu BIT",width,height,depth);
 }
 pair(358,"WORKBENCH SCREEN",text);
 if(log_pages)printf("Workbench screen: %s\n",text);
}
static void hardware_page(unsigned index) {
 page_header(index?"EXPANSION / MEDIA":"HARDWARE PROFILE");
 if(!index){
  pair(94,"AMIGA A4000TX","AGA CHIPSET");
  pair(160,"TERRIBLE FIRE","TF4060 / 68060");
  pair(226,"KICKSMASH32","FW 1.8 RECORDED");
  pair(292,"ZZ9000","DUAL A9 / 1GB DDR3");
  pair(358,"ZZ9000AX","AUDIO EXPANSION");
 } else {
  pair(94,"FREEWAY TRITON","LITE / USB");
  pair(160,"X-SURF 100","LAN / ROADSHOW");
  pair(226,"BUDDHA IDE","64GB COMPACTFLASH");
  pair(292,"SD STORAGE","32GB / SD-TO-CF");
  pair(358,"TURING USB LCD","3.5 INCH 320x480");
 }
 label(420,"RECORDED HARDWARE",heading_colour);
}
static void totals_page(void) {
 char text[48],total_text[24],used_text[24],free_text[24];
 ULONG chip=AvailMem(MEMF_CHIP|MEMF_TOTAL),fast=AvailMem(MEMF_FAST|MEMF_TOTAL);
 unsigned long long total,used;
 volumes_read();disk_totals(&total,&used);
 page_header("MEMORY / STORAGE");
 snprintf(text,sizeof(text),"%lu KiB",chip/1024);pair(94,"CHIP TOTAL (EXEC)",text);
 storage_bytes(text,fast);pair(160,"FAST TOTAL (EXEC)",text);
 storage_bytes(total_text,total);storage_bytes(used_text,used);storage_bytes(free_text,total-used);
 pair(226,"DISK VOLUMES TOTAL",total_text);
 pair(292,"DISK VOLUMES USED",used_text);
 pair(358,"DISK VOLUMES FREE",free_text);
 label(420,"EXCLUDES RAM DISK",heading_colour);
 if(log_pages)printf("Totals: chip=%lu bytes fast=%lu bytes disk=%s used=%s free=%s (RAM excluded)\n",
                     chip,fast,total_text,used_text,free_text);
}
/* Three mounted volumes per page; additional volumes receive more pages. */
static struct volume *disk_page(struct volume *v) {
 char name[48],ip[48],text[48],capacity[24],used[24],free_space[24];unsigned i;
 page_header("DRIVES / NETWORK");network_read(name,ip);
 label(94,name,heading_colour);label(120,ip,value_colour);
 if(log_pages)printf("Network: %s %s\n",name,ip);
 if(!v)label(174,"NO MOUNTED VOLUMES",value_colour);
 for(i=0;v&&i<3;i++,v=v->next){unsigned y=162+i*92;
  label(y,v->path,heading_colour);
  if(!v->valid){label(y+24,"UNAVAILABLE",value_colour);continue;}
  storage(capacity,v->blocks,v->blocksize);storage(used,v->used,v->blocksize);
  storage(free_space,v->blocks>=v->used?v->blocks-v->used:0,v->blocksize);
  snprintf(text,sizeof(text),"TOTAL %s",capacity);label(y+24,text,value_colour);
  snprintf(text,sizeof(text),"USED  %s",used);label(y+44,text,value_colour);
  snprintf(text,sizeof(text),"FREE  %s",free_space);label(y+64,text,value_colour);
  if(log_pages)printf("Volume %s total=%s used=%s free=%s\n",v->path,capacity,used,free_space);
 }
 return v;
}
static int value(APTR output,unsigned y,const char*s,char*previous){if(!strcmp(s,previous))return 1;fill(0,y,WIDTH,20,BG);label(y,s,value_colour);if(!region(output,y,20))return 0;strcpy(previous,s);return 1;}
/* Capture the exact RGB565 buffer already rendered on the physical target.
   It is not an LCD photograph and does not read back the panel's memory. */
static int capture_frame(void) {
 char path[128];BPTR file;LONG written;
 snprintf(path,sizeof(path),"%s/page-%02u.rgb565",TURING_CAPTURE_DIR,page_number);
 file=Open(path,MODE_NEWFILE);if(!file)return 0;
 written=Write(file,frame,WIDTH*HEIGHT*2);Close(file);
 if(written!=WIDTH*HEIGHT*2)return 0;
 printf("Captured %s (%u bytes)\n",path,WIDTH*HEIGHT*2);return 1;
}
static int dashboard(APTR output,unsigned seconds){
 char text[48],prev[5][48]={{0}};struct DateStamp start,now;
 LONG elapsed=0,page_started=0;unsigned t,page=0;int ok=0,load;
 struct volume *next_volume=NULL;
 log_pages=seconds!=0||capture_pages;
 if(capture_pages){BPTR dir=CreateDir(TURING_CAPTURE_DIR);if(!dir)dir=Lock(TURING_CAPTURE_DIR,ACCESS_READ);if(!dir)return 0;UnLock(dir);}
 if(!dashboard_open())goto done;
 page_number=0;system_page();if(!region(output,0,HEIGHT))goto done;DateStamp(&start);
 printf("CPU clock estimate: %lu MHz; ZZ9000 A9: 666 MHz nominal; ARM load unavailable\n",(unsigned long)cpu_mhz);
 do {
  load=cpu_percent();
  if(log_pages)printf("68K sampled load: %d%%; page=%u\n",load,page);
  if(page==0){
   ULONG chip=AvailMem(MEMF_CHIP),fast=AvailMem(MEMF_FAST),total=AvailMem(MEMF_FAST|MEMF_TOTAL),used=total>=fast?total-fast:0;
   t=regs?*(volatile UWORD*)(regs+0xe0):0;
   if(t&&t<=1500)snprintf(text,sizeof(text),"%u.%u C",t/10,t%10);else strcpy(text,"UNAVAILABLE");
   if(!value(output,120,text,prev[0]))goto done;
   snprintf(text,sizeof(text),"%lu KB",(unsigned long)(chip/1024));if(!value(output,186,text,prev[1]))goto done;
   snprintf(text,sizeof(text),"%lu.%lu MB",(unsigned long)(fast/1048576),(unsigned long)((fast%1048576)*10/1048576));if(!value(output,252,text,prev[2]))goto done;
   snprintf(text,sizeof(text),"%lu.%lu MB",(unsigned long)(used/1048576),(unsigned long)((used%1048576)*10/1048576));if(!value(output,318,text,prev[3]))goto done;
  }
  if(page==0||page==2){
   if(load<0)strcpy(text,"MEASURING");else snprintf(text,sizeof(text),"%d%%",load);
   if(!value(output,page==0?398:186,text,prev[4]))goto done;
  }
  if(capture_pages&&capture_pending){if(!capture_frame())goto done;capture_pending=0;}
  if(SetSignal(0,0)&SIGBREAKF_CTRL_C)break;
  Delay(50);DateStamp(&now);
  elapsed=(now.ds_Days-start.ds_Days)*86400+(now.ds_Minute-start.ds_Minute)*60+(now.ds_Tick-start.ds_Tick)/50;
  if(elapsed-page_started>=PAGE_SECONDS){
   if(page==0){volumes_read();next_volume=disk_page(volumes);page=1;}
   else if(page==1&&next_volume)next_volume=disk_page(next_volume);
   else if(page==1){processor_page();page=2;}
   else if(page==2){os_page();page=3;}
   else if(page==3){hardware_page(0);page=4;}
   else if(page==4){hardware_page(1);page=5;}
   else if(page==5){totals_page();page=6;}
   else {if(capture_pages)break;system_page();page=0;}
   memset(prev,0,sizeof(prev));if(!region(output,0,HEIGHT))goto done;
   DateStamp(&now);page_started=(now.ds_Days-start.ds_Days)*86400+(now.ds_Minute-start.ds_Minute)*60+(now.ds_Tick-start.ds_Tick)/50;
  }
 }while(binding&&(!seconds||elapsed<(LONG)seconds));
 if(binding){fill(0,446,WIDTH,20,BG);label(446,"STOPPED",heading_colour);if(!region(output,446,20))goto done;}
 ok=1;
 done:dashboard_close();return ok;
}
