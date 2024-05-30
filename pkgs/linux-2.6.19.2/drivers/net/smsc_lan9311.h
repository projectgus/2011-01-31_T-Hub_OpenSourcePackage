/***************************************************************************
 *
 * Copyright (C) 2004-2007  SMSC
 * Copyright (C) 2005 ARM
 *
 * Modified by Sagemcom under GPL license on 28/04/2008 
 * Copyright (c) 2010 Sagemcom All rights reserved.
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
 ***************************************************************************/
#ifndef __SMSC_LAN9311_H__
#define __SMSC_LAN9311_H__

#define SMSC_CAN_USE_32BIT	0
#define TX_FIFO_LOW_THRESHOLD	(u32)1600
#define SMSC911X_EEPROM_SIZE	(u32)7
#define USE_DEBUG 		1

/* implements a PHY loopback test at initialisation time, to ensure a packet
 * can be succesfully looped back */
#undef USE_PHY_WORK_AROUND

/* 10/100 LED link-state inversion when media is disconnected */
#ifndef CONFIG_SMSC_LAN9311
#define USE_LED1_WORK_AROUND
#endif

/* platform_device configuration data, should be assigned to
 * the platform_device's dev.platform_data */
struct smsc_lan9311_platform_config {
	unsigned int irq_polarity;
	unsigned int irq_type;
};

