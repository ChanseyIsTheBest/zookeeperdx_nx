/* config.h -- Chaos Rings 3 Switch wrapper configuration
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

// The engine + libc++ + huge mvgl page cache need a generous newlib heap; the
// rest of system memory is handed to the .so loader (see __libnx_initheap).
#define MEMORY_MB 768

// mmap arena. Unity reserves big *256MB-aligned* pools by over-mmapping a larger
// region then munmapping the unaligned head/tail to keep an aligned middle. A
// plain malloc/free-per-mmap frees the WHOLE block when the head is trimmed and
// corrupts the kept middle -- which is the TLSF "next_free == NULL" allocator
// crash. We instead back anonymous mmaps from a dedicated, 256MB-aligned arena
// with a per-page used-bitmap (carved in __libnx_initheap) so sub-range munmap
// frees exactly the trimmed pages. Big requests are handed back 256MB-aligned so
// Unity only ever trims the (reusable) tail.
#define MMAP_ARENA_ALIGN    ((size_t)64 * 1024 * 1024)    // libunity.so patched 256MB->64MB region granularity (see unitypatch/); regions are 64MB-aligned
#define MMAP_ARENA_RESERVE  ((size_t)1792 * 1024 * 1024)  // heap-backed cap; now 28x64MB blocks (was 7x256MB), same bytes

// Stack-region overcommit (OC) arena (see libc_shim.c). svcMapMemory can only
// alias into the 2048MB stack region, so we hold the big PROT_NONE reservations
// in a stack-region window and back the (~80MB) committed pages from a small
// heap bump-pool. Heap-backed arena (1792MB) + OC window (1280MB) = 12x256MB
// blocks of reservation address space for Unity, but only ~80MB real commits.
#define OC_WINDOW_BYTES     ((size_t)1536 * 1024 * 1024)  // 24x64MB blocks of cheap PROT_NONE reservation in the stack hole
#define OC_POOL_BYTES       ((size_t) 256 * 1024 * 1024)  // commit-pool: only touched pages backed (~80MB observed) + headroom

// Overcommit (alias-region) mode: Unity reserves multi-GB of 256MB pools with
// mmap(PROT_NONE) and commits only the sub-ranges it touches via mprotect. On a
// fully-backed heap arena every reservation costs real RAM (9 pools == the whole
// arena, starving il2cpp). When svcMapPhysicalMemory + a large alias region are
// available we instead reserve a big *virtual* window there (PROT_NONE costs only
// address space) and commit physical pages on demand -- true overcommit, matching
// Android. This is the virtual window size (cheap; the alias region is tens of GB).
#define MMAP_VIRT_RESERVE   ((size_t)6144 * 1024 * 1024)  // 6 GB virtual reservation window
// Heap kept (svcSetHeapSize) in overcommit mode: newlib malloc + .so load zone.
// The rest of the physical limit is freed for on-demand commits. 2MB-aligned.
#define OVERCOMMIT_HEAP_MB  608u

// Chaos Rings 3 (Android, com.square_enix.chaosrings3gp v1.1.4) ships the real
// engine as libcrx.so (Media.Vision "MVGL"). Unlike FF4 it is a NativeActivity
// game (android_main / ANativeActivity_onCreate) and pulls its C++ runtime from
// libc++_shared.so, so the wrapper loads BOTH shared objects.
#define SO_NAME      "libcrx.so"
#define SO_CPP_NAME  "libc++_shared.so"

// the main game archive (an APK asset). The "10007" is the APK versionCode.
#define MAIN_MVGL    "main.10007.android.mvgl"

#define CONFIG_NAME "config.txt"
#define LOG_NAME    "sdmc:/switch/zookeeper/debug.log"

// Returned for getenv("HOME")/getpwuid()->pw_dir. Unity computes a home/cache dir
// during engine init; our env has no HOME and no passwd db, so point it at the
// (writable) game data root instead of letting it deref a NULL passwd.
#define GAME_HOME   "sdmc:/switch/zookeeper"

// flip to 1 (and build) to get file logging (debug.log) for on-hardware debugging
#define DEBUG_LOG 0

extern int screen_width;
extern int screen_height;

// Language. CR3 (Google Play) only has English + Japanese text; any other value
// falls back to English. 0 = follow the Switch system language (Japanese -> ja,
// otherwise en).
#define LANG_AUTO 0
#define LANG_JA   1
#define LANG_EN   2

typedef struct {
  int screen_width;
  int screen_height;
  int language;
  int portrait;    /* TATE rotation: 1 = ROT90 CW (default, right Joy-Con up),
                      2 = ROT270 CCW (left Joy-Con up), 0 = none (stretch) */
} Config;

extern Config config;

int read_config(const char *file);
int write_config(const char *file);

#endif
