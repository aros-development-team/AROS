#
# Copyright (c) 2002-2008 Sam Leffler, Errno Consulting
# Copyright (c) 2002-2008 Atheros Communications, Inc.
# Copyright (c) 2026 Neil Cafferkey
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#
# $Id$
#
# This file is intended to be included by the Makefile in the hal directory.
#
# There is support for multiple "generations" of Atheros hardware devices:
#
# AH_SUPPORT_AR5210	802.11a only MAC
# AH_SUPPORT_AR5211	802.11a+802.11b MAC (also pure g, but not yet supported)
# AH_SUPPORT_AR5212	802.11a+802.11b+802.11g MAC
# AH_SUPPORT_AR5312     5212 MAC WiSoC (requires AH_SUPPORT_AR5212)
# AH_SUPPORT_AR5416     802.11a+802.11g+802.11n MAC (requires AH_SUPPORT_5212)
#			(Also supports AR9160, AR9280 and AR9285)
#
# AH_SUPPORT_2133	2133 RF support for 5416
# AH_SUPPORT_2316	2316 RF (Cobra) support for 5212 et. al.
# AH_SUPPORT_2317	2317 RF (Spyder) support for 5212 et. al.
# AH_SUPPORT_2413	2413 RF support for 5212 et. al.
# AH_SUPPORT_2417	2425 RF (Nala) support for 5212 et. al.
# AH_SUPPORT_2425	2425 RF (Swan) support for 5212 et. al.
# AH_SUPPORT_5111	5111 RF support for 5212 et. al.
# AH_SUPPORT_5112	5112 RF support for 5212 et. al.
# AH_SUPPORT_5413	5413 RF support for 5212 et. al.
#
# and numerous optional debugging facilities (typically named AH_DEBUG_*):
#
# AH_DEBUG		enables debugging stuff that's controlled by the sysctl:
# 			hw.ath.hal.debug
# AH_ASSERT 		enables assertions that panic or trap into the debugger.
# AH_DEBUG_ALQ 		enables support for tracing register reads+writes
#			via alq
# AH_DEBUG_COUNTRY	accept the CTR_DEBUG country code (for debugging
#			the regulatory domain support)
# AH_REGOPS_FUNC	arrange for OS_REG_READ and OS_REG_WRITE to be function
#			calls and not inline expanded to memory references
# AH_ENABLE_AP_SUPPORT	enables AP-specific support not publicly available
#			(e.g. don't cap turbo power limits)
# AH_DISABLE_WME	disable HAL_TXQ_USE_LOCKOUT_BKOFF_DIS support
#
# AH_PRIVATE_DIAG	enable private diagnostic support
# AH_WRITE_EEPROM	permit EEPROM writes (use with AH_PRIVATE_DIAG)
# AH_WRITE_REGDOMAIN	permit EEPROM writes of the regulatory domain through
#			ah_setRegulatoryDomain (implies AH_WRITE_EEPROM)
#

#
# Default settings...
#
AH_SUPPORT_AR5210?=0		# 5210 support
AH_SUPPORT_AR5211?=1		# 5211 support
AH_SUPPORT_AR5212?=1		# 5212 support
AH_SUPPORT_AR5312?=0		# 5312 support
AH_SUPPORT_AR5416?=1		# 5416 support
AH_SUPPORT_2316?=1		# 2316 RF support
AH_SUPPORT_2317?=1		# 2317 RF support
AH_SUPPORT_2133?=0		# 2133 RF support
AH_SUPPORT_2413?=1		# 2413 RF support
AH_SUPPORT_2417?=1		# 2417 RF support
AH_SUPPORT_2425?=1		# 2425 RF support
AH_SUPPORT_5111?=1		# 5111 RF support
AH_SUPPORT_5112?=1		# 5112 RF support
AH_SUPPORT_5413?=1		# 5413 RF support

AH_EEPROM_V1?=0			# v1+ EEPROM support
AH_EEPROM_V3?=0			# v3+ EEPROM support
AH_EEPROM_V14?=0		# v14+ EEPROM support
AH_EEPROM_V4K?=0		# v4k EEPROM support

