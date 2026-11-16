"""Builds the release zip into dist\\ from the built DLL, plugin, scripts and MCM files.

Layout (a plain Data-folder zip; every mod manager installs it as is):
    SKSE/Plugins/WhoWasThisFromAgain.dll
    WhoWasThisFromAgain.esp                       (ESL-flagged)
    Scripts/*.pex, Scripts/Source/*.psc
    MCM/Config/WhoWasThisFromAgain/config.json, settings.ini
    LICENSE.txt, EXCEPTIONS.txt, THIRD-PARTY.txt, SOURCE.txt   (GPL-3.0 with CommonLibSSE-NG's modding exception,
                                                                MinHook's licence, where the source is)
Entries are sorted with a fixed timestamp, so the same inputs give the same zip. The zip is then scanned for
personal details (drive letters, user and PC names, the Steam folder, private tool names) and the build refuses if
any is found.

Run dll\\build.bat and plugin\\build.ps1 first.
Usage: python package.py [out dir]      (default: dist next to this repo's scripts folder)
"""
import re
import sys
import zipfile
from pathlib import Path

STAMP = (2026, 1, 1, 0, 0, 0)
REPO = Path(__file__).resolve().parent.parent
SOURCE = """Who Was This From Again {version} by sayngwyn

Licence: GNU General Public License v3.0, with CommonLibSSE-NG's modding exception
(LICENSE.txt and EXCEPTIONS.txt in this archive). The Book Menu hook uses MinHook (BSD-2-Clause,
THIRD-PARTY.txt).

Source code for this version:
https://github.com/sayngwyn/who-was-this-from-again/tree/v{version}
"""

# Fixed strings that must never be in a shipped file (the PC's drives, folders and names).
# (written in pieces, so this file does not hold the words itself)
FORBIDDEN = [b"C:\\Us" + b"ers", b"My " + b"Mods", b"Mod" + b"Labs", b"gw" + b"yn", b"Gw" + b"ynRim", b"Gw" + b"ynOut", b"Sayn" + b"grim"]


# A drive path: a letter, colon, slash and a folder name (a lone letter-colon-slash among random bytes of a binary is not a path).
DRIVE_PATH = re.compile(rb"(?<![A-Za-z0-9])[A-Za-z]:[\\/][A-Za-z][A-Za-z0-9 _.-]{2,}")


def version():
    found = re.search(r'set_version\("([^"]+)"\)', (REPO / "dll/xmake.lua").read_text("utf-8"))
    if not found:
        sys.exit("no set_version in dll/xmake.lua")
    return found.group(1)


def scan(name, data):
    """Personal details in one file; the public name is allowed, so strip it before looking for the personal one."""
    problems = []
    cleaned = re.sub(rb"sayngwyn", b"", data, flags=re.I)
    for needle in FORBIDDEN:
        if needle.lower() in cleaned.lower():
            problems.append(f"{name}: contains {needle!r}")
    for found in DRIVE_PATH.finditer(cleaned):
        problems.append(f"{name}: contains a drive path {found.group(0)!r}")
    return problems


def main(out_dir):
    ver = version()
    plugin = REPO / "plugin/build"
    entries = {
        "SKSE/Plugins/WhoWasThisFromAgain.dll": REPO / "dll/build/windows/x64/releasedbg/WhoWasThisFromAgain.dll",
        "WhoWasThisFromAgain.esp": plugin / "WhoWasThisFromAgain.esp",
        "MCM/Config/WhoWasThisFromAgain/config.json": REPO / "mcm/Config/WhoWasThisFromAgain/config.json",
        "MCM/Config/WhoWasThisFromAgain/settings.ini": REPO / "mcm/Config/WhoWasThisFromAgain/settings.ini",
        "LICENSE.txt": REPO / "LICENSE",
        "EXCEPTIONS.txt": REPO / "dll/EXCEPTIONS",
        "THIRD-PARTY.txt": REPO / "dll/lib/minhook/LICENSE.txt",
    }
    for pex in sorted((plugin / "Scripts").glob("*.pex")):
        entries[f"Scripts/{pex.name}"] = pex
    for psc in sorted((REPO / "plugin/Source/Scripts").glob("*.psc")):
        entries[f"Scripts/Source/{psc.name}"] = psc
    for name, src in entries.items():
        if not src.is_file():
            sys.exit(f"missing: {src} (for {name})")
    data = {name: src.read_bytes() for name, src in entries.items()}
    data["SOURCE.txt"] = SOURCE.format(version=ver).replace("\n", "\r\n").encode("utf-8")

    problems = []
    for name, blob in data.items():
        # The licence texts name no paths; skip scanning them for the generic word list only if they are text-only docs.
        problems += scan(name, blob)
    if problems:
        print("\n".join(problems))
        sys.exit("personal details found in the files that would be shipped; zip not written")

    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    out = out_dir / f"Who Was This From Again {ver}.zip"
    with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
        for name in sorted(data):
            info = zipfile.ZipInfo(name, STAMP)
            info.compress_type = zipfile.ZIP_DEFLATED
            z.writestr(info, data[name])
    # Scan the written zip once more, from the zip itself.
    with zipfile.ZipFile(out) as z:
        again = []
        for info in z.infolist():
            again += scan(info.filename, z.read(info))
    if again:
        print("\n".join(again))
        out.unlink()
        sys.exit("personal details found in the written zip; deleted it")
    print(f"{out}: version {ver}, {len(data)} files, scanned clean")
    for name in sorted(data):
        print(f"  {name}  {len(data[name])} bytes")


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else REPO / "dist")
