"""Stage an audited precompiled player build; optionally create its Windows ZIP."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import zipfile

try:
    import tomllib
except ImportError:
    import tomli as tomllib

ROOT = Path(__file__).resolve().parents[1]

def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()

def cache_values(path):
    return dict(re.findall(r'^([^#/:\n][^:\n]*):[^=\n]+=(.*)$',
                          path.read_text(encoding='utf-8'), re.M))

def validate_build(build):
    values = cache_values(build/'CMakeCache.txt')
    required = {'CMAKE_BUILD_TYPE': 'Release', 'PSX_SETUP_WIZARD': 'OFF',
                'PSXRECOMP_FORCE_SETUP_HOST': 'OFF', 'PSXRECOMP_REQUIRE_GAME_C': 'ON',
                'PSX_DEBUG_TOOLS': 'OFF', 'PSX_RECOMP_UI': 'ON'}
    for key, expected in required.items():
        if values.get(key) != expected:
            raise ValueError(f'{key} must be {expected}, got {values.get(key)}')
    version = (ROOT/'VERSION').read_text().strip()
    if (build/'psx_game_version.txt').read_text().strip() != version:
        raise ValueError('Built version does not match VERSION')
    audit = json.loads((ROOT/'generated/AOT_STATIC_AUDIT.json').read_text())
    profile = json.loads((ROOT/'aot/overlays.json').read_text())
    if not audit['all_pairs_valid'] or not audit['all_guards_match_known_input_bytes']:
        raise ValueError('Original-disc native audit failed')
    if audit['recipe_count'] != profile['expected_records']:
        raise ValueError('Generated native code does not match the release profile')
    if audit['generation_inputs']['game_toml_sha256'] != sha(ROOT/'game.toml'):
        raise ValueError('Game config changed since native generation')
    if audit['generation_inputs']['profile_sha256'] != sha(ROOT/'aot/overlays.json'):
        raise ValueError('Overlay profile changed since native generation')
    for name, digest in audit['files'].items():
        if sha(ROOT/'generated'/name) != digest:
            raise ValueError(f'Generated code audit mismatch: {name}')
    source_list = next(line for line in (build/'build.ninja').read_text(encoding='utf-8').splitlines()
                       if line.startswith('build MediEvil_II__Recompiled') and 'CXX_EXECUTABLE_LINKER' in line)
    for name in audit['files']:
        if name.endswith('.c') and name not in source_list:
            raise ValueError(f'Native unit absent from runtime: {name}')
    return version, audit

def check_payload(stage):
    allowed_bin = Path('bios/openbios.bin')
    forbidden = {'.cue', '.iso', '.chd', '.img', '.pst', '.mcd', '.mcr', '.pack'}
    for file in stage.rglob('*'):
        if not file.is_file():
            continue
        rel = file.relative_to(stage)
        if file.suffix.lower() in forbidden or (file.suffix.lower() == '.bin' and rel != allowed_bin):
            raise ValueError(f'Player/disc data must not be packaged: {rel}')
        if file.name in {'settings.toml', 'disc.cfg', 'bios.cfg', 'state.toml'}:
            raise ValueError(f'User configuration must not be packaged: {rel}')
    if (stage/allowed_bin).stat().st_size != 524288:
        raise ValueError('Missing or invalid bundled OpenBIOS')
    manifests = list((stage/'mods/bundled').rglob('manifest.toml'))
    ids = sorted(tomllib.loads(p.read_text(encoding='utf-8-sig'))['id'] for p in manifests)
    expected = ['medievil2.enhancement.frame-rate', 'medievil2.enhancement.seamless-loading',
                'medievil2.enhancement.texture-filtering', 'medievil2.enhancement.widescreen',
                'psx.enhancement.8mb-ram', 'psx.enhancement.pgxp', 'psx.presentation.bezel']
    if ids != sorted(expected):
        raise ValueError(f'Unexpected mod catalog: {ids}')
    return ids

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--platform', choices=['windows-x64', 'linux-x64'], required=True)
    parser.add_argument('--stage', type=Path, required=True, help='New empty staging directory')
    parser.add_argument('--source-commit', required=True, help='Commit of this exact source checkout')
    parser.add_argument('--zip', type=Path)
    parser.add_argument('--objdump', default='objdump')
    args = parser.parse_args()
    if not re.fullmatch(r'[0-9a-f]{40}', args.source_commit):
        raise ValueError('A full source commit is required')
    build, stage = args.build_dir.resolve(), args.stage.resolve()
    version, audit = validate_build(build)
    if stage.exists() and any(stage.iterdir()):
        raise ValueError(f'Refusing to reuse nonempty stage: {stage}')
    stage.mkdir(parents=True, exist_ok=True)
    extension = '.exe' if args.platform == 'windows-x64' else ''
    binary = stage/('MediEvilIIRecomp'+extension)
    shutil.copy2(build/('MediEvil_II__Recompiled'+extension), binary)
    for name in ['assets', 'bios']:
        shutil.copytree(build/name, stage/name)
    shutil.copytree(build/'mods/bundled', stage/'mods/bundled')
    for name in ['game.toml', 'game_options.toml', 'VERSION', 'README.md', 'LICENSE', 'THIRD_PARTY_NOTICES.md']:
        shutil.copy2(ROOT/name, stage/name)
    for name in ['START_HERE.txt', 'input.ini']:
        shutil.copy2(ROOT/'packaging/release'/name, stage/name)
    shutil.copy2(ROOT/'packaging/included-prs.json', stage/'INCLUDED_PRS.json')
    (stage/'docs').mkdir()
    shutil.copy2(ROOT/'docs/ENHANCEMENTS.md', stage/'docs/ENHANCEMENTS.md')
    (stage/'licenses').mkdir()
    shutil.copy2(ROOT/'psxrecomp/runtime/licenses/libchdr-NOTICES.txt', stage/'licenses')
    packages = check_payload(stage)
    pins = tomllib.loads((ROOT/'project-manifest.toml').read_text(encoding='utf-8'))['framework']
    manifest = dict(version=version, platform=args.platform, source_commit=args.source_commit,
                    framework_commit=pins['commit'], recomp_ui_commit=pins['recomp_ui_commit'],
                    simulated_release=True, debug_tools=False, setup_wizard=False,
                    native_variants=audit['published_variants'], native_images=audit['recipe_count'],
                    full_static_coverage_proven=audit['full_static_coverage_proven'],
                    aot_profile_sha256=audit['profile_sha256'],
                    aot_inputs_sha256=audit['generation_inputs_sha256'], mod_packages=packages)
    if args.platform == 'windows-x64':
        imports = re.findall(r'DLL Name:\s*(\S+)', subprocess.check_output(
            [args.objdump, '-p', str(binary)], text=True))
        system = set('kernel32 user32 gdi32 shell32 msvcrt advapi32 ws2_32 comdlg32 dbghelp ole32 oleaut32 winmm imm32 version setupapi dinput8 rpcrt4 hid cfgmgr32 opengl32 d2d1 dwrite ucrtbase bcrypt crypt32 wintrust shlwapi'.split())
        unexpected = [d for d in imports if d.lower().removesuffix('.dll') not in system and not d.lower().startswith('api-ms-win-')]
        if not imports or unexpected:
            raise ValueError(f'Windows binary is not self-contained: {unexpected}')
        manifest['system_dll_imports'] = imports
    else:
        manifest['linux_build_libc'] = subprocess.check_output(['getconf', 'GNU_LIBC_VERSION'], text=True).strip()
    manifest['files'] = {str(p.relative_to(stage)).replace(os.sep, '/'): sha(p)
                         for p in sorted(stage.rglob('*')) if p.is_file()}
    (stage/'RELEASE_MANIFEST.json').write_text(json.dumps(manifest, indent=2)+'\n', encoding='utf-8')
    if args.zip:
        args.zip.parent.mkdir(parents=True, exist_ok=True)
        with zipfile.ZipFile(args.zip, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=6) as out:
            for file in sorted(stage.rglob('*')):
                if file.is_file():out.write(file, Path('MediEvilIIRecomp')/file.relative_to(stage))
        args.zip.with_suffix(args.zip.suffix+'.sha256').write_text(f'{sha(args.zip)}  {args.zip.name}\n')
        print(args.zip)
    print(f'RESULT_STAGE={stage}')

if __name__ == '__main__':
    main()
