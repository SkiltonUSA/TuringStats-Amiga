/* Apply artwork and a safe stack minimum, preserving other launch data. */
#include <proto/exec.h>
#include <proto/icon.h>
#include <workbench/workbench.h>
#include <workbench/icon.h>
#include <intuition/intuition.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "icon-art.h"
struct Library *IconBase;
static struct Image art;
static int same(STRPTR a,STRPTR b){return (!a&&!b)||(a&&b&&!strcmp((char*)a,(char*)b));}
int main(int argc,char **argv){
 struct DiskObject *d,*n,*r,saved;int ok=0,existing;unsigned i;
 if(argc<2||argc>3)return 20;
 IconBase=OpenLibrary("icon.library",44);if(!IconBase)return 20;
 d=GetDiskObject(argv[1]);existing=d!=NULL;
 if(!d)d=GetDefDiskObject(argc>2?WBPROJECT:WBTOOL);
 if(d){
  if(!existing){d->do_StackSize=65536;d->do_CurrentX=d->do_CurrentY=NO_ICON_POSITION;}
  if(d->do_StackSize<65536)d->do_StackSize=65536;
  /* A new DiskObject avoids carrying an old ColorIcon image over the art. */
  n=NewDiskObject(d->do_Type);
  if(n){
   saved=*n;
   n->do_DefaultTool=!existing&&argc>2?(STRPTR)"C:IconX":d->do_DefaultTool;
   n->do_DrawerData=d->do_DrawerData;
   n->do_ToolTypes=d->do_ToolTypes;n->do_ToolWindow=d->do_ToolWindow;
   n->do_StackSize=d->do_StackSize;n->do_CurrentX=d->do_CurrentX;n->do_CurrentY=d->do_CurrentY;
   memset(&art,0,sizeof(art));art.Width=64;art.Height=64;art.Depth=2;
   art.ImageData=icon_planes;art.PlanePick=3;
   n->do_Gadget.GadgetRender=&art;n->do_Gadget.SelectRender=NULL;
   n->do_Gadget.Width=n->do_Gadget.Height=64;
   n->do_Gadget.Flags=GFLG_GADGIMAGE|GFLG_GADGHCOMP;
   #ifdef ICON_COLOR
   ok=IconControl(n,ICONCTRLA_SetWidth,64,ICONCTRLA_SetHeight,64,
     ICONCTRLA_SetPaletteSize1,128,ICONCTRLA_SetPalette1,icon_palette,
     ICONCTRLA_SetTransparentColor1,0,ICONCTRLA_SetImageData1,icon_pixels,
     ICONCTRLA_SetImageData2,NULL,ICONCTRLA_SetFrameless,TRUE,TAG_DONE)==8;
   if(ok)ok=PutIconTags((CONST_STRPTR)argv[1],n,ICONPUTA_NotifyWorkbench,TRUE,TAG_DONE);
#else
   ok=PutDiskObject(argv[1],n);
#endif
   if(ok){
    r=GetDiskObject(argv[1]);
    ok=r&&r->do_Type==n->do_Type&&r->do_StackSize==n->do_StackSize&&
       r->do_CurrentX==n->do_CurrentX&&r->do_CurrentY==n->do_CurrentY&&
       same(r->do_DefaultTool,n->do_DefaultTool)&&same(r->do_ToolWindow,n->do_ToolWindow);
    #ifdef ICON_COLOR
    if(ok){LONG mapped=0,width=0,height=0,count=0,transparent=-1;
     UBYTE*pixels=NULL;struct ColorRegister *palette=NULL;
     IconControl(r,ICONCTRLA_IsPaletteMapped,&mapped,ICONCTRLA_GetWidth,&width,
       ICONCTRLA_GetHeight,&height,ICONCTRLA_GetImageData1,&pixels,
       ICONCTRLA_GetPalette1,&palette,ICONCTRLA_GetPaletteSize1,&count,
       ICONCTRLA_GetTransparentColor1,&transparent,TAG_DONE);
     ok=mapped&&width==64&&height==64&&pixels&&palette;
     /* icon.library may compact/reorder the palette on disk. Compare actual
        colours and transparency, rather than requiring identical indices. */
     for(i=0;ok&&i<4096;i++){
      unsigned actual=pixels[i],expected=icon_pixels[i];
      if(!expected)ok=actual==(unsigned)transparent;
      else ok=actual<(unsigned)count&&actual!=(unsigned)transparent&&
        palette[actual].red==icon_palette[expected].red&&
        palette[actual].green==icon_palette[expected].green&&
        palette[actual].blue==icon_palette[expected].blue;
     }
    }

#endif
    if(ok)for(i=0;n->do_ToolTypes&&n->do_ToolTypes[i];i++)
     if(!r->do_ToolTypes||!same(r->do_ToolTypes[i],n->do_ToolTypes[i])){ok=0;break;}
    if(r){printf("TuringStats icon 64x64 type=%u stack=%ld verified=%d\n",r->do_Type,(long)r->do_StackSize,ok);FreeDiskObject(r);}
   }
   *n=saved;FreeDiskObject(n);
  }
  FreeDiskObject(d);
 }
 CloseLibrary(IconBase);return ok?0:20;
}
