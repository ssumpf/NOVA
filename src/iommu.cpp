/*
 * IO-MMU common handling
 *
 * Copyright (C) 2020 Alexander Boettcher <alexander.boettcher@genode-labs.com>
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

#include "iommu.hpp"
#include "iommu_amd.hpp"
#include "iommu_intel.hpp"
#include "lapic.hpp"
#include "vectors.hpp"

void Iommu::Interface::vector (unsigned vector)
{
    unsigned msi = vector - VEC_MSI;

    if (EXPECT_TRUE (msi == 0)) {
        if (Dmar::online())
            Dmar::vector(vector);
        if (Iommu::Amd::online())
            Iommu::Amd::vector(vector);
    }

    Lapic::eoi();
}

void Iommu::Interface::set_irt (unsigned const gsi, unsigned const rid,
                                unsigned const aid, unsigned const vec,
                                unsigned const trg)
{
    if (Dmar::online())
        Dmar::set_irt (gsi, rid, aid, vec, trg);

    if (Iommu::Amd::online())
        Iommu::Amd::set_irt (gsi, rid, aid, vec, trg);
}

void Iommu::Interface::release (uint16 const rid, Pd * const pd)
{
    if (Dmar::online())
        Dmar::release(rid, pd);

    if (Iommu::Amd::online())
        Iommu::Amd::release(rid, pd);
}

void Iommu::Interface::flush_pgt(uint16 const rid, Pd &pd)
{
    if (Dmar::online())
        Dmar::flush_pgt(rid, pd);

    if (Iommu::Amd::online())
        Iommu::Amd::flush_pgt(rid, pd);
}
