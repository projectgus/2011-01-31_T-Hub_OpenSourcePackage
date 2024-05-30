//#define IOMUXC_BASE_ADDR 0x43fac000

#define INT_OBS (0x0)
#define INT_OBS_DSP (0x4)
#define GP_REG (0x8)
#define SW_MUX_CTL_CSPI3_MISO_CSPI3_SCLK_CSPI3_SPI_RDY_TTM_PAD (0xc)
#define SW_MUX_CTL_ATA_RESET_B_CE_CONTROL_CLKSS_CSPI3_MOSI (0x10)
#define SW_MUX_CTL_ATA_CS1_ATA_DIOR_ATA_DIOW_ATA_DMACK (0x14)
#define SW_MUX_CTL_SD1_DATA1_SD1_DATA2_SD1_DATA3_ATA_CS0 (0x18)
#define SW_MUX_CTL_D3_SPL_SD1_CMD_SD1_CLK_SD1_DATA0 (0x1c)
#define SW_MUX_CTL_VSYNC3_CONTRAST_D3_REV_D3_CLS (0x20)
#define SW_MUX_CTL_SER_RS_PAR_RS_WRITE_READ (0x24)
#define SW_MUX_CTL_SD_D_IO_SD_D_CLK_LCS0_LCS1 (0x28)
#define SW_MUX_CTL_HSYNC_FPSHIFT_DRDY0_SD_D_I (0x2c)
#define SW_MUX_CTL_LD15_LD16_LD17_VSYNC0 (0x30)
#define SW_MUX_CTL_LD11_LD12_LD13_LD14 (0x34)
#define SW_MUX_CTL_LD7_LD8_LD9_LD10 (0x38)
#define SW_MUX_CTL_LD3_LD4_LD5_LD6 (0x3c)
#define SW_MUX_CTL_USBH2_DATA1_LD0_LD1_LD2 (0x40)
#define SW_MUX_CTL_USBH2_DIR_USBH2_STP_USBH2_NXT_USBH2_DATA0 (0x44)
#define SW_MUX_CTL_USBOTG_DATA5_USBOTG_DATA6_USBOTG_DATA7_USBH2_CLK (0x48)
#define SW_MUX_CTL_USBOTG_DATA1_USBOTG_DATA2_USBOTG_DATA3_USBOTG_DATA4 (0x4c)
#define SW_MUX_CTL_USBOTG_DIR_USBOTG_STP_USBOTG_NXT_USBOTG_DATA0 (0x50)
#define SW_MUX_CTL_USB_PWR_USB_OC_USB_BYP_USBOTG_CLK (0x54)
#define SW_MUX_CTL_TDO_TRSTB_DE_B_SJC_MOD (0x58)
#define SW_MUX_CTL_RTCK_TCK_TMS_TDI (0x5c)
#define SW_MUX_CTL_KEY_COL4_KEY_COL5_KEY_COL6_KEY_COL7 (0x60)
#define SW_MUX_CTL_KEY_COL0_KEY_COL1_KEY_COL2_KEY_COL3 (0x64)
#define SW_MUX_CTL_KEY_ROW4_KEY_ROW5_KEY_ROW6_KEY_ROW7 (0x68)
#define SW_MUX_CTL_KEY_ROW0_KEY_ROW1_KEY_ROW2_KEY_ROW3 (0x6c)
#define SW_MUX_CTL_TXD2_RTS2_CTS2_BATT_LINE (0x70)
#define SW_MUX_CTL_RI_DTE1_DCD_DTE1_DTR_DCE2_RXD2 (0x74)
#define SW_MUX_CTL_RI_DCE1_DCD_DCE1_DTR_DTE1_DSR_DTE1 (0x78)
#define SW_MUX_CTL_RTS1_CTS1_DTR_DCE1_DSR_DCE1 (0x7c)
#define SW_MUX_CTL_CSPI2_SCLK_CSPI2_SPI_RDY_RXD1_TXD1 (0x80)
#define SW_MUX_CTL_CSPI2_MISO_CSPI2_SS0_CSPI2_SS1_CSPI2_SS2 (0x84)
#define SW_MUX_CTL_CSPI1_SS2_CSPI1_SCLK_CSPI1_SPI_RDY_CSPI2_MOSI (0x88)
#define SW_MUX_CTL_CSPI1_MOSI_CSPI1_MISO_CSPI1_SS0_CSPI1_SS1 (0x8c)
#define SW_MUX_CTL_STXD6_SRXD6_SCK6_SFS6 (0x90)
#define SW_MUX_CTL_STXD5_SRXD5_SCK5_SFS5 (0x94)
#define SW_MUX_CTL_STXD4_SRXD4_SCK4_SFS4 (0x98)
#define SW_MUX_CTL_STXD3_SRXD3_SCK3_SFS3 (0x9c)
#define SW_MUX_CTL_CSI_HSYNC_CSI_PIXCLK_I2C_CLK_I2C_DAT (0xa0)
#define SW_MUX_CTL_CSI_D14_CSI_D15_CSI_MCLK_CSI_VSYNC (0xa4)
#define SW_MUX_CTL_CSI_D10_CSI_D11_CSI_D12_CSI_D13 (0xa8)
#define SW_MUX_CTL_CSI_D6_CSI_D7_CSI_D8_CSI_D9 (0xac)
#define SW_MUX_CTL_M_REQUEST_M_GRANT_CSI_D4_CSI_D5 (0xb0)
#define SW_MUX_CTL_PC_RST_IOIS16_PC_RW_B_PC_POE (0xb4)
#define SW_MUX_CTL_PC_VS1_PC_VS2_PC_BVD1_PC_BVD2 (0xb8)
#define SW_MUX_CTL_PC_CD2_B_PC_WAIT_B_PC_READY_PC_PWRON (0xbc)
#define SW_MUX_CTL_D2_D1_D0_PC_CD1_B (0xc0)
#define SW_MUX_CTL_D6_D5_D4_D3 (0xc4)
#define SW_MUX_CTL_D10_D9_D8_D7 (0xc8)
#define SW_MUX_CTL_D14_D13_D12_D11 (0xcc)
#define SW_MUX_CTL_NFWP_B_NFCE_B_NFRB_D15 (0xd0)
#define SW_MUX_CTL_NFWE_B_NFRE_B_NFALE_NFCLE (0xd4)
#define SW_MUX_CTL_SDQS0_SDQS1_SDQS2_SDQS3 (0xd8)
#define SW_MUX_CTL_SDCKE0_SDCKE1_SDCLK_SDCLK_B (0xdc)
#define SW_MUX_CTL_RW_RAS_CAS_SDWE (0xe0)
#define SW_MUX_CTL_CS5_ECB_LBA_BCLK (0xe4)
#define SW_MUX_CTL_CS1_CS2_CS3_CS4 (0xe8)
#define SW_MUX_CTL_EB0_EB1_OE_CS0 (0xec)
#define SW_MUX_CTL_DQM0_DQM1_DQM2_DQM3 (0xf0)
#define SW_MUX_CTL_SD28_SD29_SD30_SD31 (0xf4)
#define SW_MUX_CTL_SD24_SD25_SD26_SD27 (0xf8)
#define SW_MUX_CTL_SD20_SD21_SD22_SD23 (0xfc)
#define SW_MUX_CTL_SD16_SD17_SD18_SD19 (0x100)
#define SW_MUX_CTL_SD12_SD13_SD14_SD15 (0x104)
#define SW_MUX_CTL_SD8_SD9_SD10_SD11 (0x108)
#define SW_MUX_CTL_SD4_SD5_SD6_SD7 (0x10c)
#define SW_MUX_CTL_SD0_SD1_SD2_SD3 (0x110)
#define SW_MUX_CTL_A24_A25_SDBA1_SDBA0 (0x114)
#define SW_MUX_CTL_A20_A21_A22_A23 (0x118)
#define SW_MUX_CTL_A16_A17_A18_A19 (0x11c)
#define SW_MUX_CTL_A12_A13_A14_A15 (0x120)
#define SW_MUX_CTL_A9_A10_MA10_A11 (0x124)
#define SW_MUX_CTL_A5_A6_A7_A8 (0x128)
#define SW_MUX_CTL_A1_A2_A3_A4 (0x12c)
#define SW_MUX_CTL_DVFS1_VPG0_VPG1_A0 (0x130)
#define SW_MUX_CTL_CKIL_POWER_FAIL_VSTBY_DVFS0 (0x134)
#define SW_MUX_CTL_BOOT_MODE1_BOOT_MODE2_BOOT_MODE3_BOOT_MODE4 (0x138)
#define SW_MUX_CTL_RESET_IN_B_POR_B_CLKO_BOOT_MODE0 (0x13c)
#define SW_MUX_CTL_STX0_SRX0_SIMPD0_CKIH (0x140)
#define SW_MUX_CTL_GPIO3_1_SCLK0_SRST0_SVEN0 (0x144)
#define SW_MUX_CTL_GPIO1_4_GPIO1_5_GPIO1_6_GPIO3_0 (0x148)
#define SW_MUX_CTL_GPIO1_0_GPIO1_1_GPIO1_2_GPIO1_3 (0x14c)
#define SW_MUX_CTL_CAPTURE_COMPARE_WATCHDOG_RST_PWMO (0x150)

