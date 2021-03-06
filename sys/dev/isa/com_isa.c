/*	$OpenBSD: com_isa.c,v 1.4 2005/10/24 14:22:34 fgsch Exp $	*/
/*
 * Copyright (c) 1997 - 1999, Jason Downs.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name(s) of the author(s) nor the name OpenBSD
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*-
 * Copyright (c) 1993, 1994, 1995, 1996
 *	Charles M. Hannum.  All rights reserved.
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)com.c	7.5 (Berkeley) 5/16/91
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/tty.h>
#include <sys/device.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <dev/isa/isavar.h>

int com_isa_probe(struct device *, void *, void *);
void com_isa_attach(struct device *, struct device *, void *);

struct cfattach com_isa_ca = {
        sizeof(struct com_softc), com_isa_probe, com_isa_attach
};

int
com_isa_probe(struct device *parent, void *match, void *aux)
{
	struct isa_attach_args *ia = aux;
	bus_space_handle_t ioh;
	bus_space_tag_t iot;
	int iobase;
	int rv;

	iot = ia->ia_iot;
	iobase = ia->ia_iobase;

#ifdef KGDB
	if (iobase == com_kgdb_addr)
		goto out;
#endif
	if (iobase == comconsaddr && !comconsattached)
		goto out;

	if (bus_space_map(iot, iobase, COM_NPORTS, 0, &ioh))
		return (0);

	rv = comprobe1(iot, ioh);

	bus_space_unmap(iot, ioh, COM_NPORTS);

	if (rv == 0)
		return (0);

out:
	ia->ia_iosize = COM_NPORTS;
	ia->ia_msize = 0;
	return (1);
}

void
com_isa_attach(struct device *parent, struct device *self, void *aux)
{
	struct com_softc *sc = (struct com_softc *)self;
	struct isa_attach_args *ia = aux;
	bus_space_handle_t ioh;
	bus_space_tag_t iot;
	int iobase, irq;

	sc->sc_hwflags = 0;
	sc->sc_swflags = 0;

	iobase = ia->ia_iobase;
	iot = ia->ia_iot;

#ifdef KGBD
	if ((iobase != comconsaddr) &&
	    (iobase != com_kgdb_addr)) {
#else
	if (iobase != comconsaddr) {
#endif
		if (bus_space_map(iot, iobase, COM_NPORTS, 0, &ioh))
			panic("com_isa_attach: mapping failed");
	} else {
#ifdef KGDB
		if (iobase == comconsaddr)
			ioh = comconsioh;
		else
			ioh = com_kgdb_ioh;
#else
		ioh = comconsioh;
#endif
	}

	sc->sc_iot = iot;
	sc->sc_ioh = ioh;
	sc->sc_iobase = iobase;
	sc->sc_frequency = COM_FREQ;

	com_attach_subr(sc);

	irq = ia->ia_irq;
	if (irq != IRQUNK) {
#ifdef KGDB     
		if (iobase == com_kgdb_addr) {
			sc->sc_ih = isa_intr_establish(ia->ia_ic, irq,
				IST_EDGE, IPL_HIGH, kgdbintr, sc,
				sc->sc_dev.dv_xname);
		} else {
			sc->sc_ih = isa_intr_establish(ia->ia_ic, irq,
				IST_EDGE, IPL_TTY, comintr, sc,
				sc->sc_dev.dv_xname);
		}
#else   
		sc->sc_ih = isa_intr_establish(ia->ia_ic, irq,
			IST_EDGE, IPL_TTY, comintr, sc,
			sc->sc_dev.dv_xname);
#endif /* KGDB */
	}
}

