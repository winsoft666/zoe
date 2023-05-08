#ifndef ___SHA1_H___
#define ___SHA1_H___

#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "libGet/libGet.h"

namespace LIBGET_NAMESPACE {

typedef struct _Options Options;

class CSHA1 {
 public:
  enum { REPORT_HEX = 0, REPORT_DIGIT = 1 };
  CSHA1();
  virtual ~CSHA1();

  uint32_t m_state[5];
  uint32_t m_count[2];
  unsigned char m_buffer[64];
  unsigned char m_digest[20];

  void Reset();

  void Update(unsigned char* data, unsigned int len);
  bool HashFile(char* szFileName);

  void Final();
  void ReportHash(char* szReport, unsigned char uReportType = REPORT_HEX);
  void GetHash(unsigned char* uDest);

 private:
  void Transform(uint32_t state[5], unsigned char buffer[64]);
};

Result CalculateFileSHA1(const utf8string& file_path, Options* opt, utf8string& str_hash);
Result CalculateFileSHA1(FILE* f, Options* opt, utf8string& str_hash);
}  // namespace libGet

#endif