#!/bin/sh
# Tomba-style precompiled AppImage. Build and audit generated C first.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
build_dir=${BUILD_DIR:-$root/build-release-linux}
version=$(tr -d ' \t\r\n' < "$root/VERSION")
output=${OUTPUT:-$root/dist/MediEvilIIRecomp-v$version-linux-x86_64.AppImage}
: "${SOURCE_COMMIT:?Set SOURCE_COMMIT to the exact release source commit}"
tools_dir=${APPIMAGE_TOOLS_DIR:-$build_dir/appimage-tools}
mkdir -p "$build_dir" "$tools_dir" "$(dirname -- "$output")"
appdir=$(mktemp -d "$build_dir/AppDir.XXXXXX")
payload=$appdir/usr/share/medievil2recomp
set --
if [ "${PUBLIC_RELEASE:-0}" = 1 ]; then set -- --release; fi
python3 "$root/tools/package_release.py" --build-dir "$build_dir" --platform linux-x64 \
    --stage "$payload" --source-commit "$SOURCE_COMMIT" "$@"
mkdir -p "$appdir/usr/bin"
mv "$payload/MediEvilIIRecomp" "$appdir/usr/bin/MediEvilIIRecomp"
install -m 0755 "$root/packaging/linux/AppRun" "$appdir/AppRun"
install -m 0644 "$root/packaging/linux/io.github.mstan.MediEvilIIRecomp.desktop" "$appdir/"
ln -s ../share/medievil2recomp/assets "$appdir/usr/bin/assets"
if command -v magick >/dev/null 2>&1; then image_tool=magick; else image_tool=convert; fi
"$image_tool" "$root/launcher_assets/img/boxart.tga" -resize 240x240 \
    -background transparent -gravity center -extent 256x256 "$appdir/io.github.mstan.MediEvilIIRecomp.png"
ln -s io.github.mstan.MediEvilIIRecomp.png "$appdir/.DirIcon"

fetch_tool() {
    url=$1 sha=$2 dest=$3
    if [ ! -f "$dest" ] || [ "$(sha256sum "$dest" | cut -d ' ' -f 1)" != "$sha" ]; then
        curl -fL --retry 3 "$url" -o "$dest.tmp"
        printf '%s  %s\n' "$sha" "$dest.tmp" | sha256sum -c -
        mv "$dest.tmp" "$dest"
    fi
    chmod 0755 "$dest"
}
# Same named, digest-pinned packaging tools as the Tomba reference.
linuxdeploy=$tools_dir/linuxdeploy-x86_64.AppImage
appimagetool=$tools_dir/appimagetool-x86_64.AppImage
fetch_tool https://github.com/linuxdeploy/linuxdeploy/releases/download/1-alpha-20251107-1/linuxdeploy-x86_64.AppImage \
    c20cd71e3a4e3b80c3483cef793cda3f4e990aca14014d23c544ca3ce1270b4d "$linuxdeploy"
fetch_tool https://github.com/AppImage/appimagetool/releases/download/1.9.1/appimagetool-x86_64.AppImage \
    ed4ce84f0d9caff66f50bcca6ff6f35aae54ce8135408b3fa33abfc3cb384eb0 "$appimagetool"
export NO_STRIP=1
"$linuxdeploy" --appimage-extract-and-run --appdir "$appdir" \
    --executable "$appdir/usr/bin/MediEvilIIRecomp" \
    --desktop-file "$appdir/io.github.mstan.MediEvilIIRecomp.desktop" \
    --icon-file "$appdir/io.github.mstan.MediEvilIIRecomp.png"

# Retain only libraries reachable without poisoning a host file picker's
# LD_LIBRARY_PATH. The executable uses its own RUNPATH instead.
keep=$appdir/keep-libraries.txt
env -u LD_LIBRARY_PATH ldd "$appdir/usr/bin/MediEvilIIRecomp" |
    awk '{ for(i=1;i<=NF;i++) if($i ~ /^\//) print $i }' |
    while read -r file; do readlink -f "$file"; done | sort -u > "$keep"
for file in "$appdir"/usr/lib/*; do
    [ -e "$file" ] || continue
    if ! grep -qxF "$(readlink -f "$file")" "$keep"; then rm -f -- "$file"; fi
done
if env -u LD_LIBRARY_PATH ldd "$appdir/usr/bin/MediEvilIIRecomp" | grep 'not found'; then
    echo 'Unresolved AppImage dependencies' >&2; exit 1
fi
python3 - "$appdir" <<'PY'
import hashlib,json,sys
from pathlib import Path
root=Path(sys.argv[1]);p=root/'usr/share/medievil2recomp/RELEASE_MANIFEST.json'
m=json.loads(p.read_text());m['files'].pop('MediEvilIIRecomp')
with (root/'usr/bin/MediEvilIIRecomp').open('rb') as f:
    m['runtime_binary']={'relative_to':'AppDir','path':'usr/bin/MediEvilIIRecomp',
                         'sha256':hashlib.file_digest(f,'sha256').hexdigest()}
p.write_text(json.dumps(m,indent=2)+'\n')
PY
ARCH=x86_64 "$appimagetool" --appimage-extract-and-run "$appdir" "$output"
chmod 0755 "$output"
(cd "$(dirname -- "$output")" && sha256sum "$(basename -- "$output")") > "$output.sha256"
printf 'RESULT_APPIMAGE=%s\nRESULT_APPDIR=%s\n' "$output" "$appdir"
