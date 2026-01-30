import zlib
from pathlib import Path

# Run this script from inside spiffs_root/
# It compresses story image binaries in-place for both folders:
#   spiffs_root/assets_s/
#   spiffs_root/assets_l/

FOLDERS = [Path("assets_s"), Path("assets_l")]
PATTERN = "ui_img_*_png.bin"
ZLIB_LEVEL = 3

def compress_folder(folder: Path) -> None:
    if not folder.exists():
        print(f"[skip] {folder} does not exist")
        return

    files = sorted(folder.glob(PATTERN))
    if not files:
        print(f"[skip] no files matched {folder / PATTERN}")
        return

    print(f"[info] compressing {len(files)} files in {folder}/ (level={ZLIB_LEVEL})")

    for p in files:
        raw = p.read_bytes()

        comp = zlib.compress(raw, level=ZLIB_LEVEL)

        print(f"{folder.name}/{p.name} raw:{len(raw)} -> compressed:{len(comp)}")
        p.write_bytes(comp)

def main() -> None:
    for folder in FOLDERS:
        compress_folder(folder)

if __name__ == "__main__":
    main()
