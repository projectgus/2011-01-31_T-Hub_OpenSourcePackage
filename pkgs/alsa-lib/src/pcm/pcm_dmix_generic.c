#if 0
//#if defined(__i386__) || defined(__x86_64__)
#define LOCK_PREFIX "lock ; "
#define ARCH_ADD(p,a)					\
	__asm__ __volatile__(LOCK_PREFIX "addl %1,%0"	\
			     :"=m" (*p)			\
			     :"ir" (a), "m" (*p))
struct __xchg_dummy { unsigned long a[100]; };
#define __xg(x) ((struct __xchg_dummy *)(x))
static inline unsigned long __cmpxchg(volatile void *ptr, unsigned long old,
				      unsigned long new, int size)
{
	unsigned long prev;
	switch (size) {
	case 1:
		__asm__ __volatile__(LOCK_PREFIX "cmpxchgb %b1,%2"
				     : "=a"(prev)
				     : "q"(new), "m"(*__xg(ptr)), "0"(old)
				     : "memory");
		return prev;
	case 2:
		__asm__ __volatile__(LOCK_PREFIX "cmpxchgw %w1,%2"
				     : "=a"(prev)
				     : "q"(new), "m"(*__xg(ptr)), "0"(old)
				     : "memory");
		return prev;
	case 4:
		__asm__ __volatile__(LOCK_PREFIX "cmpxchgl %1,%2"
				     : "=a"(prev)
				     : "q"(new), "m"(*__xg(ptr)), "0"(old)
				     : "memory");
		return prev;
	}
	return old;
}

#define ARCH_CMPXCHG(ptr,o,n)\
	((__typeof__(*(ptr)))__cmpxchg((ptr),(unsigned long)(o),\
					(unsigned long)(n),sizeof(*(ptr))))
#define IS_CONCURRENT	1	/* check race */
#endif

#ifndef ARCH_ADD
#define ARCH_ADD(p,a) (*(p) += (a))
#define ARCH_CMPXCHG(p,a,b) (*(p)) /* fake */
#define NO_CONCURRENT_ACCESS	/* use semaphore to avoid race */
#define IS_CONCURRENT	0	/* no race check */
#endif

#if IS_CONCURRENT
static void mix_areas1(unsigned int size,
		       volatile signed short *dst, signed short *src,
		       volatile signed int *sum, size_t dst_step,
		       size_t src_step, size_t sum_step)
{
	register signed int sample, old_sample;

	for (;;) {
		sample = *src;
		old_sample = *sum;
		if (ARCH_CMPXCHG(dst, 0, 1) == 0)
			sample -= old_sample;
		ARCH_ADD(sum, sample);
		do {
			old_sample = *sum;
			if (old_sample > 0x7fff)
				sample = 0x7fff;
			else if (old_sample < -0x8000)
				sample = -0x8000;
			else
				sample = old_sample;
			*dst = sample;
		} while (IS_CONCURRENT && *sum != old_sample);
		if (!--size)
			return;
		src = (signed short *) ((char *)src + src_step);
		dst = (signed short *) ((char *)dst + dst_step);
		sum = (signed int *)   ((char *)sum + sum_step);
	}
}

static void mix_areas2(unsigned int size,
		       volatile signed int *dst, signed int *src,
		       volatile signed int *sum, size_t dst_step,
		       size_t src_step, size_t sum_step)
{
	register signed int sample, old_sample;

	for (;;) {
		sample = *src / 256;
		old_sample = *sum;
		if (ARCH_CMPXCHG(dst, 0, 1) == 0)
			sample -= old_sample;
		ARCH_ADD(sum, sample);
		do {
			old_sample = *sum;
			if (old_sample > 0x7fffff)
				sample = 0x7fffffff;
			else if (old_sample < -0x800000)
				sample = -0x80000000;
			else
				sample = old_sample * 256;
			*dst = sample;
		} while (IS_CONCURRENT && *sum != old_sample);
		if (!--size)
			return;
		src = (signed int *) ((char *)src + src_step);
		dst = (signed int *) ((char *)dst + dst_step);
		sum = (signed int *) ((char *)sum + sum_step);
	}
}

static void mix_select_callbacks(snd_pcm_direct_t *dmix)
{
	dmix->u.dmix.mix_areas1 = mix_areas1;
	dmix->u.dmix.mix_areas2 = mix_areas2;
}

#else

/* non-concurrent version, supporting both endians */
static unsigned long long dmix_supported_format =
	(1ULL << SND_PCM_FORMAT_S16_LE) | (1ULL << SND_PCM_FORMAT_S32_LE) |
	(1ULL << SND_PCM_FORMAT_S16_BE) | (1ULL << SND_PCM_FORMAT_S32_BE) |
	(1ULL << SND_PCM_FORMAT_S24_3LE);

