/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "HTMLDataElement.h"
#include "mozilla/dom/HTMLDataElementBinding.h"
#include "nsGenericHTMLElement.h"

NS_IMPL_NS_NEW_HTML_ELEMENT(Data)

namespace mozilla {
namespace dom {

HTMLDataElement::HTMLDataElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
  SetIsDOMBinding();
}

HTMLDataElement::~HTMLDataElement()
{
}

NS_IMPL_ADDREF_INHERITED(HTMLDataElement, Element)
NS_IMPL_RELEASE_INHERITED(HTMLDataElement, Element)

NS_INTERFACE_TABLE_HEAD(HTMLDataElement)
  NS_HTML_CONTENT_INTERFACE_TABLE0(HTMLDataElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(HTMLDataElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_MAP_END

NS_IMPL_ELEMENT_CLONE(HTMLDataElement)

JSObject*
HTMLDataElement::WrapNode(JSContext* aCx, JSObject* aScope, bool* aTriedToWrap)
{
  return HTMLDataElementBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}

void
HTMLDataElement::GetItemValueText(nsAString& text)
{
  GetValue(text);
}

void
HTMLDataElement::SetItemValueText(const nsAString& text)
{
  ErrorResult rv;
  SetValue(text, rv);
}

} // namespace dom
} // namespace mozilla
