"""Live driver for the component library: copies the built plugin into a run folder, injects it into the running game, unloads it, shows its log, takes
screenshots. Author's tooling: it uses the bench scripts of the stellaris-perf repository (find the game, the window) and the launcher's `stl inject`.
The game, the stellaris-guiexpand host and the mods are started with the driver of the stellaris-guiexpand repository (tools/live/guiexpand_test.py).

    ARGON_BENCH_SCRIPTS  folder with game_session.py and benchlib.py   (default D:\\stellaris-perf\\bench\\scripts)
    ARGON_STL            stl.exe of the launcher                          (default D:\\stellaris-Launcher\\target\\release\\stl.exe)
    ARGON_RUN            scratch folder                                   (default <repo>\\run)

    python tools/live/argon_test.py stage       copy build/Release/stellaris_argon_ui.dll into the run folder
    python tools/live/argon_test.py inject      inject it (the host may be loaded before or after: it is found by polling)
    python tools/live/argon_test.py unload      unload it by the development event (the elements go away; the host keeps running)
    python tools/live/argon_test.py log [n]     tail of its log
    python tools/live/argon_test.py shot out.png   screenshot of the game window's client area
"""
import ctypes
import ctypes.wintypes as w
import os
import shutil
import subprocess
import sys
import time

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
BENCH = os.environ.get("ARGON_BENCH_SCRIPTS", r"D:\stellaris-perf\bench\scripts")
STL = os.environ.get("ARGON_STL", r"D:\stellaris-Launcher\target\release\stl.exe")
RUN = os.environ.get("ARGON_RUN", os.path.join(REPO, "run"))
DLL = "stellaris_argon_ui.dll"
FOLDER = os.path.join(RUN, "stellaris-argon-ui")
sys.path.insert(0, BENCH)

user32 = ctypes.WinDLL("user32", use_last_error=True)
try:
    ctypes.WinDLL("shcore").SetProcessDpiAwareness(2)
except OSError:
    user32.SetProcessDPIAware()


def game():
    import game_session as gs
    from benchlib import game_pids, module_loaded
    pids = game_pids()
    if len(pids) != 1:
        raise SystemExit(f"expected exactly one stellaris.exe, found {pids}")
    return gs, pids[0], module_loaded


def stage():
    os.makedirs(os.path.join(FOLDER, "logs"), exist_ok=True)
    built = os.path.join(REPO, "build", "Release", DLL)
    if not os.path.exists(built):
        raise SystemExit(f"{built} is missing: build first (tools\\build.bat)")
    shutil.copyfile(built, os.path.join(FOLDER, DLL))
    print("staged in", FOLDER)


def inject():
    _, pid, module_loaded = game()
    if module_loaded(pid, DLL):
        raise SystemExit(f"{DLL} is already loaded")
    r = subprocess.run([STL, "inject", os.path.join(FOLDER, DLL), "--pid", str(pid)], capture_output=True, text=True)
    print(r.stdout.strip(), r.stderr.strip())
    time.sleep(1.5)
    print("loaded:", module_loaded(pid, DLL))


def unload():
    _, pid, module_loaded = game()
    k = ctypes.WinDLL("kernel32", use_last_error=True)
    k.OpenEventW.restype = ctypes.c_void_p
    k.OpenEventW.argtypes = [ctypes.c_uint32, ctypes.c_int, ctypes.c_wchar_p]
    k.SetEvent.argtypes = [ctypes.c_void_p]
    ev = k.OpenEventW(0x2, False, f"Local\\stellaris_argon_ui_unload_{pid}")
    if not ev:
        raise SystemExit("unload event not found (not loaded?)")
    k.SetEvent(ev)
    for _ in range(80):
        if not module_loaded(pid, DLL):
            print("unloaded")
            return
        time.sleep(0.2)
    print("still loaded")


def log(n):
    p = os.path.join(FOLDER, "logs", "stellaris_argon_ui.log")
    if os.path.exists(p):
        for line in open(p, encoding="utf-8", errors="replace").read().splitlines()[-n:]:
            print(line)
    else:
        print("(no log yet)")


def shot(out):
    from PIL import ImageGrab
    gs, pid, _ = game()
    hwnd = gs.game_window(pid)
    user32.keybd_event(0x12, 0, 0, 0)
    user32.keybd_event(0x12, 0, 0x2, 0)
    user32.ShowWindow(hwnd, 9)
    user32.SetForegroundWindow(hwnd)
    time.sleep(0.5)
    pt = w.POINT(0, 0)
    user32.ClientToScreen(hwnd, ctypes.byref(pt))
    c = w.RECT()
    user32.GetClientRect(hwnd, ctypes.byref(c))
    vx, vy = user32.GetSystemMetrics(76), user32.GetSystemMetrics(77)
    ImageGrab.grab(all_screens=True).crop((pt.x - vx, pt.y - vy, pt.x - vx + c.right, pt.y - vy + c.bottom)).save(out)
    print(f"saved {out} ({c.right}x{c.bottom})")


def main():
    a = sys.argv[1:]
    if not a:
        print(__doc__)
    elif a[0] == "stage":
        stage()
    elif a[0] == "inject":
        inject()
    elif a[0] == "unload":
        unload()
    elif a[0] == "log":
        log(int(a[1]) if len(a) > 1 else 30)
    elif a[0] == "shot":
        shot(a[1])
    else:
        print(__doc__)


main()
