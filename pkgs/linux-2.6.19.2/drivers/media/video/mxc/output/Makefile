ifeq ($(CONFIG_VIDEO_MXC_EMMA_OUTPUT),y)
	mx27_output-objs := mx27_v4l2_output.o mx27_pp.o
	obj-$(CONFIG_VIDEO_MXC_OUTPUT) += mx27_output.o 
endif

ifeq ($(CONFIG_VIDEO_MXC_IPU_OUTPUT),y)
 ifeq ($(CONFIG_MXC_MBX),y)
	obj-$(CONFIG_VIDEO_MXC_OUTPUT)	+= mxc_v4l2_output.o
#	obj-$(CONFIG_VIDEO_MXC_OUTPUT)	+= mxc_v4l2_output_gl.o
 else
	obj-$(CONFIG_VIDEO_MXC_OUTPUT)	+= mxc_v4l2_output.o
 endif
endif
