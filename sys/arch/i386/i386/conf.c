/*	$FabBSD$	*/
/*	$OpenBSD: conf.c,v 1.127 2008/06/14 21:31:46 mbalmer Exp $	*/
/*	$NetBSD: conf.c,v 1.75 1996/05/03 19:40:20 christos Exp $	*/

/*
 * Copyright (c) 1994, 1995 Charles M. Hannum.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Charles Hannum.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/vnode.h>

#include <machine/conf.h>

#include "inet.h"

#include "wd.h"
bdev_decl(wd);
#include "fdc.h"
#include "fd.h"
bdev_decl(fd);
#include "sd.h"
#include "st.h"
#include "cd.h"
#include "uk.h"
#include "mcd.h"
bdev_decl(mcd);
#include "vnd.h"
#include "ccd.h"
#include "rd.h"

struct bdevsw	bdevsw[] =
{
	bdev_disk_init(NWD,wd),		/* 0: ST506/ESDI/IDE disk */
	bdev_swap_init(1,sw),		/* 1: swap pseudo-device */
	bdev_disk_init(NFD,fd),		/* 2: floppy diskette */
	bdev_notdef(),			/* 3 */
	bdev_disk_init(NSD,sd),		/* 4: SCSI disk */
	bdev_tape_init(NST,st),		/* 5: SCSI tape */
	bdev_disk_init(NCD,cd),		/* 6: SCSI CD-ROM */
	bdev_disk_init(NMCD,mcd),	/* 7: Mitsumi CD-ROM */
	bdev_lkm_dummy(),		/* 8 */
	bdev_lkm_dummy(),		/* 9 */
	bdev_lkm_dummy(),		/* 10 */
	bdev_lkm_dummy(),		/* 11 */
	bdev_lkm_dummy(),		/* 12 */
	bdev_lkm_dummy(),		/* 13 */
	bdev_disk_init(NVND,vnd),	/* 14: vnode disk driver */
	bdev_notdef(),			/* 15 */
	bdev_disk_init(NCCD,ccd),	/* 16: concatenated disk driver */
	bdev_disk_init(NRD,rd),		/* 17: ram disk driver */
	bdev_notdef(),			/* 18 */
	bdev_notdef(),			/* 19 */
};
int	nblkdev = sizeof(bdevsw) / sizeof(bdevsw[0]);

/* open, close, read, write, ioctl, tty, mmap */
#define cdev_pc_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
	dev_init(c,n,write), dev_init(c,n,ioctl), dev_init(c,n,stop), \
	dev_init(c,n,tty), ttpoll, dev_init(c,n,mmap), D_TTY }

/* open, close, read, ioctl */
#define cdev_joy_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
	(dev_type_write((*))) enodev, dev_init(c,n,ioctl), \
	(dev_type_stop((*))) enodev, 0, seltrue, \
	(dev_type_mmap((*))) enodev }

/* open, close, ioctl, poll -- XXX should be a generic device */
#define cdev_ocis_init(c,n) { \
        dev_init(c,n,open), dev_init(c,n,close), (dev_type_read((*))) enodev, \
        (dev_type_write((*))) enodev, dev_init(c,n,ioctl), \
        (dev_type_stop((*))) enodev, 0,  dev_init(c,n,poll), \
        (dev_type_mmap((*))) enodev, 0 }

/* open, close, read */
#define cdev_nvram_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
	(dev_type_write((*))) enodev, (dev_type_ioctl((*))) enodev, \
	(dev_type_stop((*))) enodev, 0, seltrue, \
	(dev_type_mmap((*))) enodev, 0 }


#define	mmread	mmrw
#define	mmwrite	mmrw
cdev_decl(mm);
cdev_decl(wd);
#include "pty.h"
#include "com.h"
cdev_decl(com);
cdev_decl(fd);
#include "lpt.h"
cdev_decl(lpt);
#include "ch.h"
#include "spkr.h"
cdev_decl(spkr);
#if 0 /* old (non-wsmouse) drivers */
#include "mms.h"
cdev_decl(mms);
#include "lms.h"
cdev_decl(lms);
#include "opms.h"
cdev_decl(pms);
#endif
#include "cy.h"
cdev_decl(cy);
cdev_decl(mcd);
#include "tun.h"
#include "joy.h"
#include "bthub.h"
#include "pctr.h"
#include "bios.h"
#include "iop.h"
#include "ksyms.h"
#include "usb.h"
#include "uhid.h"
#include "ugen.h"
#include "ulpt.h"
#include "ucom.h"
#include "cz.h"
cdev_decl(cztty);
#include "gpr.h"
#include "nvram.h"
cdev_decl(nvram);
#include "agp.h"
cdev_decl(agp);

