# Rauli's ytree fork

This is my personal fork of [YTree], which is an open source clone of [XTree]
file manager for *nix systems originally created by Werner Bregulla.

[YTree]: https://www.han.de/~werner/ytree.html
[XTree]: https://en.wikipedia.org/wiki/XTree

New features include:

- Cleaned up [C++20] codebase. (Work in progress.)
- Mouse support.
- [TOML] configuration file.
- [Gruvbox] colorscheme.
- Support for [7zip], [xz]/lzma and [zstd] archives.
- [GTK] based GUI. (Very very work in progress.)

[C++20]: https://fi.wikipedia.org/wiki/C++20
[TOML]: https://en.wikipedia.org/wiki/TOML
[Gruvbox]: https://github.com/morhetz/gruvbox
[7zip]: https://www.7-zip.org/
[xz]: https://tukaani.org/xz/
[zstd]: https://facebook.github.io/zstd/
[GTK]: https://www.gtk.org/

## Building

Requires a C++20 compiler and CMake 3.32+. The terminal UI is drawn with
[libterm](https://github.com/rizukirr/libterm), which CMake fetches while
configuring.

In most instances, it should be sufficient to:

```shell
git clone https://github.com/RauliL/ytree.git
cd ytree
cmake -S . -B build
cmake --build build
sudo cmake --install build
```

### Optional GTK frontend

An experimental GTKmm 4 UI (`ytree-gtk`) shares the same core and directory/file
controllers as the curses binary. Enable it with:

```shell
cmake -S . -B build -DWITH_GTK=ON
cmake --build build
```

This requires `gtkmm-4.0` development packages (for example `libgtkmm-4.0-dev`
on Debian/Ubuntu).

For customizing ytree edit `ytree.toml` and copy it to
`$XDG_CONFIG_HOME/ytree/config.toml` (typically `~/.config/ytree/config.toml`).
For using the "QuitTo" feature you have to add a bash wrapper to
your `~/.bashrc`. See the man page for details.

## Copyright

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
