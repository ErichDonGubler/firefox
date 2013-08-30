/* -*- Mode: C++; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef nsMIMEInfoUnix_h_
#define nsMIMEInfoUnix_h_

#include "nsMIMEInfoImpl.h"

class nsMIMEInfoUnix : public nsMIMEInfoImpl
{
public:
  nsMIMEInfoUnix(const char *aMIMEType = "") : nsMIMEInfoImpl(aMIMEType) {}
  nsMIMEInfoUnix(const nsACString& aMIMEType) : nsMIMEInfoImpl(aMIMEType) {}
  nsMIMEInfoUnix(const nsACString& aType, HandlerClass aClass) :
    nsMIMEInfoImpl(aType, aClass) {}
  static bool HandlerExists(const char *aProtocolScheme);

protected:
  NS_IMETHOD GetHasDefaultHandler(bool *_retval);

  virtual NS_HIDDEN_(nsresult) LoadUriInternal(nsIURI *aURI);

  virtual NS_HIDDEN_(nsresult) LaunchDefaultWithFile(nsIFile *aFile);
#if defined(MOZ_ENABLE_CONTENTACTION)
  NS_IMETHOD GetPossibleApplicationHandlers(nsIMutableArray * *aPossibleAppHandlers);
#endif
};

#endif // nsMIMEInfoUnix_h_
