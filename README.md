# kommando

cli library for c23

## features

### argument parsing
- declarative
- subcommands
- gnu style (`-xvzf`, `-vvv`, `--key=value`)
- non-intrusive and flexible

## building

clone the repo and build with meson, no deps.

```sh
git clone https://github.com/chkc0x0/kommando
cd kommando
meson setup build
ninja -C build

```

or add it as a dependency/subproject

## license

unlicense, do whatever