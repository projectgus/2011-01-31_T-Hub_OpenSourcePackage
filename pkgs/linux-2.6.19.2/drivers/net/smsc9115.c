/***************************************************************************
 *
 * Copyright (C) 2004-2005  SMSC
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 ***************************************************************************
 * File: simp911x.c
 *
 */

/*********************************************************
 ****************** USERS GUIDE ***************************
 **********************************************************
This is the users guide for the LAN911x Simple Linux Driver.
LAN911x refers to the following chips.
	LAN9118
	LAN9117
	LAN9116
	LAN9115
	LAN9112
The following sections can be found below.
*  Revision History
*  Features
*  Driver Parameters
*  Tested Platforms
*  Rx Code Path
*  Tx Code Path

#######################################################
############### REVISION HISTORY ######################
#######################################################

12/02/2004 Bryan Whitehead, Version 1.00
Initial release 

12/13/2004 Bryan Whitehead, Version 1.01
Added Tests Platforms section in comments.
Ran the code through the LINT tool, and made necessary
    changes.

12/17/2004 Bryan Whitehead, Version 1.02
Simplified synchronization policies.
Reduced Spin lock count to 2.
Added a comment block describing how every register is
  protected from concurrent access or why it does not 
  need protection.
Added support for one driver to handle multiple
  instances of the LAN911x

12/20/2004 Bryan Whitehead, Version 1.03
Removed GPL spin lock wrappers
Removed BWL spin locks
Added netperf scores to tested platforms, and kernel versions
Changed INSTANCE_COUNT to MAX_INSTANCES
Added support for the external Phy

12/21/2004 Bryan Whitehead, Version 1.04
Removed references to Stop lock
Changed MAX_INSTANCES to 8
Reduced some timeout lengths
change phy reset to use the reset bit in the PHY_BCR register
  this method will remain compatible with external phys
Elaborated on the comment for MacPhyAccessLock in PRIVATE_DATA

12/22/2004 Bryan Whitehead, Version 1.05
Changed initialization of SIMP911X and registered to a 
programmatic initialization in smsc9115_init_module
removed MULTI_INSTANCE_MODULE_PARM macro which didn't work right
   will use MODULE_PARM instead.

01/07/2005 Bryan Whitehead, Version 1.06
Safely disable IRQ in smsc9115_stop
Safely disable Tx queue in smsc9115_stop

01/25/2005 Bryan Whitehead, Version 1.07
Fixed Faulty define, TX_CMD_A_BUF_SIZE_
Fixed faulty define, TX_CMD_B_PKT_BYTE_LENGTH_
Increased Rx_FastForward, timeout to 500

02/03/2005 Bryan Whitehead, Version 1.08
Fixed a potential problem with regards to the
  use of multiple instances, the global variable
  Rx_TaskletParameter is now a global array designed 
  for use with multiple instances.
  NOTE: since we have no platform for testing
  multiple instances yet, the multi instance code
  used in this driver is for illustrative purposes only.
  Do keep in mind that it has not been tested. 

02/28/2005 Bryan Whitehead, Version 1.09
Updated Rx_CountErrors
	Only count length error if length/type field is set
	If there is a CRC error don't count length error or
	  multicast
Added date_code

03/01/2005 Bryan Whitehead, Version 1.10
Added support for LAN9112

05/23/2005 Bryan Whitehead, Version 1.11
Added Phy Work Around code.
  Enabled by defining USE_PHY_WORK_AROUND
  See section marked PHY WORK AROUND for changes.

05/25/2005 M. David Gelbman, Version 1.12
Added 10/100 LED1 Work Around code.
  Enabled by defining USE_LED1_WORK_AROUND
  See section marked LED1 WORK AROUND for changes.

############################################################
################### FEATURES ###############################
############################################################
Only minimum features are included. This is to allow for 
easy understanding of how to use the LAN911x chips

PIO is used for transmit and receive.
Link management
Multicast capable
Debug messages

############################################################
################## DRIVER PARAMETERS #######################
############################################################
The following are load time modifiable driver parameters.
They can be set when using insmod
Example:
# insmod simp911x.o lan_base=0xB4000000 irq=8

lan_base
    specifies the physical base location in memory where the 
    LAN911x can be accessed. If a LAN911x can not be found
    at the location specified then driver initialization 
    will fail.
        
irq
    specifies the irq number to use. If this number is 
    incorrect, then driver initialization will fail.
    
mac_addr_hi16
    Specifies the high word of the mac address. If it was not 
    specified then the driver will try to read the mac address 
    from the eeprom. If that failes then the driver will set it 
    to a valid value. Both mac_addr_hi16 and mac_addr_lo32 must be 
    used together or not at all.
    
mac_addr_lo32
    Specifies the low dword of the mac address. If it was not 
    specified then the driver will try to read the mac address 
    from the eeprom. If that failes then the driver will set it to 
    a valid value. Both mac_addr_hi16 and mac_addr_lo32 must be 
    used together or not at all.
    
debug_mode
    specifies the debug mode
        0x01, bit 0 display trace messages
        0x02, bit 1 display warning messages
    Trace messages will only display if the driver was compiled
        with USE_TRACE defined.
    Warning message will only display if the driver was compiled
        with USE_WARNING defined.

phy_addr
	The default value of 0xFFFFFFFF tells the driver to use the internal phy.
	A value less than or equal to 31 tells the driver to use the external phy
	    at that address. If the external phy is not found the internal phy
	    will be used.
	Any other value tells the driver to search for the external phy on all
	    addresses from 0 to 31. If the external phy is not found the internal 
	    phy will be used.
	

###########################################################
################### TESTED PLATFORMS ######################
###########################################################
This driver has been tested on the following platforms.
The driver was loaded as a module with the following command
line.

	insmod simp911x.o lan_base=LAN_BASE irq=IRQ

Where LAN_BASE, and IRQ are replaced with appropriate
driver resources for the particular platform as indicated
below.

It should also be noted that the LAN911x may not be accessable
unless the bus as already been configured properly. Usually
this is done by the BIOS or some architecture initialization
code. Bus configuration is platform specific and therefore 
beyond the scope of this demonstration driver.
===========================================================
Platform: 
	SH3 SE01
Motherboard: 
	Hitachi/Renesas, MS7727SE01/02
SMSC LAN911x Board: 
	LAN9118 FPGA DEV BOARD, ASSY 6337 REV A
LAN911x:
	LAN9118
Linux Kernel Version:
	2.4.18
Driver Resources: 
	LAN_BASE=0xB4000000
	IRQ=8
netperf Scores:
	TCP RX:                  49.70 Mbps
	TCP TX:                  39.54 Mbps
	UDP RX -m1472 -w10 -b59: 52.08 Mbps
	UDP TX -m1472:           68.06 Mbps

===========================================================
Platform: 
	XSCALE
Motherboard:
	Intel Corp, MAINSTONE II MAIN BOARD REV 2.1
SMSC LAN911x Board:
	LAN9118 XSCALE FPGA DEV BOARD, ASSY 6343 REV A
LAN911x:
	LAN9118
Linux Kernel Version:
	2.4.21
Driver Resources:
	LAN_BASE=0xF2000000
	IRQ=198	
netperf Scores:
	TCP RX:                  58.97 Mbps
	TCP TX:                  88.04 Mbps
	UDP RX -m1472 -w10 -b61: 60.87 Mbps 
	UDP TX -m1472:           94.73 Mbps

###########################################################
##################### RX CODE PATH ########################
###########################################################
The purpose of this section is to describe how the driver
receives packets out of the LAN911x and passes them to the
Linux OS. Most functions in the Rx code path start with Rx_

During initialization (smsc9115_open) the function 
Rx_Initialize is called. This call enables interrupts for 
receiving packets.

When a packet is received the LAN911x signals an interrupt, 
which causes smsc9115_ISR to be called. The smsc9115_ISR function
is the main ISR which passes control to various handler routines.
One such handler routine that gets called is Rx_HandleInterrupt.

Rx_HandleInterrupt first checks to make sure the proper 
interrupt has been signaled (INT_STS_RSFL_). If it has not
it returns immediately. If INT_STS_RSFL_ has been signaled
then it schedules Rx_Tasklet which will shortly after call
Rx_ProcessPacketsTasklet to service the packets that have arrived.

The tasklet runs the following algorithm to service packets.

    An Rx status DWORD is read using Rx_PopRxStatus
    If there is an error 
        the packet is purged from the LAN911x
            data fifo with Rx_FastForward
    If there is no error 
        an sk_buff is allocated to receive the data
        The data is read into the sk_buff using PIO.
        After data is read the sk_buff is sent to linux using
            Rx_HandOffSkb
    The process repeats until Rx_PopRxStatus returns 0

The driver must also participate in Rx flow control because of
platform speed limitations. Rx flow control in the driver is handled
in the functions Rx_HandOffSkb, and Rx_PopRxStatus. The function
Rx_HandOffSkb calls netif_rx which returns a congestion level.
If any congestion is detected then Rx_HandOffSkb will set the
RxCongested flag. The next time the driver calls Rx_PopRxStatus it
will see the RxCongested flag is set and will not Pop a new status
off the RX_STATUS_FIFO, but rather it will disable and re-enable
interrupts. This action will cause the interrupt deassertion interval
to begin. The ISR will return, and not be called again until the 
deassertion interval has expired. This gives CPU time to linux so it
can handle the packets that have been sent to it and lower the
congestion level. In this case the driver voluntarily stops
servicing packets, which means the RX Data Fifo will fill up.
Eventually the hardware flow control will start up to slow down the
sender. Many times this will still result in an overflow of the 
Rx Data fifo and packets will be lost under heavy traffic conditions.
But if the driver did not release the CPU to linux, then linux would
drop almost all the packets. So it is better to let the packets be
lost on the wire, rather than over consuming resources to service
them.


###########################################################
##################### TX CODE PATH ########################
###########################################################
When Linux wants to transmit a packet it will call 
smsc9115_hard_start_xmit. This function performs some checks
then calls Tx_SendSkb to send the packet.

The basic sequence is this.
First Write TxCmdA and TxCmdB into the TX_DATA_FIFO.
Then Write the packet data into the TX_DATA_FIFO with 
    adjustments for offset and end alignment.
Then free the skb using dev_kfree_skb

Tx Flow control works like this.
If the free space in the TX_DATA_FIFO after writing the data
is determined to be less than about the size of one full size
packet then the driver can't be sure it can transmit the next
packet without overflowing. Therefore the driver turns off the 
Tx queue by calling netif_stop_queue. Then it prepares an interrupt
to be signaled when the free space in the TX_DATA_FIFO has risen 
to an acceptable level. When that level of free space has been 
reached the interrupt is signaled and the handler will turn on
the Tx queue by calling netif_wake_queue

Statistic counters are updated after about every 30 packets or
when linux calls smsc9115_get_stats

###########################################################
################### PHY WORK AROUND #######################
###########################################################
This section provides a brief overview of the changes necessary
to implement the phy work around.

First, the nature of the problem is this. During initialization
the phy gets reset. It has been found to be possible that
the phy will come out of reset in a state that may cause transmitted
packets to be inverted.

The fix for this problem is too run a phy loop back test after
a phy reset in order to detect inverted packets. If an inverted
packet is detected then the phy is reset again, and the loop back
test is repeated. If the loop back packet returns with out error,
then that is confirmation that the phy came out of reset properly,
so the code resumes normal initialization.

The loop back test was added to the function Phy_Initialize.
To see all necessary changes search for USE_PHY_WORK_AROUND

*/

#ifndef __KERNEL__
#define __KERNEL__
#endif


#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/ioport.h>
#include <linux/errno.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/delay.h>
#include <linux/mii.h>
#include <linux/timer.h>
#include <linux/moduleparam.h>

#include <asm/irq.h>
#include <asm/dma.h>
#include <asm/bitops.h>
#include <linux/version.h>
#include <linux/proc_fs.h>


MODULE_LICENSE("Dual BSD/GPL");


#ifdef CONFIG_ARCH_MX3
#include <asm/arch/gpio.h>
#include <asm/arch/board.h>
#include "../../kernel/arch/arm/mach-mx3/iomux.h"
#else
#error CONFIG_ARCH_MX3 not defined !!!
#endif



static const char date_code[]="0623015";



#define USE_DEBUG_PROC

#define USE_PHY_WORK_AROUND
//#define USE_LED1_WORK_AROUND	// 10/100 LED link-state inversion

#define LINUX_2_6_OR_NEWER

#define USE_DEBUG

#ifdef USE_DEBUG
//select debug modes
#define USE_WARNING
#define USE_TRACE
#define USE_ASSERT
#endif //USE_DEBUG

typedef unsigned long DWORD;
typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef unsigned char BOOLEAN;
#define TRUE	((BOOLEAN)1)
#define FALSE	((BOOLEAN)0)

#define HIBYTE(word)  ((BYTE)(((WORD)(word))>>8))
#define LOBYTE(word)  ((BYTE)(((WORD)(word))&0x00FFU))
#define HIWORD(dWord) ((WORD)(((DWORD)(dWord))>>16))
#define LOWORD(dWord) ((WORD)(((DWORD)(dWord))&0x0000FFFFUL))

/*******************************************************
 * Macro: SMSC_TRACE
 * Description: 
 *    This macro is used like printf. 
 *    It can be used anywhere you want to display information
 *    For any release version it should not be left in 
 *      performance sensitive Tx and Rx code paths.
 *    To use this macro define USE_TRACE and set bit 0 of debug_mode
 *******************************************************/
