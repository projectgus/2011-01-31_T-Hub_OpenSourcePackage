
//Below are the register offsets and bit definitions
//  of the Lan911x memory space
#define RX_DATA_FIFO	    (0x00UL)

#define TX_DATA_FIFO        (0x20UL)
#define		TX_CMD_A_ON_COMP_			(0x80000000UL)
#define		TX_CMD_A_BUF_END_ALGN_		(0x03000000UL)
#define		TX_CMD_A_4_BYTE_ALGN_		(0x00000000UL)
#define		TX_CMD_A_16_BYTE_ALGN_		(0x01000000UL)
#define		TX_CMD_A_32_BYTE_ALGN_		(0x02000000UL)
#define		TX_CMD_A_DATA_OFFSET_		(0x001F0000UL)
#define		TX_CMD_A_FIRST_SEG_			(0x00002000UL)
#define		TX_CMD_A_LAST_SEG_			(0x00001000UL)
#define		TX_CMD_A_BUF_SIZE_			(0x000007FFUL)
#define		TX_CMD_B_PKT_TAG_			(0xFFFF0000UL)
#define		TX_CMD_B_ADD_CRC_DISABLE_	(0x00002000UL)
#define		TX_CMD_B_DISABLE_PADDING_	(0x00001000UL)
#define		TX_CMD_B_PKT_BYTE_LENGTH_	(0x000007FFUL)

#define RX_STATUS_FIFO      (0x40UL)
#define		RX_STS_ES_			(0x00008000UL)
#define		RX_STS_MCAST_		(0x00000400UL)
#define RX_STATUS_FIFO_PEEK (0x44UL)
#define TX_STATUS_FIFO		(0x48UL)
#define TX_STATUS_FIFO_PEEK (0x4CUL)
#define ID_REV              (0x50UL)
#define		ID_REV_CHIP_ID_		(0xFFFF0000UL)	// RO
#define		ID_REV_REV_ID_		(0x0000FFFFUL)	// RO

#define INT_CFG				(0x54UL)
#define		INT_CFG_INT_DEAS_		(0xFF000000UL)	// R/W
#define     INT_CFG_INT_DEAS_CLR_	(0x00004000UL)  // SC
#define     INT_CFG_INT_DEAS_STS_	(0x00002000UL)  // SC
#define		INT_CFG_IRQ_INT_		(0x00001000UL)	// RO
#define		INT_CFG_IRQ_EN_			(0x00000100UL)	// R/W
#define		INT_CFG_IRQ_POL_		(0x00000010UL)	// R/W Not Affected by SW Reset
#define		INT_CFG_IRQ_TYPE_		(0x00000001UL)	// R/W Not Affected by SW Reset

#define INT_STS				(0x58UL)
#define		INT_STS_SW_INT_		(0x80000000UL)	// R/WC
#define		INT_STS_TXSTOP_INT_	(0x02000000UL)	// R/WC
#define		INT_STS_RXSTOP_INT_	(0x01000000UL)	// R/WC
#define		INT_STS_RXDFH_INT_	(0x00800000UL)	// R/WC
#define		INT_STS_RXDF_INT_	(0x00400000UL)	// R/WC
#define		INT_STS_TX_IOC_		(0x00200000UL)	// R/WC
#define		INT_STS_RXD_INT_	(0x00100000UL)	// R/WC
#define		INT_STS_GPT_INT_	(0x00080000UL)	// R/WC
#define		INT_STS_PHY_INT_	(0x00040000UL)	// RO
#define		INT_STS_PME_INT_	(0x00020000UL)	// R/WC
#define		INT_STS_TXSO_		(0x00010000UL)	// R/WC
#define		INT_STS_RWT_		(0x00008000UL)	// R/WC
#define		INT_STS_RXE_		(0x00004000UL)	// R/WC
#define		INT_STS_TXE_		(0x00002000UL)	// R/WC
#define		INT_STS_TDFU_		(0x00000800UL)	// R/WC
#define		INT_STS_TDFO_		(0x00000400UL)	// R/WC
#define		INT_STS_TDFA_		(0x00000200UL)	// R/WC
#define		INT_STS_TSFF_		(0x00000100UL)	// R/WC
#define		INT_STS_TSFL_		(0x00000080UL)	// R/WC
#define		INT_STS_RXDF_		(0x00000040UL)	// R/WC
#define		INT_STS_RDFL_		(0x00000020UL)	// R/WC
#define		INT_STS_RSFF_		(0x00000010UL)	// R/WC
#define		INT_STS_RSFL_		(0x00000008UL)	// R/WC
#define		INT_STS_GPIO2_INT_	(0x00000004UL)	// R/WC
#define		INT_STS_GPIO1_INT_	(0x00000002UL)	// R/WC
#define		INT_STS_GPIO0_INT_	(0x00000001UL)	// R/WC

