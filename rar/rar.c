/* Copyright 2015 the unarr project authors (see AUTHORS file).
   License: LGPLv3 */

#include "rar.h"

#define RAR_SIGNATURE_PREFIX_SIZE 7

ar_archive *ar_open_rar_archive(ar_stream *stream)
{
    char signature[RAR_SIGNATURE_PREFIX_SIZE];

    if (!ar_seek(stream, 0, SEEK_SET))
        return NULL;
    if (ar_read(stream, signature, sizeof(signature)) != sizeof(signature))
        return NULL;

    if (memcmp(signature, "Rar!\x1A\x07\x00", sizeof(signature)) == 0)
        return rar4_open_archive(stream);

    if (memcmp(signature, "Rar!\x1A\x07\x01", sizeof(signature)) == 0)
        warn("RAR 5 format isn't supported");
    else if (memcmp(signature, "RE~^", 4) == 0)
        warn("Ancient RAR format isn't supported");
    else if (memcmp(signature, "MZ", 2) == 0 || memcmp(signature, "\x7F\x45LF", 4) == 0)
        warn("SFX archives aren't supported");

    return NULL;
}
