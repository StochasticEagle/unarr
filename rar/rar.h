/* Copyright 2015 the unarr project authors (see AUTHORS file).
   License: LGPLv3 */

#ifndef rar_rar_h
#define rar_rar_h

#include "../common/unarr-imp.h"

/* Version-specific archive openers used by the common RAR dispatcher. */
ar_archive *rar4_open_archive(ar_stream *stream);

#endif
