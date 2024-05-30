
#include <linux/config.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	     /* error codes */
#include <linux/types.h>	     /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	     /* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/dma-mapping.h>
#include <linux/dmapool.h>
#include <asm/system.h>		
#include <asm/uaccess.h>	     /* copy_*_user */
#include <asm/signal.h>
#include <asm/semaphore.h>

#include <asm/bitops.h>
#include <asm/atomic.h>


#include <ssi/ssi.h> /* mxc ssi driver */ 
#include <spi/spi.h>
#include <ssi/registers.h> /* mxc ssi driver */ 
#include <dam/dam.h> /* mxc audiomux driver */ 
#include <dam/registers.h>

#include "iomuxc_mem_map.h"

#ifndef CONFIG_ARCH_MX3
#error CONFIG_ARCH_MX3 not defined !!!
#else
/* #include <asm/arch-mx3/spi.h> */ /*does not exist anymore in kernel_lb2pdb_4*/
#include <asm/arch/spba.h>
#include <asm/arch/mx31.h> 
#include <asm/arch/gpio.h> 
#include <asm/arch/irqs.h> 
#include <asm/arch/board-mx31ads.h>
#include <asm/arch/mx31_pins.h>
#include <asm/arch/clock.h>
#include "../../kernel/arch/arm/mach-mx3/crm_regs.h"
#include "../../kernel/arch/arm/mach-mx3/iomux.h"
#include <asm/arch/sdma.h>
#endif



#include "cs4341a.h"

#ifdef USE_DEBUG
//select debug modes
#define USE_WARNING
#define USE_TRACE
#define USE_ASSERT

#endif //USE_DEBUG

#define DRIVER_VERSION (0x00000111UL)


MODULE_DESCRIPTION("Cirrus Logic 4341a Driver");
/* MODULE_LICENSE("Sagem"); */



