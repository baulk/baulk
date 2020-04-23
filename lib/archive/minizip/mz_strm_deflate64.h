/* mz_strm_deflate64.h -- Stream for zlib inflate64/deflate64
*/

#ifndef MZ_STREAM_DEFALTE64_H
#define MZ_STREAM_DEFALTE64_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************/

int32_t mz_stream_deflate64_open(void *stream, const char *filename, int32_t mode);
int32_t mz_stream_deflate64_is_open(void *stream);
int32_t mz_stream_deflate64_read(void *stream, void *buf, int32_t size);
int32_t mz_stream_deflate64_write(void *stream, const void *buf, int32_t size);
int64_t mz_stream_deflate64_tell(void *stream);
int32_t mz_stream_deflate64_seek(void *stream, int64_t offset, int32_t origin);
int32_t mz_stream_deflate64_close(void *stream);
int32_t mz_stream_deflate64_error(void *stream);

int32_t mz_stream_deflate64_get_prop_int64(void *stream, int32_t prop, int64_t *value);
int32_t mz_stream_deflate64_set_prop_int64(void *stream, int32_t prop, int64_t value);

void*   mz_stream_deflate64_create(void **stream);
void    mz_stream_deflate64_delete(void **stream);

void*   mz_stream_deflate64_get_interface(void);

/***************************************************************************/

#ifdef __cplusplus
}
#endif

#endif