# ytree

ytree is a [DOS-XTREE(tm)](https://en.wikipedia.org/wiki/XTree) similar file
manager.

## Author

Werner Bregulla eMail: werner@frolix.han.de

Fork by Rauli Laine

## Platforms

Any kind of POSIX compatible system I guess.

## Building

In most instances, it should be sufficient to

```sh
$ mkdir build
$ cd build
$ cmake ..
$ build
$ sudo make install
$ ytree
```

For customizing ytree edit ytree.conf and copy it to $HOME/.ytree
For using the "QuitTo" feature you have to add a bash wrapper to
your ~/.bashrc. See the man page for details.

## Copyright

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
