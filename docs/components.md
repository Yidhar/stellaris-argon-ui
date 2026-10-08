# Components

[English](components.md) | [简体中文](components.zh-CN.md)

stellaris-argon-ui is a plugin that registers its components as **elements** of [stellaris-guiexpand](https://github.com/Yidhar/stellaris-guiexpand). A mod uses them in the
panels it declares in `interface/stl_gui/*.txt`, with the same syntax as the host's own elements (`text`, `row`, `button`, ...; see the host's
[mod author guide](https://github.com/Yidhar/stellaris-guiexpand/blob/main/docs/mod-authors.md)). No DLL, no C++.

![A panel made of components: cards in a row, stat tiles, a radar, rings and a chart](images/components_panel.jpg)

```
stl_gui_version = 1
stl_gui_requires = { stellaris-argon-ui }       # only improves the message when the plugin is missing

panel = {
    id = status
    title = MYMOD_STATUS_TITLE
    size = { 640 360 }
    content = {
        argon_row = {
            gap = 12
            height = 110
            col = { content = { argon_stat_tile = { label = MYMOD_COLONIES  stat = colonies  accent = 0 height = fill } } }
            col = { content = { argon_stat_tile = { label = MYMOD_POPS      stat = pops      accent = 1 height = fill } } }
        }
        argon_gap = { height = 12 }
        argon_card = {
            kicker = MYMOD_KICKER
            title  = MYMOD_ENERGY
            content = { argon_chart = { series = energy  height = 140 } }
        }
    }
}
```

Without the plugin every `argon_*` entry shows a dim note that names it; the rest of the panel is drawn as usual.

## Conventions

These hold for every component.

| | |
|---|---|
| **Text** | every parameter that is shown (`title`, `label`, `caption`, ...) is a **localisation key** of your mod, and may contain scoped text such as `[Root.GetName]`. A value that is not a key is shown as written |
| **Width** | `width = 0.4` is a fraction of the line (or of the column) when it is `<= 1`, `width = 220` is pixels when it is larger. Pixels are multiplied by the layout scale, so a layout looks the same on every screen |
| **Height** | `height = 120` in pixels, `height = fill` takes what is left down to the bottom of the window (inside an `argon_row`: the height of the row), absent = as tall as the content |
| **Accent** | `accent = 0` is the theme's first colour, `1` the second, a value between mixes them. The player's theme (aurora, ember, verdant, crimson) changes both everywhere, at once |
| **Resources** | `resource = energy` is the key of the game's resource (`energy`, `minerals`, `food`, `alloys`, `consumer_goods`, `influence`, `unity`, the research kinds, ...). Names shown for a resource come from the game's own localisation, in the player's language |
| **No game** | in the main menu there is no empire: components show `-` or `?` instead of data and do not fail |
| **Unknown parameters** | are ignored. A missing parameter takes its default |

## Containers

### `argon_card`

A glass card with an optional header (`kicker` in small capitals, `title` below it) and `content` inside.

| Parameter | |
|---|---|
| `kicker`, `title` | loc keys; with neither, the card has no header |
| `content = { ... }` | any entries, including other cards and rows |
| `accent` | colour of the line over the header |
| `glow = yes` | a soft glow in the top right corner |
| `width`, `height` | see conventions. A card of a fixed height **clips** its content; one without a height grows with it |
| `padding` | px, default 18 |

### `argon_row` and `col`

Columns side by side. Each column is `col = { width = ...  content = { ... } }`; columns without a width share what is left.

| Parameter | |
|---|---|
| `gap` | px between columns, default 16 |
| `height` | px, `fill`, or the tallest column. `height = fill` of a card inside a column is the height of the row |

### `argon_gap`

Vertical space that scales with the layout (the host's own `spacer` is in raw pixels). `height` in px, default 16.

### `argon_window` and `argon_tabs`

The frame of a window and the pages inside it. They are made for a `kind = hud` panel (no title bar and no background of its own), usually `movable = yes`, `open = no` and with a `hotkey`.

`argon_window`: shadow, glass, glows, a starfield, a header and a close button. The entries after it are the body, which starts 82 px down.

| Parameter | |
|---|---|
| `kicker`, `title`, `subtitle` | loc keys of the header |
| `inset` | px that the header starts at; leave room for a rail |
| `close` | the id of the panel that the close button hides, `"modid:panelid"` |

`argon_tabs`: an icon rail on the left, the selected page on the right.

```
argon_tabs = {
    tab = { icon = overview  title = MYMOD_T1  subtitle = MYMOD_S1  content = { ... } }
    tab = { icon = economy   title = MYMOD_T2  subtitle = MYMOD_S2  content = { ... } }
}
```

`icon` is one of `overview`, `economy`, `time`, `script`, `settings`.

## Data

![The economy page: ledger, a resource's detail card, history chart and net bars](images/deck_economy.jpg)

| Component | What it shows | Parameters |
|---|---|---|
| `argon_stat_tile` | a figure with a label and an accent bar | `label`; `stat` = `colonies`, `pops`, `empire_size`, `military_power`, `tech_power`, `economy_power`, **or** `resource` + `show` = `stock` (default), `net`, `income`, `expense`, `max`; `accent`, `width`, `height` |
| `argon_ring` | a gauge of one resource: stock against its cap (or against its recent peak when it has none), the net in it, the icon and the name under it | `resource`, `label` (the game's name when absent), `size` (diameter, px, default 96), `width` |
| `argon_radar` | five axes of the country (military, technology, economy, territory, population) against the strongest empire of the galaxy on each | `size` (px, default 240), `label_military`, `label_tech`, `label_economy`, `label_territory`, `label_population`, `note` |
| `argon_chart` | the history of a series as a gradient area (or a line) with a crosshair that reads a value | `series`, `kind` = `area` (default) or `line`, `color` (a resource key, `accent`, `accent2`), `caption`, `height` (px, default 170) |
| `argon_spark` | a small area chart of a series with its latest value | `series`, `caption`, `unit`, `max` (the top of the scale), `color`, `height` |
| `argon_icon` | the icon of a resource | `resource`, `size` (px, default 20) |
| `argon_heading` | a line of large text | `text`, `scale` (default 1) |
| `argon_ledger` | every resource of the country, one row each: icon, name, stock, monthly net and a small history. Clicking a row selects it | `id` |
| `argon_resource_detail` | the resource selected in the `argon_ledger` with the same `id`: icon, name, stock, income against expense, the history, the monthly net as bars | `id`, `kicker_chart`, `kicker_net`, `label_income`, `label_expense`, `label_net`, `label_cap`, `height` |
| `argon_calendar` | the date as two rings: months of the year (outer) and days of the month (inner), the date in the middle | `label_month`, `label_day` |

**Series** for `argon_chart` and `argon_spark`: a resource key (its stock, one sample per game day, the last 160), `<resource>.net` (its monthly net), `@frame_ms` (frame time) and
`@tick_rate` (turn ticks per second, measured by the host).

## Time

![The time page: a speed dial, the calendar and two charts](images/deck_time.jpg)

| Component | What it does | Parameters |
|---|---|---|
| `argon_speed_dial` | a dial of the game speed, the state under it, and the pause and speed buttons | `size`, `label_paused`, `label_running` |

The buttons act through the game's own setters, in the same way as its speed controls.

## Script

![The script page: effect cards with the engine's verdict, and a log](images/deck_script.jpg)

| Component | What it does | Parameters |
|---|---|---|
| `argon_effect_card` | a card for one `button_effect` of a mod: its key, a title and a description, a chip that says what the engine says (may run, or refused with the engine's own reason) and a run button. The effect is posted through the engine's command path, for the player country | `effect` (the key in `common/button_effects`), `title`, `desc`, `chip_ready`, `chip_refused`, `run` (the button's text), `height` |
| `argon_effect_log` | what the effect cards of this plugin sent, newest first, with how long ago | `empty` (text when nothing was sent), `limit` (default 6) |

## Settings

| Component | What it does | Parameters |
|---|---|---|
| `argon_theme_picker` | a row per theme; clicking one makes it the player's theme for every component and for other plugins that read it from the host | |
| `argon_switch` | a switch with a label. `state = stars` turns the starfield of `argon_window` on and off; `panel = "modid:panelid"` shows and hides that panel | `label`, `state` or `panel` |
| `argon_info` | a caption and a value | `item` = `imgui` (version), `context` (the shared ImGui context), `display` (screen size and layout scale), `draw` (what the frame renders), `host` (the host's interface version and the game build it was made for); `label` |

## A HUD capsule

The Command Deck's status bar is an ordinary `kind = hud` panel made of these parts. They are laid out in a `row`, inside a pill the size of the window.

| Component | What it is | Parameters |
|---|---|---|
| `argon_pill` | the capsule's background: a pill the size of the window with a soft shadow, a lit rim and the accent line under it. Put it **first** | |
| `argon_orb` | the orb at the left: it shows and hides a panel | `toggle` = `"modid:panelid"` |
| `argon_date` | the game date, with a small caption | `label`, `empty` (shown outside a game), `width` (px, default 168) |
| `argon_speed` | pause and the five speeds | `height` (px; the HUD's height less 30 by default) |
| `argon_vline` | a thin separator | |
| `argon_chip` | one resource: icon, stock and monthly net | `resource`, `width` (px, default 118) |

```
panel = {
    id = hud  title = MYMOD_HUD  kind = hud  anchor = bottom_center  offset = { 0 24 }  size = { 700 68 }
    content = {
        argon_pill = { }
        row = {
            argon_orb   = { toggle = "mymod:window" }
            argon_date  = { label = MYMOD_DATE  empty = MYMOD_NO_GAME }
            argon_speed = { }
            argon_vline = { }
            argon_chip  = { resource = energy }
            argon_chip  = { resource = minerals }
        }
    }
}
```

## The Command Deck

[`examples/command-deck`](../examples/command-deck) is a complete mod: a HUD capsule (`deck_hud.txt`) and a five-page window (`deck_window.txt`: overview, economy, time, script,
settings). Two text files and the localisation, no code. Its script page runs the effects of the
[stellaris-guiexpand-test-mod](https://github.com/Yidhar/stellaris-guiexpand-test-mod), so that mod has to be enabled for those four cards to be available.
[`examples/demo-mod`](../examples/demo-mod) is the small panel in the picture at the top.

![The overview page](images/deck_overview.jpg)

## Writing your own components

A component is a plugin that registers an element through the host's C interface. This library is built that way: `src/argon.cpp` registers every entry of the table in
`Components()`, and a component is a function that gets the node of its declaration. Start with the host's `examples/element` (plain C) and its
[developer guide](https://github.com/Yidhar/stellaris-guiexpand/blob/main/docs/developers.md); `src/components.cpp` here shows the layout helpers (`Region`, `CursorKeep`, the
parameter readers) that make a component work inside a card, a row or a tab.
