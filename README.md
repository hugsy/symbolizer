# Symbolizer - A fast execution trace symbolizer for Windows
![Builds](https://github.com/0vercl0k/symbolizer/workflows/Builds/badge.svg)

![grep](https://github.com/hugsy/symbolizer/assets/590234/76ca8d44-5f76-43e7-9f59-20fdb0b60033)

## Overview

This is utility that symbolizes an execution trace via the [dbgeng](https://docs.microsoft.com/en-us/windows-hardware/drivers/debugger/debugger-engine-api-overview) APIs and a crash-dump. The dbgeng APIs are used to load the crash-dump and symbolizes RIP values by querying the available symbols.

Transform the below raw execution trace:

```
0xfffff8053b9ca5c0
0xfffff8053b9ca5c1
0xfffff8053b9ca5c8
0xfffff8053b9ca5d0
0xfffff8053b9ca5d4
0xfffff8053b9ca5d8
0xfffff8053b9ca5dc
0xfffff8053b9ca5e0
```

Into a full symbolized trace:

```
nt!KiPageFault+0x0
nt!KiPageFault+0x1
nt!KiPageFault+0x8
nt!KiPageFault+0x10
nt!KiPageFault+0x14
nt!KiPageFault+0x18
nt!KiPageFault+0x1c
nt!KiPageFault+0x20
```

Or a `mod+offset` trace to load it into [Lighthouse](https://github.com/gaasedelen/lighthouse) for code-coverage exploration:

```
nt+0x1ca5c0
nt+0x1ca5c1
nt+0x1ca5c8
nt+0x1ca5d0
nt+0x1ca5d4
nt+0x1ca5d8
nt+0x1ca5dc
nt+0x1ca5e0
nt+0x1ca5e4
nt+0x1ca5e8
```

_Note_: Recently added minidump types (0x09, 0x0a and possibly more) will fail to load as no COM interface was provided by Microsoft (yet?) to interact with them.

## Usage

In order for symbolizer to work you need to place `dbghelp.dll` as well as `symsrv.dll` in the directory of the symbolizer executable. Symbolizer will copy the two files if they are found in the default Windows SDK's Debuggers install location: `c:\Program Files (x86)\Windows Kits\10\Debuggers\<arch>`.

```
Symbolizer - A fast execution trace symbolizer for Windows
Usage: src\x64\Release\symbolizer.exe [OPTIONS]

Options:
  -h,--help                   Print this help message and exit
  --help-all                  Expand all help
  -i,--input TEXT:PATH(existing) REQUIRED
                              Input trace file or directory
  -c,--crash-dump TEXT:FILE REQUIRED
                              Crash-dump path
  -o,--output TEXT            Output trace (default: stdout)
  -s,--skip UINT=0            Skip a number of lines
  -m,--max UINT=20000000      Stop after a number of lines
  --style ENUM:value in {modoff->0,fullsym->1} OR {0,1}=fullsym
                              Trace style
  --overwrite=0               Overwrite the output file if necessary
  --line-numbers=0            Include line numbers
```

### Batch mode

The batch mode is designed to symbolize an entire directory filled with execution traces. You can turn on batch mode by simply specifying a directory for the `--input` command line option and an output directory for the `--output` option.

![batch](https://github.com/hugsy/symbolizer/assets/590234/239d46b1-ac65-41d2-8794-c10683da9280)


### Single file mode

As opposed to batch mode, you might be interested in just symbolizing a single trace file which in this case you can specify a file path via the `--input` command line option.

![single](https://github.com/hugsy/symbolizer/assets/590234/a81da779-1a1a-4acf-9597-aaa28c93ab85)


## Build

Clone the repository with:

```
git clone https://github.com/0vercl0k/symbolizer.git
```

You can build with `cmake` (the minimum version is 3.20):

```
cmake -S ./symbolizer -B ./build
cmake --build ./build --config RelWithDebInfo
```

You can install it as such:

```
cmake --install ./build --config RelWithDebInfo --prefix /my/install/path
```

## Python Bindings

The easiest way to build & install the Python bindings is with `pip`. Python bindings are supported from Python 3.8 and newer.

```
python -m pip install /path/to/symbolizer/ --user --upgrade
```

### Usage

#### Import

```py
>>> import symbolizer
```

#### Load a Dump File

```py
>>> sym = symbolizer.DebugEngine('c:/temp/win10x64_bmp.dmp')
```

Optionally a symbol path can be passed to constructor:

```py
>>> sym = symbolizer.DebugEngine('c:/temp/win10x64_bmp.dmp', 'z:/symbols')
```


#### Symbolize an address

```py
>>> addr = 0xfffff8065bc231b0
>>> sym = sym.symbolize(dmp.context.Rip)
>>> print(f"{addr:#x} -> {sym}")
0xfffff8065bc231b0 -> nt!DbgBreakPointWithStatus+0x0
```

#### Resolve a symbol

```py
>>> sym = "nt!PsLoadedModuleList"
>>> addr = sym.resolve(sym)
>>> print(f"{sym} -> {addr:#x}")
nt!PsLoadedModuleList -> 0xfffff8065c4482d0
```
