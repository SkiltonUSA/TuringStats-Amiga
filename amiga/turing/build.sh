#!/bin/sh
set -eu
src=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
out=${1:-"$src/../../.context/amiga/turing-1023"}
mkdir -p "$out"
cc=${AMIGA_CC:-m68k-amigaos-gcc}
"$cc" -std=c99 -noixemul -m68020 -O2 -Wall -Wextra -Werror "$src/main.c" -lamiga -o "$out/TuringStats"
"$cc" -std=c99 -noixemul -m68020 -O2 -Wall -Wextra -Werror "$src/make-icons.c" -lamiga -o "$out/MakeTuringIcons"
"$cc" -std=c99 -noixemul -m68020 -O2 -Wall -Wextra -Werror "$src/launch.c" -lamiga -o "$out/TuringStatsLaunch"
"$cc" -std=c99 -noixemul -m68020 -O2 -Wall -Wextra -Werror "$src/apply-icon.c" -lamiga -o "$out/ApplyTuringIcon"
