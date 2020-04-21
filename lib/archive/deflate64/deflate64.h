//
#ifndef BAULK_DEFLATE64_H
#define BAULK_DEFLATE64_H
#include <zlib.h>

#ifdef __cplusplus
extern "C" {
#endif

int inflateBack9(z_stream *strm, in_func in, void *in_desc, out_func out,
                 void *out_desc);
//int inflate9(z_streamp *strm, int flush); //
int inflateBack9End(z_stream *strm);
int inflateBack9Init_(z_stream *strm, unsigned char *window,
                      const char *version, int stream_size);
#define inflateBack9Init(strm, window)                                         \
  inflateBack9Init_((strm), (window), ZLIB_VERSION, sizeof(z_stream))

#ifdef __cplusplus
}
#endif

#endif
