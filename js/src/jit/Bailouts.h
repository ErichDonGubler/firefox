/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef jit_Bailouts_h
#define jit_Bailouts_h

#include "jstypes.h"

#include "jit/IonFrameIterator.h"
#include "jit/IonFrames.h"
#include "vm/Stack.h"

namespace js {
namespace jit {

// A "bailout" is a condition in which we need to recover an interpreter frame
// from an IonFrame. Bailouts can happen for the following reasons:
//   (1) A deoptimization guard, for example, an add overflows or a type check
//       fails.
//   (2) A check or assumption held by the JIT is invalidated by the VM, and
//       JIT code must be thrown away. This includes the GC possibly deciding
//       to evict live JIT code, or a Type Inference reflow.
//
// Note that bailouts as described here do not include normal Ion frame
// inspection, for example, if an exception must be built or the GC needs to
// scan an Ion frame for gcthings.
//
// The second type of bailout needs a different name - "deoptimization" or
// "deep bailout". Here we are concerned with eager (or maybe "shallow")
// bailouts, that happen from JIT code. These happen from guards, like:
//
//  cmp [obj + shape], 0x50M37TH1NG
//  jmp _bailout
//
// The bailout target needs to somehow translate the Ion frame (whose state
// will differ at each program point) to an interpreter frame. This state is
// captured into the IonScript's snapshot buffer, and for each bailout we know
// which snapshot corresponds to its state.
//
// Roughly, the following needs to happen at the bailout target.
//   (1) Move snapshot ID into a known stack location (registers cannot be
//       mutated).
//   (2) Spill all registers to the stack.
//   (3) Call a Bailout() routine, whose argument is the stack pointer.
//   (4) Bailout() will find the IonScript on the stack, use the snapshot ID
//       to find the structure of the frame, and then use the stack and spilled
//       registers to perform frame conversion.
//   (5) Bailout() returns, and the JIT must immediately return to the
//       interpreter (all frames are converted at once).
//
// (2) and (3) are implemented by a trampoline held in the compartment.
// Naively, we could implement (1) like:
//
//   _bailout_ID_1:
//     push 1
//     jmp _global_bailout_handler
//   _bailout_ID_2:
//     push 2
//     jmp _global_bailout_handler
//
// This takes about 10 extra bytes per guard. On some platforms, we can reduce
// this overhead to 4 bytes by creating a global jump table, shared again in
// the compartment:
//
//     call _global_bailout_handler
//     call _global_bailout_handler
//     call _global_bailout_handler
//     call _global_bailout_handler
//      ...
//    _global_bailout_handler:
//
// In the bailout handler, we can recompute which entry in the table was
// selected by subtracting the return addressed pushed by the call, from the
// start of the table, and then dividing by the size of a (call X) entry in the
// table. This gives us a number in [0, TableSize), which we call a
// "BailoutId".
//
// Then, we can provide a per-script mapping from BailoutIds to snapshots,
// which takes only four bytes per entry.
//
// This strategy does not work as given, because the bailout handler has no way
// to compute the location of an IonScript. Currently, we do not use frame
// pointers. To account for this we segregate frames into a limited set of
// "frame sizes", and create a table for each frame size. We also have the
// option of not using bailout tables, for platforms or situations where the
// 10 byte cost is more optimal than a bailout table. See IonFrames.h for more
// detail.

static const BailoutId INVALID_BAILOUT_ID = BailoutId(-1);

static const uint32_t BAILOUT_KIND_BITS = 3;
static const uint32_t BAILOUT_RESUME_BITS = 1;

// Keep this arbitrarily small for now, for testing.
static const uint32_t BAILOUT_TABLE_SIZE = 16;

// Bailout return codes.
// N.B. the relative order of these values is hard-coded into ::GenerateBailoutThunk.
static const uint32_t BAILOUT_RETURN_OK = 0;
static const uint32_t BAILOUT_RETURN_FATAL_ERROR = 1;
static const uint32_t BAILOUT_RETURN_OVERRECURSED = 2;

class IonCompartment;

// BailoutStack is an architecture specific pointer to the stack, given by the
// bailout handler.
class BailoutStack;
class InvalidationBailoutStack;

// Must be implemented by each architecture.

// This iterator is constructed at a time where there is no exit frame at the
// moment. They must be initialized to the first JS frame instead of the exit
// frame as usually done with IonFrameIterator.
class IonBailoutIterator : public IonFrameIterator
{
    MachineState machine_;
    uint32_t snapshotOffset_;
    size_t topFrameSize_;
    IonScript *topIonScript_;

  public:
    IonBailoutIterator(const JitActivationIterator &activations, BailoutStack *sp);
    IonBailoutIterator(const JitActivationIterator &activations, InvalidationBailoutStack *sp);
    IonBailoutIterator(const JitActivationIterator &activations, const IonFrameIterator &frame);

    SnapshotOffset snapshotOffset() const {
        JS_ASSERT(topIonScript_);
        return snapshotOffset_;
    }
    const MachineState &machineState() const {
        return machine_;
    }
    size_t topFrameSize() const {
        JS_ASSERT(topIonScript_);
        return topFrameSize_;
    }
    IonScript *ionScript() const {
        if (topIonScript_)
            return topIonScript_;
        return IonFrameIterator::ionScript();
    }

    void dump() const;
};

bool EnsureHasScopeObjects(JSContext *cx, AbstractFramePtr fp);

struct BaselineBailoutInfo;

// Called from a bailout thunk. Returns a BAILOUT_* error code.
uint32_t Bailout(BailoutStack *sp, BaselineBailoutInfo **info);

// Called from the invalidation thunk. Returns a BAILOUT_* error code.
uint32_t InvalidationBailout(InvalidationBailoutStack *sp, size_t *frameSizeOut,
                             BaselineBailoutInfo **info);

struct ExceptionBailoutInfo
{
    size_t frameNo;
    jsbytecode *resumePC;
    size_t numExprSlots;
};

// Called from the exception handler to enter a catch or finally block.
// Returns a BAILOUT_* error code.
uint32_t ExceptionHandlerBailout(JSContext *cx, const InlineFrameIterator &frame,
                                 const ExceptionBailoutInfo &excInfo,
                                 BaselineBailoutInfo **bailoutInfo);

uint32_t FinishBailoutToBaseline(BaselineBailoutInfo *bailoutInfo);

bool CheckFrequentBailouts(JSContext *cx, JSScript *script);

} // namespace jit
} // namespace js

#endif /* jit_Bailouts_h */