#define INT_EN				(0x5CUL)
#define		INT_EN_SW_INT_EN_		(0x80000000UL)	// R/W
#define		INT_EN_TXSTOP_INT_EN_	(0x02000000UL)	// R/W
#define		INT_EN_RXSTOP_INT_EN_	(0x01000000UL)	// R/W
#define		INT_EN_RXDFH_INT_EN_	(0x00800000UL)	// R/W
#define		INT_EN_TIOC_INT_EN_		(0x00200000UL)	// R/W
#define		INT_EN_RXD_INT_EN_		(0x00100000UL)	// R/W
#define		INT_EN_GPT_INT_EN_		(0x00080000UL)	// R/W
#define		INT_EN_PHY_INT_EN_		(0x00040000UL)	// R/W
#define		INT_EN_PME_INT_EN_		(0x00020000UL)	// R/W
#define		INT_EN_TXSO_EN_			(0x00010000UL)	// R/W
#define		INT_EN_RWT_EN_			(0x00008000UL)	// R/W
#define		INT_EN_RXE_EN_			(0x00004000UL)	// R/W
#define		INT_EN_TXE_EN_			(0x00002000UL)	// R/W
#define		INT_EN_TDFU_EN_			(0x00000800UL)	// R/W
#define		INT_EN_TDFO_EN_			(0x00000400UL)	// R/W
#define		INT_EN_TDFA_EN_			(0x00000200UL)	// R/W
#define		INT_EN_TSFF_EN_			(0x00000100UL)	// R/W
#define		INT_EN_TSFL_EN_			(0x00000080UL)	// R/W
#define		INT_EN_RXDF_EN_			(0x00000040UL)	// R/W
#define		INT_EN_RDFL_EN_			(0x00000020UL)	// R/W
#define		INT_EN_RSFF_EN_			(0x00000010UL)	// R/W
#define		INT_EN_RSFL_EN_			(0x00000008UL)	// R/W
#define		INT_EN_GPIO2_INT_		(0x00000004UL)	// R/W
#define		INT_EN_GPIO1_INT_		(0x00000002UL)	// R/W
#define		INT_EN_GPIO0_INT_		(0x00000001UL)	// R/W

#define BYTE_TEST				(0x64UL)
#define FIFO_INT				(0x68UL)
#define		FIFO_INT_TX_AVAIL_LEVEL_	(0xFF000000UL)	// R/W
#define		FIFO_INT_TX_STS_LEVEL_		(0x00FF0000UL)	// R/W
#define		FIFO_INT_RX_AVAIL_LEVEL_	(0x0000FF00UL)	// R/W
#define		FIFO_INT_RX_STS_LEVEL_		(0x000000FFUL)	// R/W

#define RX_CFG					(0x6CUL)
#define		RX_CFG_RX_END_ALGN_		(0xC0000000UL)	// R/W
#define			RX_CFG_RX_END_ALGN4_		(0x00000000UL)	// R/W
#define			RX_CFG_RX_END_ALGN16_		(0x40000000UL)	// R/W
#define			RX_CFG_RX_END_ALGN32_		(0x80000000UL)	// R/W
#define		RX_CFG_RX_DMA_CNT_		(0x0FFF0000UL)	// R/W
#define		RX_CFG_RX_DUMP_			(0x00008000UL)	// R/W
#define		RX_CFG_RXDOFF_			(0x00001F00UL)	// R/W

