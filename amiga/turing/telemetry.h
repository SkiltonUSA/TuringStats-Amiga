/* Passive 68K sampling and read-only DOS/Roadshow inventory. */
#include <exec/execbase.h>
#include <exec/interrupts.h>
#include <hardware/intbits.h>
#include <dos/dosextens.h>
#include <proto/bsdsocket.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
extern struct ExecBase *SysBase;
struct Library *SocketBase;
struct cpu_samples { volatile ULONG busy, total; };
static struct cpu_samples samples;
static struct Interrupt cpu_interrupt;
static int cpu_installed;
/* Called by a small register-preserving VBlank wrapper: no library calls. */
void turing_cpu_sample(struct cpu_samples *s) {
 struct Task *task=SysBase->ThisTask;
 if(task && task->tc_State==TS_RUN) s->busy++;
 s->total++;
}
extern void turing_cpu_entry(void);
__asm__(".text\n.globl _turing_cpu_entry\n_turing_cpu_entry:\n"
 "movem.l %d1-%d7/%a0-%a6,-(%sp)\nmove.l %a1,-(%sp)\n"
 "jsr _turing_cpu_sample\naddq.l #4,%sp\n"
 "movem.l (%sp)+,%d1-%d7/%a0-%a6\nmoveq #0,%d0\nrts\n");
