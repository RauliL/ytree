# ytree

ytree is a [DOS-XTREE(tm)](https://en.wikipedia.org/wiki/XTree) similar file
manager.

## Author

Werner Bregulla eMail: werner@frolix.han.de

Fork by Rauli Laine

## Platforms

POSIX systems (Linux, *BSD, macOS) with ncurses, and Windows via PDCursesMod
(disk browsing; archive browsing is not yet supported on Windows).

## Building

Requires a C++20 compiler and CMake 3.14+.

- On Unix: ncurses (default), or optionally PDCursesMod
- On Windows: PDCursesMod is fetched and built automatically

```sh
$ mkdir build
$ cd build
$ cmake ..
$ cmake --build .
$ sudo cmake --install .
$ ytree
```

Curses backend selection:

```sh
# default: ncurses on Unix, PDCursesMod on Windows
cmake -DYTREE_CURSES_BACKEND=auto ..

# force ncurses
cmake -DYTREE_CURSES_BACKEND=ncurses ..

# force PDCursesMod (VT port on Unix, wincon on Windows)
cmake -DYTREE_CURSES_BACKEND=pdcurses ..
```

For customizing ytree edit ytree.toml and copy it to
`$XDG_CONFIG_HOME/ytree/config.toml` (typically `~/.config/ytree/config.toml`).
For using the "QuitTo" feature you have to add a bash wrapper to
your ~/.bashrc. See the man page for details.

## Copyright

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
