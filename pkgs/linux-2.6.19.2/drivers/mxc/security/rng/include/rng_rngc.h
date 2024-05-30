/*
 * Copyright 2005-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#ifndef RNG_RNGC_H
#define RNG_RNGC_H

#define RNGC_VERSION_MAJOR3 3

/*! RNGC Version ID Register R/W */
#define RNGC_VERSION_ID 0x0000
/*! RNGC Command Register R/W */
#define RNGC_COMMAND 0x0004
/*! RNGC Control Register R/W */
#define RNGC_CONTROL 0x0008
/*! RNGC Status Register R */
#define RNGC_STATUS 0x000C
/*! RNGC Error Status Register R */
#define RNGC_ERROR 0x0010
/*! RNGC FIFO Register W */
#define RNGC_FIFO 0x0014
/*! RNGC Verification Control Register1 R/W */
#define RNGC_VERIFICATION_CONTROL 0x0020
/*! RNGC Oscillator Counter Control Register1 R/W */
#define RNGC_OSCILLATOR_CONTROL_COUNTER 0x0028
/*! RNGC Oscillator Counter Register1 R */
#define RNGC_OSC_COUNTER 0x002C
/*! RNGC Oscillator Counter Status Register1 R */
#define RNGC_OSC_COUNTER_STATUS 0x0030
/*! @} */

/*! @defgroup rngcveridreg RNGC Version ID Register Definitions 
 *  @ingroup RNG
 */
/*! @addtogroup rngcveridreg */
/*! @{ */
/*! These bits are unimplemented or reserved */
#define RNGC_VERID_ZEROS_MASK          0x0f000000
/*! Mask for RNG TYPE */
#define RNGC_VERID_RNG_TYPE_MASK       0xf0000000
/*! Shift to make RNG TYPE be LSB */
#define RNGC_VERID_RNG_TYPE_SHIFT      28
/*! Mask for RNG Chip Version */
#define RNGC_VERID_CHIP_VERSION_MASK   0x00ff0000
/*! Shift to make RNG Chip version be LSB */
#define RNGC_VERID_CHIP_VERSION_SHIFT  16
/*! Mask for RNG Major Version */
#define RNGC_VERID_VERSION_MAJOR_MASK  0x0000ff00
/*! Shift to make RNG Major version be LSB */
#define RNGC_VERID_VERSION_MAJOR_SHIFT 8
/*! Mask for RNG Minor Version */
#define RNGC_VERID_VERSION_MINOR_MASK  0x000000ff
/*! Shift to make RNG Minor version be LSB */
#define RNGC_VERID_VERSION_MINOR_SHIFT 0
/*! @} */

/*! @defgroup rngccommandreg RNGC Command Register Definitions 
 *  @ingroup RNG
 */
/*! @addtogroup rngccommandreg */
/*! @{ */
/*! These bits are unimplemented or reserved. */
#define RNGC_COMMAND_ZEROS_MASK      0xffffff8c
/*! Perform a software reset of the RNGC. */
#define RNGC_COMMAND_SOFTWARE_RESET          0x00000040
/*! Clear error from Error Status register (and interrupt). */
#define RNGC_COMMAND_CLEAR_ERROR             0x00000020
/*! Clear interrupt & status. */
#define RNGC_COMMAND_CLEAR_INTERRUPT         0x00000010
/*! Start RNGC seed generation. */
#define RNGC_COMMAND_SEED                    0x00000002
/*! Perform a self test of (and reset) the RNGC. */
#define RNGC_COMMAND_SELF_TEST               0x00000001
/*! @} */

/*! @defgroup rngccontrolreg RNGC Control Register Definitions 
 *  @ingroup RNG
 */
/*! @addtogroup rngccontrolreg */
/*! @{ */
/*! These bits are unimplemented or reserved */
#define RNGC_CONTROL_ZEROS_MASK       0xfffffc88
/*! Allow access to verification registers. */
#define RNGC_CONTROL_CTL_ACC          0x00000200
/*! Put RNGC into deterministic verifcation mode. */
#define RNGC_CONTROL_VERIF_MODE       0x00000100
/*! Prevent RNGC from generating interrupts caused by errors. */
#define RNGC_CONTROL_MASK_ERROR       0x00000040
/*! Masking all interrupts */
#define RNGC_CONTROL_MASK_INTERRUPTS 0x00000060

/*!
 * Prevent RNGC from generating interrupts after Seed Done or Self Test Mode
 * completion.
 */
