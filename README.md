# stellaris-argon-ui

[English](README.md) | [简体中文](README.zh-CN.md)

A component library for **Stellaris 4.5.2**, the way Element or Vuetify is one for a web page: cards, rows of columns, rings, a radar, area charts, a resource ledger, tabs, a HUD
capsule and more, which a **mod** uses from its text files. No DLL, no programming.

It is a plugin for [stellaris-guiexpand](https://github.com/Yidhar/stellaris-guiexpand) (the host that draws panels inside the game with the engine's own Dear ImGui). The host
reads a mod's panels from `interface/stl_gui/*.txt`; this plugin adds elements that those files can name:

```
argon_card = {
    kicker  = MYMOD_KICKER
    title   = MYMOD_ENERGY
    content = { argon_chart = { series = energy  height = 140 } }
}
```

The *Command Deck* in the pictures, a status capsule and a five-page window, is **two text files and a localisation file** that use this library
([`examples/command-deck`](examples/command-deck)).

![The Command Deck: overview page](docs/images/deck_overview.jpg)

| Economy | Time | Script |
|---|---|---|
| ![](docs/images/deck_economy.jpg) | ![](docs/images/deck_time.jpg) | ![](docs/images/deck_script.jpg) |

## What is in it

27 components, all named `argon_*`; the parameters of each are in [docs/components.md](docs/components.md) ([简体中文](docs/components.zh-CN.md)).

| | |
|---|---|
| **Layout** | `argon_card`, `argon_row` (with `col`), `argon_gap`, `argon_window`, `argon_tabs` |
| **Data** | `argon_stat_tile`, `argon_ring`, `argon_radar`, `argon_chart`, `argon_spark`, `argon_icon`, `argon_heading`, `argon_ledger`, `argon_resource_detail`, `argon_calendar` |
| **Time and script** | `argon_speed_dial`, `argon_effect_card`, `argon_effect_log` |
| **Settings** | `argon_theme_picker`, `argon_switch`, `argon_info` |
| **HUD capsule** | `argon_pill`, `argon_orb`, `argon_date`, `argon_speed`, `argon_vline`, `argon_chip` |

They follow the player's theme (four palettes), the layout scale of the screen and the language of the game, and they are drawn with the host's data (resources with their history, the
date, the speed, the empire's figures), so a mod does not read anything itself.

Without this plugin, a mod that uses it still loads: each `argon_*` entry shows a dim note that names the plugin, and the rest of the panel is drawn.

## Install and use

You need **stellaris-guiexpand** (a version with elements and HUD panels, 0.1.0 or newer) and the **Stellaris launcher**, which loads plugins.

1. Download `stellaris-argon-ui-v<version>.zip` from the [Releases page](https://github.com/Yidhar/stellaris-argon-ui/releases) (with its `.sha256`).
2. Unpack it into `Documents\Paradox Interactive\Stellaris\plugins\stellaris-argon-ui\` (no top folder: `stl-plugin.json` ends up directly in it), or unpack it anywhere
   and install the folder with `stl plugin install <folder>`.
3. Enable it in your playset (`stl plugin enable stellaris-argon-ui`) next to stellaris-guiexpand. The order does not matter: this plugin waits for the host.
4. Start the game with the launcher.

To see it, use the mods in `stellaris-argon-ui-samples-v<version>.zip` (or [`examples/`](examples)): `python tools/mod_install.py install deck` copies the Command Deck into your mod
folder and enables it (`demo` is the small panel of components). The deck's script page runs the effects of the
[stellaris-guiexpand-test-mod](https://github.com/Yidhar/stellaris-guiexpand-test-mod), so enable that too for those four cards. In a game the capsule is at the bottom of the
screen; its orb or **Ctrl + Alt + G** opens the deck. `python tools/mod_install.py uninstall all` removes the examples again.

## For mod authors

Write a panel as in the [host's guide](https://github.com/Yidhar/stellaris-guiexpand/blob/main/docs/mod-authors.md) and use the components of [docs/components.md](docs/components.md).
Put `stl_gui_requires = { stellaris-argon-ui }` into the file, so that a player who lacks the plugin is told what to install.

## Compatibility

- It uses only the **public C interface** of stellaris-guiexpand and reads no engine memory, so it does **not** depend on the game build; the host does. After a game patch the
  host is updated, not this library.
- The interface headers in `include/stellaris_guiexpand/` are copies of the host's. `python tools/sync_interface.py <host checkout>` updates them, `--check` says whether they differ.
- A host that is too old for elements is detected and logged (`logs\stellaris_argon_ui.log` in the plugin folder); nothing is drawn then.

## What is in the repository

```
src/               argon.cpp (the plugin: finds the host, registers the components), draw.* (drawing helpers), components*.cpp (the components)
include/           the host's interface headers (copies)
plugin/            stl-plugin.json
examples/          demo-mod (a panel of components), command-deck (the Command Deck mod)
docs/              components.md (also .zh-CN.md) and its images
tools/             build.bat, check_plugin.py, sync_interface.py, mod_install.py, live/argon_test.py (the author's live driver)
```

## Building

Visual Studio 2022 (x64) and CMake 3.20. Dear ImGui 1.85 is fetched by CMake: the plugin draws with its own copy, bound to the engine's context through the host.

```
tools\build.bat [path\to\vcvars64.bat]     # -> build\plugin\stellaris-argon-ui
python tools\check_plugin.py --dir build\plugin\stellaris-argon-ui
```

## Licence

MIT, see [LICENSE](LICENSE).