#define SW_PAD_CTL_TTM_PAD__X__X (0x154)
#define SW_PAD_CTL_CSPI3_MISO_CSPI3_SCLK_CSPI3_SPI_RDY (0x158)
#define SW_PAD_CTL_CE_CONTROL_CLKSS_CSPI3_MOSI (0x15c)
#define SW_PAD_CTL_ATA_DIOW_ATA_DMACK_ATA_RESET_B (0x160)
#define SW_PAD_CTL_ATA_CS0_ATA_CS1_ATA_DIOR (0x164)
#define SW_PAD_CTL_SD1_DATA1_SD1_DATA2_SD1_DATA3 (0x168)
#define SW_PAD_CTL_SD1_CMD_SD1_CLK_SD1_DATA0 (0x16c)
#define SW_PAD_CTL_D3_REV_D3_CLS_D3_SPL (0x170)
#define SW_PAD_CTL_READ_VSYNC3_CONTRAST (0x174)
#define SW_PAD_CTL_SER_RS_PAR_RS_WRITE (0x178)
#define SW_PAD_CTL_SD_D_CLK_LCS0_LCS1 (0x17c)
#define SW_PAD_CTL_DRDY0_SD_D_I_SD_D_IO (0x180)
#define SW_PAD_CTL_VSYNC0_HSYNC_FPSHIFT (0x184)
#define SW_PAD_CTL_LD15_LD16_LD17 (0x188)
#define SW_PAD_CTL_LD12_LD13_LD14 (0x18c)
#define SW_PAD_CTL_LD9_LD10_LD11 (0x190)
#define SW_PAD_CTL_LD6_LD7_LD8 (0x194)
#define SW_PAD_CTL_LD3_LD4_LD5 (0x198)
#define SW_PAD_CTL_LD0_LD1_LD2 (0x19c)
#define SW_PAD_CTL_USBH2_NXT_USBH2_DATA0_USBH2_DATA1 (0x1a0)
#define SW_PAD_CTL_USBH2_CLK_USBH2_DIR_USBH2_STP (0x1a4)
#define SW_PAD_CTL_USBOTG_DATA5_USBOTG_DATA6_USBOTG_DATA7 (0x1a8)
#define SW_PAD_CTL_USBOTG_DATA2_USBOTG_DATA3_USBOTG_DATA4 (0x1ac)
#define SW_PAD_CTL_USBOTG_NXT_USBOTG_DATA0_USBOTG_DATA1 (0x1b0)
#define SW_PAD_CTL_USBOTG_CLK_USBOTG_DIR_USBOTG_STP (0x1b4)
#define SW_PAD_CTL_USB_PWR_USB_OC_USB_BYP (0x1b8)
#define SW_PAD_CTL_TRSTB_DE_B_SJC_MOD (0x1bc)
#define SW_PAD_CTL_TMS_TDI_TDO (0x1c0)
#define SW_PAD_CTL_KEY_COL7_RTCK_TCK (0x1c4)
#define SW_PAD_CTL_KEY_COL4_KEY_COL5_KEY_COL6 (0x1c8)
#define SW_PAD_CTL_KEY_COL1_KEY_COL2_KEY_COL3 (0x1cc)
#define SW_PAD_CTL_KEY_ROW6_KEY_ROW7_KEY_COL0 (0x1d0)
#define SW_PAD_CTL_KEY_ROW3_KEY_ROW4_KEY_ROW5 (0x1d4)
#define SW_PAD_CTL_KEY_ROW0_KEY_ROW1_KEY_ROW2 (0x1d8)
#define SW_PAD_CTL_RTS2_CTS2_BATT_LINE (0x1dc)
#define SW_PAD_CTL_DTR_DCE2_RXD2_TXD2 (0x1e0)
#define SW_PAD_CTL_DSR_DTE1_RI_DTE1_DCD_DTE1 (0x1e4)
#define SW_PAD_CTL_RI_DCE1_DCD_DCE1_DTR_DTE1 (0x1e8)
#define SW_PAD_CTL_CTS1_DTR_DCE1_DSR_DCE1 (0x1ec)
#define SW_PAD_CTL_RXD1_TXD1_RTS1 (0x1f0)
#define SW_PAD_CTL_CSPI2_SS2_CSPI2_SCLK_CSPI2_SPI_RDY (0x1f4)
#define SW_PAD_CTL_CSPI2_MISO_CSPI2_SS0_CSPI2_SS1 (0x1f8)
#define SW_PAD_CTL_CSPI1_SCLK_CSPI1_SPI_RDY_CSPI2_MOSI (0x1fc)
#define SW_PAD_CTL_CSPI1_SS0_CSPI1_SS1_CSPI1_SS2 (0x200)
#define SW_PAD_CTL_SFS6_CSPI1_MOSI_CSPI1_MISO (0x204)
#define SW_PAD_CTL_STXD6_SRXD6_SCK6 (0x208)
#define SW_PAD_CTL_SRXD5_SCK5_SFS5 (0x20c)
#define SW_PAD_CTL_SCK4_SFS4_STXD5 (0x210)
#define SW_PAD_CTL_SFS3_STXD4_SRXD4 (0x214)
#define SW_PAD_CTL_STXD3_SRXD3_SCK3 (0x218)
#define SW_PAD_CTL_CSI_PIXCLK_I2C_CLK_I2C_DAT (0x21c)
#define SW_PAD_CTL_CSI_MCLK_CSI_VSYNC_CSI_HSYNC (0x220)
#define SW_PAD_CTL_CSI_D13_CSI_D14_CSI_D15 (0x224)
#define SW_PAD_CTL_CSI_D10_CSI_D11_CSI_D12 (0x228)
#define SW_PAD_CTL_CSI_D7_CSI_D8_CSI_D9 (0x22c)
#define SW_PAD_CTL_CSI_D4_CSI_D5_CSI_D6 (0x230)
#define SW_PAD_CTL_PC_POE_M_REQUEST_M_GRANT (0x234)
#define SW_PAD_CTL_PC_RST_IOIS16_PC_RW_B (0x238)
#define SW_PAD_CTL_PC_VS2_PC_BVD1_PC_BVD2 (0x23c)
#define SW_PAD_CTL_PC_READY_PC_PWRON_PC_VS1 (0x240)
#define SW_PAD_CTL_PC_CD1_B_PC_CD2_B_PC_WAIT_B (0x244)
#define SW_PAD_CTL_D2_D1_D0 (0x248)
#define SW_PAD_CTL_D5_D4_D3 (0x24c)
#define SW_PAD_CTL_D8_D7_D6 (0x250)
#define SW_PAD_CTL_D11_D10_D9 (0x254)
#define SW_PAD_CTL_D14_D13_D12 (0x258)
#define SW_PAD_CTL_NFCE_B_NFRB_D15 (0x25c)
#define SW_PAD_CTL_NFALE_NFCLE_NFWP_B (0x260)
#define SW_PAD_CTL_SDQS3_NFWE_B_NFRE_B (0x264)
#define SW_PAD_CTL_SDQS0_SDQS1_SDQS2 (0x268)
#define SW_PAD_CTL_SDCKE1_SDCLK_SDCLK_B (0x26c)
#define SW_PAD_CTL_CAS_SDWE_SDCKE0 (0x270)
#define SW_PAD_CTL_BCLK_RW_RAS (0x274)
#define SW_PAD_CTL_CS5_ECB_LBA (0x278)
#define SW_PAD_CTL_CS2_CS3_CS4 (0x27c)
#define SW_PAD_CTL_OE_CS0_CS1 (0x280)
#define SW_PAD_CTL_DQM3_EB0_EB1 (0x284)
#define SW_PAD_CTL_DQM0_DQM1_DQM2 (0x288)
#define SW_PAD_CTL_SD29_SD30_SD31 (0x28c)
#define SW_PAD_CTL_SD26_SD27_SD28 (0x290)
#define SW_PAD_CTL_SD23_SD24_SD25 (0x294)
#define SW_PAD_CTL_SD20_SD21_SD22 (0x298)
#define SW_PAD_CTL_SD17_SD18_SD19 (0x29c)
#define SW_PAD_CTL_SD14_SD15_SD16 (0x2a0)
#define SW_PAD_CTL_SD11_SD12_SD13 (0x2a4)
#define SW_PAD_CTL_SD8_SD9_SD10 (0x2a8)
#define SW_PAD_CTL_SD5_SD6_SD7 (0x2ac)
#define SW_PAD_CTL_SD2_SD3_SD4 (0x2b0)
#define SW_PAD_CTL_SDBA0_SD0_SD1 (0x2b4)
#define SW_PAD_CTL_A24_A25_SDBA1 (0x2b8)
#define SW_PAD_CTL_A21_A22_A23 (0x2bc)
#define SW_PAD_CTL_A18_A19_A20 (0x2c0)
#define SW_PAD_CTL_A15_A16_A17 (0x2c4)
#define SW_PAD_CTL_A12_A13_A14 (0x2c8)
#define SW_PAD_CTL_A10_MA10_A11 (0x2cc)
#define SW_PAD_CTL_A7_A8_A9 (0x2d0)
#define SW_PAD_CTL_A4_A5_A6 (0x2d4)
#define SW_PAD_CTL_A1_A2_A3 (0x2d8)
#define SW_PAD_CTL_VPG0_VPG1_A0 (0x2dc)
#define SW_PAD_CTL_VSTBY_DVFS0_DVFS1 (0x2e0)
#define SW_PAD_CTL_BOOT_MODE4_CKIL_POWER_FAIL (0x2e4)
#define SW_PAD_CTL_BOOT_MODE1_BOOT_MODE2_BOOT_MODE3 (0x2e8)
#define SW_PAD_CTL_POR_B_CLKO_BOOT_MODE0 (0x2ec)
#define SW_PAD_CTL_SIMPD0_CKIH_RESET_IN_B (0x2f0)
#define SW_PAD_CTL_SVEN0_STX0_SRX0 (0x2f4)
#define SW_PAD_CTL_GPIO3_1_SCLK0_SRST0 (0x2f8)
#define SW_PAD_CTL_GPIO1_5_GPIO1_6_GPIO3_0 (0x2fc)
#define SW_PAD_CTL_GPIO1_2_GPIO1_3_GPIO1_4 (0x300)
#define SW_PAD_CTL_PWMO_GPIO1_0_GPIO1_1 (0x304)
#define SW_PAD_CTL_CAPTURE_COMPARE_WATCHDOG_RST (0x308)