#include <byteswap.h>

static void mix_areas1_native(unsigned int size,
			      volatile signed short *dst, signed short *src,
			      volatile signed int *sum, size_t dst_step,
			      size_t src_step, size_t sum_step)
{
	register signed int sample;

	for (;;) {
		sample = *src;
		if (! *dst) {
			*sum = sample;
			*dst = *src;
		} else {
			sample += *sum;
			*sum = sample;
			if (sample > 0x7fff)
				sample = 0x7fff;
			else if (sample < -0x8000)
				sample = -0x8000;
			*dst = sample;
		}
		if (!--size)
			return;
		src = (signed short *) ((char *)src + src_step);
		dst = (signed short *) ((char *)dst + dst_step);
		sum = (signed int *)   ((char *)sum + sum_step);
	}
}

static void mix_areas2_native(unsigned int size,
			      volatile signed int *dst, signed int *src,
			      volatile signed int *sum, size_t dst_step,
			      size_t src_step, size_t sum_step)
{
	register signed int sample;

	for (;;) {
		sample = *src / 256;
		if (! *dst) {
			*sum = sample;
			*dst = *src;
		} else {
			sample += *sum;
			*sum = sample;
			if (sample > 0x7fffff)
				sample = 0x7fffffff;
			else if (sample < -0x800000)
				sample = -0x80000000;
			else
				sample *= 256;
			*dst = sample;
		}
		if (!--size)
			return;
		src = (signed int *) ((char *)src + src_step);
		dst = (signed int *) ((char *)dst + dst_step);
		sum = (signed int *) ((char *)sum + sum_step);
	}
}

static void mix_areas1_swap(unsigned int size,
			    volatile signed short *dst, signed short *src,
			    volatile signed int *sum, size_t dst_step,
			    size_t src_step, size_t sum_step)
{
	register signed int sample;

	for (;;) {
		sample = bswap_16(*src);
		if (! *dst) {
			*sum = sample;
			*dst = *src;
		} else {
			sample += *sum;
			*sum = sample;
			if (sample > 0x7fff)
				sample = 0x7fff;
			else if (sample < -0x8000)
				sample = -0x8000;
			*dst = bswap_16((signed short)sample);
		}
		if (!--size)
			return;
		src = (signed short *) ((char *)src + src_step);
		dst = (signed short *) ((char *)dst + dst_step);
		sum = (signed int *)   ((char *)sum + sum_step);
	}
}

static void mix_areas2_swap(unsigned int size,
			    volatile signed int *dst, signed int *src,
			    volatile signed int *sum, size_t dst_step,
			    size_t src_step, size_t sum_step)
{
	register signed int sample;

	for (;;) {
		sample = bswap_32(*src) / 256;
		if (! *dst) {
			*sum = sample;
			*dst = *src;
		} else {
			sample += *sum;
			*sum = sample;
			if (sample > 0x7fffff)
				sample = 0x7fffffff;
			else if (sample < -0x800000)
				sample = -0x80000000;
			else
				sample *= 256;
			*dst = bswap_32(sample);
		}
		if (!--size)
			return;
		src = (signed int *) ((char *)src + src_step);
		dst = (signed int *) ((char *)dst + dst_step);
		sum = (signed int *) ((char *)sum + sum_step);
	}
}

/* always little endian */
static void mix_areas3(unsigned int size,
		       volatile unsigned char *dst, unsigned char *src,
		       volatile signed int *sum, size_t dst_step,
		       size_t src_step, size_t sum_step)
{
	register signed int sample;

	for (;;) {
		sample = src[0] | (src[1] << 8) | (((signed char *)src)[2] << 16);
		if (!(dst[0] | dst[1] | dst[2])) {
			*sum = sample;
		} else {
			sample += *sum;
			*sum = sample;
			if (sample > 0x7fffff)
				sample = 0x7fffff;
			else if (sample < -0x800000)
				sample = -0x800000;
		}
		dst[0] = sample;
		dst[1] = sample >> 8;
		dst[2] = sample >> 16;
		if (!--size)
			return;
		dst += dst_step;
		src += src_step;
		sum = (signed int *) ((char *)sum + sum_step);
	}
}


static void mix_select_callbacks(snd_pcm_direct_t *dmix)
{
	if (snd_pcm_format_cpu_endian(dmix->shmptr->s.format)) {
		dmix->u.dmix.mix_areas1 = mix_areas1_native;
		dmix->u.dmix.mix_areas2 = mix_areas2_native;
	} else {
		dmix->u.dmix.mix_areas1 = mix_areas1_swap;
		dmix->u.dmix.mix_areas2 = mix_areas2_swap;
	}
	dmix->u.dmix.mix_areas3 = mix_areas3;
}

#endif
