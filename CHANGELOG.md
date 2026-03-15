# Changelog

## 2.0.1

**Bug fixes**:

- Fixed buffer underflow during cursor position requests on hardware terminals
- Removed unused terminfo entries and variables

## 2.0.0

- *meta*: Split from c-utils
- *meta*: Complete refactor
- Example code!

**Breaking API changes**:

- `NRL_LIB_VER` macro changed to `char *nrl_version`
- Changed `nrl_error` enum values
- Changed `nrl_echo_mode` enum values
- `nrl_opts` replaced with `nrl_config`
- Changed `nanorl` function signature
- Removed `nanorl_fd` and `nanorl_opts` (not needed with new config system)
- Removed option to provide echo replacement character

**New features**:

- Expanded configuration: file descriptors, smkx skip
- Provide `nrl_readline` / `readline` function signature matching GNU readline
- Input preloading (editing)
- Unicode conversion
- Custom escape sequence handler API
- Terminal resize concious multi-line input support

**Bug fixes**:

- Added workaround for xterm's backspace problem
- Fixed input misalignment for non-ASCII characters (from unicode support)

## 1.2.2

**Bug fixes**:

- Corrected typo in xterm fastload fragment
- Fixed segmentation faults with unsupported terminal capabilities

## 1.2.1

**Bug fixes**:

- Fixed escape key problems
- Handle end-of-file (Ctrl+D) correctly

## 1.2r

- *c-utils/meta*: Re-tag with build system updates
- *c-utils/meta*: No debug builds prior to this version

## 1.2

**New features**:

- Support deleting with the 'Delete' key
- More database paths (FreeBSD, NetBSD)

**Bug fixes**:

- `DEBIAN_DIRS` is now used in the code correctly

## 1.1

**New features**:

- Rework of input acquisition
- Unused escape sequences are printed in a readable format
- Improved support for non-tty input

**Bug Fixes**:

- Pasted text is now processed

## 1.0

- Initial release
