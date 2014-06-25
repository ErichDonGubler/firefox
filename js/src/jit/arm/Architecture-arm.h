/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef jit_arm_Architecture_arm_h
#define jit_arm_Architecture_arm_h

#include <limits.h>
#include <stdint.h>

#include "js/Utility.h"

// gcc appears to use __ARM_PCS_VFP to denote that the target is a hard-float target.
#if defined(__ARM_PCS_VFP)
#define JS_CODEGEN_ARM_HARDFP
#endif

namespace js {
namespace jit {

// In bytes: slots needed for potential memory->memory move spills.
//   +8 for cycles
//   +4 for gpr spills
//   +8 for double spills
static const uint32_t ION_FRAME_SLACK_SIZE   = 20;

// These offsets are specific to nunboxing, and capture offsets into the
// components of a js::Value.
static const int32_t NUNBOX32_TYPE_OFFSET    = 4;
static const int32_t NUNBOX32_PAYLOAD_OFFSET = 0;

static const uint32_t ShadowStackSpace = 0;
////
// These offsets are related to bailouts.
////

// Size of each bailout table entry. On arm, this is presently
// a single call (which is wrong!). the call clobbers lr.
// For now, I've dealt with this by ensuring that we never allocate to lr.
// it should probably be 8 bytes, a mov of an immediate into r12 (not
// allocated presently, or ever) followed by a branch to the apropriate code.
static const uint32_t BAILOUT_TABLE_ENTRY_SIZE    = 4;

class Registers
{
  public:
    enum RegisterID {
        r0 = 0,
        r1,
        r2,
        r3,
        S0 = r3,
        r4,
        r5,
        r6,
        r7,
        r8,
        S1 = r8,
        r9,
        r10,
        r11,
        r12,
        ip = r12,
        r13,
        sp = r13,
        r14,
        lr = r14,
        r15,
        pc = r15,
        invalid_reg
    };
    typedef RegisterID Code;

    static const char *GetName(Code code) {
        static const char * const Names[] = { "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
                                              "r8", "r9", "r10", "r11", "r12", "sp", "r14", "pc"};
        return Names[code];
    }
    static const char *GetName(uint32_t i) {
        MOZ_ASSERT(i < Total);
        return GetName(Code(i));
    }

    static Code FromName(const char *name);

    static const Code StackPointer = sp;
    static const Code Invalid = invalid_reg;

    static const uint32_t Total = 16;
    static const uint32_t Allocatable = 13;

    static const uint32_t AllMask = (1 << Total) - 1;
    static const uint32_t ArgRegMask = (1 << r0) | (1 << r1) | (1 << r2) | (1 << r3);

    static const uint32_t VolatileMask =
        (1 << r0) |
        (1 << r1) |
        (1 << Registers::r2) |
        (1 << Registers::r3);

    static const uint32_t NonVolatileMask =
        (1 << Registers::r4) |
        (1 << Registers::r5) |
        (1 << Registers::r6) |
        (1 << Registers::r7) |
        (1 << Registers::r8) |
        (1 << Registers::r9) |
        (1 << Registers::r10) |
        (1 << Registers::r11) |
        (1 << Registers::r12) |
        (1 << Registers::r14);

    static const uint32_t WrapperMask =
        VolatileMask |         // = arguments
        (1 << Registers::r4) | // = outReg
        (1 << Registers::r5);  // = argBase

    static const uint32_t SingleByteRegs =
        VolatileMask | NonVolatileMask;

    static const uint32_t NonAllocatableMask =
        (1 << Registers::sp) |
        (1 << Registers::r12) | // r12 = ip = scratch
        (1 << Registers::lr) |
        (1 << Registers::pc);

    // Registers that can be allocated without being saved, generally.
    static const uint32_t TempMask = VolatileMask & ~NonAllocatableMask;

    // Registers returned from a JS -> JS call.
    static const uint32_t JSCallMask =
        (1 << Registers::r2) |
        (1 << Registers::r3);

    // Registers returned from a JS -> C call.
    static const uint32_t CallMask =
        (1 << Registers::r0) |
        (1 << Registers::r1);  // used for double-size returns