#define TX_CFG					(0x70UL)
#define		TX_CFG_TXS_DUMP_		(0x00008000UL)	// Self Clearing
#define		TX_CFG_TXD_DUMP_		(0x00004000UL)	// Self Clearing
#define		TX_CFG_TXSAO_			(0x00000004UL)	// R/W
#define		TX_CFG_TX_ON_			(0x00000002UL)	// R/W
#define		TX_CFG_STOP_TX_			(0x00000001UL)	// Self Clearing

#define HW_CFG					(0x74UL)
#define		HW_CFG_TTM_				(0x00200000UL)	// R/W
#define		HW_CFG_SF_				(0x00100000UL)	// R/W
#define		HW_CFG_TX_FIF_SZ_		(0x000F0000UL)	// R/W
#define		HW_CFG_TR_				(0x00003000UL)	// R/W
#define     HW_CFG_PHY_CLK_SEL_		(0x00000060UL)  // R/W //only available on 115/117
#define         HW_CFG_PHY_CLK_SEL_INT_PHY_	(0x00000000UL) //R/W //only available on 115/117
#define         HW_CFG_PHY_CLK_SEL_EXT_PHY_	(0x00000020UL) //R/W //only available on 115/117
#define         HW_CFG_PHY_CLK_SEL_CLK_DIS_	(0x00000040UL) //R/W //only available on 115/117
#define     HW_CFG_SMI_SEL_			(0x00000010UL)  // R/W //only available on 115/117
#define     HW_CFG_EXT_PHY_DET_		(0x00000008UL)  // RO  //only available on 115/117
#define     HW_CFG_EXT_PHY_EN_		(0x00000004UL)  // R/W //only available on 115/117
#define		HW_CFG_32_16_BIT_MODE_	(0x00000004UL)	// RO  //only available on 116/118
#define     HW_CFG_SRST_TO_			(0x00000002UL)  // RO  //only available on 115/117
#define		HW_CFG_SRST_			(0x00000001UL)	// Self Clearing

#define RX_DP_CTRL				(0x78UL)
#define		RX_DP_CTRL_RX_FFWD_		(0x80000000UL)	// RO

#define RX_FIFO_INF				(0x7CUL)
#define		RX_FIFO_INF_RXSUSED_	(0x00FF0000UL)	// RO
#define		RX_FIFO_INF_RXDUSED_	(0x0000FFFFUL)	// RO

#define TX_FIFO_INF				(0x80UL)
#define		TX_FIFO_INF_TSUSED_		(0x00FF0000UL)  // RO
#define		TX_FIFO_INF_TDFREE_		(0x0000FFFFUL)	// RO

#define PMT_CTRL				(0x84UL)
#define		PMT_CTRL_PM_MODE_			(0x00003000UL)	// Self Clearing
#define	        PMT_CTRL_PM_MODE_D0_	(0x00000000UL)  // Self Clearing
#define         PMT_CTRL_PM_MODE_D1_	(0x00001000UL)  // Self Clearing
#define         PMT_CTRL_PM_MODE_D2_	(0x00002000UL)  // Self Clearing
#define         PMT_CTRL_PM_MODE_D3_	(0x00003000UL)  // Self Clearing
#define		PMT_CTRL_PHY_RST_			(0x00000400UL)	// Self Clearing
#define		PMT_CTRL_WOL_EN_			(0x00000200UL)	// R/W
#define		PMT_CTRL_ED_EN_				(0x00000100UL)	// R/W
#define		PMT_CTRL_PME_TYPE_			(0x00000040UL)	// R/W Not Affected by SW Reset
#define		PMT_CTRL_WUPS_				(0x00000030UL)	// R/WC
#define			PMT_CTRL_WUPS_NOWAKE_		(0x00000000UL)	// R/WC
#define			PMT_CTRL_WUPS_ED_			(0x00000010UL)	// R/WC
#define			PMT_CTRL_WUPS_WOL_			(0x00000020UL)	// R/WC
#define			PMT_CTRL_WUPS_MULTI_		(0x00000030UL)	// R/WC
#define		PMT_CTRL_PME_IND_		(0x00000008UL)	// R/W
#define		PMT_CTRL_PME_POL_		(0x00000004UL)	// R/W
#define		PMT_CTRL_PME_EN_		(0x00000002UL)	// R/W Not Affected by SW Reset
#define		PMT_CTRL_READY_			(0x00000001UL)	// RO

