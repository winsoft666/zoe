/*******************************************************************************
*    Copyright (C) <2019-2022>, winsoft666, <winsoft666@outlook.com>.
*
*    This program is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "verbose.h"
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <strsafe.h>
#else
#include <string>
#include <stdarg.h>
#endif
#include "string_encode.h"

namespace teemo {

void OutputVerbose(VerboseOuputFunctor functor, const char* fmt, ...) {
  if (!fmt)
    return;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
  char* pMsgBuffer = NULL;
  unsigned int iMsgBufCount = 0;

  va_list arglist;
  va_start(arglist, fmt);
  HRESULT hr = STRSAFE_E_INSUFFICIENT_BUFFER;

  while (hr == STRSAFE_E_INSUFFICIENT_BUFFER) {
    iMsgBufCount += 1024;

    if (pMsgBuffer) {
      free(pMsgBuffer);
      pMsgBuffer = NULL;
    }

    pMsgBuffer = (char*)malloc(iMsgBufCount * sizeof(char));

    if (!pMsgBuffer) {
      break;
    }

    hr = StringCchVPrintfA(pMsgBuffer, iMsgBufCount, fmt, arglist);
  }

  va_end(arglist);

  if (hr == S_OK) {
    if (pMsgBuffer) {
      OutputDebugStringW(Utf8ToUnicode(pMsgBuffer).c_str());
      if (functor) {
        functor(pMsgBuffer);
      }
    }
  }

  if (pMsgBuffer) {
    free(pMsgBuffer);
    pMsgBuffer = NULL;
  }
#else
  if (functor) {
    char msgBuf[1024] = {0};
    va_list arglist;
    va_start(arglist, fmt);
    int err = vsnprintf(msgBuf, 1024, fmt, arglist);
    va_end(arglist);

    functor(msgBuf);
  }
#endif
}

}  // namespace teemo