static void cpu_start(void) {
 cpu_interrupt.is_Node.ln_Type=NT_INTERRUPT;
 cpu_interrupt.is_Node.ln_Pri=120;
 cpu_interrupt.is_Node.ln_Name="Turing CPU samples";
 cpu_interrupt.is_Data=&samples;
 cpu_interrupt.is_Code=(void*)turing_cpu_entry;
 AddIntServer(INTB_VERTB,&cpu_interrupt);cpu_installed=1;
}
static int cpu_percent(void) {
 static ULONG last_busy,last_total;
 ULONG busy,total,db,dt;
 Disable();busy=samples.busy;total=samples.total;Enable();
 db=busy-last_busy;dt=total-last_total;last_busy=busy;last_total=total;
 if(!dt)return -1;
 return (int)((db*100+dt/2)/dt);
}
struct volume { struct volume *next; char path[258]; ULONG blocks,used,blocksize; int valid,is_ram; };
static struct volume *volumes;
static void volumes_free(void) {
 while(volumes){struct volume *next=volumes->next;FreeVec(volumes);volumes=next;}
}
static void volumes_read(void) {
 struct DosList *entry; struct volume **tail=&volumes,*v;
 struct Process *process=(struct Process*)FindTask(NULL);APTR window=process->pr_WindowPtr;
 BPTR ram_volume=0,ram_lock;struct InfoData ram_info;
 volumes_free();
 entry=LockDosList(LDF_VOLUMES|LDF_READ);
 if(entry){
  while((entry=NextDosEntry(entry,LDF_VOLUMES))){
   UBYTE *name=(UBYTE*)BADDR(entry->dol_Name);unsigned n;
   if(!entry->dol_Task||!name||!(n=name[0]))continue;
   v=AllocVec(sizeof(*v),MEMF_PUBLIC|MEMF_CLEAR);if(!v)break;
   memcpy(v->path,name+1,n);v->path[n]=':';v->path[n+1]=0;
   *tail=v;tail=&v->next;
  }
  UnLockDosList(LDF_VOLUMES|LDF_READ);
 }
 /* Release the DOS list before sending filesystem packets. Suppress missing
    media requesters; never traverse files or start an offline filesystem. */
 process->pr_WindowPtr=(APTR)-1;
 ram_lock=Lock("RAM:",ACCESS_READ);
 if(ram_lock){if(Info(ram_lock,&ram_info))ram_volume=ram_info.id_VolumeNode;UnLock(ram_lock);}
 for(v=volumes;v;v=v->next){
  struct InfoData info;BPTR lock=Lock(v->path,ACCESS_READ);
  if(lock){if(Info(lock,&info)&&info.id_NumBlocks>=0&&info.id_NumBlocksUsed>=0&&info.id_BytesPerBlock>0){
   v->is_ram=ram_volume&&info.id_VolumeNode==ram_volume;
   v->blocks=info.id_NumBlocks;v->used=info.id_NumBlocksUsed;v->blocksize=info.id_BytesPerBlock;v->valid=1;
  }UnLock(lock);}
 }
 process->pr_WindowPtr=window;
}
/* 64-bit multiplication avoids overflowing at 4 GiB. Display IEC units. */
static void storage_bytes(char *out,unsigned long long bytes) {
 ULONG unit=bytes>=1073741824ULL?1073741824UL:1048576UL;
 unsigned long tenths=(unsigned long)(bytes*10/unit);
 sprintf(out,"%lu.%lu%s",tenths/10,tenths%10,unit==1073741824UL?"GiB":"MiB");
}
static void storage(char *out,ULONG blocks,ULONG blocksize) {
 storage_bytes(out,(unsigned long long)blocks*blocksize);
}
static void disk_totals(unsigned long long *total,unsigned long long *used) {
 struct volume *v;*total=*used=0;
 for(v=volumes;v;v=v->next)if(v->valid&&!v->is_ram){
  *total+=(unsigned long long)v->blocks*v->blocksize;
  *used+=(unsigned long long)(v->used>v->blocks?v->blocks:v->used)*v->blocksize;
 }
}
static void network_read(char *name,char *ip) {
 struct ifconf conf;UBYTE buffer[4096];LONG fd;int present;unsigned offset;
 strcpy(name,"NETWORK");strcpy(ip,"OFFLINE");
 Forbid();present=FindName(&SysBase->LibList,"bsdsocket.library")!=NULL;Permit();
 if(!present)return;
 SocketBase=OpenLibrary("bsdsocket.library",4);if(!SocketBase)return;
 fd=socket(AF_INET,SOCK_DGRAM,0);
 if(fd>=0){
  conf.ifc_buf=buffer;conf.ifc_len=sizeof(buffer);
  if(IoctlSocket(fd,SIOCGIFCONF,(char*)&conf)==0){
   for(offset=0;offset+sizeof(struct ifreq)<=(unsigned)conf.ifc_len;){
    struct ifreq *r=(struct ifreq*)(buffer+offset),flags;
    unsigned step=IFNAMSIZ+(r->ifr_addr.sa_len>sizeof(struct sockaddr)?r->ifr_addr.sa_len:sizeof(struct sockaddr));
    if(offset+step>(unsigned)conf.ifc_len)break;
    offset+=step;flags=*r;
    if(r->ifr_addr.sa_family==AF_INET&&IoctlSocket(fd,SIOCGIFFLAGS,(char*)&flags)==0&&
       (flags.ifr_flags&IFF_UP)&&!(flags.ifr_flags&IFF_LOOPBACK)){
     ULONG a=((struct sockaddr_in*)&r->ifr_addr)->sin_addr.s_addr;
     if(!a||a==0xffffffffUL)continue;
     memcpy(name,r->ifr_name,IFNAMSIZ);name[IFNAMSIZ]=0;
     sprintf(ip,"%lu.%lu.%lu.%lu",a>>24,(a>>16)&255,(a>>8)&255,a&255);break;
    }
   }
  }CloseSocket(fd);
 }
 CloseLibrary(SocketBase);SocketBase=NULL;
}

#include <inline/macros.h>
static ULONG cpu_mhz,rom_version;
static void clock_read(void) {
 struct Library *identify=OpenLibrary("identify.library",8);
 if(!identify)identify=OpenLibrary("SYS:Utilities/xSysInfo/Libs/identify.library",8);
 if(identify){
  cpu_mhz=LP2(0x36,ULONG,IdHardwareNum,ULONG,35,d0,struct TagItem*,NULL,a0,,identify);
  if(identify->lib_Version>=38)
   rom_version=LP2(0x36,ULONG,IdHardwareNum,ULONG,57,d0,struct TagItem*,NULL,a0,,identify);
  CloseLibrary(identify);
 }
}
static void telemetry_close(void) {
 if(cpu_installed){RemIntServer(INTB_VERTB,&cpu_interrupt);cpu_installed=0;}
 volumes_free();
}
