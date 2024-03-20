# b173c

b173c is a Minecraft client which aims to be fully compatible with Minecraft Beta 1.7.3 servers, file formats etc. Features very high customizability along with advanced scripting capabilities.

## Requirements

Required packages:
`pkgconf sdl2 sdl2_net zlib python3 python-pillow` (naming for Arch Linux)

## Building

b173c can run on both Linux and Windows, but can only be built from Linux [(see the related issue)](https://github.com/krizej/b173c/issues/5)
  
Run `make all` in the project's root directory. The executable should be placed at `build/b173c`

# Linux
`CC=gcc make all`
The executable should be at build/b173c

# Cross-compile from Linux to Windows
`CC=x86_64-w64-mingw32-gcc make all`
The executable should be at build/b173c.exe
Additionally you can run `x86_64-w64-mingw32-strip build/b173c.exe` to reduce the size of the binary.

# Windows
No instructions yet. Grab the latest binary from [here](https://github.com/krizej/b173c/actions).

## Usage

- soon™

## License

[GNU GPLv3](LICENSE) © [krizej](https://github.com/krizej)