#define GPIO_CFG				(0x88UL)
#define		GPIO_CFG_LED3_EN_		(0x40000000UL)	// R/W
#define		GPIO_CFG_LED2_EN_		(0x20000000UL)	// R/W
#define		GPIO_CFG_LED1_EN_		(0x10000000UL)	// R/W
#define		GPIO_CFG_GPIO2_INT_POL_	(0x04000000UL)	// R/W
#define		GPIO_CFG_GPIO1_INT_POL_	(0x02000000UL)	// R/W
#define		GPIO_CFG_GPIO0_INT_POL_	(0x01000000UL)	// R/W
#define		GPIO_CFG_EEPR_EN_		(0x00700000UL)	// R/W
#define		GPIO_CFG_GPIOBUF2_		(0x00040000UL)	// R/W
#define		GPIO_CFG_GPIOBUF1_		(0x00020000UL)	// R/W
#define		GPIO_CFG_GPIOBUF0_		(0x00010000UL)	// R/W
#define		GPIO_CFG_GPIODIR2_		(0x00000400UL)	// R/W
#define		GPIO_CFG_GPIODIR1_		(0x00000200UL)	// R/W
#define		GPIO_CFG_GPIODIR0_		(0x00000100UL)	// R/W
#define		GPIO_CFG_GPIOD4_		(0x00000020UL)	// R/W
#define		GPIO_CFG_GPIOD3_		(0x00000010UL)	// R/W
#define		GPIO_CFG_GPIOD2_		(0x00000004UL)	// R/W
#define		GPIO_CFG_GPIOD1_		(0x00000002UL)	// R/W
#define		GPIO_CFG_GPIOD0_		(0x00000001UL)	// R/W

#define GPT_CFG					(0x8CUL)
#define		GPT_CFG_TIMER_EN_		(0x20000000UL)	// R/W
#define		GPT_CFG_GPT_LOAD_		(0x0000FFFFUL)	// R/W

#define GPT_CNT					(0x90UL)
#define		GPT_CNT_GPT_CNT_		(0x0000FFFFUL)	// RO

#define ENDIAN					(0x98UL)
#define FREE_RUN				(0x9CUL)
#define RX_DROP					(0xA0UL)
#define MAC_CSR_CMD				(0xA4UL)
#define		MAC_CSR_CMD_CSR_BUSY_	(0x80000000UL)	// Self Clearing
#define		MAC_CSR_CMD_R_NOT_W_	(0x40000000UL)	// R/W
#define		MAC_CSR_CMD_CSR_ADDR_	(0x000000FFUL)	// R/W

#define MAC_CSR_DATA			(0xA8UL)
#define AFC_CFG					(0xACUL)
#define		AFC_CFG_AFC_HI_			(0x00FF0000UL)	// R/W
#define		AFC_CFG_AFC_LO_			(0x0000FF00UL)	// R/W
#define		AFC_CFG_BACK_DUR_		(0x000000F0UL)	// R/W
#define		AFC_CFG_FCMULT_			(0x00000008UL)	// R/W
#define		AFC_CFG_FCBRD_			(0x00000004UL)	// R/W
#define		AFC_CFG_FCADD_			(0x00000002UL)	// R/W
#define		AFC_CFG_FCANY_			(0x00000001UL)	// R/W

