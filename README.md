[![Build Status](https://github.com/tsoding/sowon/workflows/CI/badge.svg)](https://github.com/tsoding/sowon/actions)

# Sowon

![demo](./demo.gif)

## Build

Dependencies: [SDL3](https://github.com/libsdl-org/SDL/releases/tag/release-3.2.0)

### Debian
```console
$ sudo apt-get install libsdl3-dev
$ make
```

### MacOS

```console
$ brew install sdl3 pkg-config
$ make
```

### Windows

#### Visual Studio

- Enter the Visual Studio Command Line Development Environment https://docs.microsoft.com/en-us/cpp/build/building-on-the-command-line
  - Basically just find `vcvarsall.bat` and run `vcvarsall.bat x64` inside of cmd
- Download [SDL3 VC Development Libraries](https://github.com/libsdl-org/SDL/releases/download/release-3.2.0/SDL3-devel-3.2.0-VC.zip) and copy it to `path\to\sowon`

```console
> cd path\to\sowon
> tar -xf SDL3-devel-3.2.0-VC.zip
> move SDL3-3.2.0 SDL3
> del SDL3-devel-3.2.0-VC.zip
> build_msvc
```

## Usage

### Modes

- Ascending mode: `./sowon`
- Descending mode: `./sowon <seconds>`
- Clock Mode: `./sowon clock`

### Flags

- Start in paused state: `./sowon -p <mode>`
- Exit sowon after countdown finished: `./sowon -e`

### Key bindings

| Key | Description |
| --- | --- |
| <kbd>ESC</kbd> | Quit |
| <kbd>SPACE</kbd> | Toggle pause |
| <kbd>=</kbd> | Zoom in |
| <kbd>-</kbd> | Zoom out |
| <kbd>0</kbd> | Zoom 100% |
| <kbd>F5</kbd> | Restart |
| <kbd>F11</kbd> | Fullscreen |
