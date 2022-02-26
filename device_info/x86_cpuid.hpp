// CPUID subset extraction
#pragma once


#if ! (__x86_64__ || __amd64__)
#error this header is for x86 only
#endif

#include "../compiler_detect.hpp"
#include <array>
#include <string>
#include <sstream>
#include <thread>
#include "../ints.hpp"

namespace x86_cpuid {
using namespace core::integral;

#if defined(__APPLE__) || CORE_GCC
    #include <cpuid.h>
#endif

// Used to READ simd flags
namespace SIMD {
    static constexpr u8 NO_SIMD = 0;
    static constexpr u8 SSE     = 1;
    static constexpr u8 SSE2    = 2 << 0;
    static constexpr u8 SSE3    = 2 << 1;
    static constexpr u8 SSSE3   = 2 << 2;
    static constexpr u8 SSE4_1  = 2 << 3;
    static constexpr u8 SSE4_2  = 2 << 4;
    static constexpr u8 AVX     = 2 << 5;
    static constexpr u8 AVX2    = 2 << 6;
};

struct Registers {
    u32 EAX = 0;
    u32 EBX = 0;
    u32 ECX = 0;
    u32 EDX = 0;
    
    Registers() {}
    
    Registers(Registers const& other)
    : EAX {other.EAX}
    , EBX {other.EBX}
    , ECX {other.ECX}
    , EDX {other.EDX}
    { }
    
    Registers(u32 * other)
    : EAX {other[0]}
    , EBX {other[1]}
    , ECX {other[2]}
    , EDX {other[3]}
    { }
    
    Registers& operator= (u32 * other)
    {
        EAX = other[0];
        EBX = other[1];
        ECX = other[2];
        EDX = other[3];
        
        return *this;
    }
    
    operator int* ()
    {
        return (int*) this;
    }
};


inline auto cpuid(unsigned i) -> Registers
{
    Registers regs;
    
    /* GCC's CPUID DOESN'T WORK WITH EXTENDED SUBLEAVES! @level7 AVX2 is NOT detected.
       ASSUMING THE PLAIN CPUID DOESN'T ZERO THE ECX.
     
    // GCC:
    #if C_GCC && defined(__get_cpuid)
        __get_cpuid(i, regs.EAX, regs.EBX, regs.ECX, regs.EDX);
        return regs;
    // MacOS:
    #elif (defined(MAC_OS) && defined(__cpuid))
        __cpuid(i, regs.EAX, regs.EBX, regs.ECX, regs.EDX);
        return regs;
     */
    // MSVC or ICC
    #if C_MSVC || C_ICC
        __cpuidex((int *)regs, (int)i, 0/*ECX=0 for extended leaves*/);
    
    // x86_32:
    #elif ARCH_x86_32 && defined(__PIC__)
        asm volatile (
                  "pushl %%ebx\n"\
                  "cpuid\n"\
                  "mov %%ebx, %1\n"
                  "popl %%ebx"\
                  : "=a" (regs[0]), "=r" (regs[1]), "=c" (regs[2]), "=d" (regs[3]) \
                  : "a" (i), "c" (0)
        );
    #else // x86-64 case
        __asm("cpuid" : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3])
             : "a" (i), "c" (0): ); // ECX is set to zero for CPUID function 4
    #endif
    
    return regs;
}

