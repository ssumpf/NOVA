/*
 * Memory Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
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

#include "bit_alloc.hpp"
#include "config.hpp"
#include "cpu.hpp"
#include "cpuset.hpp"
#include "dpt.hpp"
#include "ept.hpp"
#include "hpt.hpp"
#include "space.hpp"

class Pd;

class Space_mem : public Space
{
    public:
        Hpt loc[NUM_CPU];
        Hpt hpt { };
        Dpt dpt { };
        union {
            Ept ept;
            Hpt npt;
        };

        enum { NO_PCID = 2 };
        mword did { NO_PCID };

        Cpuset cpus;
        Cpuset htlb;
        Cpuset gtlb;

        static Bit_alloc<4096, NO_PCID> did_alloc;

        ALWAYS_INLINE
        inline Space_mem() : cpus(0), htlb(~0UL), gtlb(~0UL)
        {
            did = did_alloc.alloc();
        }

        ALWAYS_INLINE
        inline ~Space_mem()
        {
            did_alloc.release(did);
        }

        ALWAYS_INLINE
        inline size_t lookup (mword virt, Paddr &phys)
        {
            mword attr;
            return hpt.lookup (virt, phys, attr);
        }

        ALWAYS_INLINE
        inline void insert (Quota &quota, mword virt, unsigned o, mword attr, Paddr phys)
        {
            hpt.update (quota, virt, o, phys, attr);
        }

        ALWAYS_INLINE
        inline Paddr replace (Quota &quota, mword v, Paddr p)
        {
            return hpt.replace (quota, v, p);
        }

        INIT
        void insert_root (Quota &quota, Slab_cache &, uint64, uint64, mword = 0x7);

        bool insert_utcb (Quota &quota, Slab_cache &, mword, mword = 0);

        bool remove_utcb (mword);

        bool update (Quota_guard &quota, Mdb *, mword = 0);

        static void shootdown(Pd *);

        void init (Quota &quota, unsigned);

        ALWAYS_INLINE
        inline mword sticky_sub(mword s) { return s & 0x4; }
};
