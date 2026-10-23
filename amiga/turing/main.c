/* GPL-3.0-or-later. Turing Rev-A command encoding adapted from
 * mathoudebine/turing-smart-screen-python, copyright Matthieu Houdebine.
 * Native Poseidon transport and diagnostic pattern. No driver replacement. */
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/poseidon.h>
#include <clib/alib_protos.h>
#include <utility/hooks.h>
#include <stdio.h>
#include <string.h>
#include "version.h"
static const char version_tag[] __attribute__((used))=TURING_VER_TAG;
static int capture_pages;
struct Library *PsdBase;
static APTR volatile binding;
static struct MsgPort *port;
static ULONG released(struct Hook *h,APTR obj,APTR data){(void)h;(void)obj;(void)data;binding=NULL;return 0;}
static struct Hook hook={{NULL,NULL},(ULONG(*)())HookEntry,(ULONG(*)())released,NULL};
static int transfer(APTR pipe,UBYTE *data,ULONG length){
 unsigned ticks=0;LONG err;ULONG actual;
 if(!binding)return 0;
 SetSignal(0,1UL<<port->mp_SigBit);psdSendPipe(pipe,data,length);
 while(!(SetSignal(0,0)&(1UL<<port->mp_SigBit))){
  if(!binding||ticks++>=6000){
   psdAbortPipe(pipe);psdWaitPipe(pipe);puts("USB cancelled/timed out");return 0;
  }psdDelayMS(1);
 }
 err=psdWaitPipe(pipe);actual=psdGetPipeActual(pipe);
 if(err||actual!=length){printf("USB error=%ld actual=%lu expected=%lu\n",(long)err,(unsigned long)actual,(unsigned long)length);return 0;}
 return 1;
}
static void command(UBYTE *p,unsigned cmd,unsigned x,unsigned y,unsigned ex,unsigned ey){p[0]=x>>2;p[1]=(x&3)<<6|y>>4;p[2]=(y&15)<<4|ex>>6;p[3]=(ex&63)<<2|ey>>8;p[4]=ey;p[5]=cmd;}
#include "dashboard.h"
#define OWNER "TuringStats.1"
int main(int argc,char**argv){
 APTR device,iface,ep,control=NULL,output=NULL;STRPTR serial=NULL;int rc=20;unsigned i;
 UBYTE orient[16]={0},cmd[6],line[128],baud[]={0,0xc2,1,0,0,0,8};
 static const UWORD colors[]={0xf800,0x07e0,0x001f,0xffff};
 struct MsgPort *owner=NULL,*existing;unsigned seconds=0;
 int probe=argc==2&&!strcmp(argv[1],"PROBE");
 int paint=argc==2&&!strcmp(argv[1],"PATTERN");
 int stop=argc==2&&!strcmp(argv[1],"STOP");
 capture_pages=argc==2&&!strcmp(argv[1],"CAPTURE");
 if(argc>2){puts("TuringStats [PROBE|PATTERN|STOP|CAPTURE|seconds]");return 20;}
 if(argc==2&&!probe&&!paint&&!stop&&!capture_pages){char*end;unsigned long n=strtoul(argv[1],&end,10);if(*end||!n||n>3600)return 20;seconds=n;}
 Forbid();existing=FindPort(OWNER);if(existing&&stop)Signal(existing->mp_SigTask,SIGBREAKF_CTRL_C);Permit();
 if(existing||stop){puts(existing?"TuringStats already running / stop requested":"TuringStats not running");return 0;}
 owner=CreateMsgPort();if(!owner)return 20;owner->mp_Node.ln_Name=OWNER;AddPort(owner);
 PsdBase=OpenLibrary("poseidon.library",4);if(!PsdBase)goto done;
 port=CreateMsgPort();if(!port)goto done;
 psdLockReadPBase();
 device=psdFindDevice(NULL,DA_VendorID,0x1a86,DA_ProductID,0x5722,DA_Binding,FALSE,TAG_END);
 if(device)psdGetAttrs(PGA_DEVICE,device,DA_SerialNumber,&serial,TAG_END);
 if(device&&serial&&!strcmp((char*)serial,"USB35INCHIPSV2")){
  binding=psdClaimAppBinding(ABA_Device,device,ABA_ReleaseHook,&hook,TAG_END);
  if(binding){
   iface=psdFindInterface(device,NULL,IFA_InterfaceNum,1,IFA_Class,10,TAG_END);
   ep=iface?psdFindEndpoint(iface,NULL,EA_IsIn,FALSE,EA_EndpointNum,2,EA_TransferType,2,TAG_END):NULL;
   if(ep){control=psdAllocPipe(device,port,NULL);output=psdAllocPipe(device,port,ep);}
  }
 }
 psdUnlockPBase();
 if(!binding||!control||!output){puts("Matching unbound Turing display/endpoints unavailable");goto done;}
 puts("Turing 1a86:5722 USB35INCHIPSV2: interface1 bulk OUT2 claimed");
 psdSetAttrs(PGA_PIPE,control,PPA_NakTimeout,TRUE,PPA_NakTimeoutTime,5000,TAG_END);
 psdSetAttrs(PGA_PIPE,output,PPA_NakTimeout,TRUE,PPA_NakTimeoutTime,5000,TAG_END);
 if(probe){rc=0;goto done;}
 psdPipeSetup(control,0x21,0x20,0,0);if(!transfer(control,baud,7))goto done;
 psdPipeSetup(control,0x21,0x22,3,0);if(!transfer(control,NULL,0))goto done;
 orient[5]=121;orient[6]=100;orient[7]=1;orient[8]=64;orient[9]=1;orient[10]=224;
 if(!transfer(output,orient,16))goto done;
 command(cmd,109,0,0,0,0);if(!transfer(output,cmd,6))goto done;
 command(cmd,110,128,0,0,0);if(!transfer(output,cmd,6))goto done;
 if(!paint){rc=dashboard(output,seconds)?0:20;goto done;}
 command(cmd,197,16,16,79,79);if(!transfer(output,cmd,6))goto done;
 for(i=0;i<64;i++){unsigned x;for(x=0;x<64;x++){UWORD c=colors[x/16];line[x*2]=c;line[x*2+1]=c>>8;}if(!transfer(output,line,128))goto done;}
 puts("8192 RGB565 bytes sent: red/green/blue/white square at top-left. Visual confirmation needed.");rc=0;
 done:
 if(PsdBase){psdLockReadPBase();if(binding)psdReleaseAppBinding(binding);if(output)psdFreePipe(output);if(control)psdFreePipe(control);psdUnlockPBase();}
 if(owner){RemPort(owner);DeleteMsgPort(owner);}
 if(port)DeleteMsgPort(port);
 if(PsdBase)CloseLibrary(PsdBase);
 puts("USB binding and pipes released");return rc;
}
