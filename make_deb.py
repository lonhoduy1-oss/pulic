#!/usr/bin/env python3
"""Portable Debian ar/tar.xz packager, for macOS GitHub Actions (no dpkg dependency)."""
from __future__ import annotations
import io, os, sys, tarfile, lzma
from pathlib import Path
def archive(entries):
    bio=io.BytesIO()
    with tarfile.open(fileobj=bio,mode="w") as tf:
        for name,data,mode in entries:
            info=tarfile.TarInfo(name)
            info.size=len(data);info.mode=mode;info.uid=0;info.gid=0;info.mtime=0
            tf.addfile(info,io.BytesIO(data))
    return lzma.compress(bio.getvalue(),preset=6)
def ar_member(name,data):
    if len(name)>15:raise ValueError(name)
    header=(f"{name + '/':16}{0:12}{0:6}{0:6}{100644:8}{len(data):10}`\n").encode("ascii")
    if len(header)!=60:raise ValueError(header)
    return header+data+(b"\n" if len(data)&1 else b"")
def main():
    dylib=Path(sys.argv[1]);output=Path(sys.argv[2])
    blob=dylib.read_bytes()
    if blob[:4]!=bytes.fromhex("cffaedfe"):
        raise ValueError("Input must be a real ARM64 Mach-O dylib (not .o)")
    # arm64 == cputype 0x0100000c little-endian
    if int.from_bytes(blob[4:8],"little") != 0x0100000c:
        raise ValueError("Mach-O target is not ARM64")
    control="""Package: com.polygun.experimental.aim-esp
Name: PG NEON v0.4.0 - Head ESP HP Speed God/DMG Local (Experimental)
Version: 0.4.0
Architecture: iphoneos-arm64e
Maintainer: Local Research
Author: Local Research
Section: Tweaks
Depends: firmware (>= 15.0)
Description: Experimental head-only aim, ESP HP, speed x1-x5, local health/invulnerability flag and local damage modifier x1-x5. Server effects NOT verified; not tested on device.
""".encode()
    plist=b'{ Filter = { Bundles = ( "com.polygon.arena" ); Executables = ( "PolygunArena" ); }; }\n'
    readme=Path(__file__).resolve().parent.joinpath("README_VI.md").read_bytes()
    ctrl=archive([("control",control,0o644)])
    dat=archive([
      ("./Library/MobileSubstrate/DynamicLibraries/PGRealAimESP.dylib",blob,0o755),
      ("./Library/MobileSubstrate/DynamicLibraries/PGRealAimESP.plist",plist,0o644),
      ("./usr/share/doc/com.polygun.experimental.aim-esp/README_VI.md",readme,0o644),
    ])
    deb=b"!<arch>\n"+ar_member("debian-binary",b"2.0\n")+ar_member("control.tar.xz",ctrl)+ar_member("data.tar.xz",dat)
    output.parent.mkdir(parents=True,exist_ok=True);output.write_bytes(deb)
    print(output,len(deb))
if __name__=="__main__":main()