#define E2P_CMD					(0xB0UL)
#define		E2P_CMD_EPC_BUSY_		(0x80000000UL)	// Self Clearing
#define		E2P_CMD_EPC_CMD_		(0x70000000UL)	// R/W
#define			E2P_CMD_EPC_CMD_READ_	(0x00000000UL)	// R/W
#define			E2P_CMD_EPC_CMD_EWDS_	(0x10000000UL)	// R/W
#define			E2P_CMD_EPC_CMD_EWEN_	(0x20000000UL)	// R/W
#define			E2P_CMD_EPC_CMD_WRITE_	(0x30000000UL)	// R/W
#define			E2P_CMD_EPC_CMD_WRAL_	(0x40000000UL)	// R/W
#define			E2P_CMD_EPC_CMD_ERASE_	(0x50000000UL)	// R/W
#define			E2P_CMD_EPC_CMD_ERAL_	(0x60000000UL)	// R/W
#define			E2P_CMD_EPC_CMD_RELOAD_	(0x70000000UL)  // R/W
#define		E2P_CMD_EPC_TIMEOUT_	(0x00000200UL)	// R
#define		E2P_CMD_MAC_ADDR_LOADED_	(0x00000100UL)	// RO
#define		E2P_CMD_EPC_ADDR_		(0x000000FFUL)	// R/W



/*
****************************************************************************
****************************************************************************
*	MAC Control and Status Register (Indirect Address)
*	Offset (through the MAC_CSR CMD and DATA port)
****************************************************************************
****************************************************************************
*
*/
#define MAC_CR				(0x01UL)	// R/W

/* MAC_CR - MAC Control Register */
#define MAC_CR_RXALL_		(0x80000000UL)
#define MAC_CR_HBDIS_		(0x10000000UL)
#define MAC_CR_RCVOWN_		(0x00800000UL)
#define MAC_CR_LOOPBK_		(0x00200000UL)
#define MAC_CR_FDPX_		(0x00100000UL)
#define MAC_CR_MCPAS_		(0x00080000UL)
#define MAC_CR_PRMS_		(0x00040000UL)
#define MAC_CR_INVFILT_		(0x00020000UL)
#define MAC_CR_PASSBAD_		(0x00010000UL)
#define MAC_CR_HFILT_		(0x00008000UL)
#define MAC_CR_HPFILT_		(0x00002000UL)
#define MAC_CR_LCOLL_		(0x00001000UL)
#define MAC_CR_BCAST_		(0x00000800UL)
#define MAC_CR_DISRTY_		(0x00000400UL)
#define MAC_CR_PADSTR_		(0x00000100UL)
#define MAC_CR_BOLMT_MASK_	(0x000000C0UL)
#define MAC_CR_DFCHK_		(0x00000020UL)
#define MAC_CR_TXEN_		(0x00000008UL)
#define MAC_CR_RXEN_		(0x00000004UL)

#define ADDRH				(0x02UL)	// R/W mask 0x0000FFFFUL
#define ADDRL				(0x03UL)	// R/W mask 0xFFFFFFFFUL
#define HASHH				(0x04UL)	// R/W
#define HASHL				(0x05UL)	// R/W

#define MII_ACC				(0x06UL)	// R/W
#define MII_ACC_PHY_ADDR_	(0x0000F800UL)
#define MII_ACC_MIIRINDA_	(0x000007C0UL)
#define MII_ACC_MII_WRITE_	(0x00000002UL)
#define MII_ACC_MII_BUSY_	(0x00000001UL)

#define MII_DATA			(0x07UL)	// R/W mask 0x0000FFFFUL

#define FLOW				(0x08UL)	// R/W
#define FLOW_FCPT_			(0xFFFF0000UL)
#define FLOW_FCPASS_		(0x00000004UL)
#define FLOW_FCEN_			(0x00000002UL)
#define FLOW_FCBSY_			(0x00000001UL)

#define VLAN1				(0x09UL)	// R/W mask 0x0000FFFFUL
#define VLAN2				(0x0AUL)	// R/W mask 0x0000FFFFUL

#define WUFF				(0x0BUL)	// WO

#define WUCSR				(0x0CUL)	// R/W
#define WUCSR_GUE_			(0x00000200UL)
#define WUCSR_WUFR_			(0x00000040UL)
#define WUCSR_MPR_			(0x00000020UL)
#define WUCSR_WAKE_EN_		(0x00000004UL)
#define WUCSR_MPEN_			(0x00000002UL)



