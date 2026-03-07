/**
 * @file config.h
 * @author Vladyslav Aviedov <vladaviedov at protonmail dot com>
 * @version v2-pre0.1
 * @date 2024-2026
 * @license LGPLv3.0
 * @brief Configuration macros.
 */
#pragma once

/**
 * @def TERMINFO_DEBIAN
 * Enable searching Debian terminfo locations.
 */
#ifndef TERMINFO_DEBIAN
#define TERMINFO_DEBIAN 1
#endif // TERMINFO_DEBIAN

/**
 * @def TERMINFO_FREEBSD
 * Enable searching FreeBSD terminfo locations.
 */
#ifndef TERMINFO_FREEBSD
#define TERMINFO_FREEBSD 1
#endif // TERMINFO_FREEBSD

/**
 * @def TERMINFO_NETBSD
 * Enable searching NetBSD terminfo locations.
 */
#ifndef TERMINFO_NETBSD
#define TERMINFO_NETBSD 1
#endif // TERMINFO_NETBSD

/**
 * @def TERMINFO_COMMON
 * Enable searching common terminfo locations.
 *
 * These are observed on Arch Linux, MacOS, Fedora and others.
 */
#ifndef TERMINFO_COMMON
#define TERMINFO_COMMON 1
#endif // TERMINFO_COMMON

/**
 * @def DEBUG
 * Enable debugging print statements.
 *
 * Should not be used in releases.
 */
#ifndef DEBUG
#define DEBUG 0
#endif // DEBUG

/**
 * @def FASTLOAD
 * Enable precompiling terminfo data for some terminals.
 *
 * Skips searching the terminfo database entirely for supported terminals.
 * Supported terminals:
 * - xterm varieties
 */
#ifndef FASTLOAD
#define FASTLOAD 1
#endif // FASTLOAD

/**
 * @def CUSTOM_ESCAPES
 * Enable user-defined escape handlers.
 *
 * Allows library clients to define their own string manipulations for
 * some unused key codes.
 *
 * Example: unlike readline, nanorl does not implement an internal history
 * manager. Attaching custom handlers to the up and down arrow keys allows
 * history to be implemented on the client side.
 */
#ifndef CUSTOM_ESCAPES
#define CUSTOM_ESCAPES 1
#endif // CUSTOM_ESCAPES

/**
 * @def PATCH_XTERM_OOB_BACKSPACE
 * Enables a hidden Ctrl+H -> Backspace alias on xterm terminals.
 *
 * Some xterm terminals (including xterm itself) will send Ctrl+H on hitting the
 * backspace key for backwards compatibility with old Unixes. This archaism
 * breaks terminfo compilance causing backspace to break entirely in nanorl.
 *
 * This behavior can be disabled in the xterm configuration if the user chooses
 * to do so. Meanwhile, this patch fixes backspaces in out-of-the-box xterm.
 */
#ifndef PATCH_XTERM_OOB_BACKSPACE
#define PATCH_XTERM_OOB_BACKSPACE 1
#endif // PATCH_XTERM_OOB_BACKSPACE