AH_DEBUG?=0			# debugging (e.g. hw.ath.hal.debug)
AH_ASSERT?=0			# assertions (pretty lame)
AH_DEBUG_ALQ?=0			# ALQ register tracing support
AH_REGOPS_FUNC?=0		# use inline memory references
AH_WRITE_REGDOMAIN?=0		# support EEPROM writes of the regulatory domain
AH_DEBUG_COUNTRY?=0		# accept CTR_DEBUG country code
AH_DISABLE_WME?=0		# disable HAL_TXQ_USE_LOCKOUT_BKOFF_DIS
AH_ENABLE_AP_SUPPORT?=0		# enable AP-specific support
AH_WRITE_EEPROM?=0		# support EEPROM writes
AH_PRIVATE_DIAG?=0		# non-public diagnostic support
AH_NEED_DESC_SWAP?=0		# Need to swap TX descriptor
AH_ENABLE_FORCEBIAS?=0		# 2133 orientation sensitivity

#
# If 5312 support is enabled it's certain we're not going
# to use the 5210, 5211, or 5416 support so disable them.
#
ifeq ($(strip ${AH_SUPPORT_AR5312}),1)
AH_SUPPORT_AR5210=0
AH_SUPPORT_AR5211=0
AH_SUPPORT_AR5416=0
AH_SUPPORT_2133=0
AH_SUPPORT_2413=0
AH_SUPPORT_2417=0
AH_SUPPORT_2425=0
AH_SUPPORT_5413=0
AH_SUPPORT_AR5212=1		# force 5212 support
endif

#
# 5210 requires v1 EEPROM support.
#
ifeq ($(strip ${AH_SUPPORT_AR5210}),1)
AH_EEPROM_V1=1
endif

#
# 5211 and 5212 require v3 EEPROM support; do it here
# before we enable 5212 to support 5416 (this way we don't
# pull in v3 support as a side effect).
#
ifeq ($(strip ${AH_SUPPORT_AR5212}),1)
AH_EEPROM_V3=1
endif
ifeq ($(strip ${AH_SUPPORT_AR5211}),1)
AH_EEPROM_V3=1
endif

#
# If 5416 support is enabled force 5212 on also.
#
ifeq ($(strip ${AH_SUPPORT_AR5416}),1)
AH_SUPPORT_AR5212=1
endif

#
# 5416 support presently =>'s 2133 and force bias support;
# do those automatically (but separately from 5212 for now).
#
ifeq ($(strip ${AH_SUPPORT_AR5416}),1)
AH_SUPPORT_2133=1
AH_ENABLE_FORCEBIAS=1
AH_EEPROM_V14=1
AH_EEPROM_V4K=1
endif

#
# 2417 support presently requires 2425 support
#
ifeq ($(strip ${AH_SUPPORT_2417}),1)
AH_SUPPORT_2425=1
endif

#
# Define required object files.
#
HAL_OBJS=ah.o ah_regdomain.o

ifeq ($(strip ${AH_EEPROM_V1}),1)
HAL_OBJS+=ah_eeprom_v1.o
endif

ifeq ($(strip ${AH_EEPROM_V3}),1)
HAL_OBJS+=ah_eeprom_v3.o
endif

ifeq ($(strip ${AH_EEPROM_V14}),1)
HAL_OBJS+=ah_eeprom_v14.o
endif

ifeq ($(strip ${AH_EEPROM_V4K}),1)
HAL_OBJS+=ah_eeprom_v4k.o
endif

ifeq ($(strip ${AH_SUPPORT_AR5210}),1)
HAL_OBJS+=ar5210/ar5210_attach.o ar5210/ar5210_beacon.o \
	ar5210/ar5210_interrupts.o ar5210/ar5210_keycache.o \
	ar5210/ar5210_misc.o ar5210/ar5210_power.o \
	ar5210/ar5210_phy.o ar5210/ar5210_recv.o \
	ar5210/ar5210_reset.o ar5210/ar5210_xmit.o
endif

