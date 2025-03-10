#
# Makefile for the kernel character device drivers.
#
obj-$(CONFIG_TCC_VIOCMG)	+= viocmg/
obj-$(CONFIG_TCC_DISPMG)	+= tcc_dispman.o
ifdef CONFIG_ARM64
else
#obj-y						+= tcc_wdma.o
endif

obj-y				+= mem.o random.o
obj-$(CONFIG_TTY_PRINTK)	+= ttyprintk.o
obj-y				+= misc.o
obj-$(CONFIG_ATARI_DSP56K)	+= dsp56k.o
obj-$(CONFIG_VIRTIO_CONSOLE)	+= virtio_console.o
obj-$(CONFIG_RAW_DRIVER)	+= raw.o
obj-$(CONFIG_SGI_SNSC)		+= snsc.o snsc_event.o
obj-$(CONFIG_MSM_SMD_PKT)	+= msm_smd_pkt.o
obj-$(CONFIG_MSPEC)		+= mspec.o
obj-$(CONFIG_MMTIMER)		+= mmtimer.o
obj-$(CONFIG_UV_MMTIMER)	+= uv_mmtimer.o
obj-$(CONFIG_IBM_BSR)		+= bsr.o
obj-$(CONFIG_SGI_MBCS)		+= mbcs.o
obj-$(CONFIG_BFIN_OTP)		+= bfin-otp.o

obj-$(CONFIG_PRINTER)		+= lp.o

obj-$(CONFIG_APM_EMULATION)	+= apm-emulation.o

obj-$(CONFIG_DTLK)		+= dtlk.o
obj-$(CONFIG_APPLICOM)		+= applicom.o
obj-$(CONFIG_SONYPI)		+= sonypi.o
obj-$(CONFIG_RTC)		+= rtc.o
obj-$(CONFIG_HPET)		+= hpet.o
obj-$(CONFIG_GEN_RTC)		+= genrtc.o
obj-$(CONFIG_EFI_RTC)		+= efirtc.o
obj-$(CONFIG_DS1302)		+= ds1302.o
obj-$(CONFIG_XILINX_HWICAP)	+= xilinx_hwicap/
ifeq ($(CONFIG_GENERIC_NVRAM),y)
  obj-$(CONFIG_NVRAM)	+= generic_nvram.o
else
  obj-$(CONFIG_NVRAM)	+= nvram.o
endif
obj-$(CONFIG_TOSHIBA)		+= toshiba.o
obj-$(CONFIG_I8K)		+= i8k.o
obj-$(CONFIG_DS1620)		+= ds1620.o
obj-$(CONFIG_HW_RANDOM)		+= hw_random/
obj-$(CONFIG_PPDEV)		+= ppdev.o
obj-$(CONFIG_NWBUTTON)		+= nwbutton.o
obj-$(CONFIG_NWFLASH)		+= nwflash.o
obj-$(CONFIG_SCx200_GPIO)	+= scx200_gpio.o
obj-$(CONFIG_PC8736x_GPIO)	+= pc8736x_gpio.o
obj-$(CONFIG_NSC_GPIO)		+= nsc_gpio.o
obj-$(CONFIG_GPIO_TB0219)	+= tb0219.o
obj-$(CONFIG_TELCLOCK)		+= tlclk.o


obj-$(CONFIG_TCC_WDMA)				+= tcc_wdma.o
obj-$(CONFIG_TCC_LUT_DRV)			+= tcc_lut.o 
obj-$(CONFIG_TCC_HW_JPEG_ENC)		+= tcc_jpegenc.o tcc_jpegenc_app.o 
obj-$(CONFIG_TCC_HW_JPEG_DEC)		+= tcc_jpegdec.o tcc_jpegdec_app.o
obj-$(CONFIG_TCC_HDMI_DRIVER_V1_3)	+= hdmi_v1_3/
obj-$(CONFIG_TCC_HDMI_DRIVER_V1_4)	+= hdmi_v1_4/
obj-$(CONFIG_TCC_HDMI_DRIVER_V2_0)	+= hdmi_v2_0/
obj-$(CONFIG_TCC_HDMI_DRIVER_V2_0)	+= hdmi_v2_0_cec/

# HDCP 2.2 for HDMI
ifeq ($(CONFIG_ARCH_TCC898X),y)
obj-$(CONFIG_TCC_HDCP2_CORE_DRIVER)	+= hdcp2_core/
endif

obj-$(CONFIG_TCC_VPU_DRV)			+= vpu/
ifeq ($(CONFIG_TCC_HDMI_DRIVER_V2_0),y)
obj-$(CONFIG_TCC_OUTPUT_STARTER)	+= tcc_output_starter_hdmi_v2_0.o
endif
obj-$(CONFIG_TCC_OUTPUT_STARTER)	+= tcc_output_starter.o

obj-y				+= tcc_cipher/
obj-y				+= tcc_otp/

obj-$(CONFIG_TCC_MEM)		+= tcc_mem.o

obj-$(CONFIG_MWAVE)		+= mwave/
obj-y				+= agp/
obj-$(CONFIG_PCMCIA)		+= pcmcia/

obj-$(CONFIG_HANGCHECK_TIMER)	+= hangcheck-timer.o
obj-$(CONFIG_TCG_TPM)		+= tpm/

obj-$(CONFIG_DCC_TTY)		+= dcc_tty.o
obj-$(CONFIG_PS3_FLASH)		+= ps3flash.o

obj-$(CONFIG_JS_RTC)		+= js-rtc.o
js-rtc-y = rtc.o

obj-$(CONFIG_TILE_SROM)		+= tile-srom.o
obj-$(CONFIG_XILLYBUS)		+= xillybus/
ifeq ($(CONFIG_ANDROID),y)
obj-$(CONFIG_TCC_BT_DEV)  += tcc_bluetooth.o
endif
ifeq ($(CONFIG_ARCH_TCC898X),y)
obj-$(CONFIG_TCC_BT_DEV)  += tcc_bluetooth.o
endif
obj-$(CONFIG_TCC_BT_DEV)	+= tcc_bt_dev.o


#//+[TCCQB] QB driver ( qb_com )
#obj-$(CONFIG_SNAPSHOT_BOOT)	+= qb_common.o
#//-[TCCQB]
#//

obj-$(CONFIG_SENSOR_BMA150) += sensor_bma150.o
obj-$(CONFIG_SENSOR_BMA220) += sensor_bma220.o
obj-$(CONFIG_SENSOR_BMA023) += sensor_bma023.o
obj-$(CONFIG_SENSOR_BMA250) += sensor_bma250.o

obj-$(CONFIG_TCC_EARLY_VIEW_CM_CONTROL)	+= tcc_early_view_control/
obj-$(CONFIG_TCC_CP)		+= tcc_cp.o
obj-$(CONFIG_TCC_CP)		+= tcc_admem.o
obj-$(CONFIG_TCC_CRANKING_DETECT)	+= tcc_crankig_detect.o

# D-AUdio
obj-$(CONFIG_DAUDIO_IE)			+= daudio_ie.o daudio_ie_twxxxx.o daudio_ie_tcc.o
obj-$(CONFIG_DAUDIO)			+= daudio_eng.o
obj-y += daudio/

ifeq ($(CONFIG_ARCH_TCC802X),y)
obj-$(CONFIG_TCC_MICOM_IPC) += tcc_ipc/
endif
