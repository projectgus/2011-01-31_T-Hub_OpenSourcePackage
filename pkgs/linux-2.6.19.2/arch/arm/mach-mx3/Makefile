#
# Makefile for the linux kernel.
#

# Object file lists.

obj-y			:= system.o iomux.o cpu.o mm.o time.o clock.o usb.o devices.o serial.o dma.o dptc.o dvfs_v2.o
obj-$(CONFIG_MACH_MX31ADS)	+= mx31ads.o mx31ads_gpio.o mxc_pm.o
obj-$(CONFIG_MACH_MX31LSV0)	+= mx31livescreen.o mx31livescreen_gpio.o mxc_pm.o
CFLAGS_mx31homescreen.o  = -I$(TOPDIR)/drivers/spi/
obj-$(CONFIG_MACH_MX31HSV1)	+= mx31homescreen.o mx31homescreen_gpio.o mxc_pm.o
obj-$(CONFIG_FACTORY_ZONE)	+= factory_conf.o
obj-$(CONFIG_ATLAS_IMX_REGS)	+= atlas_imx_regs.o
obj-$(CONFIG_BATT_CH)		+= batt_ch.o

# power management
obj-$(CONFIG_PM) 		+= pm.o
obj-$(CONFIG_DPM)		+= dpm.o