/*
****************************************************************************
*	Chip Specific MII Defines
****************************************************************************
*
*	Phy register offsets and bit definitions
*
*/
#define LAN9118_PHY_ID	(0x00C0001C)

#define PHY_BCR		((DWORD)0U)
#define PHY_BCR_RESET_					((WORD)0x8000U)
#define PHY_BCR_SPEED_SELECT_		((WORD)0x2000U)
#define PHY_BCR_AUTO_NEG_ENABLE_	((WORD)0x1000U)
#define PHY_BCR_RESTART_AUTO_NEG_	((WORD)0x0200U)
#define PHY_BCR_DUPLEX_MODE_		((WORD)0x0100U)

#define PHY_BSR		((DWORD)1U)
#define PHY_BSR_LINK_STATUS_	((WORD)0x0004U)
#define PHY_BSR_REMOTE_FAULT_	((WORD)0x0010U)
#define PHY_BSR_AUTO_NEG_COMP_	((WORD)0x0020U)

#define PHY_ID_1	((DWORD)2U)
#define PHY_ID_2	((DWORD)3U)

#define PHY_ANEG_ADV    ((DWORD)4U)
#define PHY_ANEG_ADV_PAUSE_ ((WORD)0x0C00)
#define PHY_ANEG_ADV_ASYMP_	((WORD)0x0800)
#define PHY_ANEG_ADV_SYMP_	((WORD)0x0400)
#define PHY_ANEG_ADV_10H_	((WORD)0x020)
#define PHY_ANEG_ADV_10F_	((WORD)0x040)
#define PHY_ANEG_ADV_100H_	((WORD)0x080)
#define PHY_ANEG_ADV_100F_	((WORD)0x100)
#define PHY_ANEG_ADV_SPEED_	((WORD)0x1E0)

#define PHY_ANEG_LPA	((DWORD)5U)
#define PHY_ANEG_LPA_ASYMP_		((WORD)0x0800)
#define PHY_ANEG_LPA_SYMP_		((WORD)0x0400)
#define PHY_ANEG_LPA_100FDX_	((WORD)0x0100)
#define PHY_ANEG_LPA_100HDX_	((WORD)0x0080)
#define PHY_ANEG_LPA_10FDX_		((WORD)0x0040)
#define PHY_ANEG_LPA_10HDX_		((WORD)0x0020)

#define PHY_MODE_CTRL_STS		((DWORD)17)	// Mode Control/Status Register
#define MODE_CTRL_STS_EDPWRDOWN_	((WORD)0x2000U)
#define MODE_CTRL_STS_ENERGYON_		((WORD)0x0060U)

#define PHY_SPECIAL_MODE		((DWORD)18)	// Special Mode
#define MODE_OPERATION_MASK_	((WORD)0x00E0U)
#define MODE_OPERATION_100FD_ 	((WORD)0x0002U)

#define PHY_INT_SRC			((DWORD)29)
#define PHY_INT_SRC_ENERGY_ON_			((WORD)0x0080U)
#define PHY_INT_SRC_ANEG_COMP_			((WORD)0x0040U)
#define PHY_INT_SRC_REMOTE_FAULT_		((WORD)0x0020U)
#define PHY_INT_SRC_LINK_DOWN_			((WORD)0x0010U)

#define PHY_INT_MASK		((DWORD)30)
#define PHY_INT_MASK_ENERGY_ON_		((WORD)0x0080U)
#define PHY_INT_MASK_ANEG_COMP_		((WORD)0x0040U)
#define PHY_INT_MASK_REMOTE_FAULT_	((WORD)0x0020U)
#define PHY_INT_MASK_LINK_DOWN_		((WORD)0x0010U)

#define PHY_SPECIAL			((DWORD)31)
#define PHY_SPECIAL_SPD_	((WORD)0x001CU)
#define PHY_SPECIAL_SPD_10HALF_		((WORD)0x0004U)
#define PHY_SPECIAL_SPD_10FULL_		((WORD)0x0014U)
#define PHY_SPECIAL_SPD_100HALF_	((WORD)0x0008U)
#define PHY_SPECIAL_SPD_100FULL_	((WORD)0x0018U)



