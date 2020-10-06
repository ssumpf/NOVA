/*
 * Bit allocator
 *
 * Copyright (C) 2020 Alexander Boettcher, Genode Labs GmbH
 *
 * This file is part of the NOVA microhypervisor.
 *
 * NOVA is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NOVA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License version 2 for more details.
 */


#pragma once

#include <bits.hpp>
#include <atomic.hpp>

template<unsigned C, unsigned INV>
class Bit_alloc
{
    private:

        mword bits [C / 8 / sizeof(mword)];
        mword last { 0 };

        enum {
            BITS_CNT = sizeof(bits[0]) * 8,
            MAX      = sizeof(bits) / sizeof(bits [0])
        };

    public:

        ALWAYS_INLINE
        inline Bit_alloc()
        {
            static_assert(MAX*BITS_CNT == C, "bit allocator");
            static_assert(INV < C, "bit allocator");

            Atomic::test_set_bit(bits[INV / BITS_CNT], INV % BITS_CNT);
        }

        ALWAYS_INLINE
        inline mword alloc()
        {
            for (mword i = ACCESS_ONCE(last), j = 0; j < MAX; i++, j++)
            {
                i %= MAX;

                if (ACCESS_ONCE(bits[i]) == ~0UL)
                    continue;

                long b = bit_scan_forward (~bits[i]);
                if (b < 0 || b >= BITS_CNT || Atomic::test_set_bit (bits[i], b)) {
                    j--;
                    i--;
                    continue;
                }

                if (bits[i] != ~0UL && last != i)
                    last = i;

                return i * BITS_CNT + b;
            }

            return INV;
        }

        ALWAYS_INLINE
        inline void release(mword const id)
        {
            if (id == INV || id >= C)
               return;

            mword i = id / BITS_CNT;
            mword b = id % BITS_CNT;

            while (ACCESS_ONCE(bits[i]) & (1ul << b))
                Atomic::test_clr_bit (ACCESS_ONCE(bits[i]), b);
        }
};
