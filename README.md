# AC.Syndicate.PatchFix

ASI plugin for Assassin's Creed Syndicate that unlocks region-locked languages.

## Features

- **Language unlock** — all languages available regardless of purchase region
- **UI language override** — force any language independent of system/registry settings
- **Hot-reload** — edit the INI file while the game is running, changes apply immediately

## Installation

### Prerequisites

An ASI loader is required. Install one of the following into the game directory:

- [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases)

### Steps

1. Download the [latest release](https://github.com/playday3008/AC.Syndicate-PatchFix/releases)
2. Place `AC.Syndicate.PatchFix.asi` and `AC.Syndicate.PatchFix.ini` into `<path-to-game>/plugins/`
3. Edit `AC.Syndicate.PatchFix.ini` to configure the fixes
4. Launch the game

### Steam Deck / Proton

After installing the ASI loader and patch files, add the following to the game's launch options:

```shell
WINEDLLOVERRIDES="version.dll=n,b" %command%
```

Replace `version.dll` with the actual DLL name used by your ASI loader.

## Configuration

### [Language]

| Key          | Default | Values | Description |
|--------------|---------| ------ | ----------- |
| `UnlockAll`  | `false` | `true`, `false` | Make all languages available regardless of purchase region. Requires language data files to be present. |
| `UILanguage` | `None`  | Language name or index `1`-`21` | Override UI language. |

### [Hooks]

| Key                | Default | Description |
|--------------------|---------| ----------- |
| `LanguageUnlock`   | `true`  | Language unlock and override |

## Building

See `CLAUDE.md` for build instructions.

## License

See `LICENSE`.