namespace CPUID {

enum ECX_L1 {
    sse3   = 1 << 0,
    vmx    = 1 << 5,  // Intel (VM eXtensions)
    ssse3  = 1 << 9,
    fma    = 1 << 12, // Fused Multiply-Add
    sse4_1 = 1 << 19,
    sse4_2 = 1 << 20,
    popcnt = 1 << 23, // Popcount hardware instruction
    aes    = 1 << 25, // AES encryption
    avx    = 1 << 28, // AVX
    f16c   = 1 << 29,
    rdrnd  = 1 << 30  // Intel rdrand (hardware-generated random)
};

enum EDX_L1 {
    fpu   = 1 << 0,  // x87 floating point unit on-chip
    tsc   = 1 << 4,  // TimeStampCounter (RDTSC & RDTSCP instruction support)
    mmx   = 1 << 23, // MMX
    sse   = 1 << 25, // SSE
    sse2  = 1 << 26, // SSE2
    htt   = 1 << 28  // Hyper-Threading
};

enum EBX_XL7 {
    avx2 = 1 << 5 // AVX2
    // TODO: add AVX512
};


// used ONLY to SET simd flags
namespace VEXMODE {
constexpr u8 NO_SIMD = 0;
constexpr u8 SSE = 1;   // 00...01
constexpr u8 SSE2 = 3;  // 0...011
constexpr u8 SSE3 = 7;  // 0..0111
constexpr u8 SSSE3 = 15;
constexpr u8 SSE4_1 = 31;
constexpr u8 SSE4_2 = 63;
constexpr u8 AVX = 127;
constexpr u8 AVX2 = 255; // 1..111
};

// Class CPU -> SIMD info
class CPU {
private:
    size_t leaves;
    size_t xleaves; // extended leaves
    u8 SIMD; // SSE - AVX2 simd
public:
    CPU()
    : leaves {cpuid(0).EAX} // get max.number of 'leaves'
    {
        if (leaves == 0) {
            // really old CPU
        }
        else {
            auto xtended = cpuid(0x80000000);
            xleaves = xtended.EAX;
        }
        
        
        if (leaves >= 7)
        {
            auto L7_EBX = cpuid(7).EBX;
            if (L7_EBX & avx2)
            {
                SIMD = VEXMODE::AVX2;
                return;
            }
        }
        
        // leaf 1:
        auto regs = cpuid(1);
        auto EDX = regs.EDX; // CPUID Level 1
        auto ECX = regs.ECX; // CPUID Level 1
        
        if (ECX & avx)
        {
            SIMD = VEXMODE::AVX;
        }
        else if (ECX & sse4_2)
        {
            SIMD = VEXMODE::SSE4_2;
        }
        else if (ECX & sse4_1)
        {
            SIMD = VEXMODE::SSE4_1;
        }
        else if (ECX & ssse3)
        {
            SIMD = VEXMODE::SSSE3;
        }
        else if (EDX & sse2)
        {
            SIMD = VEXMODE::SSE2;
        }
        #if ARCH_x86_32
        else if (EDX & sse)
        {
            SIMD = VEXMODE::SSE;
        }
        else
        {
            SIMD = VEXMODE::NO_SIMD;
        }
        #elif ARCH_x86_64
            SIMD = VEXMODE::SSE; // SSE ALWAYS AVAILABLE ON x64!
        #endif
    }
    
    u8 supported_simd() const
    {
        return SIMD;
    }
    
    auto n_threads() -> size_t
    {
        return std::thread::hardware_concurrency();
    }
};



// Other funcs:
inline auto getCPUVendorString() -> std::string
{
    using namespace std;
    auto regs = cpuid(0);
    ostringstream tmp;
    tmp << string((const char *) &regs.EBX, 4)
        << string((const char *) &regs.EDX, 4)
        << string((const char *) &regs.ECX, 4);
    
    return tmp.str();
}

inline auto getProcLeaves() -> std::pair<unsigned, unsigned>
{
    auto leaf = cpuid(0);
    auto xtended = cpuid(0x80000000);
    
    auto max_leaf = leaf.EAX;
    auto ext_leaf = xtended.EAX;
    
    return std::make_pair(max_leaf, ext_leaf);
}

inline bool hasAVX(u32 ECX)
{
    return ECX & CPUID::avx;
}

inline auto printCPUFeatures() -> std::string {
    auto rpack = cpuid(1);
    auto ECX = rpack.ECX;
    auto EDX = rpack.EDX;
    auto EBX = cpuid(7).EBX;
    auto LEnabled = [](bool flag){return (flag ? "Enabled" : "Disabled");};
    
    std::ostringstream features;
    features << "Hyper Threading : " << LEnabled( EDX & CPUID::htt ) << "\n"
             << "MMX      : " << LEnabled( EDX & CPUID::mmx ) << "\n"
             << "SSE      : " << LEnabled( EDX & CPUID::sse ) << "\n"
             << "SSE2     : " << LEnabled( EDX & CPUID::sse2 ) << "\n"
             << "FPU      : " << LEnabled( EDX & CPUID::fpu ) << "\n"
             << "SSE4_1   : " << LEnabled( ECX & CPUID::sse4_1 ) << "\n"
             << "SSE4_2   : " << LEnabled( ECX & CPUID::sse4_2 ) << "\n"
             << "AVX      : " << LEnabled( ECX & CPUID::avx ) << "\n"
             << "AVX2     : " << LEnabled( EBX & CPUID::avx2) << "\n"
             << "popcount : " << LEnabled( ECX & CPUID::popcnt ) << "\n";
    
    return features.str();
}

inline auto L2CacheData() -> std::string
{
    auto data = cpuid(0x80000006);
    std::ostringstream s;
    s << "L2 Cache:\n"
      << "Line size (B)   : " <<( data.ECX & 0xff )<< "\n"
      << "Assoc. Type     : " <<( (data.ECX >> 12) & 0x07 )<< "x\n"
      << "Cache Size (KB) :" <<( (data.ECX >> 16) & 0xffff )<< "\n";
    
    return s.str();
}


static auto simd_flags() -> decltype(SIMD::AVX)
{
    const static CPUID::CPU cpu;
    const static auto flags = cpu.supported_simd();
    return flags;
}

}// namespace x86_cpuid