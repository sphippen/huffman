#include "huff.h"

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <stdio.h>

int main(int argc, char* argv)
{
  uint8_t *original;
  int originalSize;
  int res;
  int i;
  int processed;
  uint8_t *encoded;
  int encodedSize;
  uint8_t *decoded;
  int decodedSize;

  HuffCounter counter;
  HuffEncoder encoder;
  HuffDecoder decoder;

  /* Load file */
  FILE* in = fopen("E:/Users/Andy/Downloads/bananastand.sdf", "rb");
  if (in == NULL)
    exit(1);

  originalSize = 2445312;
  original = malloc(originalSize);
  if (original == NULL)
    exit(1);

  res = fread(original, 1, originalSize, in);
  if (res != originalSize)
    exit(1);

  counter = HuffCounterInit();
  if (counter == NULL)
    exit(1);

  res = HuffCounterFeedData(counter, original, originalSize);
  if (res != HUFF_SUCCESS)
    exit(1);

  encoder = HuffEncoderInit(counter, 0);
  if (encoder == NULL)
    exit(1);

  res = HuffEncoderFeedData(encoder, original, originalSize, &processed);
  if (res != HUFF_SUCCESS || processed != originalSize)
    exit(1);

  res = HuffEncoderEndData(encoder);
  if (res != HUFF_SUCCESS)
    exit(1);

  encodedSize = HuffEncoderByteCount(encoder);

  encoded = malloc(encodedSize);
  if (encoded == NULL)
    exit(1);

  res = HuffEncoderWriteBytes(encoder, encoded, encodedSize);
  if (res != encodedSize)
    exit(1);

  decoder = HuffDecoderInit(0);
  if (decoder == NULL)
    exit(1);

  res = HuffDecoderFeedData(decoder, encoded, encodedSize, &processed);
  if (res != HUFF_SUCCESS || processed != encodedSize)
    exit(1);

  decodedSize = HuffDecoderByteCount(decoder);

  decoded = malloc(decodedSize);
  if (decoded == NULL)
    exit(1);

  res = HuffDecoderWriteBytes(decoder, decoded, decodedSize);
  if (res != decodedSize)
    exit(1);

  /* Verify that the data is the same after going through */
  /*if (decodedSize != originalSize)
    exit(1);

  for (i = 0; i < decodedSize; i++)
    if (original[i] != decoded[i])
      exit(1);*/
  
  HuffCounterDestroy(counter);
  HuffEncoderDestroy(encoder);
  HuffDecoderDestroy(decoder);
  free(original);
  free(encoded);
  free(decoded);

  printf("Good!\n");
  _CrtDumpMemoryLeaks();
  getchar();

  return 0;
}