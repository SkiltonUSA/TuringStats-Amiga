TuringStats 0.1.0 Beta 1
Native Amiga dashboard for a Turing 3.5-inch USB LCD

INSTALL
Extract the entire archive to a writable drawer. Open the TuringStats package
and double-click Install. From Shell: CD to the package drawer, Execute Install.
Launch TuringStats from the System drive. Start/Stop are also available in
SYS:Utilities/TuringStats. Enable-Autostart is optional; it adds a WBStartup
entry. Existing autostart is retained when updating an installation.

The installer checks icon.library 44+ and Poseidon 4+. An existing monitor
binary is retained as TuringStats.previous. Existing icon settings are kept.
No Startup-Sequence or User-Startup changes are made.

REQUIREMENTS
68020 or newer, AmigaOS with icon.library 44+, Poseidon USB 4+, supported USB
controller, and the verified Rev-A LCD: VID/PID 1a86:5722, USB35INCHIPSV2.
Tested on AmigaOS 3.2.3, A4000TX/TF4060 and Freeway Triton Lite. A TF4060 is not
an intrinsic requirement. No MUI, SDL, AHI or Mac helper is required.
The framebuffer needs about 300 KiB in addition to the program and 64 KiB stack.
Optional identify.library supplies CPU MHz and ROM version; the private library
in SYS:Utilities/xSysInfo/Libs is supported. No libraries/drivers are bundled.
ZZ9000 is needed for temperature; bsdsocket.library is needed for network data.

STARTUP AND RECONNECTION
Automatic startup skips quietly if the display or loaded USB stack is absent.
It has no retry loop and does not wait for the monitor to exit. After reconnecting
USB, double-click TuringStats again. Multiple clicks do not start duplicates.
To disable autostart, remove SYS:WBStartup/TuringStats and its .info file.

PAGES
System statistics; mounted volumes/network; processors; OS/Workbench;
hardware profile; expansion/media; memory/storage totals. Pages rotate every
10 seconds with alternating blue/white and orange/blue text.

CPU load is a statistical 68K estimate; CPU MHz is an identify.library estimate.
ARM utilization is unavailable. ZZ9000 speed is nominal, not measured.
Hardware-profile pages describe the recorded development A4000TX setup; they
are not automatic hardware detection. Disk totals exclude RAM Disk and cover
mounted filesystems, not unpartitioned space. Temperature is the Zynq die.

SCREENSHOTS
Screenshots/ contains exact rendered framebuffers from the physical Amiga,
converted from RGB565 to PNG. They are not photographs of the LCD.

CLI
SYS:Utilities/TuringStats/TuringStats STOP
SYS:Utilities/TuringStats/TuringStats 30
SYS:Utilities/TuringStats/TuringStats CAPTURE
Stop the existing instance before running timed/capture modes. CAPTURE runs
one full rotation, writes RAM:TuringStats-Captures-v010b1/page-NN.rgb565, then
exits. The repository includes a converter for these 320x480 little-endian files.

SOURCE AND LICENCE
GPL-3.0-or-later; see COPYING.
Complete source for this release:
https://github.com/SkiltonUSA/TuringStats-Amiga/tree/v0.1.0-beta.1
Display protocol: mathoudebine/turing-smart-screen-python (Matthieu Houdebine).
USB API: Chris Hodges' Poseidon. ZZ9000 hardware: MNT Research.
Development: Codex assisted, Amiga-Bridge MCP.