/* XXX -- this needs to be supported by config(8)! */
#if (NCOM > 0) && (NPCCOM > 0)
#error com and pccom are mutually exclusive.  Sorry.
#endif

#include "wsdisplay.h"
#include "wskbd.h"
#include "wsmouse.h"
#include "wsmux.h"

#ifdef USER_PCICONF
#include "pci.h"
cdev_decl(pci);
#endif

#include "hotplug.h"
#include "gpio.h"
#include "amdmsr.h"
#include "cnc.h"

struct cdevsw	cdevsw[] =
{
	cdev_cn_init(1,cn),			/* 0: virtual console */
	cdev_ctty_init(1,ctty),			/* 1: controlling terminal */
	cdev_mm_init(1,mm),			/* 2: /dev/{null,mem,kmem,...} */
	cdev_disk_init(NWD,wd),			/* 3: ST506/ESDI/IDE disk */
	cdev_swap_init(1,sw),			/* 4: /dev/drum (swap pseudo-device) */
	cdev_tty_init(NPTY,pts),		/* 5: pseudo-tty slave */
	cdev_ptc_init(NPTY,ptc),		/* 6: pseudo-tty master */
	cdev_log_init(1,log),			/* 7: /dev/klog */
#if NPCCOM > 0
	cdev_tty_init(NPCCOM,com),		/* 8: serial port */
#else
	cdev_tty_init(NCOM,com),		/* 8: serial port */
#endif
	cdev_disk_init(NFD,fd),			/* 9: floppy disk */
	cdev_notdef(),				/* 10 */
	cdev_notdef(),				/* 11 */
	cdev_wsdisplay_init(NWSDISPLAY,	wsdisplay), /* 12: frame buffers, etc. */
	cdev_disk_init(NSD,sd),			/* 13: SCSI disk */
	cdev_tape_init(NST,st),			/* 14: SCSI tape */
	cdev_disk_init(NCD,cd),			/* 15: SCSI CD-ROM */
	cdev_lpt_init(NLPT,lpt),		/* 16: parallel printer */
	cdev_ch_init(NCH,ch),			/* 17: SCSI autochanger */
	cdev_disk_init(NCCD,ccd),		/* 18: concatenated disk driver */
	cdev_notdef(),				/* 19 */
	cdev_uk_init(NUK,uk),			/* 20: unknown SCSI */
	cdev_acpiapm_init(1,acpiapm),		/* 21: Power Management stuff */
	cdev_fd_init(1,filedesc),		/* 22: file descriptor pseudo-device */
	cdev_notdef(),				/* 23 */
	cdev_notdef(),				/* 24 */
	cdev_notdef(),				/* 25 */
	cdev_joy_init(NJOY,joy),        	/* 26: joystick */
	cdev_spkr_init(NSPKR,spkr),		/* 27: PC speaker */
	cdev_lkm_init(NLKM,lkm),		/* 28: loadable module driver */
	cdev_lkm_dummy(),			/* 29 */
	cdev_lkm_dummy(),			/* 30 */
	cdev_lkm_dummy(),			/* 31 */
	cdev_lkm_dummy(),			/* 32 */
	cdev_lkm_dummy(),			/* 33 */
	cdev_lkm_dummy(),			/* 34 */
	cdev_notdef(),				/* 35: Microsoft mouse */
	cdev_notdef(),				/* 36: Logitech mouse */
	cdev_notdef(),				/* 37: Extended PS/2 mouse */
	cdev_tty_init(NCY,cy),			/* 38: Cyclom serial port */
	cdev_disk_init(NMCD,mcd),		/* 39: Mitsumi CD-ROM */
	cdev_tun_init(NTUN,tun),		/* 40: network tunnel */
	cdev_disk_init(NVND,vnd),		/* 41: vnode disk driver */
	cdev_notdef(),				/* 42 */
	cdev_notdef(),				/* 43 */
	cdev_notdef(),				/* 44 */
	cdev_random_init(1,random),		/* 45: random data source */
	cdev_ocis_init(NPCTR,pctr),		/* 46: pentium performance counters */
	cdev_disk_init(NRD,rd),			/* 47: ram disk driver */
	cdev_ocis_init(NBIOS,bios),		/* 48: onboard BIOS PROM */
	cdev_notdef(),				/* 49 */
	cdev_ksyms_init(NKSYMS,ksyms),		/* 50: Kernel symbols device */
	cdev_notdef(),				/* 51 */
	cdev_notdef(),				/* 52 */
	cdev_notdef(),				/* 53 */
	cdev_notdef(),				/* 54 */
	cdev_notdef(),				/* 55 */
	cdev_notdef(),				/* 56 */
	cdev_notdef(),				/* 57 */
	cdev_notdef(),				/* 58 */
	cdev_notdef(),				/* 59 */
	cdev_notdef(),				/* 60 */
	cdev_usb_init(NUSB,usb),		/* 61: USB controller */
	cdev_usbdev_init(NUHID,uhid),		/* 62: USB generic HID */
	cdev_usbdev_init(NUGEN,ugen),		/* 63: USB generic driver */
	cdev_ulpt_init(NULPT,ulpt), 		/* 64: USB printers */
	cdev_notdef(),				/* 65 */
	cdev_tty_init(NUCOM,ucom),		/* 66: USB tty */
	cdev_mouse_init(NWSKBD, wskbd),		/* 67: keyboards */
	cdev_mouse_init(NWSMOUSE, wsmouse),	/* 68: mice */
	cdev_mouse_init(NWSMUX, wsmux),		/* 69: ws multiplexor */
	cdev_crypto_init(NCRYPTO,crypto),	/* 70: /dev/crypto */
	cdev_tty_init(NCZ,cztty),		/* 71: Cyclades-Z serial port */
#ifdef USER_PCICONF
	cdev_pci_init(NPCI,pci),		/* 72: PCI user */
#else
	cdev_notdef(),
#endif
	cdev_notdef(),				/* 73 */
	cdev_notdef(),				/* 74 */
	cdev_iop_init(NIOP,iop),		/* 75: I2O IOP control interface */
	cdev_notdef(),				/* 76 */
	cdev_notdef(),				/* 77 */
	cdev_notdef(),				/* 78 */
 	cdev_notdef(),				/* 79 */
	cdev_ch_init(NGPR,gpr),			/* 80: GPR400 SmartCard reader */
	cdev_ptm_init(NPTY,ptm),		/* 81: pseudo-tty ptm device */
	cdev_hotplug_init(NHOTPLUG,hotplug),	/* 82: devices hot plugging */
	cdev_gpio_init(NGPIO,gpio),		/* 83: GPIO interface */
	cdev_nvram_init(NNVRAM,nvram),		/* 84: NVRAM interface */
	cdev_notdef(),				/* 85 */
	cdev_bthub_init(NBTHUB,bthub),		/* 86: bthub */
	cdev_agp_init(NAGP,agp),		/* 87: agp */
	cdev_notdef(),				/* 88: drm (not implemented) */
	cdev_amdmsr_init(NAMDMSR,amdmsr),	/* 89: amdmsr */
	cdev_cnc_init(NCNC,cnc),		/* 90: CNC interface */
};
int	nchrdev = sizeof(cdevsw) / sizeof(cdevsw[0]);

