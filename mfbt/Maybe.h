/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* A class for optional values and in-place lazy construction. */

#ifndef mozilla_Maybe_h
#define mozilla_Maybe_h

#include "mozilla/Alignment.h"
#include "mozilla/Assertions.h"
#include "mozilla/Move.h"
#include "mozilla/TypeTraits.h"

#include <new>  // for placement new

namespace mozilla {

struct Nothing { };

/*
 * Maybe is a container class which contains either zero or one elements. It
 * serves two roles. It can represent values which are *semantically* optional,
 * augmenting a type with an explicit 'Nothing' value. In this role, it provides
 * methods that make it easy to work with values that may be missing, along with
 * equality and comparison operators so that Maybe values can be stored in
 * containers. Maybe values can be constructed conveniently in expressions using
 * type inference, as follows:
 *
 *   void doSomething(Maybe<Foo> aFoo) {
 *     if (aFoo)                  // Make sure that aFoo contains a value...
 *       aFoo->takeAction();      // and then use |aFoo->| to access it.
 *   }                            // |*aFoo| also works!
 *
 *   doSomething(Nothing());      // Passes a Maybe<Foo> containing no value.
 *   doSomething(Some(Foo(100))); // Passes a Maybe<Foo> containing |Foo(100)|.
 *
 * You'll note that it's important to check whether a Maybe contains a value
 * before using it, using conversion to bool, |isSome()|, or |isNothing()|. You
 * can avoid these checks, and sometimes write more readable code, using
 * |valueOr()|, |ptrOr()|, and |refOr()|, which allow you to retrieve the value
 * in the Maybe and provide a default for the 'Nothing' case.  You can also use
 * |apply()| to call a function only if the Maybe holds a value, and |map()| to
 * transform the value in the Maybe, returning another Maybe with a possibly
 * different type.
 *
 * Maybe's other role is to support lazily constructing objects without using
 * dynamic storage. A Maybe directly contains storage for a value, but it's
 * empty by default. |emplace()|, as mentioned above, can be used to construct a
 * value in Maybe's storage.  The value a Maybe contains can be destroyed by
 * calling |reset()|; this will happen automatically if a Maybe is destroyed
 * while holding a value.
 *
 * It's a common idiom in C++ to use a pointer as a 'Maybe' type, with a null
 * value meaning 'Nothing' and any other value meaning 'Some'. You can convert
 * from such a pointer to a Maybe value using 'ToMaybe()'.
 *
 * Maybe is inspired by similar types in the standard library of many other
 * languages (e.g. Haskell's Maybe and Rust's Option). In the C++ world it's
 * very similar to std::optional, which was proposed for C++14 and originated in
 * Boost. The most important differences between Maybe and std::optional are:
 *
 *   - std::optional<T> may be compared with T. We deliberately forbid that.
 *   - std::optional allows in-place construction without a separate call to
 *     |emplace()| by using a dummy |in_place_t| value to tag the appropriate
 *     constructor.
 *   - std::optional has |valueOr()|, equivalent to Maybe's |valueOr()|, but
 *     lacks corresponding methods for |refOr()| and |ptrOr()|.
 *   - std::optional lacks |map()| and |apply()|, making it less suitable for
 *     functional-style code.
 *   - std::optional lacks many convenience functions that Maybe has. Most
 *     unfortunately, it lacks equivalents of the type-inferred constructor
 *     functions |Some()| and |Nothing()|.
 *
 * N.B. GCC has missed optimizations with Maybe in the past and may generate
 * extra branches/loads/stores. Use with caution on hot paths; it's not known
 * whether or not this is still a problem.
 */
template<class T>
class Maybe
{
  typedef void (Maybe::* ConvertibleToBool)(float*****, double*****);
  void nonNull(float*****, double*****) {}

  bool mIsSome;
  AlignedStorage2<T> mStorage;

public:
  typedef T ValueType;

  Maybe() : mIsSome(false) { }
  ~Maybe() { reset(); }

  Maybe(Nothing) : mIsSome(false) { }

  Maybe(const Maybe& aOther)
    : mIsSome(false)
  {
    if (aOther.mIsSome) {
      emplace(*aOther);
    }
  }

  Maybe(Maybe&& aOther)
    : mIsSome(aOther.mIsSome)
  {
    if (aOther.mIsSome) {
      ::new (mStorage.addr()) T(Move(*aOther));
      aOther.reset();
    }
  }