#define LINK_OFF				(0x00UL)
#define LINK_SPEED_10HD			(0x01UL)
#define LINK_SPEED_10FD			(0x02UL)
#define LINK_SPEED_100HD		(0x04UL)
#define LINK_SPEED_100FD		(0x08UL)
#define LINK_SYMMETRIC_PAUSE	(0x10UL)
#define LINK_ASYMMETRIC_PAUSE	(0x20UL)
#define LINK_AUTO_NEGOTIATE		(0x40UL)

#define E2P_DATA			(0xB4UL)
#define E2P_DATA_EEPROM_DATA_	(0x000000FFUL)	// R/W
//end of lan register offsets and bit definitions
#define LAN_REGISTER_EXTENT		(0x00000100UL)


void Mac_Initialize(PPRIVATE_DATA privateData);
static BOOLEAN MacNotBusy(PPRIVATE_DATA privateData);
DWORD Mac_GetRegDW(PPRIVATE_DATA privateData,DWORD dwRegOffset);
void Mac_SetRegDW(PPRIVATE_DATA privateData,DWORD dwRegOffset,DWORD dwVal);


BOOLEAN Phy_Initialize(
				   PPRIVATE_DATA privateData,
				   DWORD dwPhyAddr);
void Phy_SetLink(PPRIVATE_DATA privateData);
WORD Phy_GetRegW(
			  PPRIVATE_DATA privateData,
			  DWORD dwRegIndex);
void Phy_SetRegW(
			  PPRIVATE_DATA privateData,
			  DWORD dwRegIndex,
			  WORD wVal);
void Phy_UpdateLinkMode(
				    PPRIVATE_DATA privateData);
void Phy_GetLinkMode(
				 PPRIVATE_DATA privateData);
void Phy_CheckLink(unsigned long ptr);

void Tx_Initialize(
			    PPRIVATE_DATA privateData);
static void Tx_WriteFifo(
					DWORD dwLanBase,
					DWORD *pdwBuf,
					DWORD dwDwordCount);
static DWORD Tx_GetTxStatusCount(
						   PPRIVATE_DATA privateData);
static DWORD Tx_CompleteTx(
					  PPRIVATE_DATA privateData);
void Tx_SendSkb(
			 PPRIVATE_DATA privateData,
			 struct sk_buff *skb);
BOOLEAN Tx_HandleInterrupt(
					  PPRIVATE_DATA privateData,DWORD dwIntSts);

void Tx_UpdateTxCounters(
					PPRIVATE_DATA privateData);


void Rx_Initialize(
			    PPRIVATE_DATA privateData);
static void Rx_ReadFifo(
				    DWORD dwLanBase,
				    DWORD *pdwBuf,
				    DWORD dwDwordCount);
static void Rx_HandOffSkb(
					 PPRIVATE_DATA privateData,
					 struct sk_buff *skb);
static DWORD Rx_PopRxStatus(
					   PPRIVATE_DATA privateData);
void Rx_CountErrors(PPRIVATE_DATA privateData,DWORD dwRxStatus);
void Rx_FastForward(PPRIVATE_DATA privateData,DWORD dwDwordCount);
void Rx_ProcessPackets(PPRIVATE_DATA privateData);

void Rx_ProcessPacketsTasklet(unsigned long data);
BOOLEAN Rx_HandleInterrupt(
					  PPRIVATE_DATA privateData,
					  DWORD dwIntSts);

static DWORD Hash(BYTE addr[6]);
void Rx_SetMulticastList(
					struct net_device *dev);

#ifdef USE_LED1_WORK_AROUND
extern volatile DWORD g_GpioSetting;
volatile DWORD g_GpioSettingOriginal;
#endif

BOOLEAN Lan_Initialize( PPRIVATE_DATA privateData,DWORD dwIntCfg);

static DWORD Lan_GetRegDW(DWORD dwOffset);
static void Lan_SetRegDW(DWORD dwOffset,DWORD dwVal);
static void Lan_ClrBitsDW(DWORD dwOffset,DWORD dwBits);
static void Lan_SetBitsDW(DWORD dwOffset,DWORD dwBits);

