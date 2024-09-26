/**
 * @cond internal
 * @file config.h
 * @author Vladyslav Aviedov <vladaviedov at protonmail dot com>
 * @version v2-pre0.1
 * @date 2024
 * @license LGPLv3.0
 * @brief Configuration macros.
 */
#pragma once

#ifndef TERMINFO_DEBIAN
#define TERMINFO_DEBIAN 1
#endif // TERMINFO_DEBIAN

#ifndef TERMINFO_FREEBSD
#define TERMINFO_FREEBSD 1
#endif // TERMINFO_FREEBSD

#ifndef TERMINFO_NETBSD
#define TERMINFO_NETBSD 1
#endif // TERMINFO_NETBSD

#ifndef TERMINFO_COMMON
#define TERMINFO_COMMON 1
#endif // TERMINFO_COMMON

#ifndef DFA_DEBUG
#define DFA_DEBUG 0
#endif // DFA_DEBUG

// @endcond
