/// zstd
#include "mz.h"
#include "mz_strm.h"
#include "mz_strm_zstd.h"
#include <stdio.h>

#include <zstd.h>

static mz_stream_vtbl mz_stream_zstd_vtbl = {
    // zstd vtbl
    mz_stream_zstd_open,           //
    mz_stream_zstd_is_open,        //
    mz_stream_zstd_read,           //
    mz_stream_zstd_write,          //
    mz_stream_zstd_tell,           //
    mz_stream_zstd_seek,           //
    mz_stream_zstd_close,          //
    mz_stream_zstd_error,          //
    mz_stream_zstd_create,         //
    mz_stream_zstd_delete,         //
    mz_stream_zstd_get_prop_int64, //
    mz_stream_zstd_set_prop_int64  //
};

/***************************************************************************/

typedef struct mz_stream_zstd_s {
  mz_stream stream;
  ZSTD_CStream *zcstream;
  ZSTD_DStream *zdstream;
  ZSTD_outBuffer out;
  ZSTD_inBuffer in;
  int32_t mode;
  int32_t error;
  uint8_t buffer[INT16_MAX];
  int32_t buffer_len;
  int64_t total_in;
  int64_t total_out;
  int64_t max_total_in;
  int64_t max_total_out;
  int8_t initialized;
  uint32_t preset;
} mz_stream_zstd;

/***************************************************************************/

int32_t mz_stream_zstd_open(void *stream, const char *path, int32_t mode) {
  mz_stream_zstd *zstd = (mz_stream_zstd *)stream;
  MZ_UNUSED(path);
  if (mode & MZ_OPEN_MODE_WRITE) {
    zstd->zcstream = ZSTD_createCStream();
    zstd->out.dst = zstd->buffer;
    zstd->out.pos = 0;
    zstd->out.size = sizeof(zstd->buffer);
  } else if (mode & MZ_OPEN_MODE_READ) {
    zstd->zdstream = ZSTD_createDStream();
    memset(&zstd->out, 0, sizeof(ZSTD_outBuffer));
  }
  memset(&zstd->in, 0, sizeof(ZSTD_inBuffer));
  zstd->initialized = 1;
  zstd->mode = mode;
  zstd->error = MZ_OK;
  return MZ_OK;
}

int32_t mz_stream_zstd_is_open(void *stream) {
  mz_stream_zstd *zstd = (mz_stream_zstd *)stream;
  if (zstd->initialized != 1)
    return MZ_OPEN_ERROR;
  return MZ_OK;
}

// zstd read stream
int32_t mz_stream_zstd_read(void *stream, void *buf, int32_t size) {
  mz_stream_zstd *zstd = (mz_stream_zstd *)stream;
  uint32_t out_bytes = 0;
  int32_t bytes_to_read = sizeof(zstd->buffer);
  size_t before_pos = 0;
  size_t beforce_in = 0;
  int32_t read = 0;
  zstd->out.pos = 0;
  zstd->out.dst = buf;
  zstd->out.size = size;
  size_t result = 0;
  do {
    // read data from compressed file
    if (zstd->in.pos == zstd->in.size) {
      if (zstd->max_total_in > 0) {
        if ((int64_t)bytes_to_read > (zstd->max_total_in - zstd->total_in))
          bytes_to_read = (int32_t)(zstd->max_total_in - zstd->total_in);
      }
      read = mz_stream_read(zstd->stream.base, zstd->buffer, bytes_to_read);
      if (read < 0)
        return read;
      zstd->in.src = zstd->buffer;
      zstd->in.size = read;
      zstd->in.pos = 0;
    }

    before_pos = zstd->out.pos;
    beforce_in = zstd->in.pos;
    result = ZSTD_decompressStream(zstd->zdstream, &zstd->out, &zstd->in);
    if (ZSTD_isError(result)) {
      // need read buffer again
      return 0;
    }
    out_bytes += zstd->out.pos - before_pos;
    zstd->total_in += zstd->in.pos - beforce_in;
    zstd->total_out += out_bytes;
  } while (zstd->out.pos < zstd->out.size && result > 0);

  return out_bytes;
}