  Maybe& operator=(const Maybe& aOther)
  {
    if (&aOther != this) {
      if (aOther.mIsSome) {
        if (mIsSome) {
          // XXX(seth): The correct code for this branch, below, can't be used
          // due to a bug in Visual Studio 2010. See bug 1052940.
          /*
          ref() = aOther.ref();
          */
          reset();
          emplace(*aOther);
        } else {
          emplace(*aOther);
        }
      } else {
        reset();
      }
    }
    return *this;
  }

  Maybe& operator=(Maybe&& aOther)
  {
    MOZ_ASSERT(this != &aOther, "Self-moves are prohibited");

    if (aOther.mIsSome) {
      if (mIsSome) {
        ref() = Move(aOther.ref());
      } else {
        mIsSome = true;
        ::new (mStorage.addr()) T(Move(*aOther));
      }
      aOther.reset();
    } else {
      reset();
    }

    return *this;
  }

  /* Methods that check whether this Maybe contains a value */
  operator ConvertibleToBool() const { return mIsSome ? &Maybe::nonNull : 0; }
  bool isSome() const { return mIsSome; }
  bool isNothing() const { return !mIsSome; }

  /* Returns the contents of this Maybe<T> by value. Unsafe unless |isSome()|. */
  T value() const
  {
    MOZ_ASSERT(mIsSome);
    return ref();
  }

  /* Returns the contents of this Maybe<T> by pointer. Unsafe unless |isSome()|. */
  T* ptr()
  {
    MOZ_ASSERT(mIsSome);
    return &ref();
  }

  const T* ptr() const
  {
    MOZ_ASSERT(mIsSome);
    return &ref();
  }

  T* operator->()
  {
    MOZ_ASSERT(mIsSome);
    return ptr();
  }

  const T* operator->() const
  {
    MOZ_ASSERT(mIsSome);
    return ptr();
  }

  /* Returns the contents of this Maybe<T> by ref. Unsafe unless |isSome()|. */
  T& ref()
  {
    MOZ_ASSERT(mIsSome);
    return *mStorage.addr();
  }

  const T& ref() const
  {
    MOZ_ASSERT(mIsSome);
    return *mStorage.addr();
  }

  T& operator*()
  {
    MOZ_ASSERT(mIsSome);
    return ref();
  }

  const T& operator*() const
  {
    MOZ_ASSERT(mIsSome);
    return ref();
  }

  /* If |isSome()|, empties this Maybe and destroys its contents. */
  void reset()
  {
    if (isSome()) {
      ref().~T();
      mIsSome = false;
    }
  }

  /*
   * Constructs a T value in-place in this empty Maybe<T>'s storage. The
   * arguments to |emplace()| are the parameters to T's constructor.
   *
   * WARNING: You can't pass a literal nullptr to these methods without
   * hitting GCC 4.4-only (and hence B2G-only) compile errors.
   */
  void emplace()
  {
    MOZ_ASSERT(!mIsSome);
    ::new (mStorage.addr()) T();
    mIsSome = true;
  }

  template<typename T1>
  void emplace(T1&& t1)
  {
    MOZ_ASSERT(!mIsSome);
    ::new (mStorage.addr()) T(Forward<T1>(t1));
    mIsSome = true;
  }

  template<typename T1, typename T2>
  void emplace(T1&& t1, T2&& t2)
  {
    MOZ_ASSERT(!mIsSome);
    ::new (mStorage.addr()) T(Forward<T1>(t1), Forward<T2>(t2));
    mIsSome = true;
  }

  template<typename T1, typename T2, typename T3>
  void emplace(T1&& t1, T2&& t2, T3&& t3)
  {
    MOZ_ASSERT(!mIsSome);
    ::new (mStorage.addr()) T(Forward<T1>(t1), Forward<T2>(t2), Forward<T3>(t3));
    mIsSome = true;
  }

  template<typename T1, typename T2, typename T3, typename T4>
  void emplace(T1&& t1, T2&& t2, T3&& t3, T4&& t4)
  {
    MOZ_ASSERT(!mIsSome);
    ::new (mStorage.addr()) T(Forward<T1>(t1), Forward<T2>(t2), Forward<T3>(t3),
                              Forward<T4>(t4));
    mIsSome = true;
  }

