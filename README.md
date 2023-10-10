# ffremux

## Requirements:
`libavformat`, `libavcodec`, `libavutil` (part of FFmpeg)
Development packages for these libraries are needed to build the code.

## Building
Build with the Makefile:
```
make ffremux
```
or compile directly with:
```
$ cc -g -o ffremux ffremux.c -lavformat -lavutil -lavcodec
```
