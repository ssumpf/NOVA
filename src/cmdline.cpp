/*
 * Command Line Parser
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012-2013 Udo Steinberg, Intel Corporation.
 * Copyright (C) 2014 Udo Steinberg, FireEye, Inc.
 * Copyright (C) 2015-2018 Alexander Boettcher, Genode Labs GmbH.
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

#include "cmdline.hpp"
#include "string.hpp"
#include "pd.hpp"

bool Cmdline::iommu;
bool Cmdline::keyb;
bool Cmdline::serial;
bool Cmdline::spinner;
bool Cmdline::vtlb;
bool Cmdline::nodl;
bool Cmdline::nopcid;
bool Cmdline::novga;
bool Cmdline::novpid;
bool Cmdline::logmem;
bool Cmdline::fpu_lazy;

struct Cmdline::param_map Cmdline::map[] INITDATA =
{
    { "iommu",      &Cmdline::iommu     },
    { "keyb",       &Cmdline::keyb      },
    { "serial",     &Cmdline::serial    },
    { "spinner",    &Cmdline::spinner   },
    { "vtlb",       &Cmdline::vtlb      },
    { "nodl",       &Cmdline::nodl      },
    { "nopcid",     &Cmdline::nopcid    },
    { "novga",      &Cmdline::novga     },
    { "novpid",     &Cmdline::novpid    },
    { "logmem",     &Cmdline::logmem    },
    { "fpu_lazy",   &Cmdline::fpu_lazy  },
};

char const *Cmdline::get_arg (char const **line, unsigned &len)
{
    len = 0;

    for (; **line == ' '; ++*line) ;

    if (!**line)
        return nullptr;

    char const *arg = *line;

    for (; **line != ' '; ++*line) {
        if (!**line)
            return arg;
        len ++;
    }

    return arg;
}

void Cmdline::init (char const * line)
{
    char const *arg;
    unsigned len;

    while ((arg = get_arg (&line, len)))
        for (unsigned i = 0; i < sizeof map / sizeof *map; i++) {
            if (strmatch (map[i].arg, arg, len))
                *map[i].ptr = true;
        }
}
