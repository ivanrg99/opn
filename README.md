# Description

xdg-open and mimeopen alternative. Read the man page for more information.

# Sample usage

`opn -d mpv ~/Downloads/movie.mp4`

`opn ~/Downloads/movie.mp4`

# Building

Just clone the repo and type `make install`. You might need to change the
Makefile to suit your needs and overwrite defaults.

# Dependencies

Just libmagic. For building: GNU Make, clang and pkg-config, although it would be trivial
to change.
