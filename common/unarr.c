/* Copyright 2015 the unarr project authors (see AUTHORS file).
   License: LGPLv3 */

#include "unarr.h"
#include "unarr-imp.h"
#include <stdint.h>

uint32_t ar_get_version(void)
{
    return UNARR_API_VERSION;
}

const char* ar_get_version_str(void)
{
    return UNARR_VERSION;
}

ar_archive *ar_open_archive(ar_stream *stream, size_t struct_size, ar_archive_close_fn close, ar_parse_entry_fn parse_entry,
                            ar_entry_get_name_fn get_name, ar_entry_uncompress_fn uncompress, ar_get_global_comment_fn get_comment,
                            off64_t first_entry_offset)
{
    ar_archive *ar = malloc(struct_size);
    if (!ar)
        return NULL;
    memset(ar, 0, struct_size);
    ar->close = close;
    ar->parse_entry = parse_entry;
    ar->get_name = get_name;
    ar->uncompress = uncompress;
    ar->get_comment = get_comment;
    ar->stream = stream;
    ar->entry_offset_first = first_entry_offset;
    ar->entry_offset_next = first_entry_offset;
    return ar;
}

void ar_close_archive(ar_archive *ar)
{
    if (ar)
        ar->close(ar);
    free(ar);
}

bool ar_at_eof(ar_archive *ar)
{
    return ar->at_eof;
}

bool ar_parse_entry(ar_archive *ar)
{
    bool result = ar->parse_entry(ar, ar->entry_offset_next);
    if (result)
        ar->entry_position = 0;
    return result;
}

bool ar_parse_entry_at(ar_archive *ar, off64_t offset)
{
    bool result;
    ar->at_eof = false;
    result = ar->parse_entry(ar, offset ? offset : ar->entry_offset_first);
    if (result)
        ar->entry_position = 0;
    return result;
}

bool ar_parse_entry_for(ar_archive *ar, const char *entry_name)
{
    ar->at_eof = false;
    if (!entry_name)
        return false;
    if (!ar_parse_entry_at(ar, ar->entry_offset_first))
        return false;
    do {
        const char *name = ar_entry_get_name(ar);
        if (name && strcmp(name, entry_name) == 0)
            return true;
    } while (ar_parse_entry(ar));
    return false;
}

const char *ar_entry_get_name(ar_archive *ar)
{
    return ar->get_name(ar, false);
}

const char *ar_entry_get_raw_name(ar_archive *ar)
{
    return ar->get_name(ar, true);
}

off64_t ar_entry_get_offset(ar_archive *ar)
{
    return ar->entry_offset;
}

size_t ar_entry_get_size(ar_archive *ar)
{
    return ar->entry_size_uncompressed;
}

bool ar_entry_is_directory(ar_archive *ar)
{
    return ar->entry_is_directory;
}

time64_t ar_entry_get_filetime(ar_archive *ar)
{
    return ar->entry_filetime;
}

bool ar_entry_uncompress(ar_archive *ar, void *buffer, size_t count)
{
    if (ar->entry_is_directory)
        return count == 0;
    return ar->uncompress(ar, buffer, count);
}

size_t ar_entry_read(ar_archive *ar, void *buffer, size_t count)
{
    size_t remaining;
    if (!ar || (!buffer && count) || ar->entry_position > ar->entry_size_uncompressed)
        return 0;
    remaining = ar->entry_size_uncompressed - ar->entry_position;
    if (count > remaining)
        count = remaining;
    if (!count)
        return 0;
    if (!ar->uncompress(ar, buffer, count))
        return 0;
    ar->entry_position += count;
    return count;
}

bool ar_entry_seek(ar_archive *ar, off64_t offset, int origin)
{
    off64_t target;
    unsigned char discard[4096];
    if (!ar)
        return false;
    if (origin == SEEK_SET)
        target = offset;
    else if (origin == SEEK_CUR)
        target = (off64_t)ar->entry_position + offset;
    else if (origin == SEEK_END)
        target = (off64_t)ar->entry_size_uncompressed + offset;
    else
        return false;
    if (target < 0 || (uint64_t)target > (uint64_t)ar->entry_size_uncompressed)
        return false;
    if ((size_t)target < ar->entry_position) {
        off64_t entry_offset = ar->entry_offset;
        if (!ar_parse_entry_at(ar, entry_offset) || ar->entry_offset != entry_offset)
            return false;
    }
    while (ar->entry_position < (size_t)target) {
        size_t count = (size_t)target - ar->entry_position;
        if (count > sizeof(discard))
            count = sizeof(discard);
        if (ar_entry_read(ar, discard, count) != count)
            return false;
    }
    return true;
}

off64_t ar_entry_tell(ar_archive *ar)
{
    return ar ? (off64_t)ar->entry_position : -1;
}

size_t ar_entry_size(ar_archive *ar)
{
    return ar ? ar->entry_size_uncompressed : 0;
}

size_t ar_get_global_comment(ar_archive *ar, void *buffer, size_t count)
{
    if (!ar->get_comment)
        return 0;
    return ar->get_comment(ar, buffer, count);
}

void ar_log(const char *prefix, const char *file, int line, const char *msg, ...)
{
    va_list args;
    va_start(args, msg);
    if (prefix)
        fprintf(stderr, "%s ", prefix);
    if (strrchr(file, '/'))
        file = strrchr(file, '/') + 1;
    if (strrchr(file, '\\'))
        file = strrchr(file, '\\') + 1;
    fprintf(stderr, "%s:%d: ", file, line);
    vfprintf(stderr, msg, args);
    fprintf(stderr, "\n");
    va_end(args);
}