ifeq ($(strip ${AH_SUPPORT_AR5211}),1)
HAL_OBJS+=ar5211/ar5211_attach.o ar5211/ar5211_beacon.o \
	ar5211/ar5211_interrupts.o ar5211/ar5211_keycache.o \
	ar5211/ar5211_misc.o ar5211/ar5211_power.o \
	ar5211/ar5211_phy.o ar5211/ar5211_recv.o \
	ar5211/ar5211_reset.o ar5211/ar5211_xmit.o
endif

ifeq ($(strip ${AH_SUPPORT_AR5212}),1)
HAL_OBJS+=ar5212/ar5212_attach.o ar5212/ar5212_beacon.o \
	ar5212/ar5212_eeprom.o ar5212/ar5212_gpio.o \
	ar5212/ar5212_interrupts.o ar5212/ar5212_keycache.o \
	ar5212/ar5212_misc.o ar5212/ar5212_power.o \
	ar5212/ar5212_phy.o ar5212/ar5212_recv.o \
	ar5212/ar5212_reset.o ar5212/ar5212_xmit.o \
	ar5212/ar5212_ani.o ar5212/ar5212_rfgain.o
endif

ifeq ($(strip ${AH_SUPPORT_AR5312}),1)
HAL_OBJS+=ar5312/ar5312_attach.o ar5312/ar5312_interrupts.o \
	ar5312/ar5312_eeprom.o \
	ar5312/ar5312_gpio.o ar5312/ar5315_gpio.o \
	ar5312/ar5312_misc.o ar5312/ar5312_power.o \
	ar5312/ar5312_reset.o
endif

ifeq ($(strip ${AH_SUPPORT_AR5416}),1)
HAL_OBJS+=ar5416/ar5416_ani.o \
	ar5416/ar5416_attach.o ar5416/ar5416_beacon.o \
	ar5416/ar5416_cal.o ar5416/ar5416_cal_adcdc.o \
	ar5416/ar5416_cal_adcgain.o ar5416/ar5416_cal_iq.o \
	ar5416/ar5416_eeprom.o ar5416/ar5416_gpio.o \
	ar5416/ar5416_interrupts.o ar5416/ar5416_keycache.o \
	ar5416/ar5416_misc.o ar5416/ar5416_power.o \
	ar5416/ar5416_phy.o ar5416/ar5416_recv.o \
	ar5416/ar5416_reset.o ar5416/ar5416_xmit.o \
	ar5416/ar9160_attach.o ar5416/ar9280_attach.o \
	ar5416/ar9280.o ar5416/ar9285_attach.o \
	ar5416/ar9285.o ar5416/ar9285_reset.o
endif

ifeq ($(strip ${AH_SUPPORT_2316}),1)
HAL_OBJS+=ar5212/ar2316.o
endif

ifeq ($(strip ${AH_SUPPORT_2317}),1)
HAL_OBJS+=ar5212/ar2317.o
endif

ifeq ($(strip ${AH_SUPPORT_2133}),1)
HAL_OBJS+=ar5416/ar2133.o
endif

ifeq ($(strip ${AH_SUPPORT_2413}),1)
HAL_OBJS+=ar5212/ar2413.o
endif

ifeq ($(strip ${AH_SUPPORT_2425}),1)
HAL_OBJS+=ar5212/ar2425.o
endif

ifeq ($(strip ${AH_SUPPORT_5111}),1)
HAL_OBJS+=ar5212/ar5111.o
endif

ifeq ($(strip ${AH_SUPPORT_5112}),1)
HAL_OBJS+=ar5212/ar5112.o
endif

ifeq ($(strip ${AH_SUPPORT_5413}),1)
HAL_OBJS+=ar5212/ar5413.o
endif

#
# Generate build options header file.
#
OPT_AH_PATH?=opt_ah.h

${OPT_AH_PATH}:
	echo "#define AH_HAS_RF 1" >opt_ah.h
ifeq ($(strip ${AH_SUPPORT_AR5210}),1)
	echo "#define AH_SUPPORT_AR5210 1" >>opt_ah.h
endif
ifeq ($(strip ${AH_SUPPORT_AR5211}),1)
	echo "#define AH_SUPPORT_AR5211 1" >>opt_ah.h