#ifdef USE_TRACE
static DWORD debug_mode;
#ifndef USE_WARNING
#define USE_WARNING
#endif
#	define SMSC_TRACE(msg,args...)			\
	if(debug_mode&0x01UL) {					\
		printk("SMSC: " msg "\n", ## args);	\
	}
#else
#	define SMSC_TRACE(msg,args...)
#endif

/*******************************************************
 * Macro: SMSC_WARNING
 * Description: 
 *    This macro is used like printf. 
 *    It can be used anywhere you want to display warning information
 *    For any release version it should not be left in 
 *      performance sensitive Tx and Rx code paths.
 *    To use this macro define USE_TRACE or 
 *      USE_WARNING and set bit 1 of debug_mode
 *******************************************************/
#ifdef USE_WARNING
static DWORD debug_mode;
#ifndef USE_ASSERT
#define USE_ASSERT
#endif
#	define SMSC_WARNING(msg, args...)				\
	if(debug_mode&0x02UL) {							\
		printk("SMSC_WARNING: " msg "\n",## args);	\
	}
#else
#	define SMSC_WARNING(msg, args...)
#endif


/*******************************************************
 * Macro: SMSC_ASSERT
 * Description: 
 *    This macro is used to test assumptions made when coding. 
 *    It can be used anywhere, but is intended only for situations
 *      where a failure is fatal. 
 *    If code execution where allowed to continue it is assumed that 
 *      only further unrecoverable errors would occur and so this macro
 *      includes an infinite loop to prevent further corruption.
 *    Assertions are only intended for use during developement to 
 *      insure consistency of logic through out the driver.
 *    A driver should not be released if assertion failures are 
 *      still occuring.
 *    To use this macro define USE_TRACE or USE_WARNING or
 *      USE_ASSERT
 *******************************************************/
#ifdef USE_ASSERT
#	define SMSC_ASSERT(condition)													\
	if(!(condition)) {																\
		printk("SMSC_ASSERTION_FAILURE: File=" __FILE__ ", Line=%d\n",__LINE__);	\
		while(1);																	\
	}
#else
#	define SMSC_ASSERT(condition)
#endif


//The following describes the synchronization policies used in this driver.
//Register Name				Policy
//RX_DATA_FIFO				Only used by the Rx Thread, Rx_ProcessPackets
//TX_DATA_FIFO				Only used by the Tx Thread, Tx_SendSkb
//RX_STATUS_FIFO			Only used by the Rx Thread, Rx_ProcessPackets
//RX_STATUS_FIFO_PEEK		Not used.
//TX_STATUS_FIFO			Used in	Tx_CompleteTx in Tx_UpdateTxCounters.
//                          Tx_UpdateTxCounters is called by Tx_SendSkb in
//                              smsc9115_hard_start_xmit.
//							Tx_UpdateTxCounters is also called by smsc9115_stop
//                              but only after disabling the tx queue in a multithread
//                              safe manner using the xmit_lock
//
//TX_STATUS_FIFO_PEEK		Not used.
//ID_REV					Read only
//INT_CFG					Set in Lan_Initialize, 
//							read in smsc9115_ISR, 
//							1) ClrBits in smsc9115_ISR
//							2) ClrBits in Rx_HandleInterrupt,
//                          3) SetBits in Rx_PopRxStatus,
//							4) SetBits in Rx_ProcessPacketsTasklet
//                            items 1,2,3,4 are not in contention 
//                            because 1 and 2, are part of the ISR, 
//                            and 3 and 4 is the tasklet which is only called
//                            while the ISR is disabled.
//INT_STS					Sharable, 
//INT_EN					Initialized at startup, 
//                          used in smsc9115_stop.
//BYTE_TEST					Read Only
//FIFO_INT					Initialized at startup,
//                          During run time only accessed by 
//                              Tx_HandleInterrupt, and Tx_SendSkb and done in a safe manner
//RX_CFG					Only used during initialization
//TX_CFG					Only used during initialization
//HW_CFG					Only used during initialization
//RX_DP_CTRL				Only used in Rx Thread, in Rx_FastForward
//RX_FIFO_INF				Read Only, Only used in Rx Thread, in Rx_PopRxStatus
 //TX_FIFO_INF				Read Only, Only used in Tx Thread, in Tx_GetTxStatusCount, Tx_SendSkb, Tx_CompleteTx
//PMT_CTRL					Not Used
//GPIO_CFG					Only used during initialization, in Lan_Initialize
//GPT_CFG					Not Used
//GPT_CNT					Not Used
//ENDIAN					Not Used
//FREE_RUN					Not Used
//RX_DROP					Only used in Rx Thread, Rx_ProcessPackets
//MAC_CSR_CMD				Protected by MacPhyAccessLock
//MAC_CSR_DATA				Protected by MacPhyAccessLock
//                          Because the two previous MAC_CSR_ registers are protected
//                            All MAC, and PHY registers are protected as well.
//AFC_CFG					Used during initialization, in Lan_Initialize
//                          During run time, used in timer call back, in Phy_UpdateLinkMode
//E2P_CMD					Used during initialization, in Lan_Initialize
//E2P_DATA					Not Used


typedef struct _PRIVATE_DATA {
	DWORD dwLanBase;
	DWORD dwIdRev;
	struct net_device *dev;

	iomux_pin_icfg_t gpioIrq;

	spinlock_t MacPhyAccessLock;
	//The MacPhyAccessLock must be acquired before 
	//  calling any of the following functions
	//    Mac_GetRegDW
	//    Mac_SetRegDW
	//    Phy_GetRegW
	//    Phy_SetRegW
	//  but only when multiple threads may be calling these functions
	//  concurrently. For this Driver this only occurs during runtime
	//  when the Timer callback calls Phy_UpdateLinkMode(), which
	//  can run concurrently with Rx_SetMulticastList().
	//  Acquiring the lock is not necessary during initialization
	//  and shutdown, since the other threads are not running
	//  at those times.


#ifdef USE_DEBUG_PROC
	spinlock_t LanAccessLock;
#endif


	BOOLEAN RxCongested;

	struct net_device_stats stats;

	DWORD dwPhyAddress;
#ifdef USE_LED1_WORK_AROUND
	DWORD NotUsingExtPhy;
#endif
	DWORD dwLinkSpeed;
	DWORD dwLinkSettings;
	struct timer_list LinkPollingTimer;
	BOOLEAN StopLinkPolling;

	BOOLEAN RequestIrqDisable;
	BOOLEAN SoftwareInterruptSignal;

#ifdef USE_PHY_WORK_AROUND
#define MIN_PACKET_SIZE (64)
	DWORD dwTxStartMargen;
	BYTE LoopBackTxPacket[MIN_PACKET_SIZE];
	DWORD dwTxEndMargen;
	DWORD dwRxStartMargen;
	BYTE LoopBackRxPacket[MIN_PACKET_SIZE];
	DWORD dwRxEndMargen;
	DWORD dwResetCount;	
#endif
} PRIVATE_DATA, *PPRIVATE_DATA;

//This driver supports multiple instances
//  of the LAN911x
//NOTE: at the time of this driver release
//  we do not have a multiple instance platform
//  to test with. Therefore the support this 
//  driver has for multiple instances is largely
//  for illustrative purposes only, and has
//  not been thoroughly tested.
//To load 2 instances follow this example
//  insmod simp911x.o lan_base=ADDR1,ADDR2 irq=IRQ1,IRQ2
//Where ADDR1 and IRQ1 are replaced with the lan_base address
//   and irq of the first instance. And ADDR2 and IRQ2 are
//   replaced with the lan_base address and irq of the 
//   second instance.
//To change the supported instances
//  change MAX_INSTANCES and the number of
//  initializing elements in the following resource arrays 
//    lan_base,
//    irq,
//    mac_addr_hi16,
//    mac_addr_lo32,
//    phy_addr
//  also must modify the second parameter of all
//    MODULE_PARM macros (except for MODULE_PARM(debug_mode,"i");)
//    to "0-Xi" where X is replaced
//    with the same value of MAX_INSTANCES
//  also must modify the Rx_TaskletParameter,
//    And make sure there are enough Rx_Tasklets declared
//    And make sure Rx_HandleInterrupt will schedule 
//       the correct Rx_Tasklet

#include "smsc9115.h"

#define SMSC_BASE_ADDRESS SMSC_LAN9115_ETH_BASE_ADDRESS
#define IOMUX_IRQ_NAME MX31_PIN_GPIO1_1

#define DRV_NAME "SIMP_LAN911X_ISR"

static DWORD mac_addr_hi16=0xFFFFFFFFUL;

module_param(mac_addr_hi16,ulong,S_IRUGO);
//MODULE_PARM_DESC(mac_addr_hi16,"Specifies the high 16 bits of the mac address");

static DWORD mac_addr_lo32=0xFFFFFFFFUL;
module_param(mac_addr_lo32,ulong,S_IRUGO);
/* MODULE_PARM_DESC(mac_addr_lo32,"Specifies the low 32 bits of the mac address"); */

#ifdef USE_DEBUG
static DWORD debug_mode=0x3UL;
#else
static DWORD debug_mode=0x0UL;
#endif
module_param(debug_mode,ulong,S_IRUGO);
/* MODULE_PARM_DESC(debug_mode,"bit 0 enables trace points, bit 1 enables warning points"); */


static WORD auto_neg=0x0;
module_param(auto_neg,ushort,S_IRUGO);


static WORD fullduplex=1;
module_param(fullduplex,ushort,S_IRUGO);

static WORD speed=100;
module_param(speed,ushort,S_IRUGO);

DWORD phy_addr=0xFFFFFFFFUL;
/* Phy address is no longer a parameter, we use the internal one */

//  TODO change type to boolean 
DWORD registered;

unsigned long Rx_TaskletParameter=0UL;
DECLARE_TASKLET(Rx_Tasklet0,Rx_ProcessPacketsTasklet,0);


MODULE_LICENSE("GPL");

int smsc9115_init(struct net_device *dev);
int smsc9115_open(struct net_device *dev);
int smsc9115_stop(struct net_device *dev);
int smsc9115_hard_start_xmit(struct sk_buff *skb, struct net_device *dev);
struct net_device_stats * smsc9115_get_stats(struct net_device *dev);
void smsc9115_set_multicast_list(struct net_device *dev);
irqreturn_t smsc9115_ISR(int irq,void *dev_id,struct pt_regs *regs);


struct net_device* simp911x;


#define DRIVER_VERSION (0x00000111UL)




#ifdef USE_DEBUG_PROC
int smsc9115_read_procmem(char *buf, char **start, off_t offset,
                   int count, int *eof, void *data)
{
	int len = 0;

	PPRIVATE_DATA privateData = netdev_priv(simp911x) ;

	len += sprintf(buf+len,"\nINT_CFG REGISTER = 0x%08lX",Lan_GetRegDW(INT_CFG));
	len += sprintf(buf+len,"\nINT_STATUS = 0x%08lX ", Lan_GetRegDW(INT_STS));
	len += sprintf(buf+len,"\nINT_ENABLE REGISTER = 0x%08lX",Lan_GetRegDW(INT_EN));
	len += sprintf(buf+len,"\nFIFO_INT REGISTER = 0x%08lX",Lan_GetRegDW(FIFO_INT));
	len += sprintf(buf+len,"\nRX_CFG REGISTER = 0x%08lX",Lan_GetRegDW(RX_CFG));
	len += sprintf(buf+len,"\nTX_CFG REGISTER = 0x%08lX",Lan_GetRegDW(TX_CFG));
	len += sprintf(buf+len,"\nHW_CFG REGISTER = 0x%08lX",Lan_GetRegDW(HW_CFG));
	len += sprintf(buf+len,"\nTX_FIFO_INF = 0x%08lX ", Lan_GetRegDW(TX_FIFO_INF));
	len += sprintf(buf+len,"\nRX_FIFO_INF = 0x%08lX ", Lan_GetRegDW(RX_FIFO_INF));

	spin_lock(&(privateData->MacPhyAccessLock));

	len += sprintf(buf+len,"\nMAC_CR REGISTER = 0x%08lX",Mac_GetRegDW(privateData,MAC_CR));
	len += sprintf(buf+len,"\nPHY_BCR REGISTER = %x ",Phy_GetRegW(privateData,PHY_BCR));

	len += sprintf(buf+len,"\nPHY_BSR REGISTER = %x",Phy_GetRegW(privateData,PHY_BSR));	

	len += sprintf(buf+len,"\nPHY AUTO NEG ADVERTISEMENT = %x",Phy_GetRegW(privateData,PHY_ANEG_ADV));	

	len += sprintf(buf+len,"\nPHY AUTO NEG LINK PARTNER ABILITY = %x",Phy_GetRegW(privateData,PHY_ANEG_LPA));
	len += sprintf(buf+len,"\nPHY SPECIAL REGISTER = %x",Phy_GetRegW(privateData,PHY_SPECIAL_MODE));
	len += sprintf(buf+len,"\nPHY SPECIAL CTRL_STS REGISTER = %x",Phy_GetRegW(privateData,PHY_SPECIAL));
	len += sprintf(buf+len,"\n");
	
	spin_unlock(&(privateData->MacPhyAccessLock));

	return len;
}


/*
 * Actually create (and remove) the /proc file(s).
 */
static void smsc9115_create_proc(void)
{
	create_proc_read_entry("smsc9115", 0 /* default mode */,
			NULL /* parent dir */, smsc9115_read_procmem,
			NULL /* client data */);

}

static void smsc9115_remove_proc(void)
{
	/* no problem if it was not registered */
	remove_proc_entry("smsc9115 ", NULL /* parent dir */);

}

#endif //USE_DEBUG_PROC

//entry point for loading the module
static int __init smsc9115_init_module(void)
{
	int result=0;
	int device_present=0;

	
	SMSC_TRACE("--> init_module()");
#ifdef USE_PHY_WORK_AROUND
	SMSC_TRACE("Simple Driver Version = %lX.%02lX, with phy work around",	 (DRIVER_VERSION>>8),(DRIVER_VERSION&0xFFUL));
#else
	SMSC_TRACE("Simple Driver Version = %lX.%02lX", (DRIVER_VERSION>>8),(DRIVER_VERSION&0xFFUL));
#endif
	SMSC_TRACE("Compiled: %s, %s",__DATE__,__TIME__);

#ifdef USE_DEBUG_PROC
	SMSC_TRACE("/proc/smsc9115 debug activated");
#endif

#ifdef USE_DEBUG
	switch(auto_neg) {

	case 0:
		SMSC_TRACE("Auto_negiocatio Disabled");
		break;
	case 1:
		SMSC_TRACE("Auto_negiocation SMSC enabled");
		break;
	    
	case 2:
		SMSC_TRACE("Auto_negiocation SAGEM enabled");
		break;

	default:
		SMSC_WARNING("auto_neg parameter wrong, default value = %i",auto_neg);

	}
#endif

	
/* 	simp911x = alloc_netdev(sizeof(PRIVATE_DATA), "eth%d", (void *) smsc9115_init); */
	simp911x = alloc_etherdev(sizeof(PRIVATE_DATA));
	if(simp911x == NULL) {
		SMSC_WARNING("Can't allocate net-device struct !");
		return -ENOMEM;
	}

	simp911x->init=smsc9115_init;
	result=register_netdev(simp911x);
	if(result) {
			SMSC_WARNING("error %i registering device",result);
			registered=0;
	} else {
		device_present=1;
		registered=1;
		SMSC_TRACE("  Interface Name = \"%s\"",simp911x->name);
			
	}
	
	
	SMSC_TRACE("<-- init_module()");
	return device_present ? 0 : -ENODEV;
}

//entry point for unloading the module
static void __exit smsc9115_cleanup_module(void)
{
	PPRIVATE_DATA privateData = netdev_priv(simp911x);
	
	SMSC_TRACE("--> cleanup_module()");

	mxc_free_iomux(privateData->gpioIrq,OUTPUTCONFIG_GPIO,INPUTCONFIG_GPIO); 

	if(registered) {
		unregister_netdev(simp911x);
		free_netdev(simp911x);
	}

#ifdef USE_DEBUG_PROC /* use proc only if debugging */
	smsc9115_remove_proc();
#endif

	
	SMSC_TRACE("<-- cleanup_module()");
}

//entry point for initializing the driver
int smsc9115_init(struct net_device *dev)
{
	DWORD dwIdRev=0UL;
	PPRIVATE_DATA privateData=NULL;
	BOOLEAN acquired_gpio=FALSE;
	int result=-ENODEV;


	SMSC_TRACE("-->smsc9115_init(dev=0x%08lX)",(DWORD)dev);

	if(dev==NULL) {
		SMSC_WARNING("smsc9115_init(dev==NULL)");
		result=-EFAULT;
		goto DONE;
	}

	SMSC_TRACE("Driver Parameters");
	if(mac_addr_hi16==0xFFFFFFFFUL) {
		SMSC_TRACE("  mac_addr_hi16    = 0x%08lX, will attempt to read from LAN911x",mac_addr_hi16);
		SMSC_TRACE("  mac_addr_lo32    = 0x%08lX, will attempt to read from LAN911x",mac_addr_lo32);
	} else {
		if(mac_addr_hi16&0xFFFF0000UL) {
			//The high word is reserved
			SMSC_WARNING("  mac_addr_hi16 = 0x%08lX, reserved bits are high.",
					   mac_addr_hi16);
			mac_addr_hi16&=0x0000FFFFUL;
			SMSC_WARNING("    reseting to mac_addr_hi16 = 0x%08lX",
					   mac_addr_hi16);
		}
		if(mac_addr_lo32&0x00000001UL) {
			//bit 0 is the I/G bit
			SMSC_WARNING("  mac_addr_lo32 = 0x%08lX, I/G bit is set.",
					   mac_addr_lo32);
			mac_addr_lo32&=0xFFFFFFFEUL;
			SMSC_WARNING("    reseting to mac_addr_lo32 = 0x%08lX",
					   mac_addr_lo32);			
		}
		SMSC_TRACE("  mac_addr_hi16    = 0x%08lX",
				 mac_addr_hi16);
		SMSC_TRACE("  mac_addr_lo32    = 0x%08lX",
				 mac_addr_lo32);
	}
	SMSC_TRACE(    "  debug_mode       = 0x%08lX",debug_mode);
	if(phy_addr==0xFFFFFFFFUL) {
		SMSC_TRACE("  phy_addr         = 0xFFFFFFFF, Use internal phy");
	} else if(phy_addr<=31) {
		SMSC_TRACE("  phy_addr         = 0x%08lX, use this address for external phy", phy_addr);
	} else {
		SMSC_TRACE("  phy_addr         = 0x%08lX, auto detect external phy", phy_addr);
	}

	privateData = netdev_priv(dev);
	memset(dev->priv,0,sizeof(PRIVATE_DATA));
	privateData->dwLanBase = SMSC_BASE_ADDRESS;

	if(privateData->dwLanBase==0UL) {
		SMSC_WARNING("lan_base==0x00000000");
		result=-ENODEV;
		goto DONE;
	}

	if(check_mem_region(privateData->dwLanBase,LAN_REGISTER_EXTENT)!=0) {
		SMSC_WARNING("  Memory Region specified (0x%08lX to 0x%08lX) is not available.",privateData->dwLanBase,privateData->dwLanBase+LAN_REGISTER_EXTENT-1);
		result=-ENOMEM;
		goto DONE;
	}

	privateData->gpioIrq = IOMUX_IRQ_NAME;
 	if(!(mxc_request_gpio(privateData->gpioIrq)==0)){
		SMSC_WARNING("  REQUEST_IOMUX FOR THAT GPIO NOT AVAILABLE");
		result=-ENODEV;
		goto DONE;
	}
	acquired_gpio=TRUE;

	SMSC_TRACE("GPIO %i PORT %i reserved",IOMUX_TO_GPIO(privateData->gpioIrq),GPIO_TO_PORT(IOMUX_TO_GPIO(privateData->gpioIrq)));

	dwIdRev=Lan_GetRegDW(ID_REV);
	if(HIWORD(dwIdRev)==LOWORD(dwIdRev)) {
		//this may mean the chip is set for 32 bit 
		//  while the bus is reading as 16 bit
	UNKNOWN_CHIP:
		SMSC_WARNING("LAN911x NOT Identified, dwIdRev==0x%08lX",dwIdRev);
		result=-ENODEV;
		goto DONE;
	}
	switch(dwIdRev&0xFFFF0000UL) {
	case 0x01180000UL:
		SMSC_TRACE("LAN9118 identified, dwIdRev==0x%08lX",dwIdRev);break;
	case 0x01170000UL:
		SMSC_TRACE("LAN9117 identified, dwIdRev==0x%08lX",dwIdRev);break;
	case 0x01160000UL:
		SMSC_TRACE("LAN9116 identified, dwIdRev==0x%08lX",dwIdRev);break;
	case 0x01150000UL:
		SMSC_TRACE("LAN9115 identified, dwIdRev==0x%08lX",dwIdRev);break;
	case 0x01120000UL:
		SMSC_TRACE("LAN9112 identified, dwIdRev==0x%08lX",dwIdRev);break;
	default:
		goto UNKNOWN_CHIP;
	}
	privateData->dwIdRev=dwIdRev;

	if((dwIdRev&0x0000FFFFUL)==0UL) {
		SMSC_WARNING("This driver is not intended for this chip revision");
	}

	//	ether_setup(dev);

	dev->open=				smsc9115_open;
	dev->stop=				smsc9115_stop;
	dev->hard_start_xmit=	     smsc9115_hard_start_xmit;
	dev->get_stats=			smsc9115_get_stats;
	dev->set_multicast_list=      smsc9115_set_multicast_list;
	dev->flags|=IFF_MULTICAST;

	SET_MODULE_OWNER(dev);

	privateData->dev=dev;

	result=0;
    
 DONE:
	if(result!=0) {
		//something went wrong, perform clean up
		if(dev!=NULL) {
			if(dev->priv!=NULL) {
				if (acquired_gpio) {
					mxc_free_iomux(privateData->gpioIrq,OUTPUTCONFIG_GPIO,INPUTCONFIG_GPIO); 
				}
				kfree(dev->priv);
				dev->priv=NULL;
			}
		}
	}

#ifdef USE_DEBUG_PROC /* only when debugging */
	spin_lock_init(&(privateData->LanAccessLock));
	smsc9115_create_proc();
#endif


	SMSC_TRACE("<--smsc9115_init(), result=%d",result);
	return result;
}

//entry point for starting/opening the interface
int smsc9115_open(struct net_device *dev)
{
	int result=-ENODEV;
	PPRIVATE_DATA privateData = netdev_priv(simp911x) ;
	BOOLEAN acquired_mem_region=FALSE;
	BOOLEAN acquired_isr=FALSE;
	
	SMSC_TRACE("-->smsc9115_open(dev=0x%08lX)",(DWORD)dev);
	if(dev==NULL) {
		SMSC_WARNING("smsc9115_open(dev==NULL)");
		result=-EFAULT;
		goto DONE;
	}

	if(privateData==NULL) {
		SMSC_WARNING("smsc9115_open(privateData==NULL)");
		result=-EFAULT;
		goto DONE;
	}

	//get memory region
	if(check_mem_region(privateData->dwLanBase,LAN_REGISTER_EXTENT)!=0)
		{
			SMSC_WARNING("Device memory is already in use.");
			result=-ENOMEM;
			goto DONE;
		}
	request_mem_region(privateData->dwLanBase, LAN_REGISTER_EXTENT,
                           DRV_NAME);
	acquired_mem_region=TRUE;

	//initialize the LAN911x
	{
		const DWORD dwIntCfg=(((DWORD)22UL)<<24);//set interrupt deassertion to 220uS
		//dwIntCfg|=INT_CFG_IRQ_POL_;  //use this line to set IRQ_POL bit
		//dwIntCfg|=INT_CFG_IRQ_TYPE_; //use this line to set IRQ_TYPE bit
		if(!Lan_Initialize(privateData,dwIntCfg)) {
			goto DONE;
		}
	}


	// REQUEST GPIO INTERRUPT
	// mxc_set_gpio_edge_ctrl(privateData->gpioIrq,GPIO_INT_FALL_EDGE);
	set_irq_type(IOMUX_TO_IRQ(privateData->gpioIrq), IRQT_FALLING);
	mxc_set_gpio_direction(privateData->gpioIrq,1);   //1 for input
	if(request_irq(IOMUX_TO_IRQ(privateData->gpioIrq),
				smsc9115_ISR,
				SA_INTERRUPT,
				DRV_NAME,
				privateData)!=0)
	{
		result=-ENODEV;
		SMSC_WARNING("Can't get IRQ line");
		goto DONE;
	}
	acquired_isr=TRUE;

	//must now test the IRQ connection to the ISR
	SMSC_TRACE("Testing ISR using IRQ %d", IOMUX_TO_IRQ(privateData->gpioIrq));
	{
		DWORD dwTimeOut=1000;
		privateData->RequestIrqDisable=FALSE;
		privateData->SoftwareInterruptSignal=FALSE;
		Lan_SetBitsDW(INT_EN,INT_EN_SW_INT_EN_);
		do {
			udelay(10);
			dwTimeOut--;
		} while((dwTimeOut)&&(!(privateData->SoftwareInterruptSignal)));
		if(!(privateData->SoftwareInterruptSignal)) {
			SMSC_WARNING("ISR failed signaling test");
			result=-ENODEV;
			goto DONE;
		}
	}
	SMSC_TRACE("ISR passed test using IRQ %d", IOMUX_TO_IRQ(privateData->gpioIrq));


	Mac_Initialize(privateData);
	{//get mac address
		DWORD dwHigh16=0;
		DWORD dwLow32=0;
		//Because this is part of the single threaded initialization
		//  path there is no need to acquire the MacPhyAccessLock
		if(mac_addr_hi16==0xFFFFFFFF) {
			dwHigh16=Mac_GetRegDW(privateData,ADDRH);
			dwLow32=Mac_GetRegDW(privateData,ADDRL);
			if((dwHigh16==0x0000FFFFUL)&&(dwLow32==0xFFFFFFFF)){
				dwHigh16=0x00000070UL;
				dwLow32=0x110F8000UL;
				Mac_SetRegDW(privateData,ADDRH,dwHigh16);
				Mac_SetRegDW(privateData,ADDRL,dwLow32);
				SMSC_TRACE("Mac Address is set by default to 0x%04lX%08lX",
						 dwHigh16,dwLow32);
			} else {
				SMSC_TRACE("Mac Address is read from LAN9115 as 0x%04lX%08lX",
						 dwHigh16,dwLow32);
			}
		} else {
			dwHigh16=mac_addr_hi16;
			dwLow32=mac_addr_lo32;
			Mac_SetRegDW(privateData,ADDRH,dwHigh16);
			Mac_SetRegDW(privateData,ADDRL,dwLow32);
			SMSC_TRACE("Mac Address is set by parameter to 0x%04lX%08lX",
					 dwHigh16,dwLow32);

		}
#ifdef CONFIG_SMSC9115_OLD_BOOTLOADER_HANDLING
		if(dwLow32 == 0x02030405 && dwHigh16 == 0x00000001){
		  dwLow32 = 0x03020100;
		  dwHigh16 = 0x00000504;
		  Mac_SetRegDW(privateData,ADDRH,dwHigh16);
		  Mac_SetRegDW(privateData,ADDRL,dwLow32);
		}
#endif
		dev->dev_addr[0]=LOBYTE(LOWORD(dwLow32));
		dev->dev_addr[1]=HIBYTE(LOWORD(dwLow32));
		dev->dev_addr[2]=LOBYTE(HIWORD(dwLow32));
		dev->dev_addr[3]=HIBYTE(HIWORD(dwLow32));
		dev->dev_addr[4]=LOBYTE(LOWORD(dwHigh16));
		dev->dev_addr[5]=HIBYTE(LOWORD(dwHigh16));
	}

	netif_carrier_off(dev);
	if(!Phy_Initialize( privateData , phy_addr))	{
		SMSC_WARNING("Failed to initialize Phy");
		result=-ENODEV;
		goto DONE;
	}
	
	Tx_Initialize(privateData);
	Rx_Initialize(privateData);
	
	netif_start_queue(dev);

	result=0;

 DONE:
	if(result!=0) {
		if((acquired_isr)&&(privateData!=NULL)) {
			free_irq(IOMUX_TO_IRQ(privateData->gpioIrq),privateData);
/* 			gpio_free_irq(GPIO_TO_PORT(IOMUX_TO_GPIO(privateData->gpioIrq)),  ,    ); */
		}
		if((acquired_mem_region)&&(privateData!=NULL)) {
			release_mem_region( privateData->dwLanBase, LAN_REGISTER_EXTENT);
		}
	}

	SMSC_TRACE("<--smsc9115_open, result=%d",result);
	return result;
}

//entry point for stopping the interface
int smsc9115_stop(struct net_device *dev)
{
	int result=0;
	PPRIVATE_DATA privateData = netdev_priv(dev);

	SMSC_TRACE("-->smsc9115_stop(dev=0x%08lX)",(DWORD)dev);
	if(dev==NULL) {
		SMSC_WARNING("smsc9115_stop(dev==NULL)");
		result=-EFAULT;
		goto DONE;
	}
	if(privateData==NULL) {
		SMSC_WARNING("smsc9115_stop(privateData==NULL)");
		result=-EFAULT;
		goto DONE;
	}

	privateData->StopLinkPolling=TRUE;
	del_timer_sync(&(privateData->LinkPollingTimer));

	{//Disable IRQ safely, by using the software interrupt
		// to do it in the ISR itself.
		DWORD dwTimeOut=10000;
		privateData->RequestIrqDisable=TRUE;
		privateData->SoftwareInterruptSignal=FALSE;
		Lan_SetBitsDW(INT_EN,INT_EN_SW_INT_EN_);
		do {
			udelay(10);
			dwTimeOut--;
		} while((dwTimeOut)&&(!(privateData->SoftwareInterruptSignal)));
		if(!(privateData->SoftwareInterruptSignal)) {
			SMSC_WARNING("ISR Disable failed signal test,irq number=%d",IOMUX_TO_IRQ(privateData->gpioIrq));
			//disable IRQ anyway
			Lan_ClrBitsDW(INT_CFG,INT_CFG_IRQ_EN_);
		}
	}

	spin_lock(&dev->xmit_lock);
	netif_stop_queue(privateData->dev);
	spin_unlock(&dev->xmit_lock);

	//at this point all Rx and Tx activity is stopped

	privateData->stats.rx_dropped+=Lan_GetRegDW(RX_DROP);
	Tx_UpdateTxCounters(privateData);

#ifndef LINUX_2_6_OR_NEWER
	MOD_DEC_USE_COUNT;
#endif

	free_irq(IOMUX_TO_IRQ(privateData->gpioIrq),privateData);

	release_mem_region(privateData->dwLanBase,LAN_REGISTER_EXTENT);
	{//preserve important items, clear the rest
		DWORD dwLanBase=privateData->dwLanBase;
		DWORD dwIdRev=privateData->dwIdRev;
		//    DWORD dwInstanceIndex=privateData->dwInstanceIndex;
		struct net_device *tempDev=privateData->dev;
		struct net_device_stats tempStats;
		memcpy(&tempStats,&(privateData->stats),sizeof(struct net_device_stats));

		memset(privateData,0,sizeof(PRIVATE_DATA));

		memcpy(&(privateData->stats),&tempStats,sizeof(struct net_device_stats));
		privateData->dwLanBase=dwLanBase;
		privateData->gpioIrq=IOMUX_IRQ_NAME;
		privateData->dev=tempDev;
		privateData->dwIdRev=dwIdRev;
	}

 DONE:

	SMSC_TRACE("<--smsc9115_stop, result=%d",result);
	return result;
}

//entry point for transmitting a packet
int smsc9115_hard_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	int result=0;
	PPRIVATE_DATA privateData = netdev_priv(dev);
	if(skb==NULL) {
		SMSC_WARNING("smsc9115_hard_start_xmit(skb==NULL)");
		result=-EFAULT;
		goto DONE;
	}
	if(dev==NULL) {
		SMSC_WARNING("smsc9115_hard_start_xmit(dev==NULL)");
		result=-EFAULT;
		goto DONE;
	}
	if(dev->priv==NULL) {
		SMSC_WARNING("smsc9115_hard_start_xmit(dev->priv==NULL)");
		result=-EFAULT;
		goto DONE;
	}

	Tx_SendSkb(privateData,skb);

 DONE:
	return result;
}

//entry point for getting status counters
struct net_device_stats * smsc9115_get_stats(struct net_device *dev)
{
	PPRIVATE_DATA privateData = netdev_priv(dev)  ; 
	if(dev==NULL) {
		SMSC_WARNING("smsc9115_get_stats(dev==NULL)");
		return NULL;
	}
	if(dev->priv==NULL) {
		SMSC_WARNING("smsc9115_get_stats(dev->priv==NULL)");
		return NULL;
	}

	return &(privateData->stats);
}

//entry point for setting addressing modes
void smsc9115_set_multicast_list(struct net_device *dev)
{
	SMSC_ASSERT(dev!=NULL);
	Rx_SetMulticastList(dev);
}

//primary entry point for ISR
irqreturn_t smsc9115_ISR(int Irq, void *dev_id, struct pt_regs *regs)
{
	DWORD dwIntCfg=0;
	DWORD dwIntSts=0;
	PPRIVATE_DATA privateData=(PPRIVATE_DATA)dev_id;
	BOOLEAN serviced=FALSE;
	//	DWORD dummyRead;
	if(privateData==NULL) {
		SMSC_WARNING("smsc9115_ISR(privateData==NULL)");
		goto DONE;
	}
	if(privateData->dwLanBase==0) {
		SMSC_WARNING("smsc9115_ISR(dwLanBase==0)");
		goto DONE;
	}

	dwIntCfg=Lan_GetRegDW(INT_CFG);
	if((dwIntCfg&0x00001100)!=0x00001100) {
		SMSC_TRACE("In ISR, not my interrupt, dwIntCfg=0x%08lX",
				 dwIntCfg);
		goto DONE;
	}

	{
		DWORD reservedBits=0x00FFCEEEUL;
		if(dwIntCfg&reservedBits) {
			SMSC_WARNING("In ISR, reserved bits are high.");
			//this could mean surprise removal
			goto DONE;
		}
	}

	dwIntSts=Lan_GetRegDW(INT_STS);
	if(dwIntSts&INT_STS_SW_INT_) {
		Lan_ClrBitsDW(INT_EN,INT_EN_SW_INT_EN_);
		Lan_SetRegDW(INT_STS,INT_STS_SW_INT_);
		privateData->SoftwareInterruptSignal=TRUE;
		serviced=TRUE;
		if(privateData->RequestIrqDisable) {
			Lan_ClrBitsDW(INT_CFG,INT_CFG_IRQ_EN_);
			dwIntSts=0;//prevent any pending interrupts from being handled.
		}
	}
	if(Tx_HandleInterrupt(privateData,dwIntSts)) {
		serviced=TRUE;
	}
	if(Rx_HandleInterrupt(privateData,dwIntSts)) {
		serviced=TRUE;
	}
	if(!serviced) {
		SMSC_WARNING("unserviced interrupt dwIntCfg=0x%08lX,dwIntSts=0x%08lX,INT_EN=0x%08lX",
				   dwIntCfg,dwIntSts,Lan_GetRegDW(INT_EN));
	}
 DONE:
	return IRQ_RETVAL(serviced);
}

#ifdef USE_PHY_WORK_AROUND
BOOLEAN Phy_Reset(PPRIVATE_DATA privateData)
{
	BOOLEAN result=FALSE;
	WORD wTemp=0;
	DWORD dwLoopCount=100000;
	SMSC_TRACE("Performing PHY BCR Reset");
	Phy_SetRegW(privateData,PHY_BCR,PHY_BCR_RESET_);
	do {
		udelay(10);
		wTemp=Phy_GetRegW(privateData,PHY_BCR);
		dwLoopCount--;
	} while((dwLoopCount>0)&&(wTemp&PHY_BCR_RESET_));
	if(wTemp&PHY_BCR_RESET_) {
		SMSC_WARNING("Phy Reset failed to complete.");
		goto DONE;
	}
	//extra delay required because the phy may not be completed with its reset
	//  when PHY_BCR_RESET_ is cleared.
	//  They say 256 uS is enough delay but I'm using 500 here to be safe
	udelay(500);
	result=TRUE;
 DONE:
	return result;
}

DWORD Phy_LBT_GetTxStatus(PPRIVATE_DATA privateData)
{
	DWORD result=Lan_GetRegDW(TX_FIFO_INF);
	result&=TX_FIFO_INF_TSUSED_;
	if(result!=0x00000000UL) {
		result=Lan_GetRegDW(TX_STATUS_FIFO);
	} else {
		result=0;
	}
	return result;
}

DWORD Phy_LBT_GetRxStatus(PPRIVATE_DATA privateData)
{
	DWORD result=Lan_GetRegDW(RX_FIFO_INF);
	if(result&0x00FF0000UL) {
		//Rx status is available, read it
		result=Lan_GetRegDW(RX_STATUS_FIFO);
	} else {
		result=0;
	}
	return result;
}

BOOLEAN Phy_CheckLoopBackPacket(PPRIVATE_DATA privateData)
{
	BOOLEAN result=FALSE;
	DWORD tryCount=0;
	DWORD dwLoopCount=0;
	for(tryCount=0;tryCount<10;tryCount++)
		{
			DWORD dwTxCmdA=0;
			DWORD dwTxCmdB=0;
			DWORD dwStatus=0;
			DWORD dwPacketLength=0;

			//zero-out Rx Packet memory
			memset(privateData->LoopBackRxPacket,0,MIN_PACKET_SIZE);

			//write Tx Packet to 118
			dwTxCmdA=
				((((DWORD)(privateData->LoopBackTxPacket))&0x03UL)<<16) | //DWORD alignment adjustment
				TX_CMD_A_FIRST_SEG_ | TX_CMD_A_LAST_SEG_ |
				((DWORD)(MIN_PACKET_SIZE));
			dwTxCmdB=
				(((DWORD)(MIN_PACKET_SIZE))<<16) |
				((DWORD)(MIN_PACKET_SIZE));


			Lan_SetRegDW(TX_DATA_FIFO,dwTxCmdA);
			Lan_SetRegDW(TX_DATA_FIFO,dwTxCmdB);

			Tx_WriteFifo( privateData->dwLanBase,
					   (DWORD *)(((DWORD)(privateData->LoopBackTxPacket))&0xFFFFFFFCUL),
					   (((DWORD)(MIN_PACKET_SIZE))+3+
					    (((DWORD)(privateData->LoopBackTxPacket))&0x03UL))>>2);

			//wait till transmit is done
			dwLoopCount=60;
			while((dwLoopCount>0)&&((dwStatus=Phy_LBT_GetTxStatus(privateData))==0)) {
				udelay(5);
				dwLoopCount--;
			}
			if(dwStatus==0) {
				SMSC_WARNING("Failed to Transmit during Loop Back Test");
				continue;
			}
			if(dwStatus&0x00008000UL) {
				SMSC_WARNING("Transmit encountered errors during Loop Back Test");
				continue;
			}

			//wait till receive is done
			dwLoopCount=60;
			while((dwLoopCount>0)&&((dwStatus=Phy_LBT_GetRxStatus(privateData))==0))
				{
					udelay(5);
					dwLoopCount--;
				}
			if(dwStatus==0) {
				SMSC_WARNING("Failed to Receive during Loop Back Test");
				continue;
			}
			if(dwStatus&RX_STS_ES_)
				{
					SMSC_WARNING("Receive encountered errors during Loop Back Test");
					continue;
				}

			dwPacketLength=((dwStatus&0x3FFF0000UL)>>16);

			Rx_ReadFifo(
					  privateData->dwLanBase,
					  ((DWORD *)(privateData->LoopBackRxPacket)),
					  (dwPacketLength+3+(((DWORD)(privateData->LoopBackRxPacket))&0x03UL))>>2);

			if(dwPacketLength!=(MIN_PACKET_SIZE+4)) {
				SMSC_WARNING("Unexpected packet size during loop back test, size=%ld, will retry",dwPacketLength);
			} else {
				DWORD byteIndex=0;
				BOOLEAN foundMissMatch=FALSE;
				for(byteIndex=0;byteIndex<MIN_PACKET_SIZE;byteIndex++) {
					if(privateData->LoopBackTxPacket[byteIndex]!=privateData->LoopBackRxPacket[byteIndex])
						{
							foundMissMatch=TRUE;
							break;
						}
				}
				if(!foundMissMatch) {
					SMSC_TRACE("Successfully Verified Loop Back Packet");
					result=TRUE;
					goto DONE;
				} else {
					SMSC_WARNING("Data miss match during loop back test, will retry.");
				}
			}
		}
 DONE:
	return result;
}

BOOLEAN Phy_LoopBackTest(PPRIVATE_DATA privateData)
{
	BOOLEAN result=FALSE;
	DWORD byteIndex=0;
	DWORD tryCount=0;

	//Initialize Tx Packet
	for(byteIndex=0;byteIndex<6;byteIndex++) {
		//use broadcast destination address
		privateData->LoopBackTxPacket[byteIndex]=(BYTE)0xFF;
	}
	for(byteIndex=6;byteIndex<12;byteIndex++) {
		//use incrementing source address
		privateData->LoopBackTxPacket[byteIndex]=(BYTE)byteIndex;
	}
	//Set length type field
	privateData->LoopBackTxPacket[12]=0x00;
	privateData->LoopBackTxPacket[13]=0x00;
	for(byteIndex=14;byteIndex<MIN_PACKET_SIZE;byteIndex++)
		{
			privateData->LoopBackTxPacket[byteIndex]=(BYTE)byteIndex;
		}
	{
		DWORD dwRegVal=Lan_GetRegDW(HW_CFG);
		dwRegVal&=HW_CFG_TX_FIF_SZ_;
		dwRegVal|=HW_CFG_SF_;
		Lan_SetRegDW(HW_CFG,dwRegVal);
	}
	Lan_SetRegDW(TX_CFG,TX_CFG_TX_ON_);
	Lan_SetRegDW(RX_CFG,(((DWORD)(privateData->LoopBackRxPacket))&0x03)<<8);

	//Set Phy to 10/FD, no ANEG,
	Phy_SetRegW(privateData,PHY_BCR,0x0100);

	//enable MAC Tx/Rx, FD
	Mac_SetRegDW(privateData,MAC_CR,MAC_CR_FDPX_|MAC_CR_TXEN_|MAC_CR_RXEN_);

	//set Phy to loopback mode
	Phy_SetRegW(privateData,PHY_BCR,0x4100);
	
	for(tryCount=0;tryCount<10;tryCount++) {
		if(Phy_CheckLoopBackPacket(privateData))
			{
				result=TRUE;
				goto DONE;
			}
		privateData->dwResetCount++;
		//disable MAC rx
		Mac_SetRegDW(privateData,MAC_CR,0UL);

		Phy_Reset(privateData);

		//Set Phy to 10/FD, no ANEG, and Loopbackmode
		Phy_SetRegW(privateData,PHY_BCR,0x4100);

		//enable MAC Tx/Rx, FD
		Mac_SetRegDW(privateData,MAC_CR,MAC_CR_FDPX_|MAC_CR_TXEN_|MAC_CR_RXEN_);
	}
 DONE:
	//disable MAC
	Mac_SetRegDW(privateData,MAC_CR,0UL);
	//Cancel Phy loopback mode
	Phy_SetRegW(privateData,PHY_BCR,0U);

	Lan_SetRegDW(TX_CFG,0UL);
	Lan_SetRegDW(RX_CFG,0UL);

	return result;
}

#endif //USE_PHY_WORK_AROUND

//Sets the link mode
void Phy_SetLink(PPRIVATE_DATA privateData) 
{

	WORD wTemp;
	DWORD dwLoopCount;

	switch(auto_neg) {
		
	case 0:
		//We set directly the special mode register to 100FD
		wTemp = 0x0000;
		if (fullduplex==1)
			wTemp |= PHY_BCR_DUPLEX_MODE_;
		if (speed==100)
			wTemp |= PHY_BCR_SPEED_SELECT_ ;
		Phy_SetRegW(privateData,PHY_BCR,wTemp );
		break;
	case 1:
		//Because this is part of the single threaded initialization
		//  path there is no need to acquire the MacPhyAccessLock
		wTemp=Phy_GetRegW(privateData,
					   PHY_ANEG_ADV);
		//Advertise all speeds and pause capabilities
		wTemp|=(PHY_ANEG_ADV_PAUSE_|PHY_ANEG_ADV_SPEED_);
		Phy_SetRegW(privateData,PHY_ANEG_ADV,wTemp);
		
		// begin to establish link
		Phy_SetRegW(privateData,
				  PHY_BCR,
				  PHY_BCR_AUTO_NEG_ENABLE_|
				  PHY_BCR_RESTART_AUTO_NEG_);
		break;
	    
	case 2:
		SMSC_TRACE("Starting Auto-Negociation");
		Phy_SetRegW(privateData,PHY_ANEG_ADV,0x1e1);
		
		// begin to establish link
		Phy_SetRegW(privateData, PHY_BCR, PHY_BCR_AUTO_NEG_ENABLE_ | PHY_BCR_RESTART_AUTO_NEG_);
		dwLoopCount = 200000;
		do {
			udelay(10);
			wTemp=Phy_GetRegW(privateData,PHY_BSR);
			dwLoopCount--;
		} while((dwLoopCount>0)&&(!(wTemp&PHY_BSR_AUTO_NEG_COMP_)));
		if(!(wTemp&PHY_BSR_AUTO_NEG_COMP_)){
			
			SMSC_WARNING("Auto Negociate failed to complete");
			SMSC_WARNING("PHY_BCR REGISTER = 0x%x",Phy_GetRegW(privateData,PHY_BCR));
			SMSC_WARNING("PHY_BSR REGISTER = 0x%x",Phy_GetRegW(privateData,PHY_BSR));
		}
		else {
			SMSC_TRACE("Auto Negociation Completed");
			//TODO set full duplex in function of PHY_BSR status
		}

		break;
		
 	default:
		break;
	}

}

//Initializes the phy
BOOLEAN Phy_Initialize( PPRIVATE_DATA privateData, DWORD dwPhyAddr)
{
	BOOLEAN result=FALSE;
#ifndef USE_PHY_WORK_AROUND
	WORD wTemp=0;
#endif
	WORD wPhyId1=0;
	WORD wPhyId2=0;
#ifndef USE_PHY_WORK_AROUND
	DWORD dwLoopCount=0;
#endif

	SMSC_TRACE("-->Phy_Initialize");
	SMSC_ASSERT(privateData!=NULL);
	SMSC_ASSERT(privateData->dwLanBase!=0);

	if(dwPhyAddr!=0xFFFFFFFFUL) {
		switch(privateData->dwIdRev&0xFFFF0000) {
		case 0x01170000UL:
		case 0x01150000UL:
			{
				DWORD dwHwCfg=Lan_GetRegDW(HW_CFG);
				if(dwHwCfg&HW_CFG_EXT_PHY_DET_) {
					//External phy is requested, supported, and detected
					//Attempt to switch
					//NOTE: Assuming Rx and Tx are stopped
					//  because Phy_Initialize is called before 
					//  Rx_Initialize and Tx_Initialize

					//Disable phy clocks to the mac
					dwHwCfg&= (~HW_CFG_PHY_CLK_SEL_);
					dwHwCfg|= HW_CFG_PHY_CLK_SEL_CLK_DIS_;
					Lan_SetRegDW(HW_CFG,dwHwCfg);
					udelay(10);//wait for clocks to acutally stop

					//switch to external phy
					dwHwCfg|=HW_CFG_EXT_PHY_EN_;
					Lan_SetRegDW(HW_CFG,dwHwCfg);

					//Enable phy clocks to the mac
					dwHwCfg&= (~HW_CFG_PHY_CLK_SEL_);
					dwHwCfg|= HW_CFG_PHY_CLK_SEL_EXT_PHY_;
					Lan_SetRegDW(HW_CFG,dwHwCfg);
					udelay(10);//wait for clocks to actually start

					dwHwCfg|=HW_CFG_SMI_SEL_;
					Lan_SetRegDW(HW_CFG,dwHwCfg);

					//Because this is part of the single threaded initialization
					//  path there is no need to acquire the MacPhyAccessLock
					if(dwPhyAddr<=31) {
						//only check the phy address specified
						privateData->dwPhyAddress=dwPhyAddr;
						wPhyId1=Phy_GetRegW(privateData,PHY_ID_1);
						wPhyId2=Phy_GetRegW(privateData,PHY_ID_2);
					} else {
						//auto detect phy
						DWORD address=0;
						for(address=0;address<=31;address++) {
							privateData->dwPhyAddress=address;
							wPhyId1=Phy_GetRegW(privateData,PHY_ID_1);
							wPhyId2=Phy_GetRegW(privateData,PHY_ID_2);
							if((wPhyId1!=0xFFFFU)||(wPhyId2!=0xFFFFU)) {
								SMSC_TRACE("Detected Phy at address = 0x%02lX = %ld",
										 address,address);
								break;
							}
						}
						if(address>=32) {
							SMSC_WARNING("Failed to auto detect external phy");
						}
					}
					if((wPhyId1==0xFFFFU)&&(wPhyId2==0xFFFFU)) {
						SMSC_WARNING("External Phy is not accessable");
						SMSC_WARNING("  using internal phy instead");
						//revert back to interal phy settings.

						//Disable phy clocks to the mac
						dwHwCfg&= (~HW_CFG_PHY_CLK_SEL_);
						dwHwCfg|= HW_CFG_PHY_CLK_SEL_CLK_DIS_;
						Lan_SetRegDW(HW_CFG,dwHwCfg);
						udelay(10);//wait for clocks to actually stop

						//switch to internal phy
						dwHwCfg&=(~HW_CFG_EXT_PHY_EN_);
						Lan_SetRegDW(HW_CFG,dwHwCfg);
	
						//Enable phy clocks to the mac
						dwHwCfg&= (~HW_CFG_PHY_CLK_SEL_);
						dwHwCfg|= HW_CFG_PHY_CLK_SEL_INT_PHY_;
						Lan_SetRegDW(HW_CFG,dwHwCfg);
						udelay(10);//wait for clocks to actually start

						dwHwCfg&=(~HW_CFG_SMI_SEL_);
						Lan_SetRegDW(HW_CFG,dwHwCfg);
						goto USE_INTERNAL_PHY;
					} else {
						SMSC_TRACE("Successfully switched to external phy");
#ifdef USE_LED1_WORK_AROUND
						privateData->NotUsingExtPhy=0;
#endif
					}
				} else {
					SMSC_WARNING("No External Phy Detected");
					SMSC_WARNING("  using internal phy instead");
					goto USE_INTERNAL_PHY;
				}
			};break;
		default:
			SMSC_WARNING("External Phy is not supported");
			SMSC_WARNING("  using internal phy instead");
			goto USE_INTERNAL_PHY;
		}
	} else {
	USE_INTERNAL_PHY:
		privateData->dwPhyAddress=1;
#ifdef USE_LED1_WORK_AROUND
		privateData->NotUsingExtPhy=1;
#endif

	}
    
	//Because this is part of the single threaded initialization
	//  path there is no need to acquire the MacPhyAccessLock
	wPhyId1=Phy_GetRegW(privateData,PHY_ID_1); 
	wPhyId2=Phy_GetRegW(privateData,PHY_ID_2);
	if((wPhyId1==0xFFFFU)&&(wPhyId2==0xFFFFU)) {
		SMSC_WARNING("Phy Not detected");
		goto DONE;
	}

	privateData->dwLinkSpeed=LINK_OFF;
	privateData->dwLinkSettings=LINK_OFF;
	
	//reset the PHY
#ifdef USE_PHY_WORK_AROUND
	Phy_Reset(privateData);
	if(!Phy_LoopBackTest(privateData)) {
		SMSC_WARNING("Failed Loop Back Test");
		goto DONE;
	} else {
		SMSC_TRACE("Passed Loop Back Test");
	}
#else

	Phy_SetRegW(privateData,PHY_BCR,PHY_BCR_RESET_);
	dwLoopCount=100000;
	do {
		udelay(10);
		wTemp=Phy_GetRegW(privateData,PHY_BCR);
		dwLoopCount--;
	} while((dwLoopCount>0) && (wTemp&PHY_BCR_RESET_));
	if(wTemp&PHY_BCR_RESET_) {
		SMSC_WARNING("PHY reset failed to complete.");
		goto DONE;
	}
#endif //not USE_PHY_WORK_AROUND
	
	Phy_SetLink(privateData);

	init_timer(&(privateData->LinkPollingTimer));
	privateData->LinkPollingTimer.function=Phy_CheckLink;
	privateData->LinkPollingTimer.data=(unsigned long)privateData;
	privateData->LinkPollingTimer.expires=jiffies+HZ;
	add_timer(&(privateData->LinkPollingTimer));

	result=TRUE;
 DONE:
	SMSC_TRACE("<--Phy_Initialize, result=%s",result?"TRUE":"FALSE");
	return result;
}




DWORD Lan_GetRegDW(DWORD dwOffset)
{
	volatile DWORD reg = 0x0UL;

#ifdef USE_DEBUG_PROC
	PPRIVATE_DATA privateData = netdev_priv(simp911x) ;
	spin_lock(&(privateData->LanAccessLock));
#endif

	reg =  (*(volatile DWORD *)(SMSC_BASE_ADDRESS+ dwOffset ));

#ifdef USE_DEBUG_PROC
	spin_unlock(&(privateData->LanAccessLock));
#endif


	return reg;
}

void Lan_SetRegDW(DWORD dwOffset,DWORD dwVal)
{

#ifdef USE_DEBUG_PROC
	PPRIVATE_DATA privateData = netdev_priv(simp911x) ;
	spin_lock(&(privateData->LanAccessLock));
#endif

 	(*(volatile DWORD *)( SMSC_BASE_ADDRESS + dwOffset )) = dwVal;

#ifdef USE_DEBUG_PROC
	spin_unlock(&(privateData->LanAccessLock));
#endif
}

void Lan_ClrBitsDW(DWORD dwOffset,DWORD dwBits)
{
	DWORD dwReg = 0x0;

#ifdef USE_DEBUG_PROC
	PPRIVATE_DATA privateData = netdev_priv(simp911x) ;
	spin_lock(&(privateData->LanAccessLock));
#endif

	dwReg = (DWORD) ( Lan_GetRegDW(dwOffset) & ~dwBits );

	(*(volatile DWORD *)(SMSC_BASE_ADDRESS + dwOffset)) = dwReg;

#ifdef USE_DEBUG_PROC
	spin_unlock(&(privateData->LanAccessLock));
#endif


}

void Lan_SetBitsDW(DWORD dwOffset,DWORD dwBits)
{
	DWORD dwReg = 0x0;

#ifdef USE_DEBUG_PROC
	PPRIVATE_DATA privateData = netdev_priv(simp911x) ;
	spin_lock(&(privateData->LanAccessLock));
#endif

	dwReg = (DWORD) ( Lan_GetRegDW(dwOffset) |  dwBits );

	(*(volatile DWORD *)(SMSC_BASE_ADDRESS + dwOffset)) = dwReg ;	

#ifdef USE_DEBUG_PROC
	spin_unlock(&(privateData->LanAccessLock));
#endif

}


//Gets a phy register
WORD Phy_GetRegW( PPRIVATE_DATA privateData, DWORD dwRegIndex)
{
	DWORD dwAddr=0;
	int i=0;
	WORD result=0xFFFFU;

	SMSC_ASSERT(privateData!=NULL);

	// confirm MII not busy
	if ((Mac_GetRegDW(privateData, MII_ACC) & MII_ACC_MII_BUSY_) != 0UL)
		{
			SMSC_WARNING("MII is busy in Phy_GetRegW???");
			result=0;
			goto DONE;
		}

	// set the address, index & direction (read from PHY)
	dwAddr = (((privateData->dwPhyAddress) & 0x1FUL)<<11) | ((dwRegIndex & 0x1FUL)<<6);
	Mac_SetRegDW(privateData, MII_ACC, dwAddr);
	
	// wait for read to complete w/ timeout
	for(i=0;i<100;i++) {
		// see if MII is finished yet
		if ((Mac_GetRegDW(privateData, MII_ACC) & MII_ACC_MII_BUSY_) == 0UL)
			{
				// get the read data from the MAC & return i
				result=((WORD)Mac_GetRegDW(privateData, MII_DATA));
				goto DONE;
			}
	}
	SMSC_WARNING("timeout waiting for MII write to finish");
	
 DONE:
	return result;
}

//Sets a phy register
void Phy_SetRegW( PPRIVATE_DATA privateData,  DWORD dwRegIndex,WORD wVal)
{
	DWORD dwAddr=0;
	int i=0;

	SMSC_ASSERT(privateData!=NULL);

	// confirm MII not busy
	if ((Mac_GetRegDW(privateData, MII_ACC) & MII_ACC_MII_BUSY_) != 0UL)
		{
			SMSC_WARNING("MII is busy in Phy_SetRegW???");
			goto DONE;
		}

	// put the data to write in the MAC
	Mac_SetRegDW(privateData, MII_DATA, (DWORD)wVal);

	// set the address, index & direction (write to PHY)
	dwAddr = (((privateData->dwPhyAddress) & 0x1FUL)<<11) | ((dwRegIndex & 0x1FUL)<<6) | MII_ACC_MII_WRITE_;
	Mac_SetRegDW(privateData, MII_ACC, dwAddr);

	// wait for write to complete w/ timeout
	for(i=0;i<100;i++) {
		// see if MII is finished yet
		if ((Mac_GetRegDW(privateData, MII_ACC) & MII_ACC_MII_BUSY_) == 0UL)
			{
				goto DONE;
			}
	}
	SMSC_WARNING("timeout waiting for MII write to finish");
 DONE:
	return;
}

//Update link mode if any thing has changed
void Phy_UpdateLinkMode(PPRIVATE_DATA privateData)
{
	DWORD dwOldLinkSpeed=privateData->dwLinkSpeed;

	spin_lock(&(privateData->MacPhyAccessLock));

	Phy_GetLinkMode(privateData);

	if(dwOldLinkSpeed!=(privateData->dwLinkSpeed)) {
		if(privateData->dwLinkSpeed!=LINK_OFF) {
			DWORD dwRegVal=0;
			switch(privateData->dwLinkSpeed) {
			case LINK_SPEED_10HD:
				SMSC_TRACE("Link is now UP at 10Mbps HD");
				break;
			case LINK_SPEED_10FD:
				SMSC_TRACE("Link is now UP at 10Mbps FD");
				break;
			case LINK_SPEED_100HD:
				SMSC_TRACE("Link is now UP at 100Mbps HD");
				break;
			case LINK_SPEED_100FD:
				SMSC_TRACE("Link is now UP at 100Mbps FD");
				break;
			default:
				SMSC_WARNING("Link is now UP at Unknown Link Speed, dwLinkSpeed=0x%08lX",
						   privateData->dwLinkSpeed);
				break;
			}
		
			dwRegVal=Mac_GetRegDW(privateData,MAC_CR);
			dwRegVal&=~(MAC_CR_FDPX_|MAC_CR_RCVOWN_);
			switch(privateData->dwLinkSpeed) {
			case LINK_SPEED_10HD:
			case LINK_SPEED_100HD:
				dwRegVal|=MAC_CR_RCVOWN_;
				break;
			case LINK_SPEED_10FD:
			case LINK_SPEED_100FD:
				dwRegVal|=MAC_CR_FDPX_;
				break;
			default:
				SMSC_WARNING("Unknown Link Speed, dwLinkSpeed=0x%08lX",
						   privateData->dwLinkSpeed);
				break;
			}

			Mac_SetRegDW(privateData,
					   MAC_CR,dwRegVal);

			if(privateData->dwLinkSettings&LINK_AUTO_NEGOTIATE) {
				WORD linkPartner=0;
				WORD localLink=0;
				localLink=Phy_GetRegW(privateData,4);
				linkPartner=Phy_GetRegW(privateData,5);
				switch(privateData->dwLinkSpeed) {
				case LINK_SPEED_10FD:
				case LINK_SPEED_100FD:
					if(((localLink&linkPartner)&((WORD)0x0400U)) != ((WORD)0U)) {
						//Enable PAUSE receive and transmit
						Mac_SetRegDW(privateData,FLOW,0xFFFF0002UL);
						Lan_SetBitsDW(AFC_CFG,0x0000000FUL);
					} else if(((localLink&((WORD)0x0C00U))==((WORD)0x0C00U)) &&
							((linkPartner&((WORD)0x0C00U))==((WORD)0x0800U)))
						{
							//Enable PAUSE receive, disable PAUSE transmit
							Mac_SetRegDW(privateData,FLOW,0xFFFF0002UL);
							Lan_ClrBitsDW(AFC_CFG,0x0000000FUL);
						} else {
							//Disable PAUSE receive and transmit
							Mac_SetRegDW(privateData,FLOW,0UL);
							Lan_ClrBitsDW(AFC_CFG,0x0000000FUL);
						};break;
				case LINK_SPEED_10HD:
				case LINK_SPEED_100HD:
					Mac_SetRegDW(privateData,FLOW,0UL);
					Lan_SetBitsDW(AFC_CFG,0x0000000FUL);
					break;
				default:
					SMSC_WARNING("Unknown Link Speed, dwLinkSpeed=0x%08lX\n",privateData->dwLinkSpeed);
					break;
				}
				SMSC_TRACE("LAN911x: %s,%s,%s,%s,%s,%s",
						 (localLink&PHY_ANEG_ADV_ASYMP_)?"ASYMP":"     ",
						 (localLink&PHY_ANEG_ADV_SYMP_)?"SYMP ":"     ",
						 (localLink&PHY_ANEG_ADV_100F_)?"100FD":"     ",
						 (localLink&PHY_ANEG_ADV_100H_)?"100HD":"     ",
						 (localLink&PHY_ANEG_ADV_10F_)?"10FD ":"     ",
						 (localLink&PHY_ANEG_ADV_10H_)?"10HD ":"     ");
	
				SMSC_TRACE("Partner: %s,%s,%s,%s,%s,%s",
						 (linkPartner&PHY_ANEG_LPA_ASYMP_)?"ASYMP":"     ",
						 (linkPartner&PHY_ANEG_LPA_SYMP_)?"SYMP ":"     ",
						 (linkPartner&PHY_ANEG_LPA_100FDX_)?"100FD":"     ",
						 (linkPartner&PHY_ANEG_LPA_100HDX_)?"100HD":"     ",
						 (linkPartner&PHY_ANEG_LPA_10FDX_)?"10FD ":"     ",
						 (linkPartner&PHY_ANEG_LPA_10HDX_)?"10HD ":"     ");
			} else {
				switch(privateData->dwLinkSpeed) {
				case LINK_SPEED_10HD:
				case LINK_SPEED_100HD:
					Mac_SetRegDW(privateData,FLOW,0x0UL);
					Lan_SetBitsDW(AFC_CFG,0x0000000FUL);
					break;
				default:
					Mac_SetRegDW(privateData,FLOW,0x0UL);
					Lan_ClrBitsDW(AFC_CFG,0x0000000FUL);
					break;
				}
			}
			netif_carrier_on(privateData->dev);
#ifdef USE_LED1_WORK_AROUND
			if ((g_GpioSettingOriginal & GPIO_CFG_LED1_EN_) &&
			    privateData->NotUsingExtPhy)
				{
					// Restore orginal GPIO configuration
					g_GpioSetting = g_GpioSettingOriginal;
					Lan_SetRegDW(GPIO_CFG,g_GpioSetting);
				}
#endif // USE_LED1_WORK_AROUND
		} else {
			SMSC_TRACE("Link is now DOWN");
			netif_carrier_off(privateData->dev);
			Mac_SetRegDW(privateData,FLOW,0UL);
			Lan_ClrBitsDW(AFC_CFG,0x0000000FUL);
#ifdef USE_LED1_WORK_AROUND
			// Check global setting that LED1 usage is 10/100 indicator
			g_GpioSetting = Lan_GetRegDW(GPIO_CFG);
			if ((g_GpioSetting & GPIO_CFG_LED1_EN_) &&
			    privateData->NotUsingExtPhy)
				{
					//Force 10/100 LED off, after saving orginal GPIO configuration
					g_GpioSettingOriginal = g_GpioSetting;

					g_GpioSetting &= ~GPIO_CFG_LED1_EN_;
					g_GpioSetting |=
						(GPIO_CFG_GPIOBUF0_|GPIO_CFG_GPIODIR0_|GPIO_CFG_GPIOD0_);
					Lan_SetRegDW(GPIO_CFG,g_GpioSetting);
				}
#endif // USE_LED1_WORK_AROUND
		}
	}
	spin_unlock(&(privateData->MacPhyAccessLock));
}

//entry point for the link poller
void Phy_CheckLink(unsigned long ptr)
{
	PPRIVATE_DATA privateData=(PPRIVATE_DATA)ptr;
	if(privateData==NULL) {
		SMSC_WARNING("Phy_CheckLink(ptr==0)");
		return;
	}

	//must call this twice
	Phy_UpdateLinkMode(privateData);
	Phy_UpdateLinkMode(privateData);

	if(!(privateData->StopLinkPolling)) {
		privateData->LinkPollingTimer.expires=jiffies+HZ;
		add_timer(&(privateData->LinkPollingTimer));
	}
}

//gets the current link mode
void Phy_GetLinkMode( PPRIVATE_DATA privateData)
{
	DWORD result=LINK_OFF;
	WORD wRegVal=0;
	WORD wRegBSR=0;

	//Assuming MacPhyAccessLock has already been acquired

	wRegBSR=Phy_GetRegW(privateData,PHY_BSR);
	privateData->dwLinkSettings=LINK_OFF;
	if(wRegBSR&PHY_BSR_LINK_STATUS_) {
		wRegVal=Phy_GetRegW(
						privateData,
						PHY_BCR);
		if(wRegVal&PHY_BCR_AUTO_NEG_ENABLE_) {
			DWORD linkSettings=LINK_AUTO_NEGOTIATE;
			WORD wRegADV=Phy_GetRegW(
								privateData,
								PHY_ANEG_ADV);
			WORD wRegLPA=Phy_GetRegW(
								privateData,
								PHY_ANEG_LPA);
			if(wRegADV&PHY_ANEG_ADV_ASYMP_) {
				linkSettings|=LINK_ASYMMETRIC_PAUSE;
			}
			if(wRegADV&PHY_ANEG_ADV_SYMP_) {
				linkSettings|=LINK_SYMMETRIC_PAUSE;
			}
			if(wRegADV&PHY_ANEG_LPA_100FDX_) {
				linkSettings|=LINK_SPEED_100FD;
			}
			if(wRegADV&PHY_ANEG_LPA_100HDX_) {
				linkSettings|=LINK_SPEED_100HD;
			}
			if(wRegADV&PHY_ANEG_LPA_10FDX_) {
				linkSettings|=LINK_SPEED_10FD;
			}
			if(wRegADV&PHY_ANEG_LPA_10HDX_) {
				linkSettings|=LINK_SPEED_10HD;
			}
			privateData->dwLinkSettings=linkSettings;
			wRegLPA&=wRegADV;
			if(wRegLPA&PHY_ANEG_LPA_100FDX_) {
				result=LINK_SPEED_100FD;
			} else if(wRegLPA&PHY_ANEG_LPA_100HDX_) {
				result=LINK_SPEED_100HD;
			} else if(wRegLPA&PHY_ANEG_LPA_10FDX_) {
				result=LINK_SPEED_10FD;
			} else if(wRegLPA&PHY_ANEG_LPA_10HDX_) {
				result=LINK_SPEED_10HD;
			}
		} else {
			if(wRegVal&PHY_BCR_SPEED_SELECT_) {
				if(wRegVal&PHY_BCR_DUPLEX_MODE_) {
					privateData->dwLinkSettings=result=LINK_SPEED_100FD;
				} else {
					privateData->dwLinkSettings=result=LINK_SPEED_100HD;
				}
			} else {
				if(wRegVal&PHY_BCR_DUPLEX_MODE_) {
					privateData->dwLinkSettings=result=LINK_SPEED_10FD;
				} else {
					privateData->dwLinkSettings=result=LINK_SPEED_10HD;
				}
			}
		}
	}
	privateData->dwLinkSpeed=result;
}

//Initalize the Mac data elements
void Mac_Initialize(PPRIVATE_DATA privateData)
{
	SMSC_ASSERT(privateData!=NULL);

	//nothing to do here in the simple driver
	//  but this function is kept as a place holder.

}

//Check for not busy
static BOOLEAN MacNotBusy(PPRIVATE_DATA privateData)
{
	int i=0;

	//Assuming MacPhyAccessLock has already been acquired

	// wait for MAC not busy, w/ timeout
	for(i=0;i<40;i++)
		{
			if((Lan_GetRegDW(MAC_CSR_CMD) & MAC_CSR_CMD_CSR_BUSY_)==(0UL)) {
				return TRUE;
			}
		}
	SMSC_WARNING("timeout waiting for MAC not BUSY. MAC_CSR_CMD = 0x%08lX",
			   Lan_GetRegDW(MAC_CSR_CMD));
	return FALSE;
}

//Gets a mac register value
DWORD Mac_GetRegDW(PPRIVATE_DATA privateData,DWORD dwRegOffset)
{
	DWORD result=0xFFFFFFFFUL;
	DWORD dwTemp=0;
	SMSC_ASSERT(privateData!=NULL);
	SMSC_ASSERT(privateData->dwLanBase!=0);

	
	//Assuming MacPhyAccessLock has already been acquired

	// wait until not busy
	if (Lan_GetRegDW(MAC_CSR_CMD) & MAC_CSR_CMD_CSR_BUSY_)
		{
			SMSC_WARNING("Mac_GetRegDW() failed, MAC already busy at entry");
			goto DONE;
		}

	// send the MAC Cmd w/ offset
	Lan_SetRegDW(MAC_CSR_CMD,
			   ((dwRegOffset & 0x000000FFUL) | MAC_CSR_CMD_CSR_BUSY_ | MAC_CSR_CMD_R_NOT_W_));
	dwTemp=Lan_GetRegDW(BYTE_TEST);//to flush previous write
	dwTemp=dwTemp;

	// wait for the read to happen, w/ timeout
	if (!MacNotBusy(privateData))
		{
			SMSC_WARNING("Mac_GetRegDW() failed, waiting for MAC not busy after read");
			goto DONE;
		} else {
			// finally, return the read data
			result=Lan_GetRegDW(MAC_CSR_DATA);
		}
 DONE:
	return result;
}

//Sets a Mac register
void Mac_SetRegDW(PPRIVATE_DATA privateData,DWORD dwRegOffset,DWORD dwVal)
{
	DWORD dwTemp=0;
	SMSC_ASSERT(privateData!=NULL);
	SMSC_ASSERT(privateData->dwLanBase!=0);

	//Assuming MacPhyAccessLock has already been acquired

	if (Lan_GetRegDW(MAC_CSR_CMD) & MAC_CSR_CMD_CSR_BUSY_)
		{
			SMSC_WARNING("Mac_SetRegDW() failed, MAC already busy at entry");
			goto DONE;
		}

	// send the data to write
	Lan_SetRegDW(MAC_CSR_DATA,dwVal);

	// do the actual write
	Lan_SetRegDW(MAC_CSR_CMD,((dwRegOffset & 0x000000FFUL) | MAC_CSR_CMD_CSR_BUSY_));
	dwTemp=Lan_GetRegDW(BYTE_TEST);//force flush of previous write
	dwTemp=dwTemp;

	// wait for the write to complete, w/ timeout
	if (!MacNotBusy(privateData))
		{
			SMSC_WARNING("Mac_SetRegDW() failed, waiting for MAC not busy after write");
		}
 DONE:
	return;
}

#define TX_FIFO_LOW_THRESHOLD	(1600)

//initializes the Transmitter
void Tx_Initialize(PPRIVATE_DATA privateData)
{
	DWORD dwRegVal=0;
	SMSC_ASSERT(privateData!=NULL);
	SMSC_ASSERT(privateData->dwLanBase!=0);

	dwRegVal=Lan_GetRegDW(HW_CFG);
	dwRegVal&=HW_CFG_TX_FIF_SZ_;
	dwRegVal|=HW_CFG_SF_;
	Lan_SetRegDW(HW_CFG,dwRegVal);

	Lan_SetBitsDW(FIFO_INT,0xFF000000UL);
	Lan_SetBitsDW(INT_EN,INT_EN_TDFA_EN_);

	//Because this is part of the single threaded initialization
	//  path there is no need to acquire the MacPhyAccessLock
	{
		DWORD dwMacCr=Mac_GetRegDW(privateData,MAC_CR);
		dwMacCr|=(MAC_CR_TXEN_|MAC_CR_HBDIS_);
		Mac_SetRegDW(privateData,MAC_CR,dwMacCr);
		Lan_SetRegDW(TX_CFG,TX_CFG_TX_ON_);
	}
}

//Reenables the Transmitter
BOOLEAN Tx_HandleInterrupt(PPRIVATE_DATA privateData,DWORD dwIntSts)
{
	SMSC_ASSERT(privateData!=NULL);
	if(dwIntSts & INT_STS_TDFA_) 
		{
			Lan_SetBitsDW(FIFO_INT,0xFF000000UL);
			Lan_SetRegDW(INT_STS,INT_STS_TDFA_);
			netif_wake_queue(privateData->dev);
			return TRUE;
		}
	return FALSE;
}

//Writes a packet to the TX_DATA_FIFO
static void Tx_WriteFifo(DWORD dwLanBase,DWORD *pdwBuf,DWORD dwDwordCount)
{
 	volatile DWORD *pdwReg = 
		(volatile DWORD *)(dwLanBase+TX_DATA_FIFO);

	while(dwDwordCount)	{
		*pdwReg = *pdwBuf++;
		dwDwordCount--;
	}

}

//Gets the number of Tx Statuses in the fifo
static DWORD Tx_GetTxStatusCount(PPRIVATE_DATA privateData)
{
	DWORD result=0;
	SMSC_ASSERT(privateData!=NULL);
	SMSC_ASSERT(privateData->dwLanBase!=0);
	result=Lan_GetRegDW(TX_FIFO_INF);
	result&=TX_FIFO_INF_TSUSED_;
	result>>=16;
	return result;
}

//sends a packet
void Tx_SendSkb(PPRIVATE_DATA privateData, struct sk_buff *skb)
{
	DWORD dwFreeSpace=0;
	DWORD dwTxCmdA=0;
	DWORD dwTxCmdB=0;

	SMSC_ASSERT(privateData!=NULL);
	SMSC_ASSERT(privateData->dwLanBase!=0);

	dwFreeSpace=Lan_GetRegDW(TX_FIFO_INF);
	dwFreeSpace&=TX_FIFO_INF_TDFREE_;
	if(dwFreeSpace<TX_FIFO_LOW_THRESHOLD) {
		SMSC_WARNING("Tx Data Fifo Low, space available = %ld",dwFreeSpace);
	}
	dwTxCmdA=
		((((DWORD)(skb->data))&0x03UL)<<16) | //DWORD alignment adjustment
		TX_CMD_A_FIRST_SEG_ | TX_CMD_A_LAST_SEG_ | 
		((DWORD)(skb->len));
	dwTxCmdB=
		(((DWORD)(skb->len))<<16) |
		((DWORD)(skb->len));

	Lan_SetRegDW(TX_DATA_FIFO,dwTxCmdA);
	Lan_SetRegDW(TX_DATA_FIFO,dwTxCmdB);
	Tx_WriteFifo(
			   privateData->dwLanBase,
			   (DWORD *)(((DWORD)(skb->data))&0xFFFFFFFCUL),
			   (((DWORD)(skb->len))+3+
			    (((DWORD)(skb->data))&0x03UL))>>2);
	dwFreeSpace-=(skb->len+32);
	dev_kfree_skb(skb);

	if(Tx_GetTxStatusCount(privateData)>=30)
		{
			Tx_UpdateTxCounters(privateData);
		}
	if(dwFreeSpace<TX_FIFO_LOW_THRESHOLD) {
		netif_stop_queue(privateData->dev);
		{
			DWORD temp=Lan_GetRegDW(FIFO_INT);
			temp&=0x00FFFFFFUL;
			temp|=0x32000000UL;
			Lan_SetRegDW(FIFO_INT,temp);
		}
	}
}

//gets a tx status out of the status fifo
static DWORD Tx_CompleteTx( PPRIVATE_DATA privateData)
{
	DWORD result=0;
	SMSC_ASSERT(privateData!=NULL);
	SMSC_ASSERT(privateData->dwLanBase!=0);

	result=Lan_GetRegDW(TX_FIFO_INF);

	result&=TX_FIFO_INF_TSUSED_;
	if(result!=0x00000000UL) {
		result=Lan_GetRegDW(TX_STATUS_FIFO);
	} else {
		result=0;
	}

	return result;
}

//reads tx statuses and increments counters where necessary
void Tx_UpdateTxCounters(PPRIVATE_DATA privateData)
{
	DWORD dwTxStatus=0;
	SMSC_ASSERT(privateData!=NULL);
	while((dwTxStatus=Tx_CompleteTx(privateData))!=0)
		{
			if(dwTxStatus&0x80000000UL) {
				SMSC_WARNING("Packet tag reserved bit is high");
				//In this driver the packet tag is used as the packet
				//  length. Since a packet length can never reach
				//  the size of 0x8000, I made this bit reserved
				//  so if I ever decided to use packet tracking 
				//  tags then those tracking tags would set the 
				//  reserved bit. And I would use this control path
				//  to look up the packet and perhaps free it.
				//  As you can see I never persued this idea.
				//  because it never provided any benefit in this
				//  linux environment.
				//But it is worth noting that the "reserved bit"
				//  in the warning above does not reference a
				//  hardware defined reserved bit but rather a 
				//  driver defined reserved bit. 
			} else {
				if(dwTxStatus&0x00008000UL) {
					privateData->stats.tx_errors++;
				} else {
					privateData->stats.tx_packets++;
					privateData->stats.tx_bytes+=(dwTxStatus>>16);
				}
				if(dwTxStatus&0x00000100UL) {
					privateData->stats.collisions+=16;
					privateData->stats.tx_aborted_errors+=1;
				} else {
					privateData->stats.collisions+=
						((dwTxStatus>>3)&0xFUL);
				}
				if(dwTxStatus&0x00000800UL) {
					privateData->stats.tx_carrier_errors+=1;
				}
				if(dwTxStatus&0x00000200UL) {
					privateData->stats.collisions++;
					privateData->stats.tx_aborted_errors++;
				}
			}
		}
}

//initializes the receiver
void Rx_Initialize(PPRIVATE_DATA privateData)
{
	DWORD dwMacCr=0;
	SMSC_ASSERT(privateData!=NULL);

	Lan_SetRegDW(RX_CFG,0x00000200UL);
	
	//Because this is part of the single threaded initialization
	//  path there is no need to acquire the MacPhyAccessLock
	dwMacCr = Mac_GetRegDW(privateData,MAC_CR);
	dwMacCr |= MAC_CR_RXEN_;

	Mac_SetRegDW(privateData,MAC_CR,dwMacCr);

	Lan_ClrBitsDW(FIFO_INT,0x000000FFUL);

	Lan_SetBitsDW(INT_EN,INT_EN_RSFL_EN_);
}

//reads a packet out of the RX_DATA_FIFO
static void Rx_ReadFifo(DWORD dwLanBase,
				    DWORD *pdwBuf,
				    DWORD dwDwordCount)
{
	const volatile DWORD * const pdwReg = 
		(const volatile DWORD * const)(dwLanBase+RX_DATA_FIFO);

	while (dwDwordCount)
		{
			*pdwBuf++ = *pdwReg;
			dwDwordCount--;
		}

}

//passes a packet to linux
static void Rx_HandOffSkb(PPRIVATE_DATA privateData,
					 struct sk_buff *skb)
{
	int result=0;

	skb->dev=privateData->dev;
	skb->protocol= eth_type_trans(skb,privateData->dev);
	skb->ip_summed = CHECKSUM_NONE;

	result=netif_rx(skb);

	switch(result) 
		{
		case NET_RX_SUCCESS:
			break;
		case NET_RX_CN_LOW:
		case NET_RX_CN_MOD:
		case NET_RX_CN_HIGH:
		case NET_RX_DROP:
			privateData->RxCongested=TRUE;
			break;
		default:
			privateData->RxCongested=TRUE;
			SMSC_WARNING("Unknown return value from netif_rx, result=%d",result);
			break;
		}
}

//Gets the next rx status
static DWORD Rx_PopRxStatus(PPRIVATE_DATA privateData)
{
	DWORD result=Lan_GetRegDW(RX_FIFO_INF);
	if((privateData->RxCongested==FALSE)||
	   ((privateData->RxCongested==TRUE)&&((result&0x00FF0000UL)==0UL)))
		{
			if(result&0x00FF0000UL) {
				//Rx status is available, read it
				result=Lan_GetRegDW(RX_STATUS_FIFO);
			} else {
				result=0;
			}
		} else {
			//  Initiate the interrupt deassertion interval
			Lan_SetBitsDW(INT_CFG,INT_CFG_INT_DEAS_CLR_);
			result=0;
		}
	return result;
}

//Increments the Rx error counters
void Rx_CountErrors(PPRIVATE_DATA privateData,DWORD dwRxStatus) 
{
	BOOLEAN crcError=FALSE;
	if(dwRxStatus&0x00008000UL) {
		privateData->stats.rx_errors++;
		if(dwRxStatus&0x00000002UL) {
			privateData->stats.rx_crc_errors++;
			crcError=TRUE;
		}
	}
	if(!crcError) {
		if(dwRxStatus&0x00001020UL) {
			//Frame type indicates length, and length error is set
			privateData->stats.rx_length_errors++;
		}
		if(dwRxStatus&RX_STS_MCAST_) {
			privateData->stats.multicast++;
		}
	}
}

//This function is used to quickly dump bad packets
void Rx_FastForward(PPRIVATE_DATA privateData,DWORD dwDwordCount)
{
	if(dwDwordCount>=4) 
		{
			DWORD dwTimeOut=500;
			Lan_SetRegDW(RX_DP_CTRL,RX_DP_CTRL_RX_FFWD_);
			while((dwTimeOut)&&(Lan_GetRegDW(RX_DP_CTRL)&RX_DP_CTRL_RX_FFWD_))
				{
					udelay(1);
					dwTimeOut--;
				}
			if(dwTimeOut==0) {
				SMSC_WARNING("timed out waiting for RX FFWD to finish, RX_DP_CTRL=0x%08lX",Lan_GetRegDW(RX_DP_CTRL));
			}
		} else {
			while(dwDwordCount) {
				DWORD dwTemp=Lan_GetRegDW(RX_DATA_FIFO);
				dwTemp=dwTemp;
				dwDwordCount--;
			}
		}
}

//This is the main function for reading packets out of the LAN911x
void Rx_ProcessPackets(PPRIVATE_DATA privateData)
{
	DWORD dwRxStatus=0;
	privateData->RxCongested=FALSE;

	while((dwRxStatus=Rx_PopRxStatus(privateData))!=0)
		{
			DWORD dwPacketLength=((dwRxStatus&0x3FFF0000UL)>>16);
			Rx_CountErrors(privateData,dwRxStatus);
			if((dwRxStatus&RX_STS_ES_)==0) {
				struct sk_buff *skb=NULL;
				skb=dev_alloc_skb(dwPacketLength+2);
				if(skb!=NULL) {
					skb->data=skb->head;
					skb->tail=skb->head;
					skb_reserve(skb,2); // align IP on 16B boundary
					skb_put(skb,dwPacketLength-4UL);

					//update counters
					privateData->stats.rx_packets++;
					privateData->stats.rx_bytes+=(dwPacketLength-4);	

					Rx_ReadFifo(
							  privateData->dwLanBase,
							  ((DWORD *)(skb->head)),
							  (dwPacketLength+2+3)>>2);
	
					Rx_HandOffSkb(privateData,skb);
					continue;
				} else {
					SMSC_WARNING("Unable to allocate sk_buff for RX Packet, in PIO path");
					privateData->stats.rx_dropped++;
				}
			}
			//if we get here then the packet is to be read
			//  out of the fifo and discarded
			dwPacketLength+=(2+3);
			dwPacketLength>>=2;
			Rx_FastForward(privateData,dwPacketLength);
		}
	privateData->stats.rx_dropped+=Lan_GetRegDW(RX_DROP);
	Lan_SetRegDW(INT_STS,INT_STS_RSFL_);
}


//This is the tasklet entry point
void Rx_ProcessPacketsTasklet(unsigned long data) 
{
	PPRIVATE_DATA privateData=(PPRIVATE_DATA)(Rx_TaskletParameter);
	if(privateData==NULL) {
		SMSC_WARNING("Rx_ProcessPacketsTasklet(privateData==NULL)");
		return;
	}
	Rx_ProcessPackets(privateData);
	Lan_SetBitsDW(INT_CFG,INT_CFG_IRQ_EN_);
}

//Handles receiver interrupts
BOOLEAN Rx_HandleInterrupt( PPRIVATE_DATA privateData, DWORD dwIntSts)
{
	BOOLEAN result=FALSE;
	SMSC_ASSERT(privateData!=NULL);

	if(dwIntSts&INT_STS_RXE_) {
		Lan_SetRegDW(INT_STS,INT_STS_RXE_);
		result=TRUE;
	}
	
	if(!(dwIntSts&INT_STS_RSFL_)) {
		return result;
	}
	result=TRUE;

	Lan_ClrBitsDW(INT_CFG,INT_CFG_IRQ_EN_);
	Rx_TaskletParameter=  (unsigned long) privateData;
	tasklet_schedule(&Rx_Tasklet0); 

	return result;
}

//returns hash bit number for given MAC address
//example:
//   01 00 5E 00 00 01 -> returns bit number 31
static DWORD Hash(BYTE addr[6])
{
	int i;
	DWORD crc=0xFFFFFFFFUL;
	DWORD poly=0xEDB88320UL;
	DWORD result=0;
	for(i=0;i<6;i++) 
		{
			int bit;
			DWORD data=((DWORD)addr[i]);
			for(bit=0;bit<8;bit++) 
				{
					DWORD p = (crc^((DWORD)data))&1UL;
					crc >>= 1;
					if(p!=0) crc ^= poly;
					data >>=1;
				}
		}
	result=((crc&0x01UL)<<5)|
		((crc&0x02UL)<<3)|
		((crc&0x04UL)<<1)|
		((crc&0x08UL)>>1)|
		((crc&0x10UL)>>3)|
		((crc&0x20UL)>>5);
	return result;
}

//Sets addressing modes
void Rx_SetMulticastList(
					struct net_device *dev)
{
	PPRIVATE_DATA privateData=NULL;
	SMSC_ASSERT(dev!=NULL);

	privateData=((PPRIVATE_DATA)(dev->priv));
	SMSC_ASSERT(privateData!=NULL);    

	spin_lock(&(privateData->MacPhyAccessLock));

	if(dev->flags & IFF_PROMISC) {
		//enabling promiscuous mode
		DWORD dwMacCr=0;
		dwMacCr=Mac_GetRegDW(privateData,MAC_CR);
		dwMacCr|=MAC_CR_PRMS_;
		dwMacCr&=(~MAC_CR_MCPAS_);
		dwMacCr&=(~MAC_CR_HPFILT_);
		Mac_SetRegDW(privateData,MAC_CR,dwMacCr);
		return;
	}

	if(dev->flags & IFF_ALLMULTI) {
		//enabling all multicast mode
		DWORD dwMacCr=0;
		dwMacCr=Mac_GetRegDW(privateData,MAC_CR);
		dwMacCr&=(~MAC_CR_PRMS_);
		dwMacCr|=MAC_CR_MCPAS_;
		dwMacCr&=(~MAC_CR_HPFILT_);
		Mac_SetRegDW(privateData,MAC_CR,dwMacCr);
		return;
	}

	if(dev->mc_count>0) {
		//enabling specific multicast addresses
		DWORD dwHashH=0;
		DWORD dwHashL=0;
		DWORD dwCount=0;
		struct dev_mc_list *mc_list=dev->mc_list;
		DWORD dwMacCr=0;
		dwMacCr=Mac_GetRegDW(privateData,MAC_CR);
		dwMacCr&=(~MAC_CR_PRMS_);
		dwMacCr&=(~MAC_CR_MCPAS_);
		dwMacCr|=MAC_CR_HPFILT_;
		while(mc_list!=NULL) {
			dwCount++;
			if((mc_list->dmi_addrlen)==6) {
				DWORD dwMask=0x01UL;
				DWORD dwBitNum=Hash(mc_list->dmi_addr);
				dwMask<<=(dwBitNum&0x1FUL);
				if(dwBitNum&0x20UL) {
					dwHashH|=dwMask;
				} else {
					dwHashL|=dwMask;
				}
			} else {
				SMSC_WARNING("dmi_addrlen!=6");
			}
			mc_list=mc_list->next;
		}
		if(dwCount!=((DWORD)dev->mc_count)) {
			SMSC_WARNING("dwCount!=dev->mc_count");
		}
		Mac_SetRegDW(privateData,HASHH,dwHashH);
		Mac_SetRegDW(privateData,HASHL,dwHashL);
		Mac_SetRegDW(privateData,MAC_CR,dwMacCr);
	} 
	else 
		{
			//enabling local MAC address only
			DWORD dwMacCr=0;
			dwMacCr=Mac_GetRegDW(privateData,MAC_CR);
			dwMacCr&=(~MAC_CR_PRMS_);
			dwMacCr&=(~MAC_CR_MCPAS_);
			dwMacCr&=(~MAC_CR_HPFILT_);
			Mac_SetRegDW(privateData,HASHH,0);
			Mac_SetRegDW(privateData,HASHL,0);
			Mac_SetRegDW(privateData,MAC_CR,dwMacCr);
		}
	spin_unlock(&(privateData->MacPhyAccessLock));
}

#ifdef USE_LED1_WORK_AROUND
volatile DWORD g_GpioSetting=0x00000000UL;
volatile DWORD g_GpioSettingOriginal=0x00000000UL;
#endif

BOOLEAN Lan_Initialize( PPRIVATE_DATA privateData, DWORD dwIntCfg)
{
	BOOLEAN result=FALSE;
	DWORD dwTimeOut=0;
	DWORD dwTemp=0;


	SMSC_TRACE("-->Lan_Initialize(dwIntCfg=0x%08lX)",dwIntCfg);
	SMSC_ASSERT(privateData!=NULL);

	spin_lock_init(&(privateData->MacPhyAccessLock));

	//Wa must Save mac address before Reseting the LAN9115
	mac_addr_hi16 = Mac_GetRegDW(privateData,ADDRH);
	mac_addr_lo32 = Mac_GetRegDW(privateData,ADDRL);

	//Reset the LAN911x
	Lan_SetRegDW(HW_CFG,HW_CFG_SRST_);
	dwTimeOut=10;
	do {
		udelay(10);
		dwTemp=Lan_GetRegDW(HW_CFG);
		dwTimeOut--;
	} while((dwTimeOut>0)&&(dwTemp&HW_CFG_SRST_));
	if(dwTemp&HW_CFG_SRST_) {
		SMSC_WARNING("  Failed to complete reset.");
		goto DONE;
	}

	Lan_SetRegDW(HW_CFG,0x00050000UL);
	Lan_SetRegDW(AFC_CFG,0x006E3740UL);

	Lan_ClrBitsDW(GPT_CFG,GPT_CFG_TIMER_EN_);
	Lan_SetRegDW(GPIO_CFG,0x70070000UL);

	//initialize interrupts
	dwTemp=Lan_GetRegDW(INT_EN);	
	dwTemp&=0x0UL;
	Lan_SetRegDW(INT_EN,dwTemp);		 

	Lan_SetRegDW(INT_STS,0xFFFFFFFFUL);

	dwIntCfg|=INT_CFG_IRQ_EN_;
	Lan_SetRegDW(INT_CFG,dwIntCfg);

	result=TRUE;

 DONE:
	SMSC_TRACE("<--Lan_Initialize");
	return result;
}


module_init(smsc9115_init_module);
module_exit(smsc9115_cleanup_module);

