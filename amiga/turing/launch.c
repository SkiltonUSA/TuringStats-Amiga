/* Workbench entry point: skip missing/offline USB, detach the monitor. */
#include <exec/execbase.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/poseidon.h>
#include <string.h>
#include "version.h"
static const char version_tag[] __attribute__((used))="\0$VER: TuringStatsLaunch 0.1 (23.10.2026) 0.1.0 Beta 1";
extern struct ExecBase *SysBase;
struct Library *PsdBase;
#ifndef TURING_USB_SERIAL
#define TURING_USB_SERIAL "USB35INCHIPSV2"
#endif
int main(void) {
 struct Process *process=(struct Process*)FindTask(NULL);
 APTR oldwindow=process->pr_WindowPtr,device;
 STRPTR serial=NULL;BPTR executable,input,output;
 int loaded,matched=0,running;
 /* No requesters, USB control transfers, retries, or stack autostart. */
 process->pr_WindowPtr=(APTR)-1;
 Forbid();running=FindPort("TuringStats.1")!=NULL;
 loaded=FindName(&SysBase->LibList,"poseidon.library")!=NULL;Permit();
 if(running||!loaded)goto done;
 PsdBase=OpenLibrary("poseidon.library",4);if(!PsdBase)goto done;
 psdLockReadPBase();
 device=psdFindDevice(NULL,DA_VendorID,0x1a86,DA_ProductID,0x5722,DA_Binding,FALSE,TAG_END);
 if(device){psdGetAttrs(PGA_DEVICE,device,DA_SerialNumber,&serial,TAG_END);
  matched=serial&&!strcmp((char*)serial,TURING_USB_SERIAL);}
 psdUnlockPBase();CloseLibrary(PsdBase);PsdBase=NULL;
 if(!matched)goto done;
 executable=Lock("SYS:Utilities/TuringStats/TuringStats",ACCESS_READ);
 if(!executable)goto done;
 UnLock(executable);
 input=Open("NIL:",MODE_OLDFILE);output=Open("NIL:",MODE_NEWFILE);
 if(input&&output){
  /* On success DOS owns both handles. Never wait for the monitor to exit. */
  if(SystemTags((CONST_STRPTR)"Stack 65536\nSYS:Utilities/TuringStats/TuringStats",
     SYS_Asynch,TRUE,SYS_Input,input,SYS_Output,output,TAG_DONE)!=-1)
   input=output=0;
 }
 if(input)Close(input);
 if(output)Close(output);
 done:process->pr_WindowPtr=oldwindow;return 0;
}
