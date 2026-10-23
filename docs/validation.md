# 0.1.0 Beta 1 validation

Test target: A4000TX, TF4060/68060, AmigaOS 3.2.3, Freeway Triton Lite,
Poseidon 4.5, Turing Rev-A 1a86:5722 display and ZZ9000 XX19c.

- Standalone export rebuilt byte-identically to the tested monitor and launcher.
- ZIP and LHA extraction matched all 24 packaged files byte for byte.
- Native GCC build completed with `-Wall -Wextra -Werror`.
- Binary transfers used CRC verification.
- Earlier colour-patch and dashboard layouts were visually confirmed by the owner.
- Complete versioned eight-page rotation transferred successfully to the display;
  eight framebuffer snapshots were captured on the Amiga, then converted to PNG.
- Rotation, cooperative Stop, singleton protection and USB cleanup passed.
- 68K sample probe distinguished idle (10 active / 179 samples) from busy-loop
  operation (178 active / 178 samples). This validates the basic signal, not
  precision under arbitrary interrupt or VBlank-synchronized workloads.
- Kickstart, Workbench, screen and memory readings were retrieved from the target.
- Mounted-volume totals used 64-bit arithmetic and excluded RAM Disk.
- Workbench icon colours/transparency and launch metadata passed read-back checks.
- Native launch through WBLoad returned promptly and started the monitor.
- A launcher compiled with a nonexistent display serial exercised the absent-device
  path: returned 0 promptly, no monitor started. No physical unplug was required.
- The LHA package was extracted on the Amiga; its Install script completed with
  status 0 and `Version ... FULL` identified the installed Beta 1 binary.
- Enable-Autostart and subsequent native launch completed with status 0.

A cold boot with/without the display, other USB controllers/display revisions,
and a long-duration soak have not been tested. The screenshots are rendered
framebuffers, not photographs of the panel. ARM utilization remains unavailable.