    static const uint32_t AllocatableMask = AllMask & ~NonAllocatableMask;
};

// Smallest integer type that can hold a register bitmask.
typedef uint16_t PackedRegisterMask;

class FloatRegisters
{
  public:
    enum FPRegisterID {
        d0,
        d1,
        d2,
        d3,
        d4,
        d5,
        d6,
        d7,
        d8,
        d9,
        d10,
        d11,
        d12,
        d13,
        d14,
        d15,
        d16,
        d17,
        d18,
        d19,
        d20,
        d21,
        d22,
        d23,
        d24,
        d25,
        d26,
        d27,
        d28,
        d29,
        d30,
        invalid_freg
    };
    typedef FPRegisterID Code;

    static const char *GetName(Code code) {
        static const char * const Names[] = { "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
                                              "d8", "d9", "d10", "d11", "d12", "d13", "d14", "d15"};
        return Names[code];
    }
    static const char *GetName(uint32_t i) {
        JS_ASSERT(i < Total);
        return GetName(Code(i));
    }

    static Code FromName(const char *name);

    static const Code Invalid = invalid_freg;

    static const uint32_t Total = 16;
    static const uint32_t Allocatable = 15;

    static const uint32_t AllMask = (1 << Total) - 1;

    // d15 is the ScratchFloatReg.
    static const uint32_t NonVolatileMask =
        (1 << d8) |
        (1 << d9) |
        (1 << d10) |
        (1 << d11) |
        (1 << d12) |
        (1 << d13) |
        (1 << d14);

    static const uint32_t VolatileMask = AllMask & ~NonVolatileMask;

    static const uint32_t WrapperMask = VolatileMask;

    // d15 is the ARM scratch float register.
    static const uint32_t NonAllocatableMask = (1 << d15) | (1 << invalid_freg);

    // Registers that can be allocated without being saved, generally.
    static const uint32_t TempMask = VolatileMask & ~NonAllocatableMask;

    static const uint32_t AllocatableMask = AllMask & ~NonAllocatableMask;
};

class VFPRegister
{
  public:
    // What type of data is being stored in this register?
    // UInt / Int are specifically for vcvt, where we need
    // to know how the data is supposed to be converted.
    enum RegType {
        Single = 0x0,
        Double = 0x1,
        UInt   = 0x2,
        Int    = 0x3
    };

    typedef FloatRegisters Codes;
    typedef Codes::Code Code;

  protected:
    RegType kind : 2;
    // ARM doesn't have more than 32 registers...
    // don't take more bits than we'll need.
    // Presently, I don't have plans to address the upper
    // and lower halves of the double registers seprately, so
    // 5 bits should suffice.  If I do decide to address them seprately
    // (vmov, I'm looking at you), I will likely specify it as a separate
    // field.
  public:
    Code code_ : 5;
  protected:
    bool _isInvalid : 1;
    bool _isMissing : 1;

  public:
    MOZ_CONSTEXPR VFPRegister(uint32_t r, RegType k)
      : kind(k), code_ (Code(r)), _isInvalid(false), _isMissing(false)
    { }
    MOZ_CONSTEXPR VFPRegister()
      : kind(Double), code_(Code(0)), _isInvalid(true), _isMissing(false)
    { }

    MOZ_CONSTEXPR VFPRegister(RegType k, uint32_t id, bool invalid, bool missing) :
        kind(k), code_(Code(id)), _isInvalid(invalid), _isMissing(missing) {
    }

    explicit MOZ_CONSTEXPR VFPRegister(Code id)
      : kind(Double), code_(id), _isInvalid(false), _isMissing(false)
    { }
    bool operator==(const VFPRegister &other) const {
        JS_ASSERT(!isInvalid());
        JS_ASSERT(!other.isInvalid());
        return kind == other.kind && code_ == other.code_;
    }
    bool isDouble() const { return kind == Double; }
    bool isSingle() const { return kind == Single; }
    bool isFloat() const { return (kind == Double) || (kind == Single); }
    bool isInt() const { return (kind == UInt) || (kind == Int); }
    bool isSInt() const { return kind == Int; }
    bool isUInt() const { return kind == UInt; }
    bool equiv(VFPRegister other) const { return other.kind == kind; }
    size_t size() const { return (kind == Double) ? 8 : 4; }
    bool isInvalid() const;
    bool isMissing() const;