endif
ifeq ($(strip ${AH_SUPPORT_AR5212}),1)
	echo "#define AH_SUPPORT_AR5212 1" >>opt_ah.h
endif
ifeq ($(strip ${AH_SUPPORT_AR5312}),1)
	echo "#define AH_SUPPORT_AR5312 1" >>opt_ah.h
endif
ifeq ($(strip ${AH_SUPPORT_AR5416}),1)
	echo "#define AH_SUPPORT_AR5416 1" >>opt_ah.h
endif
ifeq ($(strip ${AH_SUPPORT_2316}),1)
	echo "#define AH_SUPPORT_2316 1" >>opt_ah.h
endif
ifeq ($(strip ${AH_SUPPORT_2317}),1)
	echo "#define AH_SUPPORT_2317 1" >>opt_ah.h
endif
ifeq ($(strip ${AH_SUPPORT_2133}),1)
	echo "#define AH_SUPPORT_2133 1" >>opt_ah.h
endif
ifeq ($(strip ${AH_SUPPORT_2413}),1)
	echo "#define AH_SUPPORT_2413 1" >>opt_ah.h
endif
ifeq ($(strip ${AH_SUPPORT_2417}),1)
	echo "#define AH_SUPPORT_2417 1" >>opt_ah.h
endif
ifeq ($(strip ${AH_SUPPORT_2425}),1)
	echo "#define AH_SUPPORT_2425 1" >>opt_ah.h
endif
ifeq ($(strip ${AH_SUPPORT_5111}),1)
	echo "#define AH_SUPPORT_5111 1" >>opt_ah.h
endif
ifeq ($(strip ${AH_SUPPORT_5112}),1)
	echo "#define AH_SUPPORT_5112 1" >>opt_ah.h
endif
ifeq ($(strip ${AH_SUPPORT_5413}),1)
	echo "#define AH_SUPPORT_5413 1" >>opt_ah.h
endif
ifeq ($(strip ${AH_ASSERT}),1)
	echo "#define AH_ASSERT 1" >>opt_ah.h
endif
ifeq ($(strip ${AH_DEBUG_ALQ}),1)
	echo "#define AH_DEBUG_ALQ 1" >>opt_ah.h
endif
ifeq ($(strip ${AH_DEBUG}),1)
	echo "#define AH_DEBUG 1" >>opt_ah.h
endif
ifeq ($(strip ${AH_WRITE_REGDOMAIN}),1)
	echo "#define AH_SUPPORT_WRITE_REGDOMAIN 1" >>opt_ah.h
	echo "#define AH_SUPPORT_WRITE_EEPROM 1" >>opt_ah.h
else
ifeq ($(strip ${AH_WRITE_EEPROM}),1)
	echo "#define AH_SUPPORT_WRITE_EEPROM 1" >>opt_ah.h
endif
endif
ifeq ($(strip ${AH_DEBUG_COUNTRY}),1)
	echo "#define AH_DEBUG_COUNTRY 1" >>opt_ah.h
endif
ifeq ($(strip ${AH_REGOPS_FUNC}),1)
	echo "#define AH_REGOPS_FUNC 1" >>opt_ah.h
endif
ifeq ($(strip ${AH_DISABLE_WME}),1)
	echo "#define AH_DISABLE_WME 1" >>opt_ah.h
endif
ifeq ($(strip ${AH_ENABLE_AP_SUPPORT}),1)
	echo "#define AH_ENABLE_AP_SUPPORT 1" >>opt_ah.h
endif
ifeq ($(strip ${AH_PRIVATE_DIAG}),1)
	echo "#define AH_PRIVATE_DIAG 1" >>opt_ah.h
endif
ifeq ($(strip ${AH_NEED_DESC_SWAP}),1)
	echo "#define AH_NEED_DESC_SWAP 1" >>opt_ah.h
endif
ifeq ($(strip ${AH_ENABLE_FORCEBIAS}),1)
	echo "#define AH_ENABLE_FORCEBIAS 1" >>opt_ah.h
endif
