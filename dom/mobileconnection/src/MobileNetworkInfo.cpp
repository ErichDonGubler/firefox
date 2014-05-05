/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "MobileNetworkInfo.h"

using namespace mozilla::dom;

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(MobileNetworkInfo, mWindow)

NS_IMPL_CYCLE_COLLECTING_ADDREF(MobileNetworkInfo)
NS_IMPL_CYCLE_COLLECTING_RELEASE(MobileNetworkInfo)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(MobileNetworkInfo)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIMobileNetworkInfo)
NS_INTERFACE_MAP_END

MobileNetworkInfo::MobileNetworkInfo(nsPIDOMWindow* aWindow)
  : mWindow(aWindow)
{
  SetIsDOMBinding();
}

void
MobileNetworkInfo::Update(nsIMobileNetworkInfo* aInfo)
{
  if (!aInfo) {
    return;
  }

  aInfo->GetShortName(mShortName);
  aInfo->GetLongName(mLongName);
  aInfo->GetMcc(mMcc);
  aInfo->GetMnc(mMnc);
  aInfo->GetState(mState);
}

JSObject*
MobileNetworkInfo::WrapObject(JSContext* aCx)
{
  return MozMobileNetworkInfoBinding::Wrap(aCx, this);
}

// WebIDL interface

/* static */ already_AddRefed<MobileNetworkInfo>
MobileNetworkInfo::Constructor(const GlobalObject& aGlobal,
                               const nsAString& aShortName,
                               const nsAString& aLongName,
                               const nsAString& aMcc,
                               const nsAString& aMnc,
                               const nsAString& aState,
                               ErrorResult& aRv)
{
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aGlobal.GetAsSupports());
  nsRefPtr<MobileNetworkInfo> info = new MobileNetworkInfo(window);

  info->mShortName.Assign(aShortName);
  info->mLongName.Assign(aLongName);
  info->mMcc.Assign(aMcc);
  info->mMnc.Assign(aMnc);
  info->mState.Assign(aState);

  return info.forget();
}

// nsIMobileNetworkInfo

NS_IMETHODIMP
MobileNetworkInfo::GetShortName(nsAString& aShortName)
{
  aShortName = mShortName;
  return NS_OK;
}

NS_IMETHODIMP
MobileNetworkInfo::GetLongName(nsAString& aLongName)
{
  aLongName = mLongName;
  return NS_OK;
}

NS_IMETHODIMP
MobileNetworkInfo::GetMcc(nsAString& aMcc)
{
  aMcc = mMcc;
  return NS_OK;
}

NS_IMETHODIMP
MobileNetworkInfo::GetMnc(nsAString& aMnc)
{
  aMnc = mMnc;
  return NS_OK;
}

NS_IMETHODIMP
MobileNetworkInfo::GetState(nsAString& aState)
{
  aState = mState;
  return NS_OK;
}
