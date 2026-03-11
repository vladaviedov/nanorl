# nanorl

`nanorl` (nano readline) is a small line editing library. Unlike `readline`
(GPL) or `libedit` (BSD), `nanorl` does not (internally) provide history,
completions or editor-specific options.

`nanorl` is written for POSIX 2008 and relies on the curses terminfo database,
although not curses itself.

## Features

`nanorl` natively implements the following interactions:

- Standard text input.
- Backspace & delete.
- Arrow navigation.
- Home & end keys.

Additionally, `nanorl` is capable of:

- Multi-line input.
- Unicode.
- Data preloading (editing existing test).
- Input obfuscation.
- Receiving input from a pipe.
- Custom escape code handler callbacks.

## Requirements

### Build

- C99-capable tool chain.
- GNU make.

### Development

- `clang-format` (auto-format code)
- `doxygen` (generate documentation).

## Configuration

`nanorl` provides build-time configuration macros. These can be adjusted inside
[src/config.h](src/config.h) or passed on the command line into the
`CFLAGS_CONFIG` make variable (i.e. `make debug CFLAGS_CONFIG="-DDEBUG=1"` to
enable `DEBUG`).

|Macro|Description|Default|
|---|---|---|
|`TERMINFO_DEBIAN`|Include Debian-specific terminfo paths|Yes|
|`TERMINFO_FREEBSD`|Include FreeBSD-specific terminfo paths|Yes|
|`TERMINFO_NETBSD`|Include NetBSD-specific terminfo paths|Yes|
|`TERMINFO_COMMON`|Include standard terminfo paths|Yes|
|`DEBUG`|Prints terminfo-related information at runtime|No|
|`FASTLOAD`|Precompile terminfo fragments for common terminals (currently xterm)|Yes|
|`CUSTOM_ESCAPES`|Enable custom escape handling functionality|Yes|
|`PATCH_XTERM_OOB_BACKSPACE`|Patch backspace to work in xterm out-of-the-box (see `config.h` for more info)|Yes|

## Build

```
git submodule update --init
make
```

### Development

- `make release` - Build release binary (same as make).
- `make debug` - Build binary with debug symbols.
- `make clean` - Remove build files.
- `make checkformat` - Check for code formatting errors.
- `make format` - Reformat the code.
- `make docs` - Generate API documentation.
- `make fulldocs` - Generate full documentation.

## Usage

Normal usage:

```c
nrl_config config = nrl_default_config();
config.prompt = "my prompt: ";

nrl_error error;
char *input = nanorl(&config, &error);

// Check error state
// Use input
free(input);
```

Simplified usage:

```c
char *input = nrl_readline("my prompt: ");
// Use input
free(input);
```

For more examples, see [here](./example/nrl_example.c)
