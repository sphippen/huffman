#ifndef HUFF_H
#define HUFF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define HUFF_SUCCESS 0
#define HUFF_NOMEM -1
#define HUFF_TOOMUCHDATA -2
#define HUFF_BADLENGTH -3
#define HUFF_BADOBJ -4
#define HUFF_UNKNOWNERR -5

struct huff_freqdic_;
typedef struct huff_freqdic_ *huff_freqdic;
struct huff_encoder_;
typedef struct huff_encoder_ *huff_encoder;
struct huff_decoder_;
typedef struct huff_decoder_ *huff_decoder;

int huff_freqdic_init(huff_freqdic *dic);
int huff_freqdic_destroy(huff_freqdic *dic);
int huff_freqdic_feeddata(huff_freqdic *dic, const uint8_t* data, int length);

int huff_encoder_init(huff_encoder *encoder, huff_freqdic *dic);
int huff_encoder_destroy(huff_encoder *encoder);
int huff_encoder_feeddata(huff_encoder *encoder, const uint8_t* data, int length);
int huff_encoder_enddata(huff_encoder* encoder);
int huff_encoder_bytestowrite(huff_encoder *encoder);
int huff_encoder_writebytes(huff_encoder *encoder, uint8_t* buf, int length);

int huff_decoder_init(huff_decoder *decoder);
int huff_decoder_destroy(huff_decoder *decoder);
int huff_decoder_feeddata(huff_decoder *decoder, const uint8_t* data, int length);
int huff_decoder_bytestowrite(huff_decoder *decoder);
int huff_decoder_writebytes(huff_decoder *decoder, uint8_t* buf, int length);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // HUFF_H