    VFPRegister doubleOverlay(unsigned int which = 0) const;
    VFPRegister singleOverlay(unsigned int which = 0) const;
    VFPRegister sintOverlay(unsigned int which = 0) const;
    VFPRegister uintOverlay(unsigned int which = 0) const;

    struct VFPRegIndexSplit;
    VFPRegIndexSplit encode();

    // for serializing values
    struct VFPRegIndexSplit {
        const uint32_t block : 4;
        const uint32_t bit : 1;

      private:
        friend VFPRegIndexSplit js::jit::VFPRegister::encode();

        VFPRegIndexSplit(uint32_t block_, uint32_t bit_)
          : block(block_), bit(bit_)
        {
            JS_ASSERT(block == block_);
            JS_ASSERT(bit == bit_);
        }
    };

    Code code() const {
        JS_ASSERT(!_isInvalid && !_isMissing);
        // this should only be used in areas where we only have doubles
        // and singles.
        JS_ASSERT(isFloat());
        return Code(code_);
    }
    uint32_t id() const {
        return code_;
    }
    static VFPRegister FromCode(uint32_t i) {
        uint32_t code = i & 31;
        uint32_t kind = i >> 5;
        return VFPRegister(code, Double);
    }
    bool volatile_() const {
        if (isDouble())
            return !!((1 << (code_ >> 1)) & FloatRegisters::VolatileMask);
        return !!((1 << code_) & FloatRegisters::VolatileMask);
    }
    const char *name() const {
        return FloatRegisters::GetName(code_);
    }
    bool operator != (const VFPRegister &other) const {
        return other.kind != kind || code_ != other.code_;
    }
    bool aliases(const VFPRegister &other) {
        if (kind == other.kind)
            return code_ == other.code_;
        return doubleOverlay() == other.doubleOverlay();
    }
    static const int NumAliasedDoubles = 16;
    uint32_t numAliased() const {
        if (isDouble()) {
            if (code_ < NumAliasedDoubles)
                return 3;
            return 1;
        }
        return 2;
    }
    VFPRegister aliased(uint32_t a) {
        if (a == 0)
            return *this;
        if (isDouble()) {
            JS_ASSERT(code_ < NumAliasedDoubles);
            JS_ASSERT(a <= 2);
            return singleOverlay(a - 1);
        }
        JS_ASSERT(a == 1);
        return doubleOverlay(a - 1);
    }
    uint32_t numAlignedAliased() const {
        if (isDouble()) {
            if (code_ < NumAliasedDoubles)
                return 2;
            return 1;
        }
        // s1 has 0 other aligned aliases
        // s0 has 1 other aligned aliases
        return 2 - (code_ & 1);
    }
    VFPRegister alignedAliased(uint32_t a) {
        if (a == 0)
            return *this;
        if (isDouble()) {
            JS_ASSERT(code_ < NumAliasedDoubles);
            JS_ASSERT(a <= 1);
            return singleOverlay(a - 1);
        }
        JS_ASSERT(a == 1);
        JS_ASSERT((code_ & 1) == 0);
        return doubleOverlay(a - 1);
    }
    static Code FromName(const char *name) {
        return FloatRegisters::FromName(name);
    }
};

// The only floating point register set that we work with
// are the VFP Registers
typedef VFPRegister FloatRegister;

uint32_t GetARMFlags();
bool HasMOVWT();
bool HasVFPv3();
bool HasVFP();
bool Has16DP();
bool HasIDIV();

bool ParseARMHwCapFlags(const char *armHwCap);

// If the simulator is used then the ABI choice is dynamic.  Otherwise the ABI is static
// and useHardFpABI is inlined so that unused branches can be optimized away.
#if defined(JS_ARM_SIMULATOR)
bool UseHardFpABI();
#else
static inline bool UseHardFpABI()
{
#if defined(JS_CODEGEN_ARM_HARDFP)
    return true;
#else
    return false;
#endif
}
#endif

} // namespace jit
} // namespace js

#endif /* jit_arm_Architecture_arm_h */
