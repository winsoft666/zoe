/*
	---------------------------------------------------------------------------
	Copyright (c) 2003, Dominik Reichl <dominik.reichl@t-online.de>, Germany.
	All rights reserved.

	Distributed under the terms of the GNU General Public License v2.

	This software is provided 'as is' with no explicit or implied warranties
	in respect of its properties, including, but not limited to, correctness 
	and/or fitness for purpose.
	---------------------------------------------------------------------------
*/

#ifndef ___CRC32_H___
#define ___CRC32_H___

#include <stdlib.h>
#include <stdint.h>
#include "teemo/teemo.h"

namespace teemo {
  namespace crc32_internal {
    void crc32Init(uint32_t *pCrc32);
    void crc32Update(uint32_t *pCrc32, unsigned char *pData, uint32_t uSize);
    void crc32Finish(uint32_t *pCrc32);
  }

  utf8string CalculateFileCRC32(const utf8string &file_path);
}
#endif /* ___CRC32_H___ */
