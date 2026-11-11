#!/bin/sh
# repo_sync.sh LOG STATUS DIR URL BINARY ROM DEPS BUILD [MODE]
# Clone / fast-forward / build one source port. Runs in the background from
# NSO; one-line progress goes to STATUS as "<key> [detail]" (stable English
# keys, translated in NSO: src/repos.h Repo_StatusKey), full output to LOG.
# ROM and DEPS may be "-". DEPS is comma separated; "pkg:x" = pkg-config.
# Exit: 0 built, 10 up to date, 11 local copy (not git, left alone),
#       20 ROM needed, 21 missing deps, 1 git failed, 2 build failed.
# MODE "build" (NSO's Install action) builds the checkout already on disk:
# no clone, no fetch, no network. Default "sync" clones / updates first.
log=$1 status=$2 dir=$3 url=$4 bin=$5 rom=$6 deps=$7 build=$8 mode=${9:-sync}
mkdir -p "$(dirname "$log")" "$(dirname "$status")" || exit 1
exec >"$log" 2>&1
say() { printf '%s\n' "$*" > "$status"; echo "== $*"; }
date

missing=""
if [ "$deps" != "-" ]; then
    for d in $(printf '%s' "$deps" | tr ',' ' '); do
        case $d in
            pkg:*) pkg-config --exists "${d#pkg:}" || missing="$missing ${d#pkg:}" ;;
            *) command -v "$d" >/dev/null 2>&1 || missing="$missing $d" ;;
        esac
    done
fi
if [ -n "$missing" ]; then say missing_deps $missing; exit 21; fi

changed=0
if [ "$mode" = build ] && [ ! -e "$dir" ]; then say not_cloned; exit 1; fi
if [ ! -e "$dir" ]; then
    say cloning
    git clone --recursive "$url" "$dir" || { say clone_failed; exit 1; }
    changed=1
elif [ ! -d "$dir/.git" ]; then
    # Hand-made copy (patches, ROM, assets): never touch it.
    if [ -x "$dir/$bin" ]; then say local_ready; else say local_unbuilt; fi
    exit 11
elif [ "$mode" = build ]; then
    echo "build mode: using the checkout on disk (no fetch)"
else
    say checking
    git -C "$dir" fetch --quiet || { say fetch_failed; exit 1; }
    local_head=$(git -C "$dir" rev-parse HEAD) || { say bad_checkout; exit 1; }
    remote_head=$(git -C "$dir" rev-parse '@{u}') || { say no_upstream; exit 1; }
    if [ "$local_head" != "$remote_head" ]; then
        say updating
        git -C "$dir" merge --ff-only '@{u}' || { say update_failed; exit 1; }
        git -C "$dir" submodule update --init --recursive || { say submodule_failed; exit 1; }
        changed=1
    fi
fi

# Empty or "-" means the port needs no ROM at build time (disc picked at run).
if [ -z "$rom" ] || [ "$rom" = "-" ]; then
    rom="-"
fi
if [ "$rom" != "-" ] && [ ! -f "$dir/$rom" ]; then
    echo "ROM expected at: $dir/$rom"
    say rom_needed "$rom"
    exit 20
fi
if [ "$changed" = 0 ] && [ -x "$dir/$bin" ]; then say up_to_date; exit 10; fi

say building
(cd "$dir" && sh -c "$build") || { say build_failed; exit 2; }
[ -x "$dir/$bin" ] || { say build_no_binary "$bin"; exit 2; }
# Some ports only look for the ROM beside the executable.
if [ "$rom" != "-" ] && [ -f "$dir/$rom" ]; then
    bdir=$(dirname "$bin")
    if [ "$bdir" != "." ]; then
        mkdir -p "$dir/$bdir" || { say build_failed; exit 2; }
        command /bin/cp -f "$dir/$rom" "$dir/$bdir/$(basename "$rom")" || { say build_failed; exit 2; }
    fi
fi
say ready
exit 0
