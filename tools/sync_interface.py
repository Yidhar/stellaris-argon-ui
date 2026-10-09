"""Copies the public interface headers of stellaris-guiexpand into include/stellaris_guiexpand/ (this repository keeps its own copies, so that it builds alone).

    python tools/sync_interface.py <path to a stellaris-guiexpand checkout>          copy the headers
    python tools/sync_interface.py <path to a stellaris-guiexpand checkout> --check  only compare: exit code 1 when a copy differs or is missing

The interface only grows (append-only structs, API version in stellaris_gui_api.h), so a newer copy still works with an older host as long as the library
looks at `api->size` before it uses a member, as src/argon.cpp does.
"""
import argparse
import filecmp
import os
import shutil
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
DEST = os.path.join(ROOT, "include", "stellaris_guiexpand")
HEADERS = ("stellaris_gui_api.h", "stellaris_gui_client.h", "stellaris_gui_imgui.hpp")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("host", help="a stellaris-guiexpand checkout")
    ap.add_argument("--check", action="store_true", help="compare only")
    args = ap.parse_args()

    src_dir = os.path.join(os.path.abspath(args.host), "include", "stellaris_guiexpand")
    if not os.path.isdir(src_dir):
        print(f"{src_dir}: not found (is that a stellaris-guiexpand checkout?)", file=sys.stderr)
        return 2

    differ = []
    for name in HEADERS:
        src, dst = os.path.join(src_dir, name), os.path.join(DEST, name)
        if not os.path.isfile(src):
            print(f"{src}: missing in the host", file=sys.stderr)
            return 2
        same = os.path.isfile(dst) and filecmp.cmp(src, dst, shallow=False)
        if same:
            continue
        differ.append(name)
        if not args.check:
            os.makedirs(DEST, exist_ok=True)
            shutil.copyfile(src, dst)
    if differ:
        print(("differs: " if args.check else "updated: ") + ", ".join(differ))
        return 1 if args.check else 0
    print("the interface headers are the host's")
    return 0


if __name__ == "__main__":
    sys.exit(main())
