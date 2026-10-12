"""Package install files only; keep documentation in the source repository."""
import argparse
import hashlib
from pathlib import Path
import zipfile


def package(staging, output):
    if output.exists():
        raise ValueError("Refusing to replace an existing ZIP")
    files = [staging / "dinput8.dll"]
    for folder in ("Mods", "scripts"):
        files.extend(sorted(p for p in (staging / folder).rglob("*") if p.is_file()))
    if not files[0].is_file() or not (staging / "scripts/Custom/JishsFirstPersonMod.dll").is_file():
        raise ValueError("Install staging is incomplete")
    # Retain the dependency licence texts in the single requested licence file.
    license_files = [staging / "LICENSE.txt", *sorted((staging / "licenses").glob("*.txt"))]
    imgui_license = staging.parent / "vendor/External/imgui/LICENSE.txt"
    if imgui_license.is_file():
        license_files.append(imgui_license)
    license_text = "\n\n".join(f"--- {p.name} ---\n{p.read_text(encoding='utf-8-sig')}" for p in license_files)
    with zipfile.ZipFile(output, "w", zipfile.ZIP_DEFLATED) as archive:
        for file in files:
            archive.write(file, file.relative_to(staging).as_posix())
        archive.writestr("LICENSE.txt", license_text)
    with zipfile.ZipFile(output) as archive:
        if archive.testzip() is not None:
            raise ValueError("ZIP integrity check failed")
        assert all(n in ("dinput8.dll", "LICENSE.txt") or n.startswith(("Mods/", "scripts/")) for n in archive.namelist())
        dll = archive.read("scripts/Custom/JishsFirstPersonMod.dll")
        assert dll == (staging / "scripts/Custom/JishsFirstPersonMod.dll").read_bytes()
        print("Verified install ZIP:", output)
        print("DLL SHA-256:", hashlib.sha256(dll).hexdigest())
        print("Contents:", ", ".join(archive.namelist()))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("staging", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    package(args.staging, args.output)
