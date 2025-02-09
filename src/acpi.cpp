/*
 * Advanced Configuration and Power Interface (ACPI)
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

#include "acpi.hpp"
#include "acpi_dmar.hpp"
#include "acpi_fadt.hpp"
#include "acpi_hpet.hpp"
#include "acpi_ivrs.hpp"
#include "acpi_madt.hpp"
#include "acpi_mcfg.hpp"
#include "acpi_rsdp.hpp"
#include "acpi_rsdt.hpp"
#include "assert.hpp"
#include "bits.hpp"
#include "gsi.hpp"
#include "hpt.hpp"
#include "io.hpp"
#include "stdio.hpp"
#include "x86.hpp"
#include "pd.hpp"

Paddr       Acpi::dmar, Acpi::fadt, Acpi::hpet, Acpi::madt, Acpi::mcfg, Acpi::rsdt, Acpi::xsdt, Acpi::ivrs;
Acpi_gas    Acpi::pm1a_sts, Acpi::pm1b_sts, Acpi::pm1a_ena, Acpi::pm1b_ena, Acpi::pm1a_cnt, Acpi::pm1b_cnt, Acpi::pm2_cnt, Acpi::pm_tmr, Acpi::reset_reg;
Acpi_gas    Acpi::gpe0_sts, Acpi::gpe1_sts, Acpi::gpe0_ena, Acpi::gpe1_ena;
uint32      Acpi::feature;
uint8       Acpi::reset_val;
unsigned    Acpi::irq, Acpi::gsi;
bool        Acpi_table_madt::sci_overridden = false;

void Acpi::delay (unsigned ms)
{
    unsigned cnt = timer_frequency * ms / 1000;
    unsigned val = read (PM_TMR);

    while ((read (PM_TMR) - val) % (1UL << 24) < cnt)
        pause();
}


void Acpi::reset()
{
    write (RESET, reset_val);
}

void Acpi::setup()
{
    if (!xsdt && !rsdt)
        Acpi_rsdp::parse();

    if (xsdt)
        static_cast<Acpi_table_rsdt *>(Hpt::remap (Pd::kern.quota, xsdt))->parse (xsdt, sizeof (uint64));
    else if (rsdt)
        static_cast<Acpi_table_rsdt *>(Hpt::remap (Pd::kern.quota, rsdt))->parse (rsdt, sizeof (uint32));

    if (fadt)
        static_cast<Acpi_table_fadt *>(Hpt::remap (Pd::kern.quota, fadt))->parse();
    if (hpet)
        static_cast<Acpi_table_hpet *>(Hpt::remap (Pd::kern.quota, hpet))->parse();
    if (madt)
        static_cast<Acpi_table_madt *>(Hpt::remap (Pd::kern.quota, madt))->parse();
    if (mcfg)
        static_cast<Acpi_table_mcfg *>(Hpt::remap (Pd::kern.quota, mcfg))->parse();
    if (dmar)
        static_cast<Acpi_table_dmar *>(Hpt::remap (Pd::kern.quota, dmar))->parse();
    if (ivrs)
        static_cast<Acpi_table_ivrs *>(Hpt::remap (Pd::kern.quota, ivrs))->parse();

    if (!Acpi_table_madt::sci_overridden) {
        Acpi_intr sci_override;
        sci_override.bus = 0;
        sci_override.irq = static_cast<uint8>(irq);
        sci_override.gsi = irq;
        sci_override.flags.pol = Acpi_inti::POL_CONFORMING;
        sci_override.flags.trg = Acpi_inti::TRG_CONFORMING;
        Acpi_table_madt::parse_intr (&sci_override);
    }

    gsi = Gsi::irq_to_gsi (irq);

    clear (GPE0_ENA, 0);
    clear (GPE1_ENA, 0);

    trace (TRACE_ACPI, "ACPI: GSI:%#x", gsi);
}

unsigned Acpi::read (Register reg)
{
    switch (reg) {
        case PM1_STS:
            return hw_read (&pm1a_sts) | hw_read (&pm1b_sts);
        case PM1_ENA:
            return hw_read (&pm1a_ena) | hw_read (&pm1b_ena);
        case PM1_CNT:
            return hw_read (&pm1a_cnt) | hw_read (&pm1b_cnt);
        case PM2_CNT:
            return hw_read (&pm2_cnt);
        case PM_TMR:
            return hw_read (&pm_tmr);
        case RESET:
            break;
        default:
            Console::panic ("Unimplemented register Acpi::read");
            break;
    }

    return 0;
}

void Acpi::clear (Register reg, unsigned val)
{
    switch (reg) {
        case GPE0_ENA:
            hw_write (&gpe0_ena, val, true);
            break;
        case GPE1_ENA:
            hw_write (&gpe1_ena, val, true);
            break;
        default:
            Console::panic ("Unimplemented register Acpi::clear");
            break;
    }
}

void Acpi::write (Register reg, unsigned val)
{
    // XXX: Spec requires that certain bits be preserved.

    switch (reg) {
        case PM1_STS:
            hw_write (&pm1a_sts, val);
            hw_write (&pm1b_sts, val);
            break;
        case PM1_ENA:
            hw_write (&pm1a_ena, val);
            hw_write (&pm1b_ena, val);
            break;
        case PM1_CNT:
            hw_write (&pm1a_cnt, val);
            hw_write (&pm1b_cnt, val);
            break;
        case PM2_CNT:
            hw_write (&pm2_cnt, val);
            break;
        case PM_TMR:                    // read-only
            break;
        case RESET:
            hw_write (&reset_reg, val);
            break;
        default:
            Console::panic ("Unimplemented register Acpi::write");
            break;
    }
}

unsigned Acpi::hw_read (Acpi_gas *gas)
{
    if (!gas->bits)     // Register not implemented
        return 0;

    if (gas->asid == Acpi_gas::IO) {
        switch (gas->bits) {
            case 8:
                return Io::in<uint8>(static_cast<unsigned>(gas->addr));
            case 16:
                return Io::in<uint16>(static_cast<unsigned>(gas->addr));
            case 32:
                return Io::in<uint32>(static_cast<unsigned>(gas->addr));
        }
    }

    Console::panic ("Unimplemented ASID %d bits=%d", gas->asid, gas->bits);
}

void Acpi::hw_write (Acpi_gas *gas, unsigned val, bool prm)
{
    if (!gas->bits)     // Register not implemented
        return;

    if (gas->asid == Acpi_gas::IO) {
        switch (gas->bits) {
            case 8:
                Io::out (static_cast<unsigned>(gas->addr), static_cast<uint8>(val));
                return;
            case 16:
                Io::out (static_cast<unsigned>(gas->addr), static_cast<uint16>(val));
                return;
            case 32:
                Io::out (static_cast<unsigned>(gas->addr), static_cast<uint32>(val));
                return;
            case 64:
            case 128:
               if (!prm)
                   break;

               for (unsigned i = 0; i < gas->bits / 32; i++)
                   Io::out (static_cast<unsigned>(gas->addr) + i * 4, static_cast<uint32>(val));
               return;
        }
    }

    Console::panic ("Unimplemented ASID %d bits=%d prm=%u", gas->asid, gas->bits, prm);
}
