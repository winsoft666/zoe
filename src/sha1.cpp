#include "sha1.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "file_util.h"
#include "options.h"

namespace zoe {

namespace {
typedef union {
  unsigned char c[64];
  uint32_t l[16];
} SHA1_WORKSPACE_BLOCK;
}  // namespace

#define ROL32(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

// Detect compiler is for x86 or x64.
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#define CPU_X86 1
#endif

// Detect compiler is for arm.
#if defined(__arm__) || defined(_M_ARM)
#define CPU_ARM 1
#endif

#if defined(CPU_X86) && defined(CPU_ARM)
#error CPU_X86 and CPU_ARM both defined.
#endif

#if !defined(ARCH_CPU_BIG_ENDIAN) && !defined(ARCH_CPU_LITTLE_ENDIAN)
// x86, arm or GCC provided __BYTE_ORDER__ macros
#if defined(CPU_X86) || defined(CPU_ARM) || \
    (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define ARCH_CPU_LITTLE_ENDIAN
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define ARCH_CPU_BIG_ENDIAN
#else
#error ARCH_CPU_BIG_ENDIAN or ARCH_CPU_LITTLE_ENDIAN should be defined.
#endif
#endif

#if defined(ARCH_CPU_BIG_ENDIAN) && defined(ARCH_CPU_LITTLE_ENDIAN)
#error ARCH_CPU_BIG_ENDIAN and ARCH_CPU_LITTLE_ENDIAN both defined.
#endif

#if (defined ARCH_CPU_LITTLE_ENDIAN) || (defined LITTLE_ENDIAN)
#define SHABLK0(i) \
  (block->l[i] = (ROL32(block->l[i], 24) & 0xFF00FF00) | (ROL32(block->l[i], 8) & 0x00FF00FF))
#else
#define SHABLK0(i) (block->l[i])
#endif

#define SHABLK(i)                                                              \
  (block->l[i & 15] = ROL32(block->l[(i + 13) & 15] ^ block->l[(i + 8) & 15] ^ \
                                block->l[(i + 2) & 15] ^ block->l[i & 15],     \
                            1))

#define R0(v, w, x, y, z, i)                                          \
  {                                                                   \
    z += ((w & (x ^ y)) ^ y) + SHABLK0(i) + 0x5A827999 + ROL32(v, 5); \
    w = ROL32(w, 30);                                                 \
  }
#define R1(v, w, x, y, z, i)                                         \
  {                                                                  \
    z += ((w & (x ^ y)) ^ y) + SHABLK(i) + 0x5A827999 + ROL32(v, 5); \
    w = ROL32(w, 30);                                                \
  }
#define R2(v, w, x, y, z, i)                                 \
  {                                                          \
    z += (w ^ x ^ y) + SHABLK(i) + 0x6ED9EBA1 + ROL32(v, 5); \
    w = ROL32(w, 30);                                        \
  }
#define R3(v, w, x, y, z, i)                                               \
  {                                                                        \
    z += (((w | x) & y) | (w & x)) + SHABLK(i) + 0x8F1BBCDC + ROL32(v, 5); \
    w = ROL32(w, 30);                                                      \
  }
#define R4(v, w, x, y, z, i)                                 \
  {                                                          \
    z += (w ^ x ^ y) + SHABLK(i) + 0xCA62C1D6 + ROL32(v, 5); \
    w = ROL32(w, 30);                                        \
  }

CSHA1::CSHA1() {
  Reset();
}

CSHA1::~CSHA1() {
  Reset();
}

void CSHA1::Reset() {
  // SHA1 initialization constants
  m_state[0] = 0x67452301;
  m_state[1] = 0xEFCDAB89;
  m_state[2] = 0x98BADCFE;
  m_state[3] = 0x10325476;
  m_state[4] = 0xC3D2E1F0;

  m_count[0] = 0;
  m_count[1] = 0;
}

void CSHA1::Transform(uint32_t state[5], unsigned char buffer[64]) {
  uint32_t a = 0, b = 0, c = 0, d = 0, e = 0;

  SHA1_WORKSPACE_BLOCK* block;
  static unsigned char workspace[64];
  block = (SHA1_WORKSPACE_BLOCK*)workspace;
  memcpy(block, buffer, 64);

  // Copy state[] to working vars
  a = state[0];
  b = state[1];
  c = state[2];
  d = state[3];
  e = state[4];

  // 4 rounds of 20 operations each. Loop unrolled.
  R0(a, b, c, d, e, 0);
  R0(e, a, b, c, d, 1);
  R0(d, e, a, b, c, 2);
  R0(c, d, e, a, b, 3);
  R0(b, c, d, e, a, 4);
  R0(a, b, c, d, e, 5);
  R0(e, a, b, c, d, 6);
  R0(d, e, a, b, c, 7);
  R0(c, d, e, a, b, 8);
  R0(b, c, d, e, a, 9);
  R0(a, b, c, d, e, 10);
  R0(e, a, b, c, d, 11);
  R0(d, e, a, b, c, 12);
  R0(c, d, e, a, b, 13);
  R0(b, c, d, e, a, 14);
  R0(a, b, c, d, e, 15);
  R1(e, a, b, c, d, 16);
  R1(d, e, a, b, c, 17);
  R1(c, d, e, a, b, 18);
  R1(b, c, d, e, a, 19);
  R2(a, b, c, d, e, 20);
  R2(e, a, b, c, d, 21);
  R2(d, e, a, b, c, 22);
  R2(c, d, e, a, b, 23);
  R2(b, c, d, e, a, 24);
  R2(a, b, c, d, e, 25);
  R2(e, a, b, c, d, 26);
  R2(d, e, a, b, c, 27);
  R2(c, d, e, a, b, 28);
  R2(b, c, d, e, a, 29);
  R2(a, b, c, d, e, 30);
  R2(e, a, b, c, d, 31);
  R2(d, e, a, b, c, 32);
  R2(c, d, e, a, b, 33);
  R2(b, c, d, e, a, 34);
  R2(a, b, c, d, e, 35);
  R2(e, a, b, c, d, 36);
  R2(d, e, a, b, c, 37);
  R2(c, d, e, a, b, 38);
  R2(b, c, d, e, a, 39);
  R3(a, b, c, d, e, 40);
  R3(e, a, b, c, d, 41);
  R3(d, e, a, b, c, 42);
  R3(c, d, e, a, b, 43);
  R3(b, c, d, e, a, 44);
  R3(a, b, c, d, e, 45);
  R3(e, a, b, c, d, 46);
  R3(d, e, a, b, c, 47);
  R3(c, d, e, a, b, 48);
  R3(b, c, d, e, a, 49);
  R3(a, b, c, d, e, 50);
  R3(e, a, b, c, d, 51);
  R3(d, e, a, b, c, 52);
  R3(c, d, e, a, b, 53);
  R3(b, c, d, e, a, 54);
  R3(a, b, c, d, e, 55);
  R3(e, a, b, c, d, 56);
  R3(d, e, a, b, c, 57);
  R3(c, d, e, a, b, 58);
  R3(b, c, d, e, a, 59);
  R4(a, b, c, d, e, 60);
  R4(e, a, b, c, d, 61);
  R4(d, e, a, b, c, 62);
  R4(c, d, e, a, b, 63);
  R4(b, c, d, e, a, 64);
  R4(a, b, c, d, e, 65);
  R4(e, a, b, c, d, 66);
  R4(d, e, a, b, c, 67);
  R4(c, d, e, a, b, 68);
  R4(b, c, d, e, a, 69);
  R4(a, b, c, d, e, 70);
  R4(e, a, b, c, d, 71);
  R4(d, e, a, b, c, 72);
  R4(c, d, e, a, b, 73);
  R4(b, c, d, e, a, 74);
  R4(a, b, c, d, e, 75);
  R4(e, a, b, c, d, 76);
  R4(d, e, a, b, c, 77);
  R4(c, d, e, a, b, 78);
  R4(b, c, d, e, a, 79);

  // Add the working vars back into state[]
  state[0] += a;
  state[1] += b;
  state[2] += c;
  state[3] += d;
  state[4] += e;

  // Wipe variables
  a = 0;
  b = 0;
  c = 0;
  d = 0;
  e = 0;
}

// Use this function to hash in binary data and strings
void CSHA1::Update(unsigned char* data, unsigned int len) {
  uint32_t i = 0, j = 0;

  j = (m_count[0] >> 3) & 63;

  if ((m_count[0] += len << 3) < (len << 3))
    m_count[1]++;

  m_count[1] += (len >> 29);

  if ((j + len) > 63) {
    memcpy(&m_buffer[j], data, (i = 64 - j));
    Transform(m_state, m_buffer);

    for (; i + 63 < len; i += 64) {
      Transform(m_state, &data[i]);
    }

    j = 0;
  }
  else
    i = 0;

  memcpy(&m_buffer[j], &data[i], len - i);
}

// Hash in file contents
bool CSHA1::HashFile(char* szFileName) {
#define MAX_FILE_READ_BUFFER 8000
  uint32_t ulFileSize = 0, ulRest = 0, ulBlocks = 0;
  uint32_t i = 0;
  unsigned char uData[MAX_FILE_READ_BUFFER];
  FILE* fIn = NULL;

  if ((fIn = fopen(szFileName, "rb")) == NULL)
    return (false);

  fseek(fIn, 0, SEEK_END);
  ulFileSize = ftell(fIn);
  fseek(fIn, 0, SEEK_SET);

  ulRest = ulFileSize % MAX_FILE_READ_BUFFER;
  ulBlocks = ulFileSize / MAX_FILE_READ_BUFFER;

  for (i = 0; i < ulBlocks; i++) {
    fread(uData, 1, MAX_FILE_READ_BUFFER, fIn);
    Update(uData, MAX_FILE_READ_BUFFER);
  }

  if (ulRest != 0) {
    fread(uData, 1, ulRest, fIn);
    Update(uData, ulRest);
  }

  fclose(fIn);
  fIn = NULL;

  return (true);
}

void CSHA1::Final() {
  uint32_t i = 0, j = 0;
  unsigned char finalcount[8] = {0, 0, 0, 0, 0, 0, 0, 0};

  for (i = 0; i < 8; i++)
    finalcount[i] = (unsigned char)((m_count[(i >= 4 ? 0 : 1)] >> ((3 - (i & 3)) * 8)) &
                                    255);  // Endian independent

  Update((unsigned char*)"\200", 1);

  while ((m_count[0] & 504) != 448)
    Update((unsigned char*)"\0", 1);

  Update(finalcount, 8);  // Cause a SHA1Transform()

  for (i = 0; i < 20; i++) {
    m_digest[i] = (unsigned char)((m_state[i >> 2] >> ((3 - (i & 3)) * 8)) & 255);
  }

  // Wipe variables for security reasons
  i = 0;
  j = 0;
  memset(m_buffer, 0, 64);
  memset(m_state, 0, 20);
  memset(m_count, 0, 8);
  memset(finalcount, 0, 8);

  Transform(m_state, m_buffer);
}

// Get the final hash as a pre-formatted string
void CSHA1::ReportHash(char* szReport, unsigned char uReportType) {
  unsigned char i = 0;

  if (uReportType == REPORT_HEX) {
    sprintf(szReport, "%02x", m_digest[0]);

    for (i = 1; i < 20; i++)
      sprintf(szReport, "%s%02x", szReport, m_digest[i]);
  }
  else if (uReportType == REPORT_DIGIT) {
    sprintf(szReport, "%u", m_digest[0]);

    for (i = 1; i < 20; i++) {
      sprintf(szReport, "%s%u", szReport, m_digest[i]);
    }
  }
}

// Get the raw message digest
void CSHA1::GetHash(unsigned char* uDest) {
  unsigned char i = 0;

  for (i = 0; i < 20; i++)
    uDest[i] = m_digest[i];
}

ZoeResult CalculateFileSHA1(const utf8string& file_path, Options* opt, utf8string& str_hash) {
  FILE* f = FileUtil::Open(file_path, "rb");
  if (!f)
    return ZoeResult::CALCULATE_HASH_FAILED;

  CSHA1 sha1;
  sha1.Reset();

  size_t dwReadBytes = 0;
  unsigned char szData[1024] = {0};

  while ((dwReadBytes = fread(szData, 1, 1024, f)) > 0) {
    if (opt && (opt->internal_stop_event.isSetted() || (opt->user_stop_event && opt->user_stop_event->isSetted()))) {
      fclose(f);
      return ZoeResult::CANCELED;
    }
    sha1.Update(szData, dwReadBytes);
  }
  fclose(f);

  sha1.Final();

  char szSHA1[256] = {0};
  sha1.ReportHash(szSHA1, CSHA1::REPORT_HEX);

  str_hash = szSHA1;

  return ZoeResult::SUCCESSED;
}

ZoeResult CalculateFileSHA1(FILE* f, Options* opt, utf8string& str_hash) {
  if (!f)
    return ZoeResult::CALCULATE_HASH_FAILED;

  FileUtil::Seek(f, 0L, SEEK_SET);

  CSHA1 sha1;
  sha1.Reset();

  size_t dwReadBytes = 0;
  unsigned char szData[1024] = {0};

  while ((dwReadBytes = fread(szData, 1, 1024, f)) > 0) {
    if (opt && (opt->internal_stop_event.isSetted() ||
                (opt->user_stop_event && opt->user_stop_event->isSetted()))) {
      return ZoeResult::CANCELED;
    }
    sha1.Update(szData, dwReadBytes);
  }

  sha1.Final();

  char szSHA1[256] = {0};
  sha1.ReportHash(szSHA1, CSHA1::REPORT_HEX);

  str_hash = szSHA1;

  return ZoeResult::SUCCESSED;
}
}  // namespace zoe