int32_t mz_stream_zstd_write(void *stream, const void *buf, int32_t size) {
  mz_stream_zstd *zstd = (mz_stream_zstd *)stream;
  zstd->in.src = buf;
  zstd->in.pos = 0;
  zstd->in.size = size;
  do {
    zstd->out.dst = (char *)zstd->buffer;
    zstd->out.size = sizeof(zstd->buffer);
    zstd->out.pos = 0;
    size_t result = ZSTD_compressStream(zstd->zcstream, &zstd->out, &zstd->in);
    if (ZSTD_isError(result)) {
      return MZ_STREAM_ERROR;
    }
    if (mz_stream_write(zstd->stream.base, zstd->out.dst, zstd->out.pos) != zstd->out.pos) {
      return MZ_WRITE_ERROR;
    }
    zstd->total_in += zstd->out.pos;
  } while (zstd->in.pos < zstd->in.size);
  return size;
}

int64_t mz_stream_zstd_tell(void *stream) {
  MZ_UNUSED(stream);

  return MZ_TELL_ERROR;
}

int32_t mz_stream_zstd_seek(void *stream, int64_t offset, int32_t origin) {
  MZ_UNUSED(stream);
  MZ_UNUSED(offset);
  MZ_UNUSED(origin);

  return MZ_SEEK_ERROR;
}

int32_t mz_stream_zstd_close(void *stream) {
  mz_stream_zstd *zstd = (mz_stream_zstd *)stream;
  if (zstd->mode & MZ_OPEN_MODE_WRITE) {
    ZSTD_freeCStream(zstd->zcstream);
    zstd->zcstream = NULL;
  } else if (zstd->mode & MZ_OPEN_MODE_READ) {
    ZSTD_freeDStream(zstd->zdstream);
    zstd->zdstream = NULL;
  }
  zstd->initialized = 0;
  return MZ_OK;
}

int32_t mz_stream_zstd_error(void *stream) {
  mz_stream_zstd *zstd = (mz_stream_zstd *)stream;
  printf("%d\n", zstd->error);
  return zstd->error;
}

int32_t mz_stream_zstd_get_prop_int64(void *stream, int32_t prop, int64_t *value) {
  mz_stream_zstd *zstd = (mz_stream_zstd *)stream;
  switch (prop) {
  case MZ_STREAM_PROP_TOTAL_IN:
    *value = zstd->total_in;
    break;
  case MZ_STREAM_PROP_TOTAL_IN_MAX:
    *value = zstd->max_total_in;
    break;
  case MZ_STREAM_PROP_TOTAL_OUT:
    *value = zstd->total_out;
    break;
  case MZ_STREAM_PROP_TOTAL_OUT_MAX:
    *value = zstd->max_total_out;
    break;
  case MZ_STREAM_PROP_HEADER_SIZE:
    *value = 0;
    break;
  default:
    return MZ_EXIST_ERROR;
  }
  return MZ_OK;
}

int32_t mz_stream_zstd_set_prop_int64(void *stream, int32_t prop, int64_t value) {
  mz_stream_zstd *zstd = (mz_stream_zstd *)stream;
  switch (prop) {
  case MZ_STREAM_PROP_COMPRESS_LEVEL:
    if (value < 0)
      zstd->preset = 6;
    else
      zstd->preset = (int16_t)value;
    return MZ_OK;
  case MZ_STREAM_PROP_TOTAL_IN_MAX:
    zstd->max_total_in = value;
    return MZ_OK;
  }
  return MZ_EXIST_ERROR;
}

void *mz_stream_zstd_create(void **stream) {
  mz_stream_zstd *zstd = NULL;
  zstd = (mz_stream_zstd *)MZ_ALLOC(sizeof(mz_stream_zstd));
  if (zstd != NULL) {
    memset(zstd, 0, sizeof(mz_stream_zstd));
    zstd->stream.vtbl = &mz_stream_zstd_vtbl;
  }
  if (stream != NULL)
    *stream = zstd;
  return zstd;
}

void mz_stream_zstd_delete(void **stream) {
  mz_stream_zstd *zstd = NULL;
  if (stream == NULL)
    return;
  zstd = (mz_stream_zstd *)*stream;
  if (zstd != NULL)
    MZ_FREE(zstd);
  *stream = NULL;
}

void *mz_stream_zstd_get_interface(void) {
  //
  return (void *)&mz_stream_zstd_vtbl;
}
