#ifndef TEEMO_MD5_MAKER_34DFDR7_H__
#define TEEMO_MD5_MAKER_34DFDR7_H__
#pragma once

#include "teemo/teemo.h"

namespace TEEMO_NAMESPACE {
typedef struct _Options Options;

namespace libmd5_internal {
typedef unsigned int UWORD32;
typedef unsigned char md5byte;

struct MD5Context {
  UWORD32 buf[4];
  UWORD32 bytes[2];
  UWORD32 in[16];
};

void MD5Init(struct MD5Context* context);
void MD5Update(struct MD5Context* context, md5byte const* buf, unsigned len);
void MD5Final(md5byte digest[16], struct MD5Context* context);
void MD5Buffer(const unsigned char* buf, unsigned int len, unsigned char sig[16]);
void MD5SigToString(unsigned char sig[16], char* str, int len);
}  // namespace libmd5_internal

// Helper function.
Result CalculateFileMd5(const utf8string& file_path, Options* opt, utf8string& str_hash);
Result CalculateFileMd5(FILE* f, Options* opt, utf8string& str_hash);
}  // namespace teemo

#endif