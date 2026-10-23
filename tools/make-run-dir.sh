#!/bin/sh
# Assemble a runnable PiccuEngine directory from this build.
# usage: make-run-dir.sh <run dir> <Descent 3 data dir holding d3.hog, extra*.hog, merc.hog>
# The engine (with its piccuengine.hog) must be launched from inside the run dir: piccuengine.hog is found in the
# working directory at start-up. d3-linux.hog (scripts + string tables) comes from this build, not from the data dir.
set -e
here=$(cd "$(dirname "$0")" && pwd)
run=$1; data=$2
[ -n "$run" ] && [ -n "$data" ] || { echo "usage: $0 <run dir> <data dir>"; exit 1; }
mkdir -p "$run/online" "$run/netgames" "$run/missions" "$run/tmp"
cp "$here/obj/PiccuEngine" "$here/obj/piccuengine.hog" "$here/obj/d3-linux.hog" "$run/"
cp "$here/obj/online/Direct TCP~IP.piccucon" "$run/online/"
# -directip and +connect load the connector by the name "TCP-IP" (menu.cpp AutoConnectLANIP), while the build
# names it after the menu entry; the link lets the command-line join find it.
ln -sf "Direct TCP~IP.piccucon" "$run/online/TCP-IP.piccucon"
cp "$here"/obj/netgames/*.d3m "$run/netgames/"
for h in d3.hog extra.hog extra1.hog extra13.hog merc.hog; do
  [ -e "$data/$h" ] && ln -sf "$data/$h" "$run/$h"
done
echo "run dir ready: $run (add the server's mission .mn3 to $run/missions/)"