#ifdef USE_TRACE
static unsigned long debug_mode; 
#ifndef USE_WARNING
#define USE_WARNING
#endif
#	define CS_TRACE(msg,args...)			\
	if(debug_mode&0x01UL) {					\
		printk("CS: " msg "\n", ## args);	\
	}
#else
#	define CS_TRACE(msg,args...)
#endif

#ifdef USE_WARNING
static unsigned long debug_mode;
#ifndef USE_ASSERT  
#define USE_ASSERT
#endif
#	define CS_WARNING(msg, args...)				\
	if(debug_mode&0x02UL) {							\
		printk("CS_WARNING: " msg "\n",## args);	\
	}
#else
#	define CS_WARNING(msg, args...)
#endif


#ifdef USE_ASSERT
#	define CS_ASSERT(condition)													\
	if(!(condition)) {																\
		printk("CS_ASSERTION_FAILURE: File=" __FILE__ ", Line=%d\n",__LINE__);	\
		while(1);																	\
	}
#else
#	define CS_ASSERT(condition)
#endif


#ifdef USE_DEBUG
static unsigned long debug_mode=0x3UL;
#else
static unsigned long debug_mode=0x0UL;
#endif
module_param(debug_mode,ulong,S_IRUGO);


/* over sampling*/
static unsigned short over_sampling=256;
module_param(over_sampling,ushort,S_IRUGO);

/*fifo  watermark*/
static short wm_fifo_0 = 4;
module_param(wm_fifo_0 ,short,S_IRUGO);

/*frame sync frequence*/
static unsigned short frame_sync = 48000;
module_param(frame_sync,ushort,S_IRUGO);

/*use of dma*/
static unsigned short use_dma = 1;
module_param(use_dma,ushort,S_IRUGO);

/*nb_dma_req*/
static unsigned short nb_dma_req = NB_DMA_REQ;
module_param(nb_dma_req,ushort,S_IRUGO);

/*txdma_buf_div*/
static unsigned short txdma_buf_div = TXDMA_BUF_DIV;
module_param(txdma_buf_div,ushort,S_IRUGO);


int ssi_circ_empty(struct ssi_circ_buf *circ){
	return ((circ)->head == (circ)->tail);
}

void ssi_circ_clear(struct ssi_circ_buf *circ){
	(circ)->head = 0;
	(circ)->tail = 0;
}

unsigned long ssi_circ_chars_pending(struct ssi_circ_buf * circ,unsigned long size) {

	unsigned long result;
	if(circ->tail >= circ->head)
		result = (size - circ->tail -1 + circ->head) ;
	if(circ->tail < circ->head)
		result =  (circ->head - circ->tail) ;
	return result;
}

unsigned long ssi_circ_chars_free(struct ssi_circ_buf * circ,unsigned long size) {

	unsigned long result;
	if(circ->tail >= circ->head)
		result =  circ->tail - circ->head -1;
	if(circ->tail < circ->head)
		result =  (size - circ->head + circ->tail -1) ;
	return result;
}

struct cs4341a_dev {
    spi_config cs_spi_config;
    void* cs_spi_client;
    struct cs4341a_registers cs4341a_reg;
    struct cdev cdev;	  /* Char device structure		*/
    iomux_pin_name_t reset_pin;
    wait_queue_head_t write_queue;
    
    struct ssi_circ_buf xmit;
    unsigned char flag;
    unsigned long tx0UnderRunError;


    spinlock_t lock;
    boolean tx_started; /* Flag set if the transmitter is on */
    unsigned char opened; /* count how many times driver has been opened */ 

    unsigned char watermark_0;
    ssi_mod ssi_port;

/*     char *tx_buf; /\* DMA Transmit buffer virtual address *\/ */

    dma_addr_t tx_handle0; /* DMA Transmit buffer physical address  */
    dma_addr_t tx_handle1; /* DMA Transmit buffer physical address  */

    int dma_enabled; /* Flag to enable/disable DMA data transfer. */
	unsigned long dma_copied_bd0;
	unsigned long dma_copied_bd1;
	

    unsigned int sample_rate;
    unsigned char data_length;
    unsigned char channels;
    unsigned long wakeup_data;
    unsigned long ssi_buffer_size;
    unsigned long dma_size;
    unsigned long dma_buf_div;
    unsigned long fifo_size_byte;
    unsigned long period_size;
    
};

/* This structure is used to store the information for DMA data transfer. */
typedef struct {
    /*!
     * Holds the write channel number.
     */
    int wr_channel;
    /*!
     * SSI2 Transmit Event ID
     */
    int tx_event_id;
    /*!
     * DMA Transmit tasklet
     */
	/*     struct tasklet_struct dma_tx_tasklet; */
    /*!
     * Flag indicates if the channel is in use
     */
    int dma_txchnl_inuse;                
} dma_info;


/*! 
 * This array holds the DMA channel information for the SSI2 port
 */
static dma_info d_info = {
    .wr_channel       = 0,
    .tx_event_id      = DMA_REQ_SSI2_TX1, /* fifo 1*/
    .dma_txchnl_inuse = 0,
};


int write_to_spi(struct spi_buffer* spi_b);
int init_port_spi(void);
void init_clock_module(void);
void init_port_ssi2(void);
void init_audiomux(void);
int init_cs4341a(void);
void power_down(boolean state);
unsigned char get_fifo_0_cnt(void);
void stop_transmitter(void);
unsigned long get_space_free(struct cs4341a_dev *dev);
void set_frame_sync(unsigned int frame_sync);
static int configure_write_channel(dma_info *d_info, struct cs4341a_dev *dev);

int cs4341a_major = CS4341A_MAJOR ;
int cs4341a_minor = 0 ;

struct cs4341a_dev *cs4341a_devices;

#define USE_TEST_PIN

#ifdef USE_TEST_PIN
#define TEST_PIN MX31_PIN_GPIO1_6
void test_pin_up(void){
    mxc_set_gpio_dataout(TEST_PIN, 1);
}

void test_pin_down(void){
    mxc_set_gpio_dataout(TEST_PIN, 0);
}
#endif

unsigned char get_fifo_0_cnt(void) {
    return  (unsigned char) ((ssi_getreg_value(MXC_SSI2SFCSR,SSI2) & 0x0f00) >> 8 );
}


int write_to_spi(struct spi_buffer* spi_b)
{
    int result = 0;

    CS_ASSERT(spi_b != NULL);

    result = spi_send_frame((unsigned char*)spi_b,3,cs4341a_devices->cs_spi_client);
    /* 	result = spi_send_frame((unsigned char*)spi_b,sizeof(struct spi_buffer),cs4341a_devices->cs_spi_client); */
    if (result < 0) {
		CS_WARNING("spi_send_frame failed %i",result);
    } 
	
    return result; 
}


/*initialize SPI port of the IMX31 */
/* return 0 if ok */ 
/*        1 if error */

int init_port_spi(void) 
{ 

	cs4341a_devices->cs_spi_config.module_number           = SPI_CFG_MODULE_NUMBER;
    cs4341a_devices->cs_spi_config.master_mode             = SPI_CFG_MASTER_MODE;
    cs4341a_devices->cs_spi_config.priority                = SPI_CFG_PRIORITY;
    cs4341a_devices->cs_spi_config.ss_asserted             = SPI_CFG_SS_ASSERTED;
    cs4341a_devices->cs_spi_config.bit_rate                = SPI_CFG_BIT_RATE;
    cs4341a_devices->cs_spi_config.bit_count               = SPI_CFG_BIT_COUNT;
    cs4341a_devices->cs_spi_config.active_high_polarity    = SPI_CFG_ACTIVE_HIGH_SS_POLARITY;
    cs4341a_devices->cs_spi_config.phase                   = SPI_CFG_PHASE;
    cs4341a_devices->cs_spi_config.active_high_ss_polarity = SPI_CFG_ACTIVE_HIGH_SS_POLARITY;
    cs4341a_devices->cs_spi_config.ss_low_between_bursts   = SPI_CFG_SS_LOW_BETWEEN_BURSTS;
	 
    cs4341a_devices->cs_spi_client = spi_get_device_id(&(cs4341a_devices->cs_spi_config));

    if(!cs4341a_devices->cs_spi_client){
		CS_WARNING("Error on configure spi");
		return -1 ;
    }

    return 0;
}



void set_frame_sync(unsigned int frame_sync) {

    switch(frame_sync) {

    case 48000 :
		/*choose SPLL Clock as source for SSI2 divider*/ 
		mxc_set_clocks_pll(SSI2_BAUD,SERIALPLL);

		/*SPLL DIVIDED BY 24*/
		mxc_ccm_modify_reg(MXC_CCM_PDR1,0x3FE00,0x18A00);
		/* 		mxc_set_clocks_div(SSI2_BAUD,24); */
	
		/* Spll at 295 MHz*/
		/* 		mxc_ccm_set_reg(MXC_CCM_SRPCTL, 0xC90387); */
		mxc_ccm_modify_reg(MXC_CCM_SRPCTL,0xFFFFFFFF,0x00C90387);
		/*restart SPLL*/
		mxc_ccm_modify_reg(MXC_CCM_CCMR,MXC_CCM_CCMR_SPE,0); 
		mxc_ccm_modify_reg(MXC_CCM_CCMR,MXC_CCM_CCMR_SPE,MXC_CCM_CCMR_SPE); 
		break;
    case 44100:
		/*choose MCU Clock as source for SSI2 divider*/ 
		mxc_set_clocks_pll(SSI2_BAUD,MCUPLL);

		/*MCU PLL divided by 36*/
		mxc_ccm_modify_reg(MXC_CCM_PDR1,0x3FE00,0x28A00);


		/*MCU PLL already set to 399 MHz*/
	
		break;
    default:

		break;

    }

}

void init_clock_module(void) 
{
    set_frame_sync(frame_sync);
}

void init_port_ssi2(void) 
{
    /*SCR = 0xB9*/
    ssi_enable(SSI2,true);	/*enable SSI2 module*/
    ssi_i2s_mode(SSI2,i2s_master);	/*I2S master mode for SSI2*/
    ssi_network_mode(SSI2,true);
    ssi_synchronous_mode(SSI2,true);	/*Sync mode SCR[4]=1*/
    ssi_system_clock(SSI2,true);

    /*STCR=0xED*/
    ssi_tx_early_frame_sync(SSI2,ssi_frame_sync_one_bit_before);
    ssi_tx_frame_sync_length(SSI2,ssi_frame_sync_one_word);
    ssi_tx_frame_sync_active(SSI2,ssi_frame_sync_active_low);/*Transmit frame sync active low (STCR[2]=1)*/
    ssi_tx_clock_polarity(SSI2,ssi_clock_on_falling_edge);	/*transmitted data clocked at falling edge  of the clock STCR[3]=1*/ 
    ssi_tx_shift_direction(SSI2,ssi_msb_first);	/*Transmit shift direction,MSB transmitted first, STCR[4]=1*/ 
    ssi_tx_clock_direction(SSI2,ssi_tx_rx_internally);
    ssi_tx_frame_direction(SSI2,ssi_tx_rx_internally);

    ssi_tx_fifo_enable(SSI2,ssi_fifo_0,true);

    /*STCCR*/

    ssi_set_register(0x56100,MXC_SSISTCCR,SSI2);
	ssi_tx_word_length(SSI2,ssi_16_bits);
	
    ssi_set_register(0x0,MXC_SSI2SIER,SSI2);
}



void init_audiomux(void) 
{

    _reg_DAM_PTCR2 = 0x0;
    _reg_DAM_PTCR5 = 0x8e411000;

    (*(volatile unsigned long*) (IO_ADDRESS(IOMUXC_BASE_ADDR + SW_MUX_CTL_STXD5_SRXD5_SCK5_SFS5))) = 0x12121212; //0x94 // allocate Port 5 dedicated pins
    (*(volatile unsigned long*) (IO_ADDRESS(IOMUXC_BASE_ADDR + SW_MUX_CTL_STXD4_SRXD4_SCK4_SFS4))) &= ~(0x0FF);  //+0x98 // grab SFS4 for RXCLK5 (Alt1 function)
    (*(volatile unsigned long*) (IO_ADDRESS(IOMUXC_BASE_ADDR + SW_MUX_CTL_STXD4_SRXD4_SCK4_SFS4))) |= 0x24;
}


void power_down(boolean state)
{
    struct spi_buffer *my_spi_buffer;
	
    my_spi_buffer =  kmalloc(sizeof(struct spi_buffer),GFP_KERNEL);
    memset(my_spi_buffer,0,sizeof(struct spi_buffer));

    my_spi_buffer->chip_address =  CHIP_ADDRESS ;
    my_spi_buffer->map_pointer =  MODE_CTRL2 ;
    cs4341a_devices->cs4341a_reg.mod_ctrl2 &= ~(PDN_SHIFT);
    cs4341a_devices->cs4341a_reg.mod_ctrl2 |= state;

    CS_TRACE("cs4341a_reg.mod_ctrl2=0x%x",cs4341a_devices->cs4341a_reg.mod_ctrl2);

    memcpy(&(my_spi_buffer->data),&(cs4341a_devices->cs4341a_reg.mod_ctrl2),1);
    /* 	my_spi_buffer->data = cs4341a_devices->cs4341a_reg.mod_ctrl2; */
    write_to_spi(my_spi_buffer);
    kfree(my_spi_buffer);
}

int init_cs4341a(void) 
{
    struct spi_buffer *my_spi_buffer;

    my_spi_buffer =  kmalloc(sizeof(struct spi_buffer),GFP_KERNEL);
	
    memset(my_spi_buffer,0,sizeof(struct spi_buffer));


    my_spi_buffer->chip_address =  CHIP_ADDRESS ;
    my_spi_buffer->map_pointer =  MODE_CTRL1 ;
    cs4341a_devices->cs4341a_reg.mod_ctrl1 |= INIT_VAL_MODE_CTRL1;
    memcpy(&(my_spi_buffer->data),&(cs4341a_devices->cs4341a_reg.mod_ctrl1),1);
    /* 	my_spi_buffer->data = cs4341a_devices->cs4341a_reg.mod_ctrl1; */
    write_to_spi(my_spi_buffer);

    my_spi_buffer->chip_address =  CHIP_ADDRESS ;
    my_spi_buffer->map_pointer =  MODE_CTRL2 ;
    cs4341a_devices->cs4341a_reg.mod_ctrl2 |= INIT_VAL_MODE_CTRL2;
    memcpy(&(my_spi_buffer->data),&(cs4341a_devices->cs4341a_reg.mod_ctrl2),1);
    /* 	my_spi_buffer->data = cs4341a_devices->cs4341a_reg.mod_ctrl2; */
    write_to_spi(my_spi_buffer);
		
    my_spi_buffer->chip_address =  CHIP_ADDRESS ;
    my_spi_buffer->map_pointer =  TRANS_MIX_CTRL ;
    cs4341a_devices->cs4341a_reg.trans_mix_ctrl = INIT_VAL_TRANS_MIX_CTRL;
    memcpy(&(my_spi_buffer->data),&(cs4341a_devices->cs4341a_reg.trans_mix_ctrl),1);
    /* 	my_spi_buffer->data = cs4341a_devices->cs4341a_reg.trans_mix_ctrl; */
    write_to_spi(my_spi_buffer);


    my_spi_buffer->chip_address =  CHIP_ADDRESS ;
    my_spi_buffer->map_pointer =  CHANNEL_A_VOL ;
    cs4341a_devices->cs4341a_reg.channel_vol_a |=  INIT_VAL_CHANNEL_A_VOL;
    memcpy(&(my_spi_buffer->data),&(cs4341a_devices->cs4341a_reg.channel_vol_a),1);
    /* 	my_spi_buffer->data =  cs4341a_devices->cs4341a_reg.channel_vol_a ; */
    write_to_spi(my_spi_buffer);


    my_spi_buffer->chip_address =  CHIP_ADDRESS ;
    my_spi_buffer->map_pointer =  CHANNEL_B_VOL ;
    cs4341a_devices->cs4341a_reg.channel_vol_b |= INIT_VAL_CHANNEL_B_VOL;
    memcpy(&(my_spi_buffer->data),&(cs4341a_devices->cs4341a_reg.channel_vol_b),1);
    /* 	my_spi_buffer->data = cs4341a_devices->cs4341a_reg.channel_vol_b; */
    write_to_spi(my_spi_buffer);
	

    /* 	CS_TRACE("After Init Spi"); */
    /* 	CS_TRACE("cs4341a_reg.mod_ctrl1=0x%x",cs4341a_devices->cs4341a_reg.mod_ctrl1); */
    /* 	CS_TRACE("cs4341a_reg.mod_ctrl2=0x%x",cs4341a_devices->cs4341a_reg.mod_ctrl2); */
    /* 	CS_TRACE("cs4341a_reg.trans_mix_ctrl=0x%x",cs4341a_devices->cs4341a_reg.trans_mix_ctrl); */
    /* 	CS_TRACE("cs4341a_reg.channel_vol_a=0x%x",cs4341a_devices->cs4341a_reg.channel_vol_a); */
    /* 	CS_TRACE("cs4341a_reg.channel_vol_b=0x%x",cs4341a_devices->cs4341a_reg.channel_vol_b); */

    kfree(my_spi_buffer);
    return 0;					 	 
}

/* Wait for space for writing; caller must hold device semaphore.  On
 * error the semaphore will be released before returning. 
 */
static int circ_buf_getwritespace(struct cs4341a_dev *dev, struct file *filp)
{
/* 	    struct ssi_circ_buf * xmit = &dev->xmit; */

	/*     while( ssi_circ_chars_pending(&dev->xmit) > WAKEUP_DATA  ) { */

    while( dev->flag == 0  ) {
		DEFINE_WAIT(wait);
	
		spin_unlock(dev->lock);
		if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;
/* 		CS_TRACE("sl %li %li",xmit->tail,xmit->head); */
		prepare_to_wait(&dev->write_queue , &wait, TASK_INTERRUPTIBLE);
		if ( dev->flag == 0 ) {
			schedule();
		}
		/* 	if ( ssi_circ_chars_pending(&dev->xmit) > WAKEUP_DATA  ) { */
		/* 	    schedule(); */
		/* 	} */
		finish_wait(&dev->write_queue , &wait);
		if (signal_pending(current))
			return -ERESTARTSYS; /* signal: tell the fs layer to handle it */

		spin_lock(dev->lock);


    }
    return 0;
}	


ssize_t cs4341a_write(struct file *filp, const char __user *buf, size_t count,loff_t *f_pos)
{
    struct cs4341a_dev *dev =  filp->private_data;
    struct ssi_circ_buf *xmit = &dev->xmit;
    ssi_status_enable_mask irq_mask=0; 
    dma_request_t writechnl_request;
    int result,i,order ;
    unsigned long tx_num, data = 0 , count_to_the_end, free_space,page=0;

    CS_ASSERT(buf!=NULL);

    spin_lock(&dev->lock);    
	
    if(dev->tx_started) {

		/* Make sure there's space to write */
		result = circ_buf_getwritespace(dev, filp);
		if (result)
			return result; /* scull_getwritespace called spin_unlock(dev->lock) */

		/*How much free space*/
		free_space = ssi_circ_chars_free(xmit,dev->ssi_buffer_size);

/* 		CS_TRACE("wk%li %li %li %i",xmit->tail,xmit->head,free_space,count); */
		
		/* ti not overwrite the dma channel that is trasferring*/
		free_space -= dev->dma_size;
	
/* 		now free_space related to count bytes to be transferred */
		count = min((unsigned long)count,free_space);

		if (xmit->head >= xmit->tail) {

/* 			real space free to the end */
			count_to_the_end = dev->ssi_buffer_size - xmit->head ; /* to end-of-buf */
	    
			count_to_the_end = min( count_to_the_end , (unsigned long)count );
	    
			if (copy_from_user(xmit->buf + xmit->head, buf, count_to_the_end)) {
				return -EFAULT;
			}
			xmit->head =  (xmit->head + count_to_the_end) % (dev->ssi_buffer_size)   ; 

			if (count_to_the_end < count ) {
		
				if (copy_from_user(xmit->buf, buf + count_to_the_end, count - count_to_the_end)) {
					spin_unlock(dev->lock);
					return -EFAULT;
				}
				xmit->head += count - count_to_the_end;
				xmit->head %=  dev->ssi_buffer_size  ;
			}

		}
		else  { /* the write pointer has wrapped, fill up to rp-1 */

			if (copy_from_user(xmit->buf + xmit->head, buf, count)) {
				spin_unlock(dev->lock);
				return -EFAULT;
			}
 			xmit->head =  (xmit->head +count) % (dev->ssi_buffer_size ); 
		}

/* 		CS_TRACE("cp%li %li %i",xmit->tail,xmit->head,count); */

		if (ssi_circ_chars_pending(xmit,dev->ssi_buffer_size) > dev->wakeup_data) {
			dev->flag = 0;
		}

		spin_unlock(&dev->lock);

    } else {

		dev->period_size = (dev->watermark_0 * dev->data_length);
		dev->dma_size = nb_dma_req * dev->period_size;
		dev->ssi_buffer_size = txdma_buf_div * dev->dma_size ;
		dev->wakeup_data = ( dev->ssi_buffer_size >> 1 );

		CS_TRACE("data_length=%i",dev->data_length);
		CS_TRACE("period_size=%li",dev->period_size);
		CS_TRACE("dma_size=%li",dev->dma_size);
		CS_TRACE("ssi_buffer_size=%li",dev->ssi_buffer_size);
		CS_TRACE("wakeup_data=%li",dev->wakeup_data);

		order = get_order(dev->ssi_buffer_size);
		CS_TRACE("order=%i",order);
		/* 	page = __get_free_pages(GFP_KERNEL,order); */
		page = __get_free_pages(__GFP_DMA,order);
		CS_ASSERT((void*)page!=NULL);
		/* 	CS_TRACE("page=%x",page); */
		if (!page)
			return -ENOMEM;
		memset((void*)page , 0 , order * PAGE_SIZE);
		dev->xmit.buf = (unsigned char *) page;
	
		ssi_circ_clear(&dev->xmit);
	    
    	//Bufferize at the maximum count / dev->ssi_buffer_size ms of voice
		count = min((unsigned long)count,dev->ssi_buffer_size);
		CS_TRACE("cnt %i",count); 

		if (  copy_from_user(xmit->buf + xmit->head  ,buf,count) ) {
			CS_WARNING("Error copy_from_user");
			return -EFAULT;
		}
		xmit->head += count ;
		if (xmit->head == dev->ssi_buffer_size) 
			xmit->head = 0;

		if(dev->dma_enabled) {

			if (configure_write_channel(&d_info,dev) < 0) {
				CS_WARNING("can not configure write_channel, trying without DMA");
				return 0;
			}
			
			irq_mask |= ssi_tx_dma_interrupt_enable;
			irq_mask |= ssi_transmitter_underrun_0 ;
			irq_mask |= ssi_tx_interrupt_enable; /*needed to count underrun errors*/
			ssi_interrupt_enable(SSI2,irq_mask);

			/* 	    tx_num = ssi_circ_chars_pending(xmit); */
			tx_num = min ((unsigned long)count,dev->dma_size);

			d_info.dma_txchnl_inuse = 1;
			
			/*configuration of transfer 1*/
			dev->tx_handle0 = dma_map_single(NULL, xmit->buf + xmit->tail ,
											dev->dma_size, DMA_TO_DEVICE);
	    
			writechnl_request.sourceAddr = (__u8*) dev->tx_handle0;
    	    writechnl_request.count = tx_num;
			dev->dma_copied_bd0 = tx_num;
			writechnl_request.bd_cont = 1;
			mxc_dma_set_config(d_info.wr_channel,  &writechnl_request, 0);


			memset(&writechnl_request, 0 , sizeof(dma_request_t));

			/* configuration of transfer 2*/
			dev->tx_handle1 = dma_map_single(NULL, xmit->buf + tx_num ,
											dev->dma_size, DMA_TO_DEVICE);
	    
			writechnl_request.sourceAddr = (__u8*) dev->tx_handle1;
    	    writechnl_request.count = tx_num;
			dev->dma_copied_bd1 = tx_num;
			writechnl_request.bd_cont = 1;
			mxc_dma_set_config(d_info.wr_channel,  &writechnl_request, 1);

			xmit->tail += tx_num;

			CS_TRACE("%li %li",xmit->tail, xmit->head);

			mxc_dma_start(d_info.wr_channel);

			dev->tx_started = true ;

			spin_unlock(dev->lock);
			ssi_transmit_enable(SSI2,true);

		} else {
			/*not using dma*/

			for(i=0; i<FIFO_MAX_N_SLOT ; i++) {
				data=0;
				memcpy(&data, xmit->buf + xmit->tail  ,(dev->data_length>>3));
				ssi_set_data(SSI2  ,ssi_fifo_0, data);
				xmit->tail += (dev->data_length>>3);
			}

			irq_mask |= ssi_tx_interrupt_enable;
			ssi_interrupt_enable(SSI2,irq_mask);

			ssi_transmit_enable(SSI2,true);
			
			dev->tx_started = true ;
			spin_unlock(dev->lock);
		}
    }

    return count;
}


unsigned long  get_data_ssi_fifo(struct ssi_circ_buf *xmit,struct cs4341a_dev *dev) {

    unsigned long data = 0;
    int i;
    
    for(i=0 ; i< (dev->data_length>>3) ;i++ ) {
		data |= *(xmit->buf + xmit->tail) << (8*i) ;
		xmit->tail++ ; 
		if(xmit->tail == dev->ssi_buffer_size)
			xmit->tail=0;
    }
    return data;
}

void fill_ssi_fifo(struct cs4341a_dev *dev) {

	unsigned long data = 0;
	struct ssi_circ_buf *xmit = &dev->xmit;
#ifdef FILL_FROM_WATERMARK
    int i;    

	for(i=0;i<(FIFO_MAX_N_SLOT - dev->watermark_0) ;i++){
		data = get_data_ssi_fifo(xmit,dev);
		ssi_set_data(SSI2,ssi_fifo_0,data);
	}
	
#else
    unsigned char fifo_0_cnt;

	fifo_0_cnt = get_fifo_0_cnt();
	while (fifo_0_cnt < FIFO_MAX_N_SLOT) {
		data = get_data_ssi_fifo(xmit,dev);
		ssi_set_data(SSI2,ssi_fifo_0,data);
				fifo_0_cnt++;
	};
#endif


}

/* #define FILL_FROM_WATERMARK */
#undef FILL_FROM_WATERMARK

irqreturn_t cs4341a_interrupt(int irq,void *dev_id, struct pt_regs *regs)
{
    struct cs4341a_dev *dev = dev_id;
    struct ssi_circ_buf *xmit = &dev->xmit;
    ssi_status_enable_mask ssi_irq_sts;
    unsigned long data = 0;
    unsigned long tx_num = 0;

    ssi_irq_sts = ssi_get_status(SSI2);

    spin_lock(dev->lock);

    if (dev->dma_enabled==0) {

		if (ssi_irq_sts & ssi_tx_fifo_0_empty){

			fill_ssi_fifo(dev);

			tx_num = ssi_circ_chars_pending(xmit,dev->ssi_buffer_size);
			if(tx_num <= dev->wakeup_data ) {
				dev->flag=1;
				wake_up_interruptible(&dev->write_queue);
			}
		}
    }
    
    if (ssi_irq_sts & ssi_transmitter_underrun_0) {

/* 		fill_ssi_fifo(dev); */
/* 		data = get_data_ssi_fifo(xmit,dev); */
		ssi_set_data(SSI2,ssi_fifo_0,data);
		dev->tx0UnderRunError++;


    }

    spin_unlock(dev->lock);


    return IRQ_HANDLED;
}


int cs4341a_ioctl(struct inode *inode, struct file *filp,
				  unsigned int cmd, unsigned long arg)
{

    int retval = 0, err =0 ;
    struct spi_buffer spi_b;
    unsigned char volume = 0x0;
    unsigned long param = 0x0;
    struct cs4341a_dev *dev =  filp->private_data;

    /*
     * extract the type and number bitfields, and don't decode
     * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
     */
    if (_IOC_TYPE(cmd) != CS4341A_IOC_MAGIC) return -ENOTTY;
    //	if (_IOC_NR(cmd) > SCULL_IOC_MAXNR) return -ENOTTY;

    /*
     * the direction is a bitmask, and VERIFY_WRITE catches R/W
     * transfers. `Type' is user-oriented, while
     * access_ok is kernel-oriented, so the concept of "read" and
     * "write" is reversed
     */
    if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
    else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
    if (err) return -EFAULT;

    memset(&spi_b,0,sizeof(struct spi_buffer));
    spi_b.chip_address = CHIP_ADDRESS;

    switch(cmd) {

    case CS4341A_IOCTL_SET_VOLUME:

		retval = __get_user(volume , (unsigned char __user *)arg);

		CS_TRACE("volume=0x%x",volume);

		spi_b.map_pointer = CHANNEL_A_VOL ;
		dev->cs4341a_reg.channel_vol_a &= ~(VOLUME_MASK) ;
		dev->cs4341a_reg.channel_vol_a |=  (unsigned char) (volume & VOLUME_MASK );
		/* 	CS_TRACE("SET_VOL:cs4341a_reg.channel_vol_a=0x%x",dev->cs4341a_reg.channel_vol_a); */
		
		memcpy(&(spi_b.data),&(dev->cs4341a_reg.channel_vol_a),1);
		/* 		spi_b.data = dev->cs4341a_reg.channel_vol_a ; */


		retval = write_to_spi(&spi_b);

		memset(&spi_b,0,sizeof(struct spi_buffer));
		spi_b.chip_address = CHIP_ADDRESS;

		spi_b.map_pointer = CHANNEL_B_VOL ;
		dev->cs4341a_reg.channel_vol_b &= ~(VOLUME_MASK) ;
		dev->cs4341a_reg.channel_vol_b |= (unsigned char) (volume & VOLUME_MASK );
		/* 	CS_TRACE("SET_VOL:cs4341a_reg.channel_vol_b=0x%x",dev->cs4341a_reg.channel_vol_b); */

		memcpy(&(spi_b.data),&(dev->cs4341a_reg.channel_vol_b),1);
		/* 		spi_b.data = dev->cs4341a_reg.channel_vol_b ; */

		retval = write_to_spi(&spi_b);

		break;
		
    case CS4341A_IOCTL_SET_VOLUME_A:

		retval = __get_user(volume , (unsigned char __user *)arg);

		spi_b.map_pointer = CHANNEL_A_VOL ;
		dev->cs4341a_reg.channel_vol_a &= ~(VOLUME_MASK) ;
		dev->cs4341a_reg.channel_vol_a |=  volume & VOLUME_MASK ;

		memcpy(&(spi_b.data),&(dev->cs4341a_reg.channel_vol_a),1);
		/* 		spi_b.data = dev->cs4341a_reg.channel_vol_a ; */
		/* 	CS_TRACE("SET_VOL_A:cs4341a_reg.channel_vol_a=0x%x",dev->cs4341a_reg.channel_vol_a); */

		retval = write_to_spi(&spi_b);


		break;

    case CS4341A_IOCTL_SET_VOLUME_B:

		retval = __get_user(volume , (unsigned char __user *)arg);
		
		spi_b.map_pointer = CHANNEL_B_VOL ;
		dev->cs4341a_reg.channel_vol_b &= ~(VOLUME_MASK) ;
		dev->cs4341a_reg.channel_vol_b |= (volume & VOLUME_MASK );

		memcpy(&(spi_b.data),&(dev->cs4341a_reg.channel_vol_b),1);
		/* 		spi_b.data = dev->cs4341a_reg.channel_vol_b ; */

		/* 	CS_TRACE("SET_VOL_B:cs4341a_reg.channel_vol_b=0x%x",dev->cs4341a_reg.channel_vol_b); */

		retval = write_to_spi(&spi_b);

		break;


    case CS4341A_IOCTL_MUTE_ON:

		spi_b.map_pointer = CHANNEL_A_VOL  ;
		dev->cs4341a_reg.channel_vol_a &= ~(MUTE_SHIFT) ;
		dev->cs4341a_reg.channel_vol_a |= MUTE_SHIFT ;

		spi_b.data = dev->cs4341a_reg.channel_vol_a;
		
		retval = write_to_spi(&spi_b);
		break;
        
    case CS4341A_IOCTL_MUTE_OFF:

		spi_b.map_pointer = CHANNEL_A_VOL  ;
		dev->cs4341a_reg.channel_vol_a &= ~(MUTE_SHIFT) ;
		spi_b.data = dev->cs4341a_reg.channel_vol_a;
	
		retval = write_to_spi(&spi_b);
		break;

    case CS4341A_IOCTL_POWER_ON:
		
		spi_b.map_pointer = MODE_CTRL2;
		dev->cs4341a_reg.mod_ctrl2 &= ~(PDN_SHIFT);
		CS_TRACE("cs4341a_reg.mod_ctrl2=0x%x",dev->cs4341a_reg.mod_ctrl2);
		spi_b.data = dev->cs4341a_reg.mod_ctrl2;
		retval = write_to_spi(&spi_b);
		break;

    case CS4341A_IOCTL_POWER_OFF:
		
		spi_b.map_pointer = MODE_CTRL2;
		dev->cs4341a_reg.mod_ctrl2 &= ~(PDN_SHIFT);
		dev->cs4341a_reg.mod_ctrl2 |= PDN_SHIFT;
		CS_TRACE("cs4341a_reg.mod_ctrl2=0x%x",dev->cs4341a_reg.mod_ctrl2);
		spi_b.data = dev->cs4341a_reg.mod_ctrl2;
		retval = write_to_spi(&spi_b);
		break;


    case CS4341A_IOCTL_SSI_DATA_LENGTH:
		
		retval = __get_user(param , (unsigned long __user *)arg);
		CS_TRACE("IOCTL : data_lengthm=%li bits ",param);
		switch(param) {
		case 8:
			ssi_tx_word_length(dev->ssi_port,ssi_8_bits);
			dev->data_length = 8;
			CS_TRACE("WORD_LENGTH 8:  MXC_SSISTCCR  = 0x%08lX",ssi_getreg_value(MXC_SSISTCCR,dev->ssi_port));
		case 16:
			ssi_tx_word_length(dev->ssi_port,ssi_16_bits);
			dev->data_length = 16;
			CS_TRACE("WORD_LENGTH 16:  MXC_SSISTCCR  = 0x%08lX",ssi_getreg_value(MXC_SSISTCCR,dev->ssi_port));
			break;
		case 24:
			ssi_tx_word_length(dev->ssi_port,ssi_24_bits);
			dev->data_length = 24;
			CS_TRACE("WORD_LENGTH 24:  MXC_SSISTCCR  = 0x%08lX",ssi_getreg_value(MXC_SSISTCCR,dev->ssi_port));
			break;
		default:
			break;
		}
		dev->period_size = (dev->watermark_0 * dev->data_length);
		dev->dma_size = nb_dma_req * dev->period_size;
		dev->ssi_buffer_size = txdma_buf_div * dev->dma_size ;
		dev->wakeup_data = ( dev->ssi_buffer_size >> 1 );
		retval = 1;


		break;

    case CS4341A_IOCTL_GET_VOLUME:

		/*Warning: volume depend of the ATAPI mode here we considere (aL +bR) / 2 */
		volume = ((dev->cs4341a_reg.channel_vol_a & VOLUME_MASK )
				  + (  dev->cs4341a_reg.channel_vol_b & VOLUME_MASK ))  >> 1;
		/* 	CS_TRACE("GET_VOL_A:cs4341a_reg.channel_vol_a=0x%x",dev->cs4341a_reg.channel_vol_a); */
		/* 	CS_TRACE("GET_VOL_B:cs4341a_reg.channel_vol_b=0x%x",dev->cs4341a_reg.channel_vol_b); */

		CS_TRACE("GET_VOLUME = %i",volume);
		put_user(volume,(unsigned char*)arg);
		retval = 1;
		break;

    case CS4341A_IOCTL_SET_FSYNC:
		
		retval = __get_user(param , (unsigned long __user *)arg);
		CS_TRACE("IOCTL SET FSYNC: fsync=%li  ",param);
		switch(param) {
		case 44100:
			set_frame_sync(44100);
			dev->sample_rate = 44100;
	    
			break;
		case 48000:
			set_frame_sync(48000);
			dev->sample_rate = 48000;
	        
			break;
		}
		retval = 1;
		break;

	case CS4341A_IOCTL_GET_DMASIZE:
		put_user(dev->dma_size ,(unsigned long*)arg);;
		retval=1;
		break;

	case CS4341A_IOCTL_PAUSE:

		power_down(true);
		ssi_transmit_enable(SSI2,false);

		retval=1;
		break;

	case CS4341A_IOCTL_RESUME:

		power_down(false);		
		ssi_transmit_enable(SSI2,true);


		retval=1;
		break;

    default:
		CS_WARNING("Invalid ioctl command");
		return -ENOIOCTLCMD;
    }
    return retval;
}

static void set_next_dma_req(struct cs4341a_dev *dev, dma_request_t *last_req, int bd ) {

	dma_request_t next_request;
	unsigned long tx_num,tx_to_the_end,cnt_other_bd;
	struct ssi_circ_buf *xmit = &dev->xmit;

	tx_num = ssi_circ_chars_pending(xmit,dev->ssi_buffer_size);
	if(tx_num <= dev->wakeup_data ) {
		dev->flag = 1;
		wake_up_interruptible(&dev->write_queue);
/* 		CS_TRACE("w%i %li %li %li",bd,xmit->tail,xmit->head,tx_num); */
    }
	
	memset(&next_request, 0, sizeof(dma_request_t));

	cnt_other_bd = (bd==0) ? dev->dma_copied_bd1 : dev->dma_copied_bd0;

/* 	xmit->tail = (xmit->tail + last_req->count + cnt_other_bd ) % (dev->ssi_buffer_size); */
	xmit->tail = (xmit->tail + cnt_other_bd ) % (dev->ssi_buffer_size);

	if (tx_num > 0) {
	
		tx_to_the_end = dev->ssi_buffer_size - xmit->tail ;
		
		/*         check if dev->dma_size is bigger than char_pending */
		tx_num = min((unsigned long)  tx_to_the_end ,(unsigned long)  dev->dma_size);

		tx_num -= (tx_num % (dev->period_size));

		/* 		CS_TRACE("d%li %li %li ",tx_num,xmit->tail,xmit->head); */
	
		if (bd==0) {
			dev->tx_handle0 =  dma_map_single(NULL, xmit->buf + xmit->tail ,
													 dev->dma_size,	 DMA_TO_DEVICE);
			next_request.sourceAddr = (__u8*) dev->tx_handle0;
			dev->dma_copied_bd0 = tx_num;

/* 			CS_TRACE("d%i %li %li",bd,xmit->tail,tx_num); */
		
		} else {
			dev->tx_handle1 = dma_map_single(NULL, xmit->buf + xmit->tail ,
											 dev->dma_size,
											 DMA_TO_DEVICE);
			next_request.sourceAddr = (__u8*) dev->tx_handle1;
			dev->dma_copied_bd1 = tx_num;
			
/*  			CS_TRACE("d%i %li %li",bd,xmit->tail,tx_num); */
	
		}
		next_request.count = tx_num;
		next_request.bd_cont = 1;
		memcpy(last_req, &next_request, sizeof(dma_request_t));
    } 

	
}


/*!
 * DMA Write callback is called by the SDMA controller after it has sent out all
 * the data from the user buffer.
 * @param   arg  driver private data
 */
static void cs4341a_dma_writecallback(void *arg)
{

    struct cs4341a_dev *dev = arg;
    dma_request_t writechnl_request_bd0,writechnl_request_bd1;
    unsigned long flags;

	test_pin_down();
    spin_lock_irqsave(dev->lock,flags);
	
	mxc_dma_get_config(d_info.wr_channel, &writechnl_request_bd0, 0);
	if(writechnl_request_bd0 .bd_done == 0) {

		dma_unmap_single(NULL, dev->tx_handle0 , dev->dma_size, DMA_TO_DEVICE);
		set_next_dma_req(dev,&writechnl_request_bd0,0);
		mxc_dma_set_config(d_info.wr_channel,  &writechnl_request_bd0   , 0);
	}

	mxc_dma_get_config(d_info.wr_channel, &writechnl_request_bd1, 1);
	if(writechnl_request_bd1 .bd_done == 0) {

		dma_unmap_single(NULL, dev->tx_handle1 , dev->dma_size, DMA_TO_DEVICE);
		set_next_dma_req(dev,&writechnl_request_bd1,1);
		mxc_dma_set_config(d_info.wr_channel,  &writechnl_request_bd1   , 1);

	}

    spin_unlock_irqrestore(dev->lock, flags);
	test_pin_up();
}



static int configure_write_channel(dma_info *d_info, struct cs4341a_dev *dev) {

    dma_channel_params  writechnl_params;
    int ret = 0;


    d_info->dma_txchnl_inuse = 0;

/*     /\* Allocate the DMA Transmit Buffer *\/ */
/*     if ((dev->tx_buf = kmalloc(dev->dma_size, GFP_KERNEL)) == NULL) { */
/* 		ret = -1; */
/* 		CS_WARNING("not enough memory to allocate tx_buf dma buffer"); */
/*     } */


    /* Set the write channel parameters */

    memset(&writechnl_params, 0, sizeof(dma_channel_params));

    writechnl_params.peripheral_type = SSI;
    writechnl_params.transfer_type = emi_2_per;
    writechnl_params.event_id = d_info->tx_event_id;
    writechnl_params.per_address = (SSI2_BASE_ADDR + MXC_SSISTX0) ;

	if (dev->data_length==16)
  		writechnl_params.watermark_level = 4;
	if (dev->data_length==24)
		writechnl_params.watermark_level = 3 ;

/*     writechnl_params.bd_number = 1; */
	writechnl_params.bd_number = 2;

	if (dev->data_length==16)
		writechnl_params.word_size = TRANSFER_16BIT;
	if (dev->data_length==24)
		writechnl_params.word_size = TRANSFER_24BIT;

    writechnl_params.callback = cs4341a_dma_writecallback;
    writechnl_params.arg = dev;


    if ((ret = mxc_dma_setup_channel(d_info->wr_channel, &writechnl_params)) != 0) {
		CS_WARNING("can't setup dma_channel");
		dev->dma_enabled = 0;
		return -1 ;
    }
    return ret;
}


/*!
 * Allocates DMA read and write channels, creates DMA read and write buffers and
 * sets the channel specific parameters.
 *
 * @param   d_info the structure that holds all the DMA information
 * @param   dev
 * @return  The function returns 0 on success and a non-zero value on failure.
 */
static int cs4341a_initdma(dma_info *d_info, struct cs4341a_dev *dev)
{
    int ret = 0;
	/*     dma_channel_params  writechnl_params; */
	
    /* Request for the write channels */
    ret = mxc_request_dma(&d_info->wr_channel, "CS4341a Write");
    if (ret != 0) {
		CS_WARNING("Cannot allocate DMA write channel\n");
		return ret;
    } else {
		CS_TRACE("Successfully allocated DMA with channel %i",d_info->wr_channel);
    }

    return ret;
}

/*!
 * Stops DMA and frees the DMA resources
 *
 * @param   d_info the structure that holds all the DMA information for a
 *                 particular MXC UART
 * @param   umxc   the MXC UART port structure, this includes the \b uart_port
 *                 structure and other members that are specific to MXC UARTs
 */
static void cs4341a_freedma(dma_info *d_info, struct cs4341a_dev *dev)
{
/*     kfree(dev->tx_buf); */
    mxc_dma_stop(d_info->wr_channel);
    mxc_free_dma(d_info->wr_channel);
}


int cs4341a_open(struct inode *inode, struct file *filp)
{
    int result;

    struct cs4341a_dev *dev; /* device information */
    ssi_status_enable_mask irq_mask = 0;

    dev = container_of(inode->i_cdev, struct cs4341a_dev, cdev);
    filp->private_data = dev; /* for other methods */

    spin_lock(dev->lock);

    (dev->opened)++ ;
    CS_TRACE("OPEN: opened =%i",dev->opened);

    if((dev->opened)==1) {
	
		/*change mode of the 3 pin os SPI1*/
		CS_TRACE("change mode pins spi1");
	
		/*enable cspi1 signals on full uart pads on HW1 mode*/
		iomux_config_gpr(MUX_PGP_CSPI_BB, true);
	
		/*spi clk*/
		iomux_config_mux(MX31_PIN_DSR_DCE1,OUTPUTCONFIG_ALT1,INPUTCONFIG_NONE);

		/* bring reset low*/
		mxc_set_gpio_dataout(dev->reset_pin,1);

		/*We set clock */
		init_clock_module();
		dev->ssi_port = SSI2;
		init_port_ssi2();
		init_audiomux();
	
		/*Bring up rst signal*/
		mxc_set_gpio_dataout(dev->reset_pin,1);
	
		/*set cs4341a registers*/
		result = init_cs4341a();
		if ( result  < 0 ) {
			CS_WARNING("Erreur on init conifigure chip audio ");
		}
	
		/* PDN bit set to 0, start of power up sequence*/
		power_down(false);
		
		udelay(100);

		ssi_tx_flush_fifo(SSI2);
		ssi_tx_fifo_empty_watermark(SSI2, ssi_fifo_0, cs4341a_devices->watermark_0);

		if(dev->dma_enabled==0) {
			irq_mask |= ssi_tx_fifo_0_empty ;
			irq_mask |= ssi_transmitter_underrun_0 ;
		}

		ssi_interrupt_enable(SSI2,irq_mask);

		dev->tx_started = false;
		dev->tx0UnderRunError = 0;
		dev->flag = 0;
	
		dev->sample_rate = 48000 ;
		dev->data_length = 16;

		result = request_irq(INT_SSI2,
							 cs4341a_interrupt,
							 0,
							 /* SA_INTERRUPT, */
							 "CS4341a",
							 filp->private_data);
		if (result){
			CS_WARNING("Can't get irq line number %d",INT_SSI2);
			return result;
		}

		/* Initialize the DMA if we need SDMA data transfer */
		if (dev->dma_enabled == 1) {
			result = cs4341a_initdma(&d_info,dev);
		  
			if (result != 0) {
				CS_WARNING(" Failed to initialize DMA for SSI2 \n");
				free_irq(INT_SSI2,dev);
				return result;
			}
		}
    }
	
    return 0;
}


int cs4341a_release(struct inode *inode, struct file *filp)
{
    ssi_status_enable_mask irq_disable_mask = 0;
    struct cs4341a_dev *dev = filp->private_data;
/* 	int i; */

    spin_lock(&dev->lock);

    (dev->opened)-- ;
    CS_TRACE("RELEASE: opened =%i",dev->opened);
	
    if(dev->opened == 0) {
		CS_TRACE("RELEASE:shutdown chip");

		if(dev->dma_enabled) {
			irq_disable_mask = ssi_tx_dma_interrupt_enable;
			ssi_interrupt_disable(SSI2,irq_disable_mask);
			cs4341a_freedma(&d_info,dev);
		}
		
		irq_disable_mask = ssi_tx_interrupt_enable;
		ssi_interrupt_disable(SSI2,irq_disable_mask);
		
		ssi_transmit_enable(SSI2,false);
		dev->tx_started = false;

		free_pages((unsigned long)dev->xmit.buf,get_order(dev->ssi_buffer_size));

		power_down(true);

		ssi_enable(SSI2,false);

		free_irq(INT_SSI2,dev);

		CS_TRACE("Tx0UnderrunError=%li", dev->tx0UnderRunError);
    }

    spin_unlock(dev->lock);

    return 0;
}

struct file_operations cs4341a_fops = {
    .owner =    THIS_MODULE,
    .write =    cs4341a_write,
    .ioctl =    cs4341a_ioctl,
    .open =     cs4341a_open,
    .release =  cs4341a_release,
};


static void cs4341a_cleanup_module(void)
{

    dev_t devno = MKDEV(cs4341a_major, cs4341a_minor);

    mxc_free_iomux(cs4341a_devices->reset_pin ,OUTPUTCONFIG_GPIO,INPUTCONFIG_GPIO);

#ifdef USE_TEST_PIN
    mxc_free_gpio(TEST_PIN);
#endif

    /* SPBA configuration for SSI2 - SDMA is released */
    spba_rel_ownership(SPBA_SSI2, SPBA_MASTER_C); 

    /* 	Get rid of our char dev entries */
    if (cs4341a_devices) {
		/*
		 * Free the transmit buffer page.
		 */
		if (cs4341a_devices->xmit.buf) {
			/* 		free_page((unsigned long)cs4341a_devices->xmit.buf); */
/* 			free_pages((unsigned long)cs4341a_devices->xmit.buf,get_order(cs4341a_devices->ssi_buffer_size)); */
			cs4341a_devices->xmit.buf = NULL;
		}
		cdev_del(&cs4341a_devices->cdev);
		kfree(cs4341a_devices);
    }

    /* cleanup_module is never called if registering failed */
    unregister_chrdev_region(devno,CS4341A_NR_DEVS);
}


static int __init cs4341a_init_module(void)
{

    int result,err;
	/*     int order; */
    dev_t dev = 0;
	/*     unsigned long page = 0;     */

	/*     struct circ_buf *xmit = &cs4341a_devices->xmit; */
	
/* 	struct ssi_circ_buf xmit; */
/* 	unsigned long size; */

    CS_TRACE("--> init_module()");
    CS_TRACE("CIRRUS LOGIC CS4341A Driver Version = %lX.%02lX",
			 (DRIVER_VERSION>>8),(DRIVER_VERSION&0xFFUL));
    CS_TRACE("Compiled: %s, %s",__DATE__,__TIME__);

/* 	xmit.head=200;xmit.tail=100;size=1000; */
/* 	CS_TRACE("%li %li %li = %li", 200 , 100,1000,ssi_circ_chars_free(&xmit,1000)); */
/* 	CS_TRACE("%li %li %li = %li", 200 , 100,1000,ssi_circ_chars_pending(&xmit,1000)); */

/* 	xmit.head=100;xmit.tail=200;size=1000; */
/* 	CS_TRACE("%li %li %li = %li", 100 , 200,1000,ssi_circ_chars_free(&xmit,1000)); */
/* 	CS_TRACE("%li %li %li = %li", 100 , 200,1000,ssi_circ_chars_pending(&xmit,1000)); */

    if(cs4341a_major){
		dev = MKDEV(cs4341a_major,cs4341a_minor);

		result = register_chrdev_region(dev, CS4341A_NR_DEVS , "cs4341a");
    }else{
		result = alloc_chrdev_region(&dev, cs4341a_minor ,CS4341A_NR_DEVS ,"cs4341a");
		CS_WARNING("CS4341A_MAJOR number must be assigned in a static way DYNAMIC ALLOCATION DONE");
    }
    if (result<0){
		CS_WARNING("Cirrus can't get major number %i \n",cs4341a_major);
		return result;
    }

    CS_TRACE("over_sampling = %i",over_sampling);
     
    cs4341a_devices = kmalloc(CS4341A_NR_DEVS * sizeof(struct cs4341a_dev), GFP_KERNEL );
    if(!cs4341a_devices){
		result = -ENOMEM;
		goto fail;  /* Make this more graceful */
    }

    memset(cs4341a_devices, 0 , CS4341A_NR_DEVS *  sizeof(struct cs4341a_dev));


    /* SPBA configuration for SSI2 - SDMA and MCU are set */
    spba_take_ownership(SPBA_SSI2, SPBA_MASTER_C|SPBA_MASTER_A);

    if(wm_fifo_0 > 8) {
		wm_fifo_0 = 8;
		CS_WARNING("watermark fifo 0 value too high,reset to %i",wm_fifo_0);
    }
    if(wm_fifo_0 < 0) {
		wm_fifo_0 = 0;
		CS_WARNING("watermark fifo 0 value negative,reset to %i",wm_fifo_0);
    }
    cs4341a_devices->watermark_0 = wm_fifo_0;
/*     CS_TRACE("Watermark Fifo 0 = %i",	cs4341a_devices->watermark_0); */

	if(use_dma==0){
		CS_TRACE("DMA DISABLED");
    } else {
		CS_TRACE("DMA ENABLED");
		CS_TRACE("nb_dma_req=%i",nb_dma_req);
    }
    cs4341a_devices->dma_enabled = use_dma;
	
    spin_lock_init(&cs4341a_devices->lock);

    /* Initialize cs_4341a device. */
    result = init_port_spi();
    if (result<0){
		CS_WARNING("init port spi failed ");
		goto fail;
    }
	
    init_waitqueue_head(&cs4341a_devices->write_queue);

    /* request ownership of reset pin */
    cs4341a_devices->reset_pin = IOMUX_RESET_NAME;
    if(!(mxc_request_gpio(cs4341a_devices->reset_pin )==0)){
		CS_WARNING("  REQUEST_IOMUX FOR THAT GPIO NOT AVAILABLE");
		result=-ENODEV;
    }
    mxc_set_gpio_direction(cs4341a_devices->reset_pin,0); //0 for input

    /* Make sure no clocks from ssi to cs4341a */
    ssi_enable(SSI2,false);

    /* leave reset high before enable ssi clock, done in open*/
    mxc_set_gpio_dataout(cs4341a_devices->reset_pin,1);

#ifdef USE_TEST_PIN
    if(!(mxc_request_gpio(TEST_PIN)==0)){
		CS_WARNING("  REQUEST_IOMUX FOR TEST_PIN NOT AVAILABLE");
		result=-ENODEV;
    }
    mxc_set_gpio_direction(TEST_PIN,0); //0 for input
    mxc_set_gpio_dataout(TEST_PIN, 1); //Set to 1 by default
#endif

    /* Device registration */
    cdev_init(&cs4341a_devices->cdev, &cs4341a_fops );
    cs4341a_devices->cdev.owner = THIS_MODULE;
    cs4341a_devices->cdev.ops = &cs4341a_fops;
    err = cdev_add(&cs4341a_devices->cdev, dev, 1);
    if (err)
		CS_WARNING("Error %d adding cs4341a", err);
		
    return 0; /* succeed*/

 fail:
	
    cs4341a_cleanup_module();
    return result;
}

module_init(cs4341a_init_module);
module_exit(cs4341a_cleanup_module);

MODULE_AUTHOR("SAGEM");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MXC driver for ALSA");
MODULE_SUPPORTED_DEVICE("{{cs4341a}}");
