/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __NS_SVGGRADIENTELEMENT_H__
#define __NS_SVGGRADIENTELEMENT_H__

#include "nsIDOMSVGURIReference.h"
#include "nsIDOMSVGGradientElement.h"
#include "nsIDOMSVGUnitTypes.h"
#include "nsSVGElement.h"
#include "nsSVGLength2.h"
#include "nsSVGEnum.h"
#include "nsSVGString.h"
#include "SVGAnimatedTransformList.h"

class nsSVGGradientFrame;
class nsSVGLinearGradientFrame;
class nsSVGRadialGradientFrame;

nsresult
NS_NewSVGLinearGradientElement(nsIContent** aResult,
                               already_AddRefed<nsINodeInfo> aNodeInfo);
nsresult
NS_NewSVGRadialGradientElement(nsIContent** aResult,
                               already_AddRefed<nsINodeInfo> aNodeInfo);

namespace mozilla {

class DOMSVGAnimatedTransformList;

namespace dom {

//--------------------- Gradients------------------------

typedef nsSVGElement SVGGradientElementBase;

class SVGGradientElement : public SVGGradientElementBase
                         , public nsIDOMSVGURIReference
                         , public nsIDOMSVGUnitTypes
{
  friend class ::nsSVGGradientFrame;

protected:
  SVGGradientElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual JSObject*
  WrapNode(JSContext* aCx, JSObject* aScope, bool* aTriedToWrap) MOZ_OVERRIDE = 0;

public:
  // interfaces:
  NS_DECL_ISUPPORTS_INHERITED

  // Gradient Element
  NS_DECL_NSIDOMSVGGRADIENTELEMENT

  // URI Reference
  NS_DECL_NSIDOMSVGURIREFERENCE

  // nsIContent
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual SVGAnimatedTransformList*
    GetAnimatedTransformList(uint32_t aFlags = 0);
  virtual nsIAtom* GetTransformListAttrName() const {
    return nsGkAtoms::gradientTransform;
  }

  // WebIDL
  already_AddRefed<nsIDOMSVGAnimatedEnumeration> GradientUnits();
  already_AddRefed<DOMSVGAnimatedTransformList> GradientTransform();
  already_AddRefed<nsIDOMSVGAnimatedEnumeration> SpreadMethod();
  already_AddRefed<nsIDOMSVGAnimatedString> Href();

protected:
  virtual EnumAttributesInfo GetEnumInfo();
  virtual StringAttributesInfo GetStringInfo();

  enum { GRADIENTUNITS, SPREADMETHOD };
  nsSVGEnum mEnumAttributes[2];
  static nsSVGEnumMapping sSpreadMethodMap[];
  static EnumInfo sEnumInfo[2];

  enum { HREF };
  nsSVGString mStringAttributes[1];
  static StringInfo sStringInfo[1];

  // nsIDOMSVGGradientElement values
  nsAutoPtr<SVGAnimatedTransformList> mGradientTransform;
};

//---------------------Linear Gradients------------------------

typedef SVGGradientElement SVGLinearGradientElementBase;

class SVGLinearGradientElement : public SVGLinearGradientElementBase
                               , public nsIDOMSVGLinearGradientElement
{
  friend class ::nsSVGLinearGradientFrame;
  friend nsresult
    (::NS_NewSVGLinearGradientElement(nsIContent** aResult,
                                      already_AddRefed<nsINodeInfo> aNodeInfo));

protected:
  SVGLinearGradientElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual JSObject*
  WrapNode(JSContext* aCx, JSObject* aScope, bool* aTriedToWrap) MOZ_OVERRIDE;

public:
  // interfaces:
  NS_DECL_ISUPPORTS_INHERITED

  // Gradient Element
  NS_FORWARD_NSIDOMSVGGRADIENTELEMENT(SVGLinearGradientElementBase::)

  // Linear Gradient
  NS_DECL_NSIDOMSVGLINEARGRADIENTELEMENT

  // The Gradient Element base class implements these
  NS_FORWARD_NSIDOMSVGELEMENT(SVGLinearGradientElementBase::)

  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }

  // WebIDL
  already_AddRefed<nsIDOMSVGAnimatedLength> X1();
  already_AddRefed<nsIDOMSVGAnimatedLength> Y1();
  already_AddRefed<nsIDOMSVGAnimatedLength> X2();
  already_AddRefed<nsIDOMSVGAnimatedLength> Y2();

protected:

  virtual LengthAttributesInfo GetLengthInfo();

  // nsIDOMSVGLinearGradientElement values
  enum { ATTR_X1, ATTR_Y1, ATTR_X2, ATTR_Y2 };
  nsSVGLength2 mLengthAttributes[4];
  static LengthInfo sLengthInfo[4];
};

//-------------------------- Radial Gradients ----------------------------

typedef SVGGradientElement SVGRadialGradientElementBase;

class SVGRadialGradientElement : public SVGRadialGradientElementBase
                               , public nsIDOMSVGRadialGradientElement
{
  friend class ::nsSVGRadialGradientFrame;
  friend nsresult
    (::NS_NewSVGRadialGradientElement(nsIContent** aResult,
                                      already_AddRefed<nsINodeInfo> aNodeInfo));

protected:
  SVGRadialGradientElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual JSObject*
  WrapNode(JSContext* aCx, JSObject* aScope, bool* aTriedToWrap) MOZ_OVERRIDE;

public:
  // interfaces:

  NS_DECL_ISUPPORTS_INHERITED

  // Gradient Element
  NS_FORWARD_NSIDOMSVGGRADIENTELEMENT(SVGRadialGradientElementBase::)

  // Radial Gradient
  NS_DECL_NSIDOMSVGRADIALGRADIENTELEMENT

  // xxx I wish we could use virtual inheritance
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC
  NS_FORWARD_NSIDOMSVGELEMENT(SVGRadialGradientElementBase::)

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }

  // WebIDL
  already_AddRefed<nsIDOMSVGAnimatedLength> Cx();
  already_AddRefed<nsIDOMSVGAnimatedLength> Cy();
  already_AddRefed<nsIDOMSVGAnimatedLength> R();
  already_AddRefed<nsIDOMSVGAnimatedLength> Fx();
  already_AddRefed<nsIDOMSVGAnimatedLength> Fy();
protected:

  virtual LengthAttributesInfo GetLengthInfo();

  // nsIDOMSVGRadialGradientElement values
  enum { ATTR_CX, ATTR_CY, ATTR_R, ATTR_FX, ATTR_FY };
  nsSVGLength2 mLengthAttributes[5];
  static LengthInfo sLengthInfo[5];
};

} // namespace dom
} // namespace mozilla

#endif