#if USE_DEBUG >= 1
#define SMSC_WARNING(fmt, args...) \
		printk(KERN_EMERG "SMSC_WARNING: %s: " fmt "\n", \
			__FUNCTION__ , ## args)
#else
#define SMSC_WARNING(msg, args...)
#endif				/* USE_DEBUG >= 1 */

#if USE_DEBUG >= 2
#define SMSC_TRACE(fmt,args...) \
		printk(KERN_EMERG "SMSC_TRACE: %s: " fmt "\n", \
			__FUNCTION__ , ## args)
#else
#define SMSC_TRACE(msg,args...)
#endif				/* USE_DEBUG >= 2 */

#if USE_DEBUG >= 3
#define SMSC_DEBUG(fmt,args...) \
		if (in_irq()) \
		{ \
		printk(KERN_EMERG "SMSC_DEBUG: HW IRQ: %s:  " fmt "\n", \
			__FUNCTION__ , ## args); \
		} \
		else if (in_softirq()) \
		{ \
		printk(KERN_EMERG "SMSC_DEBUG: SW IRQ: %s:  " fmt "\n", \
			__FUNCTION__ , ## args); \
		} \
		else \
		{ \
		printk(KERN_EMERG "SMSC_DEBUG: %s: " fmt "\n", \
			__FUNCTION__ , ## args); \
		}
		
#else
#define SMSC_DEBUG(msg,args...)
#endif				/* USE_DEBUG >= 2 */

#ifdef USE_TASKLET
//define TASKLET callback
typedef void (*fp_tasklet_clbk)(void *data,unsigned int count);
#endif

struct smsc_lan9311_data {
	void __iomem *ioaddr;

	unsigned int idrev;
	unsigned int generation;	/* used to decide which workarounds apply */

	/* device configuration */
	unsigned int irq_polarity;
	unsigned int irq_type;

	//the ctx lock insure that we do no re-enter the function API either from work queue, from user land or from ip stack.
	struct mutex api_lock;
	//the hw_lock insure that we always access the registers of the chip in a sequential manner, and that there are not any irq breaking the current sequence, we also insure that there are no race conditions when accessing the driver context between irq and threads
	spinlock_t hw_lock;
	//Link work queue
	struct workqueue_struct *link_work_queue;
	//Link work
	struct work_struct link_work;
#if (defined USE_MXC_DMA)
	//dma channels
	int dma_tx_channel;
	int dma_rx_channel;
#endif
#if (defined USE_MXC_DMA) || (defined USE_TASKLET)
	// number of DMA bd slots available
	int dma_tx_nb_bd;
	int dma_rx_nb_bd;
	// index of last DMA request
	int dma_tx_last_req_index;
	int dma_rx_last_req_index;
	// index of last DMA completion
	int dma_tx_last_completed_index;
	int dma_rx_last_completed_index;
	// index of last read DMA buffer index
	int dma_tx_last_read_index;
	int dma_rx_last_read_index;
	//This value indicates if the dma is active (1) or not (0)
	int dma_active_tx;
	int dma_active_rx;
	//Table of DMA request
	struct sk_buff *skb_dma_req_rx[MAX_DMA_NB_BD];
	struct sk_buff *skb_dma_req_tx[MAX_DMA_NB_BD];
#endif
#ifdef USE_TASKLET
	// index of buffer currently copied by tasklet
	int tasklet_tx_index;
	int tasklet_rx_index;
	struct tasklet_struct tasklet_rx;
	struct tasklet_struct tasklet_tx;
	int skb_tasklet_req_size_words[MAX_DMA_NB_BD];
	fp_tasklet_clbk tasklet_clbk_rx;
	fp_tasklet_clbk tasklet_clbk_tx;
#endif
	struct net_device_stats stats;
	struct mii_if_info mii;
	unsigned int using_extphy;
	u32 msg_enable;
#ifdef USE_LED1_WORK_AROUND
	unsigned int gpio_setting;
	unsigned int gpio_orig_setting;
#endif
	struct timer_list link_poll_timer;
	unsigned int stop_link_poll;
	unsigned char port1_plugged_state;
	unsigned char port2_plugged_state;
	unsigned int software_irq_signal;

#ifdef USE_PHY_WORK_AROUND
#define MIN_PACKET_SIZE (64)
	char loopback_tx_pkt[MIN_PACKET_SIZE];
	char loopback_rx_pkt[MIN_PACKET_SIZE];
	unsigned int resetcount;
#endif

	/* Members for Multicast filter workaround */
	unsigned int multicast_update_pending;
	unsigned int set_bits_mask;
	unsigned int clear_bits_mask;
	unsigned int hashhi;
	unsigned int hashlo;
};


/* SMSC911x registers and bitfields */
#define RX_DATA_FIFO			0x00

#define TX_DATA_FIFO			0x20
#define TX_CMD_A_ON_COMP_		0x80000000
#define TX_CMD_A_BUF_END_ALGN_		0x03000000
#define TX_CMD_A_4_BYTE_ALGN_		0x00000000
#define TX_CMD_A_16_BYTE_ALGN_		0x01000000
#define TX_CMD_A_32_BYTE_ALGN_		0x02000000
#define TX_CMD_A_DATA_OFFSET_		0x001F0000
#define TX_CMD_A_FIRST_SEG_		0x00002000
#define TX_CMD_A_LAST_SEG_		0x00001000
#define TX_CMD_A_BUF_SIZE_		0x000007FF
#define TX_CMD_B_PKT_TAG_		0xFFFF0000
#define TX_CMD_B_ADD_CRC_DISABLE_	0x00002000
#define TX_CMD_B_DISABLE_PADDING_	0x00001000
#define TX_CMD_B_PKT_BYTE_LENGTH_	0x000007FF

#define RX_STATUS_FIFO			0x40
#define RX_STS_ES_			0x00008000
#define RX_STS_MCAST_			0x00000400

#define RX_STATUS_FIFO_PEEK		0x44

#define TX_STATUS_FIFO			0x48
#define TX_STS_ES_			0x00008000

#define TX_STATUS_FIFO_PEEK		0x4C

#define ID_REV				0x50
#define ID_REV_CHIP_ID_			0xFFFF0000
#define ID_REV_REV_ID_			0x0000FFFF

#define INT_CFG				0x54
#define INT_CFG_INT_DEAS_		0xFF000000
#define INT_CFG_INT_DEAS_CLR_		0x00004000
#define INT_CFG_INT_DEAS_STS_		0x00002000
#define INT_CFG_IRQ_INT_		0x00001000
#define INT_CFG_IRQ_EN_			0x00000100
#define INT_CFG_IRQ_POL_		0x00000010
#define INT_CFG_IRQ_TYPE_		0x00000001

#define INT_STS				0x58

#define INT_STS_SW_INT_			0x80000000
#ifdef CONFIG_SMSC_LAN9311
#define INT_STS_DEV_READY_		0x40000000
#define INT_STS_1588_EVT_		0x20000000
#define INT_STS_SWITCH_INT_		0x10000000
#define PHY_INT2_STS			(0x01UL << 27)
#define PHY_INT1_STS			(0x01UL << 26)
#endif
#define INT_STS_TXSTOP_INT_		0x02000000
#define INT_STS_RXSTOP_INT_		0x01000000
#define INT_STS_RXDFH_INT_		0x00800000
#ifndef CONFIG_SMSC_LAN9311
#define INT_STS_RXDF_INT_		0x00400000
#endif
#define INT_STS_TX_IOC_			0x00200000
#define INT_STS_RXD_INT_		0x00100000
#define INT_STS_GPT_INT_		0x00080000
#ifndef CONFIG_SMSC_LAN9311
#define INT_STS_PHY_INT_		0x00040000
#endif
#define INT_STS_PME_INT_		0x00020000
#define INT_STS_TXSO_			0x00010000
#define INT_STS_RWT_			0x00008000
#define INT_STS_RXE_			0x00004000
#define INT_STS_TXE_			0x00002000
#ifdef CONFIG_SMSC_LAN9311
#define INT_STS_GPIO_INT_		0x00001000
#endif
#define INT_STS_TDFU_			0x00000800
#define INT_STS_TDFO_			0x00000400
#define INT_STS_TDFA_			0x00000200
#define INT_STS_TSFF_			0x00000100
#define INT_STS_TSFL_			0x00000080
#define INT_STS_RXDF_			0x00000040
#define INT_STS_RDFL_			0x00000020
#define INT_STS_RSFF_			0x00000010
#define INT_STS_RSFL_			0x00000008
#ifndef CONFIG_SMSC_LAN9311
#define INT_STS_GPIO2_INT_		0x00000004
#define INT_STS_GPIO1_INT_		0x00000002
#define INT_STS_GPIO0_INT_		0x00000001
#endif

#define INT_EN				0x5C
#define INT_EN_SW_INT_EN_		0x80000000
#ifdef CONFIG_SMSC_LAN9311
#define INT_EN_DEV_READY_		0x40000000
#define INT_EN_1588_EVT_		0x20000000
#define SWITCH_INT_EN			(0x01UL << 28)
#define PHY_INT2_EN			(0x01UL << 27)
#define PHY_INT1_EN			(0x01UL << 26)
#endif
#define INT_EN_TXSTOP_INT_EN_		0x02000000
#define INT_EN_RXSTOP_INT_EN_		0x01000000
#define INT_EN_RXDFH_INT_EN_		0x00800000
#define INT_EN_TIOC_INT_EN_		0x00200000
#define INT_EN_RXD_INT_EN_		0x00100000
#define INT_EN_GPT_INT_EN_		0x00080000
#ifndef CONFIG_SMSC_LAN9311
#define INT_EN_PHY_INT_EN_		0x00040000
#endif
#define INT_EN_PME_INT_EN_		0x00020000
#define INT_EN_TXSO_EN_			0x00010000
#define INT_EN_RWT_EN_			0x00008000
#define INT_EN_RXE_EN_			0x00004000
#define INT_EN_TXE_EN_			0x00002000
#ifdef CONFIG_SMSC_LAN9311
#define INT_EN_GPIO_EN_			0x00001000
#endif
#define INT_EN_TDFU_EN_			0x00000800
#define INT_EN_TDFO_EN_			0x00000400
#define INT_EN_TDFA_EN_			0x00000200
#define INT_EN_TSFF_EN_			0x00000100
#define INT_EN_TSFL_EN_			0x00000080
#define INT_EN_RXDF_EN_			0x00000040
#define INT_EN_RDFL_EN_			0x00000020
#define INT_EN_RSFF_EN_			0x00000010
#define INT_EN_RSFL_EN_			0x00000008
#ifndef CONFIG_SMSC_LAN9311
#define INT_EN_GPIO2_INT_		0x00000004
#define INT_EN_GPIO1_INT_		0x00000002
#define INT_EN_GPIO0_INT_		0x00000001
#endif

#define BYTE_TEST			0x64

#define FIFO_INT			0x68
#define FIFO_INT_TX_AVAIL_LEVEL_	0xFF000000
#define FIFO_INT_TX_STS_LEVEL_		0x00FF0000
#define FIFO_INT_RX_AVAIL_LEVEL_	0x0000FF00
#define FIFO_INT_RX_STS_LEVEL_		0x000000FF

#define RX_CFG				0x6C
#define RX_CFG_RX_END_ALGN_		0xC0000000
#define RX_CFG_RX_END_ALGN4_		0x00000000
#define RX_CFG_RX_END_ALGN16_		0x40000000
#define RX_CFG_RX_END_ALGN32_		0x80000000
#define RX_CFG_RX_DMA_CNT_		0x0FFF0000
#define RX_CFG_RX_DUMP_			0x00008000
#define RX_CFG_RXDOFF_			0x00001F00

#define TX_CFG				0x70
#define TX_CFG_TXS_DUMP_		0x00008000
#define TX_CFG_TXD_DUMP_		0x00004000
#define TX_CFG_TXSAO_			0x00000004
#define TX_CFG_TX_ON_			0x00000002
#define TX_CFG_STOP_TX_			0x00000001

#define HW_CFG				0x74
#define HW_CFG_TTM_			0x00200000
#define HW_CFG_SF_			0x00100000
#define HW_CFG_TX_FIF_SZ_		0x000F0000
#define HW_CFG_TR_			0x00003000
#define HW_CFG_SRST_			0x00000001

#ifdef CONFIG_SMSC_LAN9311
#define HW_READY                        0x08000000
#define HW_AMDIX_EN_STRAP_PORT2         0x04000000
#define HW_AMDIX_EN_STRAP_PORT1         0x02000000
#else
/* only available on 115/117 */

#define HW_CFG_PHY_CLK_SEL_		0x00000060
#define HW_CFG_PHY_CLK_SEL_INT_PHY_	0x00000000
#define HW_CFG_PHY_CLK_SEL_EXT_PHY_	0x00000020
#define HW_CFG_PHY_CLK_SEL_CLK_DIS_	0x00000040
#define HW_CFG_SMI_SEL_		 	0x00000010
#define HW_CFG_EXT_PHY_DET_		0x00000008
#define HW_CFG_EXT_PHY_EN_		0x00000004
#define HW_CFG_SRST_TO_			0x00000002


/* only available  on 116/118 */
#define HW_CFG_32_16_BIT_MODE_		0x00000004
#endif //#ifdef CONFIG_SMSC_LAN9311

#define RX_DP_CTRL			0x78
#define RX_DP_CTRL_RX_FFWD_		0x80000000

#define RX_FIFO_INF			0x7C
#define RX_FIFO_INF_RXSUSED_		0x00FF0000
#define RX_FIFO_INF_RXDUSED_		0x0000FFFF

#define TX_FIFO_INF			0x80
#define TX_FIFO_INF_TSUSED_		0x00FF0000
#define TX_FIFO_INF_TDFREE_		0x0000FFFF

#define PMT_CTRL			0x84
#ifdef CONFIG_SMSC_LAN9311
#define PMT_CTRL_PM_ED_STS2             0x00020000
#define PMT_CTRL_PM_ED_STS1             0x00010000
#define PMT_CTRL_PM_ED_EN2              0x00008000
#define PMT_CTRL_PM_ED_EN1              0x00004000
#else
#define PMT_CTRL_PM_MODE_		0x00003000
#define PMT_CTRL_PM_MODE_D0_		0x00000000
#define PMT_CTRL_PM_MODE_D1_		0x00001000
#define PMT_CTRL_PM_MODE_D2_		0x00002000
#define PMT_CTRL_PM_MODE_D3_		0x00003000
#endif
#define PMT_CTRL_PHY_RST_		0x00000400
#define PMT_CTRL_WOL_EN_		0x00000200
#ifdef CONFIG_SMSC_LAN9311
#define PMT_CTRL_PME_TYPE_		0x00000040
#define PMT_CTRL_WUPS_WOL_		0x00000020
#else
#define PMT_CTRL_ED_EN_			0x00000100
#define PMT_CTRL_PME_TYPE_		0x00000040
#define PMT_CTRL_WUPS_			0x00000030
#define PMT_CTRL_WUPS_NOWAKE_		0x00000000
#define PMT_CTRL_WUPS_ED_		0x00000010
#define PMT_CTRL_WUPS_WOL_		0x00000020
#define PMT_CTRL_WUPS_MULTI_		0x00000030
#endif
#define PMT_CTRL_PME_IND_		0x00000008
#define PMT_CTRL_PME_POL_		0x00000004
#define PMT_CTRL_PME_EN_		0x00000002
#define PMT_CTRL_READY_			0x00000001

#ifndef CONFIG_SMSC_LAN9311
#define GPIO_CFG			0x88
#define GPIO_CFG_LED3_EN_		0x40000000
#define GPIO_CFG_LED2_EN_		0x20000000
#define GPIO_CFG_LED1_EN_		0x10000000
#define GPIO_CFG_GPIO2_INT_POL_		0x04000000
#define GPIO_CFG_GPIO1_INT_POL_		0x02000000
#define GPIO_CFG_GPIO0_INT_POL_		0x01000000
#define GPIO_CFG_EEPR_EN_		0x00700000
#define GPIO_CFG_GPIOBUF2_		0x00040000
#define GPIO_CFG_GPIOBUF1_		0x00020000
#define GPIO_CFG_GPIOBUF0_		0x00010000
#define GPIO_CFG_GPIODIR2_		0x00000400
#define GPIO_CFG_GPIODIR1_		0x00000200
#define GPIO_CFG_GPIODIR0_		0x00000100
#define GPIO_CFG_GPIOD4_		0x00000020
#define GPIO_CFG_GPIOD3_		0x00000010
#define GPIO_CFG_GPIOD2_		0x00000004
#define GPIO_CFG_GPIOD1_		0x00000002
#define GPIO_CFG_GPIOD0_		0x00000001
#endif

#define GPT_CFG				0x8C
#define GPT_CFG_TIMER_EN_		0x20000000
#define GPT_CFG_GPT_LOAD_		0x0000FFFF

#define GPT_CNT				0x90
#define GPT_CNT_GPT_CNT_		0x0000FFFF

#ifndef CONFIG_SMSC_LAN9311
#define ENDIAN				0x98
#endif

#define FREE_RUN			0x9C

#define RX_DROP				0xA0

#define MAC_CSR_CMD			0xA4
#define MAC_CSR_CMD_CSR_BUSY_		0x80000000
#define MAC_CSR_CMD_R_NOT_W_		0x40000000
#define MAC_CSR_CMD_CSR_ADDR_		0x000000FF

#define MAC_CSR_DATA			0xA8

#define AFC_CFG				0xAC
#define AFC_CFG_AFC_HI_			0x00FF0000
#define AFC_CFG_AFC_LO_			0x0000FF00
#define AFC_CFG_BACK_DUR_		0x000000F0
#define AFC_CFG_FCMULT_			0x00000008
#define AFC_CFG_FCBRD_			0x00000004
#define AFC_CFG_FCADD_			0x00000002
#define AFC_CFG_FCANY_			0x00000001

#ifdef CONFIG_SMSC_LAN9311
#define E2P_CMD				0x1B4
#else
#define E2P_CMD				0xB0
#endif
#define E2P_CMD_EPC_BUSY_		0x80000000
#define E2P_CMD_EPC_CMD_		0x70000000
#define E2P_CMD_EPC_CMD_READ_		0x00000000
#define E2P_CMD_EPC_CMD_EWDS_		0x10000000
#define E2P_CMD_EPC_CMD_EWEN_		0x20000000
#define E2P_CMD_EPC_CMD_WRITE_		0x30000000
#define E2P_CMD_EPC_CMD_WRAL_		0x40000000
#define E2P_CMD_EPC_CMD_ERASE_		0x50000000
#define E2P_CMD_EPC_CMD_ERAL_		0x60000000
#define E2P_CMD_EPC_CMD_RELOAD_		0x70000000
#ifdef CONFIG_SMSC_LAN9311
#define E2P_CMD_EPC_TIMEOUT_		0x00020000
#define E2P_CMD_MAC_ADDR_LOADED_	0x00010000
#define E2P_CMD_EPC_ADDR_		0x0000FFFF
#else
#define E2P_CMD_EPC_TIMEOUT_		0x00000200
#define E2P_CMD_MAC_ADDR_LOADED_	0x00000100
#define E2P_CMD_EPC_ADDR_		0x000000FF
#endif

#ifdef CONFIG_SMSC_LAN9311
#define E2P_DATA			0x1B8
#define E2P_DATA_EEPROM_DATA_		0x000000FF
#else
#define E2P_DATA			0xB4
#define E2P_DATA_EEPROM_DATA_		0x000000FF
#define LAN_REGISTER_EXTENT		0x00000100
#endif

#ifdef CONFIG_SMSC_LAN9311
#define LED_CFG				0x1BC
#endif

#ifdef CONFIG_SMSC_LAN9311
#define VPHY_BASIC_CTRL			0x1C0 //MII_BMCR
#define VPHY_AN_ADV			0x1D0 //MII_ADVERTISE
#define VPHY_AN_LPA			0x1D4 //MII_LPA
#endif

/*
 * MAC Control and Status Register (Indirect Address)
 * Offset (through the MAC_CSR CMD and DATA port)
 */
#define MAC_CR				0x01
#define MAC_CR_RXALL_			0x80000000
#ifndef CONFIG_SMSC_LAN9311
#define MAC_CR_HBDIS_			0x10000000
#endif
#define MAC_CR_RCVOWN_			0x00800000
#define MAC_CR_LOOPBK_			0x00200000
#define MAC_CR_FDPX_			0x00100000
#define MAC_CR_MCPAS_			0x00080000
#define MAC_CR_PRMS_			0x00040000
#define MAC_CR_INVFILT_			0x00020000
#define MAC_CR_PASSBAD_			0x00010000
#define MAC_CR_HFILT_			0x00008000
#define MAC_CR_HPFILT_			0x00002000
#ifndef CONFIG_SMSC_LAN9311
#define MAC_CR_LCOLL_			0x00001000
#endif
#define MAC_CR_BCAST_			0x00000800
#define MAC_CR_DISRTY_			0x00000400
#define MAC_CR_PADSTR_			0x00000100
#define MAC_CR_BOLMT_MASK_		0x000000C0
#define MAC_CR_DFCHK_			0x00000020
#define MAC_CR_TXEN_			0x00000008
#define MAC_CR_RXEN_			0x00000004

#define ADDRH				0x02

#define ADDRL				0x03

#define HASHH				0x04

#define HASHL				0x05

#define MII_ACC				0x06
#define MII_ACC_PHY_ADDR_		0x0000F800
#define MII_ACC_MIIRINDA_		0x000007C0
#define MII_ACC_MII_WRITE_		0x00000002
#define MII_ACC_MII_BUSY_		0x00000001

#define MII_DATA			0x07

#define FLOW				0x08
#define FLOW_FCPT_			0xFFFF0000
#define FLOW_FCPASS_			0x00000004
#define FLOW_FCEN_			0x00000002
#define FLOW_FCBSY_			0x00000001

#define VLAN1				0x09

#define VLAN2				0x0A

#define WUFF				0x0B

#define WUCSR				0x0C
#define WUCSR_GUE_			0x00000200
#define WUCSR_WUFR_			0x00000040
#define WUCSR_MPR_			0x00000020
#define WUCSR_WAKE_EN_			0x00000004
#define WUCSR_MPEN_			0x00000002

/*
 * Phy definitions (vendor-specific)
 */
#define PHY_ADDR_SEL_STRAP		1

#if PHY_ADDR_SEL_STRAP == 0
unsigned int virt_phy_addr = 0;
unsigned int port1_phy_addr = 1;
unsigned int port2_phy_addr = 2;
#elif PHY_ADDR_SEL_STRAP == 1
unsigned int virt_phy_addr = 1;
unsigned int port1_phy_addr = 2;
unsigned int port2_phy_addr = 3;
#endif

#define PHY_MODE_CONTROL_STATUS_x	0x11
#define ENERGYON			1 << 1
#define PHY_INTERRUPT_SOURCE_x		0x1D
#define PHY_INTERRUPT_MASK_x		0x1E
#define INT7_MASK			1 << 7
#define INT6_MASK			1 << 6
#define INT5_MASK			1 << 5
#define INT4_MASK			1 << 4
#define INT3_MASK			1 << 3
#define INT2_MASK			1 << 2
#define INT1_MASK			1 << 1


#define LAN9118_PHY_ID			0x00C0001C


#define MII_INTSTS			0x1D

#define MII_INTMSK			0x1E
#define PHY_INTMSK_AN_RCV_		(1 << 1)
#define PHY_INTMSK_PDFAULT_		(1 << 2)
#define PHY_INTMSK_AN_ACK_		(1 << 3)
#define PHY_INTMSK_LNKDOWN_		(1 << 4)
#define PHY_INTMSK_RFAULT_		(1 << 5)
#define PHY_INTMSK_AN_COMP_		(1 << 6)
#define PHY_INTMSK_ENERGYON_		(1 << 7)
#define PHY_INTMSK_DEFAULT_		(PHY_INTMSK_ENERGYON_ | \
					 PHY_INTMSK_AN_COMP_ | \
					 PHY_INTMSK_RFAULT_ | \
					 PHY_INTMSK_LNKDOWN_)

#define ADVERTISE_PAUSE_ALL		(ADVERTISE_PAUSE_CAP | \
					 ADVERTISE_PAUSE_ASYM)

#define LPA_PAUSE_ALL			(LPA_PAUSE_CAP | \
					 LPA_PAUSE_ASYM)

#define MANUAL_FC_MII			0x1a8

/** Switch Fabric Access Registers */
#define SW_CSR_CMD			0x1b0
#define SW_CSR_BUSY			0x80000000
#define SW_CSR_READ			0x40000000
#define SW_CSR_WRITE			0x00000000
#define SW_CSR_AUTOINC			0x20000000
#define SW_CSR_AUTODEC			0x10000000
#define SW_CSR_BYTE3_ENABLE		0x80000
#define SW_CSR_BYTE2_ENABLE		0x40000
#define SW_CSR_BYTE1_ENABLE		0x20000
#define SW_CSR_BYTE0_ENABLE		0x10000
#define SW_CSR_ENABLE_ALL		SW_CSR_BYTE3_ENABLE | SW_CSR_BYTE2_ENABLE | \
					SW_CSR_BYTE1_ENABLE | SW_CSR_BYTE0_ENABLE

#define SW_CSR_DATA			0x1ac

/** Switch Fabric Registers */
#define SW_DEV_ID			0x0
#define SW_RESET			0x1
#define SW_IMR				0x4
#define SW_IMR_MASK_BM			0x40
#define SW_IMR_MASK_SWE			0x20
#define SW_IMR_MASK_MAC_1		0x04
#define SW_IMR_MASK_MAC_2		0x02
#define SW_IMR_MASK_MAC_II		0x01


#define SW_IPR				0x5
#define SW_MANUAL_FC_1			0x1A0
#define SW_MANUAL_FC_2			0x1A4
#define SW_MANUAL_FC_MII		0x1A8
#define SW_MAC_VER_ID_MII		0x400
#define SW_MAC_RX_CFG_MII		0x401


#define SWE_PORT_MIRROR			0x1846
#define SNIFFER_PORT2			0x80
#define SNIFFER_PORT1			0x40
#define SNIFFER_PORT0			0x20
#define MIRRORED_PORT2			0x10
#define MIRRORED_PORT1			0x8
#define MIRRORED_PORT0			0x4
#define RX_MIRRORING_EN			0x2
#define TX_MIRRORING_EN			0x1




#endif				/* __SMSC_LAN9311_H__ */
