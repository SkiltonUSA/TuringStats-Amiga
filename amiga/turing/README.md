# TuringStats 0.1.0 Beta 1 — Amiga USB stats display

Native 68020+ utility for the Turing 3.5-inch 320×480 LCD, connected through
Poseidon USB. Shows ZZ9000 Zynq temperature and Exec-reported free Chip RAM,
free Fast RAM, used Fast RAM and sampled 68K CPU load. Pages rotate about every
ten seconds through system statistics, mounted volumes/network, processors,
OS/Workbench, hardware specifications and memory/storage totals.
Successive pages alternate blue headings/white values and orange headings/blue
values on the dark background. Three volumes fit per page; additional volumes
receive additional pages. Each
volume shows total, used and free space in MiB/GiB. RAM Disk is included and
reports its filesystem's dynamic capacity, not a physical disk size. The network
page shows the first active non-loopback IPv4 interface and its address.
No Mac, ARM worker, SDL or MUI is needed at runtime. Emulator FPS is not measured.

Double-click **TuringStats** on the System drive to start the app. Its monitor
and **Start**/**Stop** scripts live in `SYS:Utilities/TuringStats`. Stop releases
the USB connection and leaves the last figures labelled STOPPED.

`SYS:WBStartup/TuringStats` starts it automatically at boot. Both entry points
are small native Workbench launchers with DONOTWAIT icons. They check for an
already-running monitor, an already-loaded Poseidon library and the exact
unbound display in Poseidon's device list. If anything is absent they exit
quietly: no retries, USB commands, requester or stack autostart. If present,
they start the monitor asynchronously with NIL: input/output. Workbench does
not wait for its lifetime or USB transfers. If the screen is plugged in later,
double-click TuringStats to start it. Remove the WBStartup executable and its
.info icon to disable automatic startup.

For diagnostic output, run the monitor directly from Shell with redirection,
for example `SYS:Utilities/TuringStats/TuringStats 46 >RAM:TuringStats-test.log`
(after stopping the running instance).

```text
TuringStats             ; run until Ctrl-C or another invocation with STOP
TuringStats 20          ; bounded 20-second dashboard test
TuringStats STOP
TuringStats CAPTURE     ; capture one complete rotation, then exit
TuringStats PROBE       ; claim/release only, no display commands
TuringStats PATTERN     ; small red/green/blue/white test patch
```

Supports only the identified `1a86:5722 / USB35INCHIPSV2` device. Other screen
models/revisions require verification and may use different protocols. Keep
the screen connected while running; unplugging terminates this session, and
Start must be used again after reconnecting.

## Transport and rendering

The screen advertises CDC serial interfaces. On this device, Call Management
names interface 0 but the Union descriptor names the actual data interface 1.
Poseidon 4.5 cdcacm.class fails to bind with "Data interface with IN and OUT
interface missing?!?" The reviewed Poseidon source prefers Call Management
when present, consistent with that failure.

This application uses Poseidon's application-binding API on the unbound
screen, without forcing off another driver. It selects interface 1 bulk OUT
endpoint 2, sets CDC line/control state, then sends Rev-A display packets.
Transfers have a timeout and cancellation; the release hook handles removal.
It neither patches the driver nor modifies the USB stack configuration.
Stop finishes the current pixel rectangle before exiting, to preserve the
display's command stream. The transport allows a five-second NAK timeout;
device removal cancels the session.

The 307,200-byte RGB565 framebuffer is allocated with Exec. Topaz text is
rendered through graphics.library. The first update draws the whole screen;
subsequent system samples occur roughly once per second and transmit only changed
20-pixel-high value bands. Page changes redraw the framebuffer; mounted-volume
metadata and network information refresh on each drive-page cycle. This reads
filesystem capacity metadata, not files or directory trees. Absent-media
requesters are suppressed. Network checks do not open connections, change IP
settings, start an offline stack or retain a socket library between polls. This is a dashboard, not a second Workbench screen.

Temperature reuses the existing ZZTemperature read-only board discovery and
register interface. It measures the **ZZ9000 Zynq die**, not the 68060. Fast
memory figures include all memory reported as Fast by Exec, including any
ZZ9000 RAM added to the OS. "Used" is total Fast memory minus currently free
Fast memory.

68K load is a statistical estimate: a short VBlank interrupt samples whether
Exec's current task is TS_RUN. Idle-versus-busy testing on this Amiga produced
10/179 active samples (~6%) while idle and 178/178 (100%) for a busy loop. It
includes the monitor's own task activity; it is not precise cycle accounting,
and interrupt execution, disabled interrupts and workloads synchronized to
VBlank can bias it. No busy idle-counter task or scheduler patch is installed.
The interrupt server is removed before the process exits.

CPU MHz is queried once through identify.library (IDHW_CPUCLOCK), including the
private library supplied with xSysInfo at SYS:Utilities/xSysInfo/Libs. The screen
marks it EST: it is an estimate, not a hardware clock register. Missing library
or unavailable result displays UNAVAILABLE. No library is copied or installed.
ZZ9000 displays its documented nominal 666 MHz ARM clock, not a measured or
FPGA pixel clock; see the [MNT manual](https://mntre.com/media/ZZ9000_info_md/zz9000-manual-r4.pdf).
ARM Core0/Core1 utilization remains LOAD UNAVAILABLE: no verified passive load
counter was identified in the reviewed XX19c/XACP interfaces. The monitor does
not claim Core1, inject a worker or interfere with emulator workloads.

## Versions, hardware and capacity

The OS page queries the physical Kickstart ROM through identify.library,
reports active Kickstart from Exec's version/SoftVer, and separately shows
version.library's Workbench release and workbench.library's component version.
Missing ROM identification is displayed as unavailable, not guessed from Exec.
The Workbench public screen is locked briefly to sample width, height and bitmap
depth, then immediately unlocked; no screenmode settings are changed. These are
Workbench dimensions, not the mode of a separate fullscreen game screen.
Version and resolution readings refresh whenever this page appears.

Two hardware pages list this A4000TX's recorded configuration: AGA, TF4060/68060,
Kicksmash32 (firmware 1.8 from the previous identification), ZZ9000 dual A9/1 GB
DDR3, ZZ9000AX audio, Freeway Triton Lite, X-Surf 100, Buddha IDE, 64 GB
CompactFlash, 32 GB SD via SD-to-CF, and the Turing USB display. They are labelled
**RECORDED HARDWARE**, not presented as fresh probes. No Kicksmash commands,
bank changes, ARM workloads or firmware writes are used by this monitor.

The totals page reads Exec's total Chip/Fast RAM and sums total/used/free space
across mounted non-RAM volumes using 64-bit byte arithmetic. RAM Disk is excluded
by its filesystem volume identity, not its user-editable name. These are mounted
filesystem capacities, including SD/USB volumes, not raw device capacities or
unpartitioned space. Manufacturer GB capacity in the hardware profile is
separate from these measured GiB figures. Chip RAM is shown in KiB so reserved
areas are visible rather than rounding the OS total up to the physical 2 MiB.
Fast memory includes ZZ9000 RAM registered with Exec; the card's entire 1 GB
DDR3 is not added again. Counts refresh each time the totals page appears.

## Build and evidence

Run `sh amiga/turing/build.sh` inside the workspace's Amiga GCC environment.
Build uses `-m68020 -noixemul -O2 -Wall -Wextra -Werror`. Outputs are private
build artifacts under `.context/amiga/turing-1023` by default; an argument can
select another output directory. `MakeTuringIcons` creates the installed
Start/Stop and drawer icons; its paths target the installation above.

Physical A4000TX/Freeway/Poseidon4.5 checks: device and endpoint identification,
exclusive claim/release, complete colour-patch transfers and user-confirmed
colours; bounded live dashboard and user-confirmed readable figures. The
20-second run released its public port and USB binding. No throughput or CPU
cost measurement has been established. See session memory for deployment and
subsequent lifecycle checks. No firmware, startup or screenmode changes.
After increasing the initial one-second USB NAK timeout, two consecutive
five-second runs passed. The installed build passed singleton/duplicate-start,
cooperative Stop, public-port removal and restart checks.

## Credits and licensing

GPL-3.0-or-later. Display command encoding and RGB565 order are adapted from
[Matthieu Houdebine's turing-smart-screen-python](https://github.com/mathoudebine/turing-smart-screen-python),
particularly `library/lcd/lcd_comm_rev_a.py` (Copyright Matthieu Houdebine).
USB integration uses Chris Hodges' Poseidon API; the CDC binding behavior was
reviewed in the [AROS Poseidon source extraction](https://github.com/BlitterStudio/poseidon-usb).
ZZ9000 register information is based on MNT Research’s published hardware interface.

Rotating-page follow-up: a 40-second physical run completed the system, two
drive pages, processor page and return to system with exit status 0. Roadshow
reported x-surf-100 / 10.0.0.40; four volumes were read successfully. The private
xSysInfo identify.library returned 50 MHz, displayed as an estimate. The
updated executable was CRC-verified and installed in the existing drawer.
Physical appearance of the new pages awaits user observation.

Startup follow-up: native Workbench launch via WBLoad returned promptly and
started one monitor. A test launcher compiled with a nonexistent display serial
returned 0 without starting a monitor. This exercises the no-match path without
unplugging hardware. An actual reboot with or without the display has not been
performed. Startup-Sequence and User-Startup were not edited.

## Workbench artwork

The 64×64 icon depicts a portrait LCD with blue/orange activity bars and a USB
cable. `make-art.py` is the editable pixel-art master (Python/Pillow); it writes
`icon-art.h` and previews under `.context/amiga/turing-1023`. `build.sh` builds
ApplyTuringIcon. After creating the basic launch icons, apply the artwork with:

```text
ApplyTuringIcon SYS:TuringStats
ApplyTuringIcon SYS:WBStartup/TuringStats
ApplyTuringIcon SYS:Utilities/TuringStats/Start
```

The helper preserves icon type, location, default tool and tooltypes (including
DONOTWAIT), enforces the 64 KiB stack minimum, and notifies Workbench to refresh.
It reads back the saved colour image to verify RGB colours and transparency;
icon.library may reorder palette indices when saving. A planar fallback is
also included. All three installed icons passed artwork and metadata checks.

OS/hardware/totals follow-up: a 95-second physical run traversed all eight pages
and returned to system before clean shutdown. It reported Kickstart ROM47.115,
Workbench47.2, workbench.library47.33 and Workbench1920×800×24; total Fast RAM
624MiB and OS-managed Chip RAM2044KiB; mounted non-RAM volumes84.9GiB total,
13.8GiB used,71.1GiB free. These figures describe that run, not fixed constants.
The update was installed and restarted through the existing native launcher.

## Release capture mode

`CAPTURE` writes the exact already-rendered 320x480 RGB565 little-endian
framebuffer for each page to `RAM:TuringStats-Captures-v010b1/page-NN.rgb565`.
It runs one complete rotation and exits. Stop the existing instance before
using this mode. Normal startup does not capture files. These are rendered
frames from the target, not physical-panel photographs or LCD memory readbacks.
The standalone repository includes tools/convert-captures.py (Python/Pillow).
The release includes eight PNG captures from the physical development Amiga.