  template<typename T1, typename T2, typename T3, typename T4, typename T5>
  void emplace(T1&& t1, T2&& t2, T3&& t3, T4&& t4, T5&& t5)
  {
    MOZ_ASSERT(!mIsSome);
    ::new (mStorage.addr()) T(Forward<T1>(t1), Forward<T2>(t2), Forward<T3>(t3),
                              Forward<T4>(t4), Forward<T5>(t5));
    mIsSome = true;
  }

  template<typename T1, typename T2, typename T3, typename T4, typename T5,
           typename T6>
  void emplace(T1&& t1, T2&& t2, T3&& t3, T4&& t4, T5&& t5, T6&& t6)
  {
    MOZ_ASSERT(!mIsSome);
    ::new (mStorage.addr()) T(Forward<T1>(t1), Forward<T2>(t2), Forward<T3>(t3),
                              Forward<T4>(t4), Forward<T5>(t5), Forward<T6>(t6));
    mIsSome = true;
  }

  template<typename T1, typename T2, typename T3, typename T4, typename T5,
           typename T6, typename T7>
  void emplace(T1&& t1, T2&& t2, T3&& t3, T4&& t4, T5&& t5, T6&& t6,
               T7&& t7)
  {
    MOZ_ASSERT(!mIsSome);
    ::new (mStorage.addr()) T(Forward<T1>(t1), Forward<T2>(t2), Forward<T3>(t3),
                              Forward<T4>(t4), Forward<T5>(t5), Forward<T6>(t6),
                              Forward<T7>(t7));
    mIsSome = true;
  }

  template<typename T1, typename T2, typename T3, typename T4, typename T5,
           typename T6, typename T7, typename T8>
  void emplace(T1&& t1, T2&& t2, T3&& t3, T4&& t4, T5&& t5, T6&& t6,
               T7&& t7, T8&& t8)
  {
    MOZ_ASSERT(!mIsSome);
    ::new (mStorage.addr()) T(Forward<T1>(t1), Forward<T2>(t2), Forward<T3>(t3),
                              Forward<T4>(t4), Forward<T5>(t5), Forward<T6>(t6),
                              Forward<T7>(t7), Forward<T8>(t8));
    mIsSome = true;
  }

  template<typename T1, typename T2, typename T3, typename T4, typename T5,
           typename T6, typename T7, typename T8, typename T9>
  void emplace(T1&& t1, T2&& t2, T3&& t3, T4&& t4, T5&& t5, T6&& t6,
               T7&& t7, T8&& t8, T9&& t9)
  {
    MOZ_ASSERT(!mIsSome);
    ::new (mStorage.addr()) T(Forward<T1>(t1), Forward<T2>(t2), Forward<T3>(t3),
                              Forward<T4>(t4), Forward<T5>(t5), Forward<T6>(t6),
                              Forward<T7>(t7), Forward<T8>(t8), Forward<T9>(t9));
    mIsSome = true;
  }

  template<typename T1, typename T2, typename T3, typename T4, typename T5,
           typename T6, typename T7, typename T8, typename T9, typename T10>
  void emplace(T1&& t1, T2&& t2, T3&& t3, T4&& t4, T5&& t5, T6&& t6,
               T7&& t7, T8&& t8, T9&& t9, T10&& t10)
  {
    MOZ_ASSERT(!mIsSome);
    ::new (mStorage.addr()) T(Forward<T1>(t1), Forward<T2>(t2), Forward<T3>(t3),
                              Forward<T4>(t4), Forward<T5>(t5), Forward<T6>(t6),
                              Forward<T7>(t7), Forward<T8>(t8), Forward<T9>(t9),
                              Forward<T1>(t10));
    mIsSome = true;
  }
};

/*
 * Some() creates a Maybe<T> value containing the provided T value. If T has a
 * move constructor, it's used to make this as efficient as possible.
 *
 * Some() selects the type of Maybe it returns by removing any const, volatile,
 * or reference qualifiers from the type of the value you pass to it. This gives
 * it more intuitive behavior when used in expressions, but it also means that
 * if you need to construct a Maybe value that holds a const, volatile, or
 * reference value, you need to use emplace() instead.
 */
template<typename T>
Maybe<typename RemoveCV<typename RemoveReference<T>::Type>::Type>
Some(T&& aValue)
{
  typedef typename RemoveCV<typename RemoveReference<T>::Type>::Type U;
  Maybe<U> value;
  value.emplace(Forward<T>(aValue));
  return value;
}

} // namespace mozilla

#endif /* mozilla_Maybe_h */
