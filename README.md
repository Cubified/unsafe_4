## unsafe_4:  Wii Stack Smashing

This repository contains an example homebrew application which is vulnerable to a stack smashing/overflow exploit, as well as an exploit which is able to load some homebrew from an inserted SD card.  Although I have been unsuccessful in loading most homebrew (details below), this has been an excellent learning experience for me regardless.

The name is `unsafe_4` because this is the fourth iteration in a series of vulnerable example programs which I created with the intention of understanding stack overflow exploits.

### Compiling

This repository requires:

- A recent version of [DevkitPPC and libogc](https://devkitpro.org)
- Python 3
- Standard UNIX utilities (mkdir, echo, cat, sh, dd, rm, etc.)
- Make

For testing:

- [Dolphin](https://dolphin-emu.org)

If everything has been set up properly, running:

     $ make

Should produce:

     ./unsafe_4.dol        # The vulnerable program
     ./unsafe_4.elf        # The vulnerable program (with debug symbols, useful with Dolphin's debugger)
     ./exploit/exploit.gci # A Gamecube save file containing the homebrew loader

### Running

##### In Dolphin

To run this program in Dolphin, simply open either `unsafe_4.dol` or `unsafe_4.elf`.

To install the homebrew loader:

- Open Dolphin
- Click "Tools" -> "Memcard Manager (GC)"
- Click "Browse"
- Open "MemoryCardA.EUR.raw" (or potentially "MemoryCardA.USA.raw," my install defaults to the former)
- Click "<- Import GCI"
- Navigate to and open `unsafe_4/exploit/exploit.gci`
- Click "Close"

Alternatively, a GCI folder can be used to make installing a new save file much easier:

- Open Dolphin
- Click "Config"
- Click the "GameCube" tab
- Under "Device Settings," change "Slot A" to "GCI Folder"
- In the `unsafe_4/exploit` folder, running `make install` should copy and install the save file

To install homebrew to the virtual SD card, follow [this setup guide](https://wiki.dolphin-emu.org/index.php?title=Virtual_SD_Card_Guide) and copy a DOL file (with restrictions, see below) to `app.dol` in the root of the SD card's filesystem.

---

##### On a Wii

`unsafe_4` can be loaded however homebrew is normally loaded (wiiload, install on SD card and load via Homebrew Channel, USBLoader, etc.).

To install the homebrew loader:

- Copy `exploit.gci` to a SD card or USB stick which is compatible with the Wii
- Using GCMM or similar software, install the GCI file to the memory card in Slot A

To install homebrew to the SD card, copy a DOL file (with restrictions, see below) to `app.dol` in the root of the SD card's filesystem.

---

##### Expected Output

On either system, a successful run of `unsafe_4` should yield text to the screen which loads and prints the contents of the memory card in Slot A to the screen, after which the homebrew loader (assuming it is installed) hands off execution to the DOL file on the SD card.

### Unsupported Homebrew

Due to technical reasons of which I am not 100% certain, all homebrew which is larger than 256kb (as well as some homebrew smaller than this for unknown reasons) will fail to load.  My best guess as to why this occurs is that the Wii/Gamecube's Gekko processor has 256kb of L2 cache, meaning the loader would have to either swap out data to/from the cache while loading or disable the cache entirely (already tried, unsuccessful) to circumvent this.

### Technical Explanation

##### Vulnerability

The vulnerability of this program comes down to one line:

```c
char file_buf[4096];
unsigned int sec_size;
card_file cf;

CARD_Read(&cf, file_buf, sec_size, 0); /* !!! sec_size (likely 8192) > sizeof(file_buf) (4096) */
```

This line reads from a file on the memory card (`&cf`), writing `sec_size` bytes (most likely 8192) to a buffer which is 4096 bytes long (`file_buf`).  This lack of ensuring that the save file's size is not larger than that of the output buffer allows for a stack overflow exploit, as data controlling the execution of the program is stored shortly after the 4096 bytes allotted to `file_buf`.

Specifically, this image displays the stack structure on PowerPC:

![PowerPC Stack Layout](http://mirror.informatimago.com/next/developer.apple.com/documentation/mac/runtimehtml/graphics/RUN-58.jpg)

Notice on the right-hand side that "Local variables" and "Linkage area" are very close together -- overwriting the linkage area (specifically the _link register_, or the register responsible for storing the address to which execution should return upon the completion of a function) allows execution of arbitrary code as read from the save file.

##### Arbitrary Code

Although this vulnerability allows for arbitrary code execution, it is limited by the size of the stack for a given function, as any data stored after the newly-overwritten link register is inaccessible (as in, the stack is the only place where arbitrary code which will be executed can be written).  To circumvent this, the exploit in this repository uses the limited space to execute a bootstrap script which places the DOL loader into a larger chunk of non-volatile memory, then hands off execution to this loader.

##### Bootstrap

When `make` is run, the following occurs:

- The loader (`unsafe_4/exploit/loader`) is compiled
- Opcodes are stripped from the loader binary and written to an output file
- A Python script takes this output file and produces assembly source code (`unsafe_4/exploit/bootstrap/bootstrap.S`) which loads these opcodes into memory
- This assembly code is compiled into a binary
- Opcodes are stripped from the bootstrap's binary, then written to `exploit.gci` to be executed

The reason for this redundancy is that the operations done by the loader (namely, loading the DOL file into memory before relocating its individual sections) use and therefore overwrite the same memory in which the code itself exists, meaning relocating the loader code to a non-volatile region of memory before executing it ensures that it is free from corruption.

##### DOL File Format

[This](http://wiki.tockdom.com/wiki/DOL) is an excellent resource which contains the technical specification for a DOL file's 0x100-byte header, and this repository's loader is _heavily_ inspired by both [FIX94's](https://github.com/FIX94/gc-exploit-common-loader/blob/master/loader.c) and [ToadKing's](https://github.com/ToadKing/wii-loader/blob/master/source/dol.c)

### References/Further Reading

All of the following aided me in creating this project, either to gain understanding of the PowerPC architecture, or to see a proper implementation of what I was hoping to achieve.

- [PowerPC Stack Attacks, Christopher A. Shepherd](http://beefchunk.com/documentation/security/ppc-stack-1.html)
- [PowerPC Instruction Set](http://math-atlas.sourceforge.net/devel/assembly/ppc_isa.pdf)
- [PowerPC Stack Structure](http://mirror.informatimago.com/next/developer.apple.com/documentation/mac/runtimehtml/RTArch-59.html)
- [gc-exploit-common-loader, FIX94](https://github.com/FIX94/gc-exploit-common-loader)
- [wii-loader, ToadKing](https://github.com/ToadKing/wii-loader)
- [libogc, devkitPro Team](https://github.com/devkitPro/libogc)
- [Wii Savegame Exploits, Team Twiizers + Others](https://github.com/lewurm/savezelda)
