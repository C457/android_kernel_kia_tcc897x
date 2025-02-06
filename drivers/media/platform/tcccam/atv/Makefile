# Makefile for TCCxxx camera driver

obj-$(CONFIG_VIDEO_ATV_SENSOR_TVP5150) += tvp5150.o
obj-$(CONFIG_VIDEO_ATV_SENSOR_ADV7182) += adv7182.o

obj-$(CONFIG_VIDEO_ATV_SENSOR_DAUDIO) += daudio_atv.o
ifeq ($(shell echo $(BOARD3HW_GPIO)), true)
obj-$(CONFIG_VIDEO_ATV_SENSOR_DAUDIO) += tw9990.o daudio_lvds.o
else
obj-$(CONFIG_VIDEO_ATV_SENSOR_DAUDIO) += tw9921.o
endif
EXTRA_CFLAGS = -I$(src)/..
