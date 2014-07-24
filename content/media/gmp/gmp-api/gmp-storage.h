/*
* Copyright 2013, Mozilla Foundation and contributors
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef GMP_STORAGE_h_
#define GMP_STORAGE_h_

#include "gmp-errors.h"
#include <stdint.h>

// Provides basic per-origin storage for CDMs. GMPRecord instances can be
// retrieved by calling GMPPlatformAPI->openstorage. Multiple GMPRecords
// with different names can be open at once, but a single record can only
// be opened by one client at a time. This interface is asynchronous, with
// results being returned via callbacks to the GMPRecordClient pointer
// provided to the GMPPlatformAPI->openstorage call, on the main thread.
class GMPRecord {
public:

  // Opens the record. Calls OpenComplete() once the record is open.
  // Note: OpenComplete() is only called if this returns GMPNoErr.
  virtual GMPErr Open() = 0;

  // Reads the entire contents of the record, and calls
  // GMPRecordClient::ReadComplete() once the operation is complete.
  // Note: ReadComplete() is only called if this returns GMPNoErr.
  virtual GMPErr Read() = 0;

  // Writes aDataSize bytes of aData into the record, overwriting the
  // contents of the record. Overwriting with 0 bytes "deletes" the file.
  // Note: WriteComplete is only called if this returns GMPNoErr.
  virtual GMPErr Write(const uint8_t* aData, uint32_t aDataSize) = 0;

  // Closes a record. GMPRecord object must not be used after this is
  // called, request a new one with GMPPlatformAPI->openstorage to re-open
  // this record. Cancels all callbacks.
  virtual GMPErr Close() = 0;

  virtual ~GMPRecord() {}
};

// Callback object that receives the results of GMPRecord calls. Callbacks
// run asynchronously to the GMPRecord call, on the main thread.
class GMPRecordClient {
 public:

  // Response to a GMPRecord::Open() call with the open |status|.
  // aStatus values:
  // - GMPNoErr - Record opened successfully. Record may be empty.
  // - GMPRecordInUse - This record is in use by another client.
  // - GMPGenericErr - Unspecified error.
  // Do not use the GMPRecord if aStatus is not GMPNoErr.
  virtual void OpenComplete(GMPErr aStatus) = 0;

  // Response to a GMPRecord::Read() call, where aData is the record contents,
  // of length aDataSize.
  // aData is only valid for the duration of the call to ReadComplete.
  // Copy it if you want to hang onto it!
  // aStatus values:
  // - GMPNoErr - Record contents read successfully, aDataSize 0 means record
  //   is empty.
  // - GMPRecordInUse - There are other operations or clients in use on
  //   this record.
  // - GMPGenericErr - Unspecified error.
  // Do not continue to use the GMPRecord if aStatus is not GMPNoErr.
  virtual void ReadComplete(GMPErr aStatus,
                            const uint8_t* aData,
                            uint32_t aDataSize) = 0;

  // Response to a GMPRecord::Write() call.
  // - GMPNoErr - File contents written successfully.
  // - GMPRecordInUse - There are other operations or clients in use on
  //   this record.
  // - GMPGenericErr - Unspecified error.
  // Do not continue to use the GMPRecord if aStatus is not GMPNoErr.
  virtual void WriteComplete(GMPErr aStatus) = 0;

  virtual ~GMPRecordClient() {}
};

#endif // GMP_STORAGE_h_
