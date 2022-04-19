## Prerequisites

## Windows (msys2)

### Setting up msys2

Note that in msys2, Copy is Ctrl+Insert and Paste is Shift+Insert.

1. Open msys2 at C:\msys2\mingw64.exe or run `C:\msys2\msys2_shell.bat -mingw64`.

2. Certain packages are required to build pokefirered. Install these by running the following command:

    ```bash
    pacman -Syuu
    pacman -Suu
    pacman -Sy
    pacman -S base-devel development
    pacman -S make zlib-devel git mingw-w64-x86_64-gcc mingw-w64-x86_64-libpng mingw-w64-x86_64-arm-none-eabi-toolchain
    ```
    <details>
        <summary><i>Note...</i></summary>

    >   This command will ask for confirmation, just enter the yes action when prompted.
    </details>

If this works, then proceed to [Installation](#installation).

## Installation

To set up the repository:

    git clone https://github.com/pret/pokefirered
    git clone https://github.com/pret/agbcc

    cd ./agbcc
    sh build.sh
    sh install.sh ../pokefirered

    cd ../pokefirered

To build **pokefirered.gba**:

    make -j$(nproc)

To confirm it matches the official ROM image while building, do this instead:

    make compare -j$(nproc)

If only `.c` or `.s` files were changed, turn off the dependency scanning temporarily. Changes to any other files will be ignored and the build will either fail or not reflect those changes.

    make -j$(nproc) NODEP=1

Convenient targets have been defined to build Pokémon LeafGreen and the 1.1 revisions of both games:

    # FireRed 1.1
    make -j$(nproc) firered_rev1
    # LeafGreen 1.1
    make -j$(nproc) leafgreen_rev1

**Note:** If the build command is not recognized on Linux, including the Linux environment used within Windows, run `nproc` and replace `$(nproc)` with the returned value (e.g.: `make -j4`). Because `nproc` is not available on macOS, the alternative is `sysctl -n hw.ncpu`.

### Note for Mac users

The BSD make that comes with Mac XCode can be buggy, so obtain GNU make and sed using [Homebrew](https://brew.sh):

    brew install make gnu-sed

When compiling agbcc, substitute the `build.sh` line for

    gsed 's/^make/gmake/g' build.sh | sh

Finally, use `gmake` instead of `make` to compile the ROM(s).
