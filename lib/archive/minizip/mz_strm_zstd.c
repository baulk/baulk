/// zstd
#include "mz.h"
#include "mz_strm.h"
#include "mz_strm_zstd.h"

#include <zstd.h>

static mz_stream_vtbl mz_stream_zstd_vtbl = {
    mz_stream_zstd_open,
    mz_stream_zstd_is_open,
    mz_stream_zstd_read,
    mz_stream_zstd_write,
    mz_stream_zstd_tell,
    mz_stream_zstd_seek,
    mz_stream_zstd_close,
    mz_stream_zstd_error,
    mz_stream_zstd_create,
    mz_stream_zstd_delete,
    mz_stream_zstd_get_prop_int64,
    mz_stream_zstd_set_prop_int64
};

/***************************************************************************/

typedef struct mz_stream_zstd_s {
    mz_stream   stream;
    ZSTD_CCtx   *zstream;
    int32_t     mode;
    int32_t     error;
    uint8_t     buffer[INT16_MAX];
    int32_t     buffer_len;
    int64_t     total_in;
    int64_t     total_out;
    int64_t     max_total_in;
    int64_t     max_total_out;
    int8_t      initialized;
    uint32_t    preset;
} mz_stream_zstd;

/***************************************************************************/

int32_t mz_stream_zstd_open(void *stream, const char *path, int32_t mode)
{
 
    return MZ_OK;
}

int32_t mz_stream_zstd_is_open(void *stream)
{
    return MZ_OK;
}

int32_t mz_stream_zstd_read(void *stream, void *buf, int32_t size)
{
    return MZ_OK;
}


int32_t mz_stream_zstd_write(void *stream, const void *buf, int32_t size)
{
    return MZ_OK;
}

int64_t mz_stream_zstd_tell(void *stream)
{
    MZ_UNUSED(stream);

    return MZ_TELL_ERROR;
}

int32_t mz_stream_zstd_seek(void *stream, int64_t offset, int32_t origin)
{
    MZ_UNUSED(stream);
    MZ_UNUSED(offset);
    MZ_UNUSED(origin);

    return MZ_SEEK_ERROR;
}

int32_t mz_stream_zstd_close(void *stream)
{
    return MZ_OK;
}

int32_t mz_stream_zstd_error(void *stream)
{
    return MZ_OK;
}

int32_t mz_stream_zstd_get_prop_int64(void *stream, int32_t prop, int64_t *value)
{
    return MZ_OK;
}

int32_t mz_stream_zstd_set_prop_int64(void *stream, int32_t prop, int64_t value)
{
    return MZ_OK;
}

void *mz_stream_zstd_create(void **stream)
{
    return NULL;
}

void mz_stream_zstd_delete(void **stream)
{
    *stream = NULL;
}

void *mz_stream_zstd_get_interface(void)
{
    return (void *)&mz_stream_zstd_vtbl;
}
