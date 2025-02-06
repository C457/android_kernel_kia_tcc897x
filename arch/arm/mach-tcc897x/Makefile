#
# Makefile for Telechips TCC897x processors
#

# Common support

obj-y	+= cpu_early_init.o
obj-y	+= common.o io.o

ifeq ($(CONFIG_TCC_STB_PMAP_SUPPORT),y)
 obj-y	+= pmap_stb.o
else
ifeq ($(CONFIG_PLATFORM_AVN),y)
 obj-y	+= pmap_avn.o
else
 obj-y	+= pmap.o
endif
endif

obj-y	+= tcc_ckc.o
obj-y	+= pm.o tcc_sram.o
obj-y	+= restart.o

#obj-y	+= tcc_adc.o
obj-y	+= tca_serial.o

#obj-y	+= tca_spi.o
#obj-y	+= tca_hwdemux_tsif.o TCC897xHWDemux_cmd.o
#obj-y	+= tca_hwdemux_cipher_cmd.o
obj-$(CONFIG_SUPPORT_TCC_NSK)	+= tca_mailbox.o
obj-y	+= tccfb_address.o
obj-y	+= vioc/
obj-y   += tcc_g2d.o

obj-$(CONFIG_TCC_CIPHER)        += tca_cipher.o

obj-$(CONFIG_TCC_ECID_SUPPORT)  += tcc_ecid.o
obj-$(CONFIG_TCC_GMAC)			+= tca_gmac.o

obj-$(CONFIG_TCC_MIPI)          += board-tcc897x-mipi.o
# Specific board support
obj-y	+= board-tcc897x.o daudio_settings.o
obj-y	+= camera.o
obj-$(CONFIG_RTC_DRV_RX8900)    += daudio_rx8900_ext_rtc.o
obj-$(CONFIG_SMP)               += headsmp.o platsmp.o cpu_power.o
obj-$(CONFIG_HOTPLUG_CPU)	+= hotplug.o
obj-$(CONFIG_AUTO_HOTPLUG)      += auto_hotplug.o
obj-$(CONFIG_ION_TCC)           += tcc897x-ion.o

obj-$(CONFIG_SNAPSHOT_BOOT)     += snapshot_asm.o snapshot_pm.o
ifneq ($(INCLUDE_ISDB), y)
obj-y   += daudio_sabre_tuner.o
endif
obj-y  += daudio_fm1688.o
obj-$(CONFIG_RTC_DRV_RX8900) += daudio_rx8900_ext_rtc.o
obj-$(CONFIG_LK_DEBUG_LOG_BUF) += lk_debug_logbuf.o

EXTRA_CFLAGS +="-I$(srctree)/drivers/staging/android/"
