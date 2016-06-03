// The MIT License (MIT)

// Copyright (c) 2016, Microsoft

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.


#include "NativeJIT/BitOperations.h"

namespace NativeJIT
{
    namespace
    {
#ifdef _MSC_VER
        // Uses CPUID instruction to check whether POPCNT instruction is supported.
        bool IsPopCntSupported()
        {
            // CPUID instruction related constants: ID of the function which
            // retuns general information and indices of EAX and ECX registers
            // in the returned cpuInfo structure.
            const int generalInfoFunctionId = 0;
            int cpuInfo[4];
            const unsigned eaxIndex = 0;
            const unsigned ecxIndex = 2;

            // Support for popcnt instruction is indicated via the
            // CPUID.01H:ECX.POPCNT[Bit 23] flag (function ID 1, subfunction 0,
            // 23th bit in ECX).
            const int popcntFunctionId = 1;
            const int popcntSubfunctionId = 0;
            const int popcntEcxBit = 23;

            // First, check what information CPUID instruction is able to return.
            __cpuid(cpuInfo, generalInfoFunctionId);

            bool supported;

            // EAX now contains the largest function ID supported by CPUID. Check
            // that function ID with popcnt information is there.
            if (cpuInfo[eaxIndex] >= popcntFunctionId)
            {
                // If so, query that function ID.
                __cpuidex(cpuInfo, popcntFunctionId, popcntSubfunctionId);

                // Note: BitOp::TestBit is implemented through an intrinsic
                // supported on all platforms.
                supported = BitOp::TestBit(cpuInfo[ecxIndex], popcntEcxBit);
            }
            else
            {
                supported = false;
            }

            return supported;
        }
#else
        bool IsPopCntSupported()
        {
            return true;
        }
#endif
    }



    namespace BitOp
    {
        // Copied from the VS2013 bitset header.
        char const * const c_bitsSetInByte =
            "\0\1\1\2\1\2\2\3\1\2\2\3\2\3\3\4"
            "\1\2\2\3\2\3\3\4\2\3\3\4\3\4\4\5"
            "\1\2\2\3\2\3\3\4\2\3\3\4\3\4\4\5"
            "\2\3\3\4\3\4\4\5\3\4\4\5\4\5\5\6"
            "\1\2\2\3\2\3\3\4\2\3\3\4\3\4\4\5"
            "\2\3\3\4\3\4\4\5\3\4\4\5\4\5\5\6"
            "\2\3\3\4\3\4\4\5\3\4\4\5\4\5\5\6"
            "\3\4\4\5\4\5\5\6\4\5\5\6\5\6\6\7"
            "\1\2\2\3\2\3\3\4\2\3\3\4\3\4\4\5"
            "\2\3\3\4\3\4\4\5\3\4\4\5\4\5\5\6"
            "\2\3\3\4\3\4\4\5\3\4\4\5\4\5\5\6"
            "\3\4\4\5\4\5\5\6\4\5\5\6\5\6\6\7"
            "\2\3\3\4\3\4\4\5\3\4\4\5\4\5\5\6"
            "\3\4\4\5\4\5\5\6\4\5\5\6\5\6\6\7"
            "\3\4\4\5\4\5\5\6\4\5\5\6\5\6\6\7"
            "\4\5\5\6\5\6\6\7\5\6\6\7\6\7\7\x8";

        // Note: "static initialization fiasco" is not an issue here: two phase
        // initialization will ensure that this has a value of 0 before
        // initializers start executing, so the worst case scenario is that
        // GetNonZeroBitCountFallback() is used until all the initializers
        // complete and main() starts executing.
        const bool c_isPopCntSupported = IsPopCntSupported();
    }
}
