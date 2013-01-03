/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 sw=2 et tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef nsIScriptTimeoutHandler_h___
#define nsIScriptTimeoutHandler_h___

#include "nsTArray.h"

namespace JS {
class Value;
} // namespace JS
namespace mozilla {
namespace dom {
class Function;
} // namespace dom
} // namespace mozilla

#define NS_ISCRIPTTIMEOUTHANDLER_IID \
{ 0x53c8e80e, 0xcc78, 0x48bc, \
 { 0xba, 0x63, 0x0c, 0xb9, 0xdb, 0xf7, 0x06, 0x34 } }

/**
 * Abstraction of the script objects etc required to do timeouts in a
 * language agnostic way.
 */

class nsIScriptTimeoutHandler : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCRIPTTIMEOUTHANDLER_IID)

  // Get the Function to call.  If this returns nullptr, GetHandlerText() will
  // be called to get the string.
  virtual mozilla::dom::Function *GetCallback() = 0;

  // Get the handler text of not a compiled object.
  virtual const PRUnichar *GetHandlerText() = 0;

  // Get the location of the script.
  // Note: The memory pointed to by aFileName is owned by the
  // nsIScriptTimeoutHandler and should not be freed by the caller.
  virtual void GetLocation(const char **aFileName, uint32_t *aLineNo) = 0;

  // If we have a Function, get the arguments for passing to it.
  virtual const nsTArray<JS::Value>& GetArgs() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScriptTimeoutHandler,
                              NS_ISCRIPTTIMEOUTHANDLER_IID)

#endif // nsIScriptTimeoutHandler_h___
