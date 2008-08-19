/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Mozilla SVG project.
 *
 * The Initial Developer of the Original Code is
 * Robert Longson
 * Portions created by the Initial Developer are Copyright (C) 2007
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Robert Longson <longsonr@gmail.com> (original author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsSVGGFrame.h"
#include "nsSVGSwitchElement.h"
#include "nsIDOMSVGRect.h"

typedef nsSVGGFrame nsSVGSwitchFrameBase;

class nsSVGSwitchFrame : public nsSVGSwitchFrameBase
{
  friend nsIFrame*
  NS_NewSVGSwitchFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext);
protected:
  nsSVGSwitchFrame(nsStyleContext* aContext) :
    nsSVGSwitchFrameBase(aContext) {}

public:
  /**
   * Get the "type" of the frame
   *
   * @see nsGkAtoms::svgSwitchFrame
   */
  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGSwitch"), aResult);
  }
#endif

  // nsISVGChildFrame interface:
  NS_IMETHOD PaintSVG(nsSVGRenderState* aContext, nsRect *aDirtyRect);
  NS_IMETHOD GetFrameForPointSVG(float aX, float aY, nsIFrame** aResult);  
  NS_IMETHODIMP_(nsRect) GetCoveredRegion();
  NS_IMETHOD UpdateCoveredRegion();
  NS_IMETHOD InitialUpdate();
  NS_IMETHOD NotifyRedrawUnsuspended();
  NS_IMETHOD GetBBox(nsIDOMSVGRect **aRect);

private:
  nsIFrame *GetActiveChildFrame();
};

//----------------------------------------------------------------------
// Implementation

nsIFrame*
NS_NewSVGSwitchFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext)
{  
  nsCOMPtr<nsIDOMSVGSwitchElement> svgSwitch = do_QueryInterface(aContent);
  if (!svgSwitch) {
    NS_ERROR("Can't create frame. Content is not an SVG switch\n");
    return nsnull;
  }

  return new (aPresShell) nsSVGSwitchFrame(aContext);
}

nsIAtom *
nsSVGSwitchFrame::GetType() const
{
  return nsGkAtoms::svgSwitchFrame;
}

NS_IMETHODIMP
nsSVGSwitchFrame::PaintSVG(nsSVGRenderState* aContext, nsRect *aDirtyRect)
{
  const nsStyleDisplay *display = mStyleContext->GetStyleDisplay();
  if (display->mOpacity == 0.0)
    return NS_OK;

  nsIFrame *kid = GetActiveChildFrame();
  if (kid) {
    nsSVGUtils::PaintChildWithEffects(aContext, aDirtyRect, kid);
  }
  return NS_OK;
}


NS_IMETHODIMP
nsSVGSwitchFrame::GetFrameForPointSVG(float aX, float aY, nsIFrame** aResult)
{
  *aResult = nsnull;

  nsIFrame *kid = GetActiveChildFrame();
  if (kid) {
    nsISVGChildFrame* svgFrame;
    CallQueryInterface(kid, &svgFrame);
    if (svgFrame) {
      svgFrame->GetFrameForPointSVG(aX, aY, aResult);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP_(nsRect)
nsSVGSwitchFrame::GetCoveredRegion()
{
  nsRect rect;

  nsIFrame *kid = GetActiveChildFrame();
  if (kid) {
    nsISVGChildFrame* child = nsnull;
    CallQueryInterface(kid, &child);
    if (child) {
      rect = child->GetCoveredRegion();
    }
  }
  return rect;
}

NS_IMETHODIMP
nsSVGSwitchFrame::UpdateCoveredRegion()
{
  static_cast<nsSVGSwitchElement*>(mContent)->UpdateActiveChild();

  return nsSVGSwitchFrameBase::UpdateCoveredRegion();
}

NS_IMETHODIMP
nsSVGSwitchFrame::InitialUpdate()
{
  nsSVGUtils::UpdateGraphic(this);

  return nsSVGSwitchFrameBase::InitialUpdate();
}

NS_IMETHODIMP
nsSVGSwitchFrame::NotifyRedrawUnsuspended()
{
  if (GetStateBits() & NS_STATE_SVG_DIRTY)
    nsSVGUtils::UpdateGraphic(this);

  return nsSVGSwitchFrameBase::NotifyRedrawUnsuspended();
}

NS_IMETHODIMP
nsSVGSwitchFrame::GetBBox(nsIDOMSVGRect **aRect)
{
  *aRect = nsnull;

  nsIFrame *kid = GetActiveChildFrame();
  if (kid) {
    nsISVGChildFrame* svgFrame = nsnull;
    CallQueryInterface(kid, &svgFrame);
    if (svgFrame) {
      nsCOMPtr<nsIDOMSVGRect> box;
      svgFrame->GetBBox(getter_AddRefs(box));
      if (box) {
        box.swap(*aRect);
        return NS_OK;
      }
    }
  }
  return NS_ERROR_FAILURE;
}

nsIFrame *
nsSVGSwitchFrame::GetActiveChildFrame()
{
  nsIContent *activeChild =
    static_cast<nsSVGSwitchElement*>(mContent)->GetActiveChild();

  if (activeChild) {
    for (nsIFrame* kid = mFrames.FirstChild(); kid;
         kid = kid->GetNextSibling()) {

      if (activeChild == kid->GetContent()) {
        return kid;
      }
    }
  }
  return nsnull;
}
