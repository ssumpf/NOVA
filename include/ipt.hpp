/*
 * I/O Page Table (IPT)
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

#include "pte.hpp"

class Ipt : public Pte<Ipt, uint64, 4, 9, true, true>
{
    public:
        static mword ord;

        static bool active() { return ord != ~0UL; }

        enum
        {
            IPT_R   = 1ULL << 61,
            IPT_W   = 1ULL << 62,

            PTE_P   = 1ULL << 0,
            PTE_N   = IPT_R | IPT_W | PTE_P,
        };

        ALWAYS_INLINE
        inline Paddr addr() const
        {
            Paddr paddr = static_cast<Paddr>(val) & ~0xFFFul;
            return paddr & ((1ULL << 52) - 1);
        }

        ALWAYS_INLINE
        static inline uint64 hw_attr (mword a)
        {
            uint64 r = 0;
            r |= (a & 0x1) ? uint64(IPT_R) : 0;
            r |= (a & 0x2) ? uint64(IPT_W) : 0;
            r |= r ? uint64(PTE_P) : 0;
            return r;
        }

        ALWAYS_INLINE
        inline bool super(unsigned long const l) const { return l && !((val >> 9) & 0x7); }

        ALWAYS_INLINE
        static inline uint64 pte_s(unsigned long const) { return 0; }
};
