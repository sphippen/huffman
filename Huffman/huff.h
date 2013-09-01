#ifndef HUFF_H
#define HUFF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define HUFF_SUCCESS 0
#define HUFF_NOMEM -1
#define HUFF_TOOMUCHDATA -2

struct HuffCounter_;
typedef struct HuffCounter_ *HuffCounter;
struct HuffEncoder_;
typedef struct HuffEncoder_ *HuffEncoder;
struct HuffDecoder_;
typedef struct HuffDecoder_ *HuffDecoder;

HuffCounter HuffCounterInit(void);
HuffCounter HuffCounterCopy(HuffCounter from);
void HuffCounterDestroy(HuffCounter counter);
int HuffCounterFeedData(HuffCounter counter, const uint8_t *data, int length);

HuffEncoder HuffEncoderInit(HuffCounter counter, int initialBufferSize);
void HuffEncoderDestroy(HuffEncoder encoder);
int HuffEncoderFeedData(HuffEncoder encoder, const uint8_t *data, int length, int *processed);
int HuffEncoderEndData(HuffEncoder encoder);
int HuffEncoderByteCount(HuffEncoder encoder);
int HuffEncoderWriteBytes(HuffEncoder encoder, uint8_t *buf, int length);

HuffDecoder HuffDecoderInit(int initialBufferSize);
void HuffDecoderDestroy(HuffDecoder decoder);
int HuffDecoderFeedData(HuffDecoder decoder, const uint8_t *data, int length);
int HuffDecoderByteCount(HuffDecoder decoder);
int HuffDecoderWriteBytes(HuffDecoder decoder, uint8_t *buf, int length);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* HUFF_H */
