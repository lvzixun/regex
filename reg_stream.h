#include <stddef.h>

struct reg_stream;

struct reg_stream* stream_new(const unsigned char* str, size_t size);
void stream_free(struct reg_stream* p);

inline unsigned char stream_char(struct reg_stream* p);
inline int stream_end(struct reg_stream* p);
inline unsigned char stream_next(struct reg_stream* p);
inline size_t stream_pos(struct reg_stream* p);