#define RNGC_CONTROL_MASK_DONE        0x00000020
/*! Allow RNGC to generate a new seed whenever it is needed. */
#define RNGC_CONTROL_AUTO_SEED        0x00000010
/*! Set FIFO Underflow Response.*/
#define RNGC_CONTROL_FIFO_UFLOW_MASK  0x00000007
/*! Shift value to make FIFO Underflow Response be LSB. */
#define RNGC_CONTROL_FIFO_UFLOW_SHIFT 0

/*! @} */

/*! @{  */
/*! FIFO Underflow should cause ... */
#define RNGC_CONTROL_FIFO_UFLOW_ZEROS_ERROR 0
/*! FIFO Underflow should cause ... */
#define RNGC_CONTROL_FIFO_UFLOW_ZEROS_ERROR2 1
/*! FIFO Underflow should cause ... */
#define RNGC_CONTROL_FIFO_UFLOW_BUS_XFR     2
/*! FIFO Underflow should cause ... */
#define RNGC_CONTROL_FIFO_UFLOW_ZEROS_INTR  3
/*! @} */

/*! @defgroup rngcstatusreg RNGC Status Register Definitions 
 *  @ingroup RNG
 */
/*! @addtogroup rngcstatusreg */
/*! @{ */
/*! Unused or MBZ.  */
#define RNGC_STATUS_ZEROS_MASK              0x003e0080
/*!
 * Statistical tests pass-fail.  Individual bits on indicate failure of a
 * particular test.
 */
#define RNGC_STATUS_STAT_TEST_PF_MASK       0xff000000
/*! Mask to get Statistical PF to be LSB. */
#define RNGC_STATUS_STAT_TEST_PF_SHIFT      24
/*!
 * Self tests pass-fail.  Individual bits on indicate failure of a
 * particular test.
 */
#define RNGC_STATUS_ST_PF_MASK              0x00c00000
/*! Shift value to get Self Test PF field to be LSB. */
#define RNGC_STATUS_ST_PF_SHIFT             22
/*! Error detected in RNGC.  See Error Status register. */
#define RNGC_STATUS_ERROR                   0x00010000
/*! Size of the internal FIFO in 32-bit words. */
#define RNGC_STATUS_FIFO_SIZE_MASK          0x0000f000
/*! Shift value to get FIFO Size to be LSB. */
#define RNGC_STATUS_FIFO_SIZE_SHIFT         12
/*! The level (available data) of the internal FIFO in 32-bit words. */
#define RNGC_STATUS_FIFO_LEVEL_MASK         0x00000f00
/*! Shift value to get FIFO Level to be LSB. */
#define RNGC_STATUS_FIFO_LEVEL_SHIFT        8
/*! A new seed is ready for use. */
#define RNGC_STATUS_NEXT_SEED_DONE          0x00000040
/*! The first seed has been generated. */
#define RNGC_STATUS_SEED_DONE               0x00000020
/*! Self Test has been completed. */
#define RNGC_STATUS_ST_DONE                 0x00000010
/*! Reseed is necessary. */
#define RNGC_STATUS_RESEED                  0x00000008
/*! RNGC is sleeping. */
#define RNGC_STATUS_SLEEP                   0x00000004
/*! RNGC is currently generating numbers, seeding, generating next seed, or
    performing a self test. */
#define RNGC_STATUS_BUSY                    0x00000002
/*! RNGC is in secure state. */
#define RNGC_STATUS_SEC_STATE               0x00000001

/*! @} */

/*! @defgroup rngcerrstatusreg RNGC Error Status Register Definitions 
 *  @ingroup RNG
 */
/*! @addtogroup rngcerrstatusreg */
/*! @{ */
/*! Unused or MBZ. */
#define RNGC_ERROR_STATUS_ZEROS_MASK        0xffffffc0
/*! NO INFORMATION ON THIS BIT! */
#define RNGC_ERROR_STATUS_REG_ERR           0x00000020
/*! Random Compare Error.  Previous number matched the current number. */
#define RNGC_ERROR_STATUS_RAND_ERR          0x00000020
/*! FIFO Underflow.  FIFO was read while empty. */
#define RNGC_ERROR_STATUS_FIFO_ERR          0x00000010
/*! Statistic Error Statistic Test failed for the last seed. */
#define RNGC_ERROR_STATUS_STAT_ERR          0x00000008
/*! Self-test error.  Some self test has failed. */
#define RNGC_ERROR_STATUS_ST_ERR            0x00000004
/*!
 * Oscillator Error.  The oscillator may be broken.  Clear by hard or soft
 * reset.
 */
#define RNGC_ERROR_STATUS_OSC_ERR           0x00000002
/*! LFSR Error.  Clear by hard or soft reset. */
#define RNGC_ERROR_STATUS_LFSR_ERR          0x00000001

/*! @} */

/*! Total address space of the RNGC, in bytes */
#define RNG_ADDRESS_RANGE 0x34

#endif
