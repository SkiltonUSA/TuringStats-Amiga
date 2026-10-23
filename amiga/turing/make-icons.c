#include <proto/exec.h>
#include <proto/icon.h>
#include <workbench/workbench.h>
#include <stdio.h>
struct Library *IconBase;
static int icon(char*name,int type){struct DiskObject*d=GetDefDiskObject(type);int ok;STRPTR old;STRPTR *oldtypes;STRPTR boottypes[]={(STRPTR)"DONOTWAIT",NULL};if(!d)return 0;d->do_CurrentX=d->do_CurrentY=NO_ICON_POSITION;d->do_StackSize=65536;old=d->do_DefaultTool;if(type==WBPROJECT)d->do_DefaultTool=(STRPTR)"C:IconX";oldtypes=d->do_ToolTypes;if(type==WBTOOL)d->do_ToolTypes=boottypes;ok=PutDiskObject(name,d);d->do_ToolTypes=oldtypes;d->do_DefaultTool=old;FreeDiskObject(d);return ok;}
int main(void){int ok;IconBase=OpenLibrary("icon.library",37);if(!IconBase)return 20;ok=icon("SYS:Utilities/TuringStats",WBDRAWER)&&icon("SYS:Utilities/TuringStats/Start",WBPROJECT)&&icon("SYS:Utilities/TuringStats/Stop",WBPROJECT)&&icon("SYS:TuringStats",WBTOOL)&&icon("SYS:WBStartup/TuringStats",WBTOOL);CloseLibrary(IconBase);puts(ok?"TuringStats icons created":"Icon creation failed");return ok?0:20;}
