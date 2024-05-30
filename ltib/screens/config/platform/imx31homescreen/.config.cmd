deps_config := \
	../../userspace/defaults.lkc \
	../../userspace/deployment.lkc \
	customization.lkc \
	../../userspace/network.lkc \
	../../userspace/sysconfig.lkc \
	../../userspace/packages.lkc \
	../../userspace/distro.lkc \
	../../userspace/kernel_dir_build.lkc \
	../../userspace/libc_type.lkc \
	../../userspace/endian.lkc \
	main.lkc

.config include/linux/autoconf.h: $(deps_config)

$(deps_config):
