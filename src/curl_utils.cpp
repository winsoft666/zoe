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

#include "curl_utils.h"
#include "teemo/config.h"
#include "curl/curl.h"
#ifdef WITH_OPENSSL
#include <openssl/crypto.h>
#endif

namespace TEEMO_NAMESPACE {
namespace {
int THREAD_setup(void);
int THREAD_cleanup(void);
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#define MUTEX_TYPE HANDLE
#define MUTEX_SETUP(x) (x) = CreateMutex(NULL, FALSE, NULL)
#define MUTEX_CLEANUP(x) CloseHandle(x)
#define MUTEX_LOCK(x) WaitForSingleObject((x), INFINITE)
#define MUTEX_UNLOCK(x) ReleaseMutex(x)
#define THREAD_ID GetCurrentThreadId()
#else
#define MUTEX_TYPE pthread_mutex_t
#define MUTEX_SETUP(x) pthread_mutex_init(&(x), NULL)
#define MUTEX_CLEANUP(x) pthread_mutex_destroy(&(x))
#define MUTEX_LOCK(x) pthread_mutex_lock(&(x))
#define MUTEX_UNLOCK(x) pthread_mutex_unlock(&(x))
#define THREAD_ID pthread_self()
#endif

#ifdef WITH_OPENSSL
static MUTEX_TYPE* mutex_buf = NULL;
static void locking_function(int mode, int n, const char* file, int line) {
  if (mode & CRYPTO_LOCK)
    MUTEX_LOCK(mutex_buf[n]);
  else
    MUTEX_UNLOCK(mutex_buf[n]);
}

static unsigned long id_function(void) {
  return ((unsigned long)THREAD_ID);
}

int THREAD_setup(void) {
  int i;
  mutex_buf = (MUTEX_TYPE*)malloc(CRYPTO_num_locks() * sizeof(MUTEX_TYPE));
  if (!mutex_buf)
    return 0;
  for (i = 0; i < CRYPTO_num_locks(); i++)
    MUTEX_SETUP(mutex_buf[i]);
  CRYPTO_set_id_callback(id_function);
  CRYPTO_set_locking_callback(locking_function);
  return 1;
}

int THREAD_cleanup(void) {
  int i;
  if (!mutex_buf)
    return 0;
  CRYPTO_set_id_callback(NULL);
  CRYPTO_set_locking_callback(NULL);
  for (i = 0; i < CRYPTO_num_locks(); i++)
    MUTEX_CLEANUP(mutex_buf[i]);
  free(mutex_buf);
  mutex_buf = NULL;
  return 1;
}
#endif
}  // namespace

void GlobalCurlInit() {
#ifdef WITH_OPENSSL
  THREAD_setup();
#endif
  curl_global_init(CURL_GLOBAL_ALL);
}

void GlobalCurlUnInit() {
  curl_global_cleanup();
#ifdef WITH_OPENSSL
  THREAD_cleanup();
#endif
}
}  // namespace teemo
