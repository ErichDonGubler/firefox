/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef GPU_OBJECT_MODEL_H_
#define GPU_OBJECT_MODEL_H_

#include "nsWrapperCache.h"
#include "nsString.h"

class nsIGlobalObject;

namespace mozilla::webgpu {
class WebGPUChild;
class Instance;

// Base class modeling clean-up during [cycle collection]. Intended to be used
// with `GPU_DECL_CYCLE_COLLECTION` and `GPU_IMPL_CYCLE_COLLECTION`.
//
// [cycle collection]:
// https://firefox-source-docs.mozilla.org/xpcom/cc-macros.html
class GpuCycleCollected {
  virtual ~GpuCycleCollected() { BeforeUnlinkStrongRefs(); }

  // Event handler called right before cycle collected references are unlinked.
  // If you are defining a type that inherits from this class, and it
  // contains strong references to objects that need to be used for cleaning
  // up (i.e., `WebGPUChild`), then you _must_ do so in this method.
  //
  // Implementations of this method should be idempotent; that is, multiple
  // calls to it should achieve the same state of being "cleaned up".
  virtual void BeforeUnlinkStrongRefs();
};

// Base class that only inherits from `nsWrapperCache`, for the sake of a name
// that is close to other elements of WebGPU's implementation. Intended to be
// used with `GPU_DECL_JS_WRAP` and `GPU_IMPL_JS_WRAP` to remove boilerplate
// for wrapping JS objects in IDL bindings.
class GpuJsWrap : public nsWrapperCache {};

// Base class intended to remove boilerplate that IDL bindings rely on for
// getting parent objects.
template <typename T = nsIGlobalObject>
class HasParentObject {
 protected:
  explicit HasParentObject(T* const parent) : mParent(parent) {}
  virtual ~HasParentObject() = default;

  RefPtr<T> mParent;

 public:
  nsIGlobalObject* GetParentObject() const {
    return mParent->GetParentObject();
  }
}

};

class ObjectBase {
 protected:
  // False if this object is definitely invalid.
  //
  // See WebGPU §3.2, "Invalid Internal Objects & Contagious Invalidity".
  //
  // There could also be state in the GPU process indicating that our
  // counterpart object there is invalid; certain GPU process operations will
  // report an error back to use if we try to use it. But if it's useful to know
  // whether the object is "definitely invalid", this should suffice.
  bool mValid = true;

 public:
  // Return true if this WebGPU object may be valid.
  //
  // This is used by methods that want to know whether somebody other than
  // `this` is valid. Generally, WebGPU object methods check `this->mValid`
  // directly.
  bool IsValid() const { return mValid; }

  void GetLabel(nsAString& aValue) const { aValue = mLabel; }
  void SetLabel(const nsAString& aLabel) { mLabel = aValue; }

  auto CLabel() const { return NS_ConvertUTF16toUTF8(mLabel); }

 protected:
  // Object label, initialized from GPUObjectDescriptorBase.label.
  nsString mLabel;
};

}  // namespace mozilla::webgpu

#define GPU_DECL_JS_WRAP(T)                                             \
  JSObject* WrapObject(JSContext* cx, JS::Handle<JSObject*> givenProto) \
      override;

#define GPU_DECL_CYCLE_COLLECTION(T)                    \
  NS_DECL_CYCLE_COLLECTION_NATIVE_WRAPPERCACHE_CLASS(T) \
  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(T)

#define GPU_IMPL_JS_WRAP(T)                                                  \
  JSObject* T::WrapObject(JSContext* cx, JS::Handle<JSObject*> givenProto) { \
    return dom::GPU##T##_Binding::Wrap(cx, this, givenProto);                \
  }

// Note: we don't use `NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE` directly
// because there is a custom action we need to always do.
#define GPU_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(T, ...) \
  NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_CLASS(T)       \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(T)             \
    tmp->BeforeUnlinkStrongRefs();                     \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(__VA_ARGS__)       \
    NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER  \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_END                  \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(T)           \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(__VA_ARGS__)     \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

#define GPU_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_WEAK_PTR(T, ...) \
  NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_CLASS(T)                \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(T)                      \
    tmp->BeforeUnlinkStrongRefs();                              \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(__VA_ARGS__)                \
    NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER           \
    NS_IMPL_CYCLE_COLLECTION_UNLINK_WEAK_PTR                    \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_END                           \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(T)                    \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(__VA_ARGS__)              \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

#define GPU_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_INHERITED(T, P, ...) \
  NS_IMPL_CYCLE_COLLECTION_CLASS(T)                                 \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(T, P)             \
    tmp->BeforeUnlinkStrongRefs();                                  \
    NS_IMPL_CYCLE_COLLECTION_UNLINK(__VA_ARGS__)                    \
    NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER               \
    NS_IMPL_CYCLE_COLLECTION_UNLINK_WEAK_PTR                        \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_END                               \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(T, P)           \
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(__VA_ARGS__)                  \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

#define GPU_IMPL_CYCLE_COLLECTION(T, ...) \
  GPU_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(T, __VA_ARGS__)

template <typename T>
void ImplCycleCollectionTraverse(nsCycleCollectionTraversalCallback& callback,
                                 nsTArray<RefPtr<const T>>& field,
                                 const char* name, uint32_t flags) {
  for (auto& element : field) {
    CycleCollectionNoteChild(callback, const_cast<T*>(element.get()), name,
                             flags);
  }
}

template <typename T>
void ImplCycleCollectionUnlink(nsTArray<RefPtr<const T>>& field) {
  for (auto& element : field) {
    ImplCycleCollectionUnlink(element);
  }
  field.Clear();
}

#endif  // GPU_OBJECT_MODEL_H_
