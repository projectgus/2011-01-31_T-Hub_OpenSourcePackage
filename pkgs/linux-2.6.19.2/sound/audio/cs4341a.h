#ifndef _CS4341A_DRIVER_H_
#define _CS4341A_DRIVER_H_

#ifndef CS4341A_MAJOR
#define CS4341A_MAJOR 15
#endif

#ifndef CS4341A_NR_DEVS
#define CS4341A_NR_DEVS 1
#endif

#define IOMUX_RESET_NAME MX31_PIN_GPIO3_1

/* #define FECH_KHZ 48 */
/* #define FECH_HZ FECH_KHZ*1000 */
/* #define FIFO_SLOT_SIZE_BITS 24 */
/* #define FIFO_SLOT_SIZE_BYTE (FIFO_SLOT_SIZE_BITS >> 3 ) */
#define FIFO_MAX_N_SLOT 8
/* #define FIFO_SIZE_BYTE (FIFO_MAX_N_SLOT*FIFO_SLOT_SIZE_BYTE) */

/* #define TIME_BUFFER_MS 50 */
/* #define BUFFER_SSI_SIZE TIME_BUFFER_MS * FIFO_SLOT_SIZE_BYTE * FECH_KHZ  /\* in bytes *\/ */
/* #define HALF_BUFFER (BUFFER_SSI_SIZE >> 1) */
/* #define CHUNK_SIZE (HALF_BUFFER >> 1) */
/* #define CHUNK_SIZE 1024 */

/* #define INIT_FSYNC FECH * FIFO_SLOT_SIZE_BYTE */

#define N_PAGE 64
#define SSI_XMIT_SIZE PAGE_SIZE * N_PAGE


/* #define TXDMA_BUFF_SIZE (PAGE_SIZE >> 2) */
#define TXDMA_BUF_DIV 5
#define NB_DMA_REQ 500
/* #define TXDMA_BUF_SIZE (SSI_XMIT_SIZE / TXDMA_BUF_DIV) + ( (SSI_XMIT_SIZE/TXDMA_BUF_DIV) % (2*FIFO_SLOT_SIZE_BYTE) ) */


/* #define WAKEUP_DATA (SSI_XMIT_SIZE >> 1) */
 
/*
 * Ioctl definitions
*/

/* Use 'k' as magic number */
#define CS4341A_IOC_MAGIC  'k'
/* Please use a different 8-bit number in your code */

#define CS4341A_IOCTL_SET_VOLUME       _IOW(CS4341A_IOC_MAGIC,  0, int)
#define CS4341A_IOCTL_SET_VOLUME_A     _IOW(CS4341A_IOC_MAGIC,  1, int)
#define CS4341A_IOCTL_SET_VOLUME_B     _IOW(CS4341A_IOC_MAGIC,  2, int)
#define CS4341A_IOCTL_MUTE_ON          _IO(CS4341A_IOC_MAGIC,   3)
#define CS4341A_IOCTL_MUTE_OFF         _IO(CS4341A_IOC_MAGIC,   4)
#define CS4341A_IOCTL_POWER_ON         _IO(CS4341A_IOC_MAGIC,   5)
#define CS4341A_IOCTL_POWER_OFF        _IO(CS4341A_IOC_MAGIC,   6)
#define CS4341A_IOCTL_SSI_DATA_LENGTH  _IOW(CS4341A_IOC_MAGIC,   7, int)
#define CS4341A_IOCTL_GET_VOLUME       _IOR(CS4341A_IOC_MAGIC,  8,int)
#define CS4341A_IOCTL_SET_FSYNC        _IOW(CS4341A_IOC_MAGIC,  9,int)
#define CS4341A_IOCTL_GET_DMASIZE      _IOR(CS4341A_IOC_MAGIC,  10,int)
#define CS4341A_IOCTL_PAUSE            _IO(CS4341A_IOC_MAGIC,  11)
#define CS4341A_IOCTL_RESUME           _IO(CS4341A_IOC_MAGIC,  12)
#define CS4341A_IOCTL_STOP             _IO(CS4341A_IOC_MAGIC,  13)

#define SPI_CFG_MODULE_NUMBER SPI1
#define SPI_CFG_MASTER_MODE true
#define SPI_CFG_PRIORITY MEDIUM
#define SPI_CFG_SS_ASSERTED SS_1
#define SPI_CFG_BIT_RATE 6000000
#define SPI_CFG_BIT_COUNT 24
#define SPI_CFG_ACTIVE_HIGH_POLARITY true
#define SPI_CFG_PHASE true 
#define SPI_CFG_ACTIVE_HIGH_SS_POLARITY false
#define SPI_CFG_SS_LOW_BETWEEN_BURSTS false

