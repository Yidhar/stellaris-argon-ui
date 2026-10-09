"""Installs / removes the example mods of this repository in the user's Stellaris mod folder and in the active playset (dlc_load.json).

    python tools/mod_install.py install demo|deck            copy the mod into <Documents>/Paradox Interactive/Stellaris/mod and enable it
    python tools/mod_install.py install demo|deck --link     enable it in place: the .mod file's path= points into this repository
    python tools/mod_install.py uninstall demo|deck|all      disable it and remove what install made
    python tools/mod_install.py status

demo = examples/demo-mod (a panel that shows the components), deck = examples/command-deck (the Command Deck: a HUD capsule and a five-page window).
Only the mods' own entries of enabled_mods are added or removed. Mods are read when the game starts, so a running game has to be restarted.
"""
import ctypes
import json
import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, ".."))
MODS = {
    "demo": ("argon_demo", os.path.join(ROOT, "examples", "demo-mod", "mod", "argon_demo")),
    "deck": ("argon_command_deck", os.path.join(ROOT, "examples", "command-deck", "mod", "argon_command_deck")),
}


def documents():
    buf = ctypes.create_unicode_buffer(260)
    ctypes.windll.shell32.SHGetFolderPathW(None, 5, None, 0, buf)  # CSIDL_PERSONAL
    return os.path.join(buf.value, "Paradox Interactive", "Stellaris")


def paths():
    docs = documents()
    return os.path.join(docs, "mod"), os.path.join(docs, "dlc_load.json")


def load_cfg(dlc):
    if not os.path.exists(dlc):
        return {"disabled_dlcs": [], "enabled_mods": []}
    with open(dlc, encoding="utf-8") as f:
        return json.load(f)


def save_cfg(dlc, cfg):
    tmp = dlc + ".tmp"
    with open(tmp, "w", encoding="utf-8") as f:
        json.dump(cfg, f, separators=(",", ":"))
    os.replace(tmp, dlc)


def install(which, link):
    name, src = MODS[which]
    mod_dir, dlc = paths()
    os.makedirs(mod_dir, exist_ok=True)
    if link:
        target = src
    else:
        target = os.path.join(mod_dir, name)
        if os.path.exists(target):
            shutil.rmtree(target)
        shutil.copytree(src, target)
    with open(os.path.join(mod_dir, f"{name}.mod"), "w", encoding="utf-8", newline="\n") as f:
        with open(os.path.join(src, "descriptor.mod"), encoding="utf-8") as d:
            f.write(d.read().rstrip("\n") + "\n")
        f.write('path="%s"\n' % target.replace("\\", "/"))
    cfg = load_cfg(dlc)
    entry = f"mod/{name}.mod"
    if entry not in cfg.setdefault("enabled_mods", []):
        cfg["enabled_mods"].append(entry)
    save_cfg(dlc, cfg)
    print(f"{name} installed ({'linked to ' + target if link else 'copied'}); enabled_mods: {cfg['enabled_mods']}")
    print("restart the game: mods are read at start")


def uninstall(which):
    mod_dir, dlc = paths()
    cfg = load_cfg(dlc)
    for key in (MODS if which == "all" else [which]):
        name = MODS[key][0]
        entry = f"mod/{name}.mod"
        if entry in cfg.get("enabled_mods", []):
            cfg["enabled_mods"].remove(entry)
        shutil.rmtree(os.path.join(mod_dir, name), ignore_errors=True)
        p = os.path.join(mod_dir, f"{name}.mod")
        if os.path.exists(p):
            os.remove(p)
    save_cfg(dlc, cfg)
    print("uninstalled; enabled_mods:", cfg.get("enabled_mods"))


def status():
    mod_dir, dlc = paths()
    cfg = load_cfg(dlc)
    for key, (name, _) in MODS.items():
        print(f"{key:5s} {name:20s} .mod file: {'present' if os.path.exists(os.path.join(mod_dir, name + '.mod')) else 'absent':8s} enabled: {f'mod/{name}.mod' in cfg.get('enabled_mods', [])}")


def main():
    a = sys.argv[1:]
    if a[:1] == ["install"] and len(a) >= 2 and a[1] in MODS:
        install(a[1], "--link" in a)
    elif a[:1] == ["uninstall"] and len(a) >= 2 and (a[1] in MODS or a[1] == "all"):
        uninstall(a[1])
    elif a[:1] == ["status"]:
        status()
    else:
        print(__doc__)
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