int	mem_no = 2; 	/* major device number of memory special file */

/*
 * Swapdev is a fake device implemented
 * in sw.c used only internally to get to swstrategy.
 * It cannot be provided to the users, because the
 * swstrategy routine munches the b_dev and b_blkno entries
 * before calling the appropriate driver.  This would horribly
 * confuse, e.g. the hashing routines. Instead, /dev/drum is
 * provided as a character (raw) device.
 */
dev_t	swapdev = makedev(1, 0);

/*
 * Returns true if dev is /dev/mem or /dev/kmem.
 */
int
iskmemdev(dev_t dev)
{
	return (major(dev) == mem_no && (minor(dev) < 2 || minor(dev) == 14));
}

/*
 * Returns true if dev is /dev/zero.
 */
int
iszerodev(dev_t dev)
{
	return (major(dev) == mem_no && minor(dev) == 12);
}

dev_t
getnulldev(void)
{
	return makedev(mem_no, 2);
}

int chrtoblktbl[] = {
	/* XXXX This needs to be dynamic for LKMs. */
	/*VCHR*/	/*VBLK*/
	/*  0 */	NODEV,
	/*  1 */	NODEV,
	/*  2 */	NODEV,
	/*  3 */	0,		/* wd */
	/*  4 */	NODEV,
	/*  5 */	NODEV,
	/*  6 */	NODEV,
	/*  7 */	NODEV,
	/*  8 */	NODEV,
	/*  9 */	2,		/* fd */
	/* 10 */	3,
	/* 11 */	NODEV,
	/* 12 */	NODEV,
	/* 13 */	4,		/* sd */
	/* 14 */	5,		/* st */
	/* 15 */	6,		/* cd */
	/* 16 */	NODEV,
	/* 17 */	NODEV,
	/* 18 */	16,		/* ccd */
	/* 19 */	NODEV,
	/* 20 */	NODEV,
	/* 21 */	NODEV,
	/* 22 */	NODEV,
	/* 23 */	NODEV,
	/* 24 */	NODEV,
	/* 25 */	NODEV,
	/* 26 */	NODEV,
	/* 27 */	NODEV,
	/* 28 */	NODEV,
	/* 29 */	NODEV,
	/* 30 */	NODEV,
	/* 31 */	NODEV,
	/* 32 */	NODEV,
	/* 33 */	NODEV,
	/* 34 */	NODEV,
	/* 35 */	NODEV,
	/* 36 */	NODEV,
	/* 37 */	NODEV,
	/* 38 */	NODEV,
	/* 39 */	7,		/* mcd */
	/* 40 */	NODEV,
	/* 41 */	14,		/* vnd */
	/* 42 */	NODEV,
	/* 43 */	NODEV,
	/* 44 */	NODEV,
	/* 45 */	NODEV,
	/* 46 */	NODEV,
	/* 47 */	17,		/* rd */
	/* 48 */	NODEV,
	/* 49 */	NODEV,
	/* 50 */	NODEV,
	/* 51 */	NODEV,
	/* 52 */	NODEV,
	/* 53 */	NODEV,
	/* 54 */	NODEV,
};
int nchrtoblktbl = sizeof(chrtoblktbl) / sizeof(chrtoblktbl[0]);

