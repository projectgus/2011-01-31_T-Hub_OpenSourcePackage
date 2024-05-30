#
# Makefile for the linux kernel.
#

# Common support
obj-y := irq.o gpio.o io.o clock.o

obj-$(CONFIG_ARCH_MX27) += dma_mx2.o

obj-$(CONFIG_ARCH_MX3) += cpu_common.o spba.o dptc.o
obj-$(CONFIG_MXC_SDMA_API)  += sdma/

# LEDs support
obj-$(CONFIG_LEDS) += leds.o

obj-m :=
obj-n :=
obj-  :=