#define SERIAL_PLL_PD 0x0 /* (1-1) */
#define SERIAL_PLL_MFI 0x1010 /* 5 */
#define SERIAL_PLL_MFD 0xc9 /* 200 - 1 */
#define SERIAL_PLL_MFN 0x387 /* -261 sur 10 bits signés */



/*MODE CONTROL 1*/
#define MCLK_SHIFT 0x1<<1
#define AUTO_DETECT_SHIFT 0x1<<2
#define SPEED_MODE_MASK 0x2<<6


/* MODE CONTROL 2 */
#define PDN_SHIFT 1 << 0
#define POR_SHIFT 1 << 1
#define DEM_MASK 0x3 <<2
#define DIF_MASK 0x7 << 4
#define AUTO_MUTE_SHIFT 1<<7 

/* TRANSITION AND MIXING CONTROL */
#define ATAPI_MASK 0x10 << 0
#define ZERO_CROSS_SHIFT 1 << 5
#define SOFT_SHIFT 1 << 6
#define A_EQUAL_B_SHIFT 1 << 7

/* CHANNEL VOLUME */
#define MUTE_SHIFT 1<<7
#define VOLUME_MASK 0x7f


/* Memory Address pointer */
#define MAP_MASK 0x07<<0
#define INCR_SHIFT 0x1<<7


#define MODE_CTRL1 0x00
#define MODE_CTRL2 0x01
#define TRANS_MIX_CTRL 0x02
#define CHANNEL_A_VOL 0x03
#define CHANNEL_B_VOL 0x04

#define CS4341A_BASE_ADDRESS 0x00

#define CHIP_ADDRESS 0x20


#define NR_REGISTER_MAX 5



/* INIT CS4341A REGISTER */
#define INIT_VAL_MODE_CTRL1 0x00
#define INIT_VAL_MODE_CTRL2 0x03
/* #define INIT_VAL_TRANS_MIX_CTRL 0xDC // ATAPI AOUT -> (aL +bR) / 2 , AOUTB -> MUTE */
/* #define INIT_VAL_TRANS_MIX_CTRL 0x53 // ATAPI BOUT -> (aL +bR) / 2 , AOUT -> MUTE) */
#define INIT_VAL_TRANS_MIX_CTRL 0xC9 // A=B, ATAPI AOUT -> (aL , BOUT -> (bR )   */
/* #define INIT_VAL_TRANS_MIX_CTRL 0x49 */
#define INIT_VAL_CHANNEL_A_VOL 0x28
#define INIT_VAL_CHANNEL_B_VOL 0x28

typedef unsigned char boolean;

struct spi_buffer {
	unsigned char chip_address;
	unsigned char map_pointer;
	unsigned char data;
} ;

struct cs4341a_registers {
	unsigned char mod_ctrl1;
	unsigned char mod_ctrl2;
	unsigned char trans_mix_ctrl;
	unsigned char channel_vol_a;
	unsigned char channel_vol_b;
} ;

struct ssi_circ_buf {
	char *buf;
	unsigned long head;
	unsigned long tail;
};

/* /\* Return count in buffer.  *\/ */
/* #define SSI_BUF_CIRC_CNT(head,tail,size) (((head) - (tail)) & ((size)-1)) */

/* /\* Return space available, 0..size-1.  We always leave one free char */
/*    as a completely full buffer has head == tail, which is the same as */
/*    empty.  *\/ */
/* #define SSI_BUF_CIRC_SPACE(head,tail,size) CIRC_CNT((tail),((head)+1),(size)) */

/* /\* Return count up to the end of the buffer.  Carefully avoid */
/*    accessing head and tail more than once, so they can change */
/*    underneath us without returning inconsistent results.  *\/ */
/* #define SSI_BUF_CIRC_CNT_TO_END(head,tail,size) \ */
/* 	({int end = (size) - (tail); \ */
/* 	  int n = ((head) + end) & ((size)-1); \ */
/* 	  n < end ? n : end;}) */

/* /\* Return space available up to the end of the buffer.  *\/ */
/* #define SSI_BUF_CIRC_SPACE_TO_END(head,tail,size) \ */
/* 	({int end = (size) - 1 - (head); \ */
/* 	  int n = (end + (tail)) & ((size)-1); \ */
/* 	  n <= end ? n : end+1;}) */

#endif