/*
 * In order to map BSD bdev numbers of disks to their BIOS equivalents
 * we use several heuristics, one being using checksums of the first
 * few blocks of a disk to get a signature we can match with /boot's
 * computed signatures.  To know where from to read, we must provide a
 * disk driver name -> bdev major number table, which follows.
 * Note: floppies are not included as those are differentiated by the BIOS.
 */
int findblkmajor(struct device *dv);
dev_t dev_rawpart(struct device *);	/* XXX */

dev_t
dev_rawpart(struct device *dv)
{
	int majdev;

	majdev = findblkmajor(dv);

	switch (majdev) {
	/* add here any device you want to be checksummed on boot */
	case 0:		/* wd */
	case 4:		/* sd */
		return (MAKEDISKDEV(majdev, dv->dv_unit, RAW_PART));
		break;
	default:
		;
	}

	return (NODEV);
}

/*
 * This entire table could be autoconfig()ed but that would mean that
 * the kernel's idea of the console would be out of sync with that of
 * the standalone boot.  I think it best that they both use the same
 * known algorithm unless we see a pressing need otherwise.
 */
#include <dev/cons.h>

cons_decl(com);
cons_decl(ws);

struct	consdev constab[] = {
#if NWSDISPLAY > 0
	cons_init(ws),
#endif
#if NCOM + NPCCOM > 0
	cons_init(com),
#endif
	{ 0 },
};
