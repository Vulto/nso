#!/bin/sh
# game_install.sh fetch LOG DIR URL PIN
# game_install.sh build LOG BUILD BINARY ROM
# One step of NSO's "Select games" install (src/gamesel.h), run from the NSO
# directory; output appended to LOG.
# fetch: clone DIR from URL when missing (then check out PIN, "-" = upstream
#        HEAD). An existing git checkout without a pin is fast-forwarded (a
#        failed update keeps the checkout); a pinned one or a non-git copy is
#        used as it is. Exit 0 = source ready.
# build: run BUILD with sh ($ROM = the user's ROM, only read or linked), then
#        exit 0 only if BINARY exists.
mode=$1 log=$2
mkdir -p "$(dirname "$log")" || exit 1
exec >>"$log" 2>&1
date
case $mode in
fetch)
    dir=$3 url=$4 pin=$5
    if [ ! -e "$dir" ]; then
        mkdir -p "$(dirname "$dir")" || exit 1
        git clone --recursive "$url" "$dir" || exit 1
        if [ "$pin" != "-" ]; then
            git -C "$dir" checkout --quiet "$pin" && git -C "$dir" submodule update --init --recursive || exit 1
        fi
    elif [ -d "$dir/.git" ] && [ "$pin" = "-" ]; then
        git -C "$dir" pull --ff-only || echo "update failed: using the checkout on disk"
    else
        echo "using $dir as it is"
    fi
    ;;
build)
    build=$3 bin=$4
    ROM=$5
    export ROM
    sh -c "$build" || exit 2
    [ -f "$bin" ] || { echo "no $bin after the build"; exit 2; }
    ;;
*)
    exit 1
    ;;
esac
