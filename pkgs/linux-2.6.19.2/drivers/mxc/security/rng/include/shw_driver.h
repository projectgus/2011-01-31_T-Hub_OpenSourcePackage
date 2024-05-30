/*
 * Copyright 2005-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU Lesser General 
 * Public License.  You may obtain a copy of the GNU Lesser General 
 * Public License Version 2.1 or later at the following locations:
 *
 * http://www.opensource.org/licenses/lgpl-license.html
 * http://www.gnu.org/copyleft/lgpl.html
 */

#ifndef SHW_DRIVER_H
#define SHW_DRIVER_H

/* This is a Linux flag meaning 'compiling kernel code'...  */
#ifndef __KERNEL__
#include <inttypes.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <memory.h>
#include <stdio.h>
#else
#include "../../sahara2/include/portable_os.h"
#endif				/* __KERNEL__ */

#include "../../sahara2/include/fsl_platform.h"

/*! @file shw_driver.h

 * @brief Header file to use the SHW driver.
 *
 * The SHW driver is used in two modes: By a user, from the FSL SHW API in user
 * space, which goes through /dev/fsl_shw to make open(), ioctl(), and close()
 * calls; and by other kernel modules/drivers, which use the FSL SHW API, parts
 * of which are supported directly by the SHW driver.
 *
 * Testing is performed by using the apitest and kernel api test routines
 * developed for the Sahara2 driver.
 *  @ingroup RNG
 */

/*! Perform a security function.  */
#define SHW_IOCTL_REQUEST 21

/*!
 * This is part of the IOCTL request type passed between kernel and user space.
 * It is added to #SHW_IOCTL_REQUEST to generate the actual value.
 */
typedef enum shw_user_request_type {
	SHW_USER_REQ_REGISTER_USER,	/*!< Initialize user-kernel discussion. */
	SHW_USER_REQ_DEREGISTER_USER,	/*!< Terminate user-kernel discussion. */
	SHW_USER_REQ_GET_RESULTS,	/*!< Get information on outstanding
					   results. */
	SHW_USER_REQ_GET_CAPABILITIES,	/*!< Get information on hardware support. */
	SHW_USER_REQ_GET_RANDOM,	/*!< Get random data from RNG. */
	SHW_USER_REQ_ADD_ENTROPY,	/*!< Add entropy to hardware RNG. */
} shw_user_request_t;

/*!****************************************************************************
 * Enumerations
 *****************************************************************************/
/*!
 * Flags for the state of the User Context Object (#fsl_shw_uco_t).
 */
typedef enum fsl_shw_user_ctx_flags {
	FSL_UCO_BLOCKING_MODE = 0x01,	/*!< API will block the caller until operation
					   completes.  The result will be available in the
					   return code.  If this is not set, user will have
					   to get results using #fsl_shw_get_results(). */
	FSL_UCO_CALLBACK_MODE = 0x02,	/*!< User wants callback (at the function
					   specified with #fsl_shw_uco_set_callback()) when
					   the operation completes.  This flag is valid
					   only if #FSL_UCO_BLOCKING_MODE is not set. */
	FSL_UCO_SAVE_DESC_CHAIN = 0x04,	/*!< Do not free descriptor chain after
					   driver (adaptor) finishes */
	FSL_UCO_CALLBACK_SETUP_COMPLETE = 0x08,	/*!< User has made at least one
						   request with callbacks requested, so
						   API is ready to handle others. */

	FSL_UCO_CHAIN_PREPHYSICALIZED = 0x10,	/*!< (virtual) pointer to descriptor
						   chain is completely linked with physical
						   (DMA) addresses, ready for the hardware.
						   This flag should not be used by FSL SHW API
						   programs. */
	FSL_UCO_CONTEXT_CHANGED = 0x20,	/*!< The user has changed the context but
					   the changes have not been copied to the
					   kernel driver. */
	FSL_UCO_USERMODE_USER = 0x40,	/*!< Internal use.  This context belongs to a
					   user-mode API user. */
} fsl_shw_user_ctx_flags_t;

/*!
 * Return code for FSL_SHW library.
 *
 * These codes may be returned from a function call.  In non-blocking mode,
 * they will appear as the status in a Result Object.
 */
/* REQ-FSLSHW-ERR-001 */
typedef enum fsl_shw_return_t {
	FSL_RETURN_OK_S = 0,	/*!< No error.  As a function return code in
				   Non-blocking mode, this may simply mean that
				   the operation was accepted for eventual
				   execution. */
	FSL_RETURN_ERROR_S,	/*!< Failure for non-specific reason. */
	FSL_RETURN_NO_RESOURCE_S,	/*!< Operation failed because some resource was
					   not able to be allocated. */
	FSL_RETURN_BAD_ALGORITHM_S,	/*!< Crypto algorithm unrecognized or
					   improper. */
	FSL_RETURN_BAD_MODE_S,	/*!< Crypto mode unrecognized or improper. */
	FSL_RETURN_BAD_FLAG_S,	/*!< Flag setting unrecognized or
				   inconsistent. */
	FSL_RETURN_BAD_KEY_LENGTH_S,	/*!< Improper or unsupported key length for
					   algorithm. */
	FSL_RETURN_BAD_KEY_PARITY_S,	/*!< Improper parity in a (DES, TDES) key. */
	FSL_RETURN_BAD_DATA_LENGTH_S,	/*!< Improper or unsupported data length for
					   algorithm or internal buffer. */
	FSL_RETURN_AUTH_FAILED_S,	/*!< Authentication failed in
					   authenticate-decrypt operation. */
	FSL_RETURN_MEMORY_ERROR_S,	/*!< A memory error occurred. */
	FSL_RETURN_INTERNAL_ERROR_S	/*!< An error internal to the hardware
					   occurred. */
} fsl_shw_return_t;

/*!
 * Algorithm Identifier.
 *
 * Selection of algorithm will determine how large the block size of the
 * algorithm is.   Context size is the same length unless otherwise specified.
 * Selection of algorithm also affects the allowable key length.
 */
typedef enum fsl_shw_key_alg_t {
	FSL_KEY_ALG_HMAC,	/*!< Key will be used to perform an HMAC.  Key
				   size is 1 to 64 octets.  Block size is 64
				   octets. */
	FSL_KEY_ALG_AES,	/*!< Advanced Encryption Standard (Rijndael).
				   Block size is 16 octets.  Key size is 16
				   octets.  (The single choice of key size is a
				   Sahara platform limitation.) */
	FSL_KEY_ALG_DES,	/*!< Data Encryption Standard.  Block size is
				   8 octets.  Key size is 8 octets. */
	FSL_KEY_ALG_TDES,	/*!< 2- or 3-key Triple DES.  Block size is 8
				   octets.  Key size is 16 octets for 2-key
				   Triple DES, and 24 octets for 3-key. */
	FSL_KEY_ALG_ARC4	/*!< ARC4.  No block size.  Context size is 259
				   octets.  Allowed key size is 1-16 octets.
				   (The choices for key size are a Sahara
				   platform limitation.) */
} fsl_shw_key_alg_t;

/*!
 * Mode selector for Symmetric Ciphers.
 *
 * The selection of mode determines how a cryptographic algorithm will be
 * used to process the plaintext or ciphertext.
 *
 * For all modes which are run block-by-block (that is, all but
 * #FSL_SYM_MODE_STREAM), any partial operations must be performed on a text
 * length which is multiple of the block size.  Except for #FSL_SYM_MODE_CTR,
 * these block-by-block algorithms must also be passed a total number of octets
 * which is a multiple of the block size.
 *
 * In modes which require that the total number of octets of data be a multiple
 * of the block size (#FSL_SYM_MODE_ECB and #FSL_SYM_MODE_CBC), and the user
 * has a total number of octets which are not a multiple of the block size, the
 * user must perform any necessary padding to get to the correct data length.
 */
typedef enum fsl_shw_sym_mode_t {
	/*!
	 * Stream.  There is no associated block size.  Any request to process data
	 * may be of any length.  This mode is only for ARC4 operations, and is
	 * also the only mode used for ARC4.
	 */
	FSL_SYM_MODE_STREAM,

	/*!
	 * Electronic Codebook.  Each block of data is encrypted/decrypted.  The
	 * length of the data stream must be a multiple of the block size.  This
	 * mode may be used for DES, 3DES, and AES.  The block size is determined
	 * by the algorithm.
	 */
	FSL_SYM_MODE_ECB,
	/*!
	 * Cipher-Block Chaining.  Each block of data is encrypted/decrypted and
	 * then "chained" with the previous block by an XOR function.  Requires
	 * context to start the XOR (previous block).  This mode may be used for
	 * DES, 3DES, and AES.  The block size is determined by the algorithm.
	 */
	FSL_SYM_MODE_CBC,
	/*!
	 * Counter.  The counter is encrypted, then XORed with a block of data.
	 * The counter is then incremented (using modulus arithmetic) for the next
	 * block. The final operation may be non-multiple of block size.  This mode
	 * may be used for AES.  The block size is determined by the algorithm.
	 */
	FSL_SYM_MODE_CTR,
} fsl_shw_sym_mode_t;

/*!
 * Algorithm selector for Cryptographic Hash functions.
 *
 * Selection of algorithm determines how large the context and digest will be.
 * Context is the same size as the digest (resulting hash), unless otherwise
 * specified.
 */
typedef enum fsl_shw_hash_alg {
	FSL_HASH_ALG_MD5,	/*!< MD5 algorithm.  Digest is 16 octets. */
	FSL_HASH_ALG_SHA1,	/*!< SHA-1 (aka SHA or SHA-160) algorithm.
				   Digest is 20 octets. */
	FSL_HASH_ALG_SHA224,	/*!< SHA-224 algorithm.  Digest is 28 octets,
				   though context is 32 octets. */
	FSL_HASH_ALG_SHA256	/*!< SHA-256 algorithm.  Digest is 32
				   octets. */
} fsl_shw_hash_alg_t;

/*!
 * The type of Authentication-Cipher function which will be performed.
 */
typedef enum fsl_shw_acc_mode_t {
	/*!
	 * CBC-MAC for Counter.  Requires context and modulus.  Final operation may
	 * be non-multiple of block size.  This mode may be used for AES.
	 */
	FSL_ACC_MODE_CCM,
	/*!
	 * SSL mode.  Not supported.  Combines HMAC and encrypt (or decrypt).
	 * Needs one key object for encryption, another for the HMAC.  The usual
	 * hashing and symmetric encryption algorithms are supported.
	 */
	FSL_ACC_MODE_SSL
} fsl_shw_acc_mode_t;

/* REQ-FSLSHW-PINTFC-COA-HCO-001 */
/*!
 * Flags which control a Hash operation.
 */
typedef enum fsl_shw_hash_ctx_flags {
	FSL_HASH_FLAGS_INIT = 1,	/*!< Context is empty.  Hash is started
					   from scratch, with a message-processed
					   count of zero. */
	FSL_HASH_FLAGS_SAVE = 2,	/*!< Retrieve context from hardware after
					   hashing.  If used with the
					   #FSL_HASH_FLAGS_FINALIZE flag, the final
					   digest value will be saved in the
					   object. */
	FSL_HASH_FLAGS_LOAD = 4,	/*!< Place context into hardware before
					   hashing. */
	FSL_HASH_FLAGS_FINALIZE = 8,	/*!< PAD message and perform final digest
					   operation.  If user message is
					   pre-padded, this flag should not be
					   used. */
} fsl_shw_hash_ctx_flags_t;

/*!
 * Flags which control an HMAC operation.
 *
 * These may be combined by ORing them together.  See #fsl_shw_hmco_set_flags()
 * and #fsl_shw_hmco_clear_flags().
 */
typedef enum fsl_shw_hmac_ctx_flags_t {
	FSL_HMAC_FLAGS_INIT = 1,	/*!< Message context is empty.  HMAC is
					   started from scratch (with key) or from
					   precompute of inner hash, depending on
					   whether
					   #FSL_HMAC_FLAGS_PRECOMPUTES_PRESENT is
					   set. */
	FSL_HMAC_FLAGS_SAVE = 2,	/*!< Retrieve ongoing context from hardware
					   after hashing.  If used with the
					   #FSL_HMAC_FLAGS_FINALIZE flag, the final
					   digest value (HMAC) will be saved in the
					   object. */
	FSL_HMAC_FLAGS_LOAD = 4,	/*!< Place ongoing context into hardware
					   before hashing. */
	FSL_HMAC_FLAGS_FINALIZE = 8,	/*!< PAD message and perform final HMAC
					   operations of inner and outer hashes. */
	FSL_HMAC_FLAGS_PRECOMPUTES_PRESENT = 16	/*!< This means that the context
						   contains precomputed inner and outer
						   hash values. */
} fsl_shw_hmac_ctx_flags_t;

/*!
 * Flags to control use of the #fsl_shw_scco_t.
 *
 * These may be ORed together to get the desired effect.
 * See #fsl_shw_scco_set_flags() and #fsl_shw_scco_clear_flags()
 */
typedef enum fsl_shw_sym_ctx_flags_t {
	/*!
	 * Context is empty.  In ARC4, this means that the S-Box needs to be
	 * generated from the key.  In #FSL_SYM_MODE_CBC mode, this allows an IV of
	 * zero to be specified.  In #FSL_SYM_MODE_CTR mode, it means that an
	 * initial CTR value of zero is desired.
	 */
	FSL_SYM_CTX_INIT = 1,
	/*!
	 * Load context from object into hardware before running cipher.  In
	 * #FSL_SYM_MODE_CTR mode, this would refer to the Counter Value.
	 */
	FSL_SYM_CTX_LOAD = 2,
	/*!
	 * Save context from hardware into object after running cipher.  In
	 * #FSL_SYM_MODE_CTR mode, this would refer to the Counter Value.
	 */
	FSL_SYM_CTX_SAVE = 4,
	/*!
	 * Context (SBox) is to be unwrapped and wrapped on each use.
	 * This flag is unsupported.
	 * */
	FSL_SYM_CTX_PROTECT = 8,
} fsl_shw_sym_ctx_flags_t;

/*!
 * Flags which describe the state of the #fsl_shw_sko_t.
 *
 * These may be ORed together to get the desired effect.
 * See #fsl_shw_sko_set_flags() and #fsl_shw_sko_clear_flags()
 */
typedef enum fsl_shw_key_flags_t {
	FSL_SKO_KEY_IGNORE_PARITY = 1,	/*!< If algorithm is DES or 3DES, do not
					   validate the key parity bits. */
	FSL_SKO_KEY_PRESENT = 2,	/*!< Clear key is present in the object. */
	FSL_SKO_KEY_ESTABLISHED = 4,	/*!< Key has been established for use.  This
					   feature is not available for all
					   platforms, nor for all algorithms and
					   modes. */
	FSL_SKO_USE_SECRET_KEY = 8,	/*!< Use device-unique key.  Not always
					   available. */
} fsl_shw_key_flags_t;

/*!
 * Type of value which is associated with an established key.
 */
typedef uint64_t key_userid_t;

/*!
 * Flags which describe the state of the #fsl_shw_acco_t.
 *
 * The @a FSL_ACCO_CTX_INIT and @a FSL_ACCO_CTX_FINALIZE flags, when used
 * together, provide for a one-shot operation.
 */
typedef enum fsl_shw_auth_ctx_flags_t {
	FSL_ACCO_CTX_INIT = 1,	/*!< Initialize Context(s) */
	FSL_ACCO_CTX_LOAD = 2,	/*!< Load intermediate context(s).
				   This flag is unsupported. */
	FSL_ACCO_CTX_SAVE = 4,	/*!< Save intermediate context(s).
				   This flag is unsupported. */
	FSL_ACCO_CTX_FINALIZE = 8,	/*!< Create MAC during this operation. */
	FSL_ACCO_NIST_CCM = 0x10,	/*!< Formatting of CCM input data is
					   performed by calls to
					   #fsl_shw_ccm_nist_format_ctr_and_iv() and
					   #fsl_shw_ccm_nist_update_ctr_and_iv().  */
} fsl_shw_auth_ctx_flags_t;

/*!
 * The operation which controls the behavior of #fsl_shw_establish_key().
 *
 * These values are passed to #fsl_shw_establish_key().
 */
typedef enum fsl_shw_key_wrap_t {
	FSL_KEY_WRAP_CREATE,	/*!< Generate a key from random values. */
	FSL_KEY_WRAP_ACCEPT,	/*!< Use the provided clear key. */
	FSL_KEY_WRAP_UNWRAP	/*!< Unwrap a previously wrapped key. */
} fsl_shw_key_wrap_t;

/*!
 *  Modulus Selector for CTR modes.
 *
 * The incrementing of the Counter value may be modified by a modulus.  If no
 * modulus is needed or desired for AES, use #FSL_CTR_MOD_128.
 */
typedef enum fsl_shw_ctr_mod {
	FSL_CTR_MOD_8,		/*!< Run counter with modulus of 2^8. */
	FSL_CTR_MOD_16,		/*!< Run counter with modulus of 2^16. */
	FSL_CTR_MOD_24,		/*!< Run counter with modulus of 2^24. */
	FSL_CTR_MOD_32,		/*!< Run counter with modulus of 2^32. */
	FSL_CTR_MOD_40,		/*!< Run counter with modulus of 2^40. */
	FSL_CTR_MOD_48,		/*!< Run counter with modulus of 2^48. */
	FSL_CTR_MOD_56,		/*!< Run counter with modulus of 2^56. */
	FSL_CTR_MOD_64,		/*!< Run counter with modulus of 2^64. */
	FSL_CTR_MOD_72,		/*!< Run counter with modulus of 2^72. */
	FSL_CTR_MOD_80,		/*!< Run counter with modulus of 2^80. */
	FSL_CTR_MOD_88,		/*!< Run counter with modulus of 2^88. */
	FSL_CTR_MOD_96,		/*!< Run counter with modulus of 2^96. */
	FSL_CTR_MOD_104,	/*!< Run counter with modulus of 2^104. */
	FSL_CTR_MOD_112,	/*!< Run counter with modulus of 2^112. */
	FSL_CTR_MOD_120,	/*!< Run counter with modulus of 2^120. */
	FSL_CTR_MOD_128		/*!< Run counter with modulus of 2^128. */
} fsl_shw_ctr_mod_t;

/*!
 * A work type associated with a work/result queue request.
 */
typedef enum shw_work_type {
	SHW_WORK_GET_RANDOM = 1,	/*!< fsl_shw_get_random() request.  */
	SHW_WORK_ADD_RANDOM,	/*!< fsl_shw_add_entropy() request.  */
} shw_work_type_t;

/*!****************************************************************************
 * Data Structures
 *****************************************************************************/

/*!
 * Initialization Object
 */
typedef struct fsl_sho_ibo {
} fsl_sho_ibo_t;

/*!
 * Common Entry structure for work queues, results queues.
 */
typedef struct shw_queue_entry {
	struct shw_queue_entry *next;	/*!< Next entry in queue. */
	struct fsl_shw_uco_t *user_ctx;	/*!< Associated user context. */
	uint32_t flags;		/*!< User context flags at time of request. */
	void (*callback) (struct fsl_shw_uco_t * uco);	/*!< Any callback request. */
	uint32_t user_ref;	/*!< User's reference for this request. */
	fsl_shw_return_t code;	/*!< FSL SHW result of this operation. */
	uint32_t detail1;	/*!< Any extra error info.  */
	uint32_t detail2;	/*!< More any extra error info.  */
	void *user_mode_req;	/*!<  Pointer into user space.  */
	 uint32_t(*postprocess) (struct shw_queue_entry * q);	/*!< (internal)
								   function to call
								   when this operation
								   completes.
								 */
} shw_queue_entry_t;

/*!
 * A queue.  Fields must be initialized to NULL before use.
 */
typedef struct shw_queue {
	struct shw_queue_entry *head;	/*!< First entry in queue. */
	struct shw_queue_entry *tail;	/*!< Last entry. */
} shw_queue_t;

/* REQ-FSLSHW-PINTFC-COA-UCO-001 */
/*!
 * User Context Object
 */
typedef struct fsl_shw_uco_t {
	int openfd;		/*!< user-mode file descriptor */
	uint32_t user_ref;	/*!< User's reference */
	void (*callback) (struct fsl_shw_uco_t * uco);	/*!< User's callback fn  */
	uint32_t flags;		/*!< from fsl_shw_user_ctx_flags_t */
	unsigned pool_size;	/*!< maximum size of user result pool */
#ifdef __KERNEL__
	shw_queue_t result_pool;	/*!< where non-blocking results go */
	os_process_handle_t process;	/*!< remember for signalling User mode */
#endif
	struct fsl_shw_uco_t *next;	/*!< To allow user-mode chaining of contexts,
					   for signalling and in kernel, to link user
					   contexts.  */
} fsl_shw_uco_t;

/* REQ-FSLSHW-PINTFC-API-GEN-006  ??  */
/*!
 * Result object
 */
typedef struct fsl_shw_result_t {
	uint32_t user_ref;	/*!< User's reference at time of request. */
	fsl_shw_return_t code;	/*!< Return code from request. */
	uint32_t detail1;	/*!< Extra error info. Unused in SHW driver. */
	uint32_t detail2;	/*!< Extra error info. Unused in SHW driver. */
	void *user_req;		/*!< Pointer to original user request. */
} fsl_shw_result_t;

/* REQ-FSLSHW-PINTFC-COA-SKO-001 */
/*!
 * Secret Key Context Object
 */
typedef struct fsl_shw_sko_t {
	uint32_t flags;		/*!<  Flags from #fsl_shw_sym_ctx_flags_t. */
	fsl_shw_key_alg_t algorithm;	/*!< Algorithm for this key. */
	key_userid_t userid;	/*!< User's identifying value for Black key. */
	uint32_t handle;	/*!< Reference in SCC driver for Red key. */
	uint16_t key_length;	/*!< Length of stored key, in bytes. */
	uint8_t key[64];	/*!< Bytes of stored key. */
} fsl_shw_sko_t;

/* REQ-FSLSHW-PINTFC-COA-CO-001 */
/*!
 * Platform Capability Object
 *
 * Pointer to this structure is returned by fsl_shw_get_capabilities() and
 * queried with the various fsl_shw_pco_() functions.
 */
typedef struct fsl_shw_pco_t {
	int api_major;		/*!< Major version number for API. */
	int api_minor;		/*!< Minor version number for API. */
	int driver_major;	/*!< Major version of some driver. */
	int driver_minor;	/*!< Minor version of some driver. */
	unsigned sym_algorithm_count;	/*!< Number of sym_algorithms. */
	fsl_shw_key_alg_t *sym_algorithms;	/*!< Pointer to array. */
	unsigned sym_mode_count;	/*!< Number of sym_modes. */
	fsl_shw_sym_mode_t *sym_modes;	/*!< Pointer to array. */
	unsigned hash_algorithm_count;	/*!< Number of hash_algorithms. */
	fsl_shw_hash_alg_t *hash_algorithms;	/*!< Pointer to array */
	uint8_t sym_support[5][4];	/*!< indexed by key alg then mode */
} fsl_shw_pco_t;

/* REQ-FSLSHW-PINTFC-COA-HCO-001 */
/*!
 * Hash Context Object
 */
typedef struct fsl_shw_hco_t {	/* fsl_shw_hash_context_object */
	fsl_shw_hash_alg_t algorithm;	/*!< Algorithm for this context. */
	uint32_t flags;		/*!< Flags from
				   #fsl_shw_hash_ctx_flags_t. */
	uint8_t digest_length;	/*!< hash result length in bytes */
	uint8_t context_length;	/*!< Context length in bytes */
	uint8_t context_register_length;	/*!< in bytes */
	uint32_t context[9];	/*!< largest digest + msg size */
} fsl_shw_hco_t;

/* REQ-FSLSHW-PINTFC-COA-HCO-001 */
/*!
 * HMAC Context Object
 */
typedef struct fsl_shw_hmco_t {	/* fsl_shw_hmac_context_object */
	fsl_shw_hash_alg_t algorithm;	/*!< Hash algorithm for the HMAC. */
	uint32_t flags;		/*!< Flags from
				   #fsl_shw_hmac_ctx_flags_t. */
	uint8_t digest_length;	/*!< in bytes */
	uint8_t context_length;	/*!< in bytes */
	uint8_t context_register_length;	/*!< in bytes */
	uint32_t ongoing_context[9];	/*!< largest digest + msg
					   size */
	uint32_t inner_precompute[9];	/*!< largest digest + msg
					   size */
	uint32_t outer_precompute[9];	/*!< largest digest + msg
					   size */
} fsl_shw_hmco_t;

/* REQ-FSLSHW-PINTFC-COA-SCCO-001 */
/*!
 * Symmetric Crypto Context Object Context Object
 */
typedef struct fsl_shw_scco_t {
	uint32_t flags;		/*!< Flags from #fsl_shw_sym_ctx_flags_t. */
	unsigned block_size_bytes;	/*!< Both block and ctx size */
	fsl_shw_sym_mode_t mode;	/*!< Symmetric mode for this context. */
	/* Could put modulus plus 16-octet context in union with arc4
	   sbox+ptrs... */
	fsl_shw_ctr_mod_t modulus_exp;	/*!< Exponent value for CTR modulus */
	uint8_t context[259];	/*!< Stored context.  Large enough
				   for ARC4.  */
} fsl_shw_scco_t;

/*!
 * Authenticate-Cipher Context Object

 * An object for controlling the function of, and holding information about,
 * data for the authenticate-cipher functions, #fsl_shw_gen_encrypt() and
 * #fsl_shw_auth_decrypt().
 */
typedef struct fsl_shw_acco_t {
	uint32_t flags;		/*!< See #fsl_shw_auth_ctx_flags_t for
				   meanings */
	fsl_shw_acc_mode_t mode;	/*!< CCM only */
	uint8_t mac_length;	/*!< User's value for length  */
	unsigned q_length;	/*!< NIST parameter - */
	fsl_shw_scco_t cipher_ctx_info;	/*!< For running
					   encrypt/decrypt. */
	union {
		fsl_shw_scco_t CCM_ctx_info;	/*!< For running the CBC in
						   AES-CCM.  */
		fsl_shw_hco_t hash_ctx_info;	/*!< For running the hash */
	} auth_info;		/*!< "auth" info struct  */
	uint8_t unencrypted_mac[16];	/*!< max block size... */
} fsl_shw_acco_t;

/*!
 * Common header in request structures between User-mode API and SHW driver.
 */
struct shw_req_header {
	uint32_t flags;		/*!< Flags - from user-mode context. */
	uint32_t user_ref;	/*!< Reference - from user-mode context. */
	fsl_shw_return_t code;	/*!< Result code for operation. */
};

/*!
 *  Used by user-mode API to retrieve completed non-blocking results in
 *  SHW_USER_REQ_GET_RESULTS ioctl().
 */
struct results_req {
	struct shw_req_header hdr;	/*!< Boilerplate. */
	unsigned requested;	/*!< number of results requested, */
	unsigned actual;	/*!< number of results obtained. */
	fsl_shw_result_t *results;	/*!< pointer to memory to hold results. */
};

/*!
 * Used by user-mode API to retrieve hardware capabilities in
 * SHW_USER_REQ_GET_CAPABILITIES ioctl().
 */
struct capabilities_req {
	struct shw_req_header hdr;	/*!< Boilerplate. */
	unsigned size;		/*!< Size, in bytes, capabilities. */
	fsl_shw_pco_t *capabilities;	/*!< Place to copy out the info. */
};

/*!
 * Used by user-mode API to get a random number
 */
struct get_random_req {
	struct shw_req_header hdr;	/*!< Boilerplate. */
	unsigned size;		/*!< Size, in bytes, of random. */
	uint8_t *random;	/*!< Place to copy out the random number. */
};

/*!
 * Used by API to add entropy to a random number generator
 */
struct add_entropy_req {
	struct shw_req_header hdr;	/*!< Boilerplate. */
	unsigned size;		/*!< Size, in bytes, of entropy. */
	uint8_t *entropy;	/*!< Location of the entropy to be added. */
};

/*!****************************************************************************
 * External variables
 *****************************************************************************/
#ifdef __KERNEL__
extern os_lock_t shw_queue_lock;

static fsl_shw_uco_t *user_list;
#endif

/*!****************************************************************************
 * Access Macros for Objects
 *****************************************************************************/
/*!
 * Get FSL SHW API version
 *
 * @param      pcobject  The Platform Capababilities Object to query.
 * @param pcmajor   A pointer to where the major version
 *                       of the API is to be stored.
 * @param pcminor   A pointer to where the minor version
 *                       of the API is to be stored.
 */
#define fsl_shw_pco_get_version(pcobject, pcmajor, pcminor)                   \
do {                                                                          \
    *(pcmajor) = (pcobject)->api_major;                                       \
    *(pcminor) = (pcobject)->api_minor;                                       \
} while (0)

/*!
 * Get underlying driver version.
 *
 * @param      pcobject  The Platform Capababilities Object to query.
 * @param      pcmajor   A pointer to where the major version
 *                       of the driver is to be stored.
 * @param      pcminor   A pointer to where the minor version
 *                       of the driver is to be stored.
 */
#define fsl_shw_pco_get_driver_version(pcobject, pcmajor, pcminor)            \
do {                                                                          \
    *(pcmajor) = (pcobject)->driver_major;                                    \
    *(pcminor) = (pcobject)->driver_minor;                                    \
} while (0)

/*!
 * Get list of symmetric algorithms supported.
 *
 * @param pcobject           The Platform Capababilities Object to query.
 * @param  pcalgorithms  A pointer to where to store the location of
 *                           the list of algorithms.
 * @param  pcacount      A pointer to where to store the number of
 *                           algorithms in the list at @a algorithms.
 */
#define fsl_shw_pco_get_sym_algorithms(pcobject, pcalgorithms, pcacount)      \
do {                                                                          \
    *(pcalgorithms) = (pcobject)->sym_algorithms;                             \
    *(pcacount) = (pcobject)->sym_algorithm_count;                            \
} while (0)

/*!
 * Get list of symmetric modes supported.
 *
 * @param pcobject        The Platform Capababilities Object to query.
 * @param gsmodes    A pointer to where to store the location of
 *                        the list of modes.
 * @param gsacount   A pointer to where to store the number of
 *                        algorithms in the list at @a modes.
 */
#define fsl_shw_pco_get_sym_modes(pcobject, gsmodes, gsacount)                \
do {                                                                          \
    *(gsmodes) = (pcobject)->sym_modes;                                       \
    *(gsacount) = (pcobject)->sym_mode_count;                                 \
} while (0)

/*!
 * Get list of hash algorithms supported.
 *
 * @param    pcobject        The Platform Capababilities Object to query.
 * @param    gsalgorithms    A pointer which will be set to the list of
 *                           algorithms.
 * @param    gsacount        The number of algorithms in the list at @a
 *                           algorithms.
 */
#define fsl_shw_pco_get_hash_algorithms(pcobject, gsalgorithms, gsacount)     \
do {                                                                          \
    *(gsalgorithms) = (pcobject)->hash_algorithms;                            \
    *(gsacount) = (pcobject)->hash_algorithm_count;                           \
} while (0)

/*!
 * Determine whether the combination of a given symmetric algorithm and a given
 * mode is supported.
 *
 * @param pcobject   The Platform Capababilities Object to query.
 * @param pcalg      A Symmetric Cipher algorithm.
 * @param pcmode     A Symmetric Cipher mode.
 *
 * @return 0 if combination is not supported, non-zero if supported.
 */
#define fsl_shw_pco_check_sym_supported(pcobject, pcalg, pcmode)              \
    0

/*!
 * Determine whether a given Encryption-Authentication mode is supported.
 *
 * @param pcobject  The Platform Capababilities Object to query.
 * @param pcmode    The Authentication mode.
 *
 * @return 0 if mode is not supported, non-zero if supported.
 */
#define fsl_shw_pco_check_auth_supported(pcobject, pcmode)                    \
    0

/*!
 * Determine whether Black Keys (key establishment / wrapping) is supported.
 *
 * @param pcobject  The Platform Capababilities Object to query.
 *
 * @return 0 if wrapping is not supported, non-zero if supported.
 */
#define fsl_shw_pco_check_black_key_supported(pcobject)                       \
    0

/*!
 * Initialize a User Context Object.
 *
 * This function must be called before performing any other operation with the
 * Object.  It sets the User Context Object to initial values, and set the size
 * of the results pool.  The mode will be set to a default of
 * #FSL_UCO_BLOCKING_MODE.
 *
 * When using non-blocking operations, this sets the maximum number of
 * operations which can be outstanding.  This number includes the counts of
 * operations waiting to start, operation(s) being performed, and results which
 * have not been retrieved.
 *
 * Changes to this value are ignored once user registration has completed.  It
 * should be set to 1 if only blocking operations will ever be performed.
 *
 * @param ucontext     The User Context object to operate on.
 * @param usize        The maximum number of operations which can be
 *                     outstanding.
 */
#define fsl_shw_uco_init(ucontext, usize)                                     \
do {                                                                          \
    fsl_shw_uco_t* uco = ucontext;                                            \
                                                                              \
    (uco)->pool_size = usize;                                                 \
    (uco)->flags = FSL_UCO_BLOCKING_MODE | FSL_UCO_CONTEXT_CHANGED;           \
    (uco)->openfd = -1;                                                       \
    (uco)->callback = NULL;                                                   \
} while (0)

/*!
 * Set the User Reference for the User Context.
 *
 * @param ucontext     The User Context object to operate on.
 * @param uref         A value which will be passed back with a result.
 */
#define fsl_shw_uco_set_reference(ucontext, uref)                             \
do {                                                                          \
    fsl_shw_uco_t* uco = ucontext;                                            \
                                                                              \
    (uco)->user_ref = uref;                                                   \
    (uco)->flags |= FSL_UCO_CONTEXT_CHANGED;                                  \
} while (0)

/*!
 * Set the User Reference for the User Context.
 *
 * @param ucontext     The User Context object to operate on.
 * @param ucallback    The function the API will invoke when an operation
 *                     completes.
 */
#define fsl_shw_uco_set_callback(ucontext, ucallback)                         \
do {                                                                          \
    fsl_shw_uco_t* uco = ucontext;                                            \
                                                                              \
    (uco)->callback = ucallback;                                              \
    (uco)->flags |= FSL_UCO_CONTEXT_CHANGED;                                  \
} while (0)

/*!
 * Set flags in the User Context.
 *
 * Turns on the flags specified in @a flags.  Other flags are untouched.
 *
 * @param ucontext     The User Context object to operate on.
 * @param uflags       ORed values from #fsl_shw_user_ctx_flags_t.
 */
#define fsl_shw_uco_set_flags(ucontext, uflags)                               \
      (ucontext)->flags |= (uflags) | FSL_UCO_CONTEXT_CHANGED

/*!
 * Clear flags in the User Context.
 *
 * Turns off the flags specified in @a flags.  Other flags are untouched.
 *
 * @param ucontext     The User Context object to operate on.
 * @param uflags       ORed values from #fsl_shw_user_ctx_flags_t.
 */
#define fsl_shw_uco_clear_flags(ucontext, uflags)                             \
do {                                                                          \
    fsl_shw_uco_t* uco = ucontext;                                            \
                                                                              \
    (uco)->flags &= ~(uflags);                                                \
    (uco)->flags |= FSL_UCO_CONTEXT_CHANGED;                                  \
} while (0)

/*!
 * Retrieve the reference value from a Result Object.
 *
 * @param robject  The result object to query.
 *
 * @return The reference associated with the request.
 */
#define fsl_shw_ro_get_reference(robject)                                    \
       (robject)->user_ref

/*!
 * Retrieve the status code from a Result Object.
 *
 * @param robject  The result object to query.
 *
 * @return The status of the request.
 */
#define fsl_shw_ro_get_status(robject)                                       \
       (robject)->code

/*!
 * Initialize a Secret Key Object.
 *
 * This function must be called before performing any other operation with
 * the Object.
 *
 * @param skobject     The Secret Key Object to be initialized.
 * @param skalgorithm  DES, AES, etc.
 *
 */
#define fsl_shw_sko_init(skobject,skalgorithm)                               \
do {                                                                         \
       (skobject)->algorithm = skalgorithm;                                  \
       (skobject)->flags = 0;                                                \
} while (0)

/*!
 * Store a cleartext key in the key object.
 *
 * This has the side effect of setting the #FSL_SKO_KEY_PRESENT flag and
 * resetting the #FSL_SKO_KEY_ESTABLISHED flag.
 *
 * @param skobject     A variable of type #fsl_shw_sko_t.
 * @param skkey        A pointer to the beginning of the key.
 * @param skkeylen     The length, in octets, of the key.  The value should be
 *                     appropriate to the key size supported by the algorithm.
 *                     64 octets is the absolute maximum value allowed for this
 *                     call.
 */
#define fsl_shw_sko_set_key(skobject, skkey, skkeylen)                       \
do {                                                                         \
       (skobject)->key_length = skkeylen;                                    \
       memcpy((skobject)->key, skkey, skkeylen);                             \
       (skobject)->flags |= FSL_SKO_KEY_PRESENT;                             \
       (skobject)->flags &= ~FSL_SKO_KEY_ESTABLISHED;                        \
} while (0)

/*!
 * Set a size for the key.
 *
 * This function would normally be used when the user wants the key to be
 * generated from a random source.
 *
 * @param skobject   A variable of type #fsl_shw_sko_t.
 * @param skkeylen   The length, in octets, of the key.  The value should be
 *                   appropriate to the key size supported by the algorithm.
 *                   64 octets is the absolute maximum value allowed for this
 *                   call.
 */
#define fsl_shw_sko_set_key_length(skobject, skkeylen)                       \
       (skobject)->key_length = skkeylen;

/*!
 * Set the User ID associated with the key.
 *
 * @param skobject   A variable of type #fsl_shw_sko_t.
 * @param skuserid   The User ID to identify authorized users of the key.
 */
#define fsl_shw_sko_set_user_id(skobject, skuserid)                           \
       (skobject)->userid = (skuserid)

/*!
 * Set the establish key handle into a key object.
 *
 * The @a userid field will be used to validate the access to the unwrapped
 * key.  This feature is not available for all platforms, nor for all
 * algorithms and modes.
 *
 * The #FSL_SKO_KEY_ESTABLISHED will be set (and the #FSL_SKO_KEY_PRESENT flag
 * will be cleared).
 *
 * @param skobject   A variable of type #fsl_shw_sko_t.
 * @param skuserid   The User ID to verify this user is an authorized user of
 *                   the key.
 * @param skhandle   A @a handle from #fsl_shw_sko_get_established_info.
 */
#define fsl_shw_sko_set_established_info(skobject, skuserid, skhandle)        \
do {                                                                          \
       (skobject)->userid = (skuserid);                                       \
       (skobject)->handle = (skhandle);                                       \
       (skobject)->flags |= FSL_SKO_KEY_ESTABLISHED;                          \
       (skobject)->flags &=                                                   \
                       ~(FSL_SKO_KEY_PRESENT);   \
} while (0)

/*!
 * Retrieve the established-key handle from a key object.
 *
 * @param skobject   A variable of type #fsl_shw_sko_t.
 * @param skhandle   The location to store the @a handle of the unwrapped
 *                   key.
 */
#define fsl_shw_sko_get_established_info(skobject, skhandle)                  \
       *(skhandle) = (skobject)->handle

/*!
 * Extract the algorithm from a key object.
 *
 * @param      skobject     The Key Object to be queried.
 * @param skalgorithm  A pointer to the location to store the algorithm.
 */
#define fsl_shw_sko_get_algorithm(skobject, skalgorithm)                      \
       *(skalgorithm) = (skobject)->algorithm

/*!
 * Determine the size of a wrapped key based upon the cleartext key's length.
 *
 * This function can be used to calculate the number of octets that
 * #fsl_shw_extract_key() will write into the location at @a covered_key.
 *
 * If zero is returned at @a length, this means that the key length in
 * @a key_info is not supported.
 *
 * @param      wkeyinfo         Information about a key to be wrapped.
 * @param      wkeylen          Location to store the length of a wrapped
 *                              version of the key in @a key_info.
 */
#define fsl_shw_sko_calculate_wrapped_size(wkeyinfo, wkeylen)                 \
do {                                                                          \
     if ((wkeyinfo)->key_length > 32) {                                       \
         *(wkeylen) = 0;                                                      \
     } else {                                                                 \
         *(wkeylen) = 66;                                                     \
     }                                                                        \
} while (0)

/*!
 * Set some flags in the key object.
 *
 * Turns on the flags specified in @a flags.  Other flags are untouched.
 *
 * @param skobject     A variable of type #fsl_shw_sko_t.
 * @param skflags      (One or more) ORed members of #fsl_shw_key_flags_t which
 *                     are to be set.
 */
#define fsl_shw_sko_set_flags(skobject, skflags)                              \
      (skobject)->flags |= (skflags)

/*!
 * Clear some flags in the key object.
 *
 * Turns off the flags specified in @a flags.  Other flags are untouched.
 *
 * @param skobject      A variable of type #fsl_shw_sko_t.
 * @param skflags       (One or more) ORed members of #fsl_shw_key_flags_t
 *                      which are to be reset.
 */
#define fsl_shw_sko_clear_flags(skobject, skflags)                            \
      (skobject)->flags &= ~(skflags)

/* REQ-FSL-SHW-PINTFC-API-BASIC-HASH-004 */
/*!
 * Initialize a Hash Context Object.
 *
 * This function must be called before performing any other operation with the
 * Object.  It sets the current message length and hash algorithm in the hash
 * context object.
 *
 * @param      hcobject    The hash context to operate upon.
 * @param      hcalgorithm The hash algorithm to be used (#FSL_HASH_ALG_MD5,
 *                         #FSL_HASH_ALG_SHA256, etc).
 *
 */
#define fsl_shw_hco_init(hcobject, hcalgorithm)                               \
do {                                                                          \
     (hcobject)->algorithm = hcalgorithm;                                     \
     (hcobject)->flags = 0;                                                   \
     switch (hcalgorithm) {                                                   \
     case FSL_HASH_ALG_MD5:                                                   \
         (hcobject)->digest_length = 16;                                      \
         (hcobject)->context_length = 16;                                     \
         (hcobject)->context_register_length = 24;                            \
         break;                                                               \
     case FSL_HASH_ALG_SHA1:                                                  \
         (hcobject)->digest_length = 20;                                      \
         (hcobject)->context_length = 20;                                     \
         (hcobject)->context_register_length = 24;                            \
         break;                                                               \
     case FSL_HASH_ALG_SHA224:                                                \
         (hcobject)->digest_length = 28;                                      \
         (hcobject)->context_length = 32;                                     \
         (hcobject)->context_register_length = 36;                            \
         break;                                                               \
     case FSL_HASH_ALG_SHA256:                                                \
         (hcobject)->digest_length = 32;                                      \
         (hcobject)->context_length = 32;                                     \
         (hcobject)->context_register_length = 36;                            \
         break;                                                               \
     default:                                                                 \
         /* error ! */                                                        \
         (hcobject)->digest_length = 1;                                       \
         (hcobject)->context_length = 1;                                      \
         (hcobject)->context_register_length = 1;                             \
         break;                                                               \
     }                                                                        \
} while (0)

/* REQ-FSL-SHW-PINTFC-API-BASIC-HASH-001 */
/*!
 * Get the current hash value and message length from the hash context object.
 *
 * The algorithm must have already been specified.  See #fsl_shw_hco_init().
 *
 * @param hcobject   The hash context to query.
 * @param hccontext  Pointer to the location of @a length octets where to
 *                        store a copy of the current value of the digest.
 * @param hcclength  Number of octets of hash value to copy.
 * @param hcmsglen   Pointer to the location to store the number of octets
 *                        already hashed.
 */
#define fsl_shw_hco_get_digest(hcobject, hccontext, hcclength, hcmsglen)      \
do {                                                                          \
     memcpy(hccontext, (hcobject)->context, hcclength);                       \
         if ((hcobject)->algorithm == FSL_HASH_ALG_SHA224                     \
             || (hcobject)->algorithm == FSL_HASH_ALG_SHA256) {               \
             *(hcmsglen) = (hcobject)->context[8];                            \
         } else {                                                             \
             *(hcmsglen) = (hcobject)->context[5];                            \
         }                                                                    \
} while (0)

/* REQ-FSL-SHW-PINTFC-API-BASIC-HASH-002 */
/*!
 * Get the hash algorithm from the hash context object.
 *
 * @param      hcobject    The hash context to query.
 * @param hcalgorithm Pointer to where the algorithm is to be stored.
 */
#define fsl_shw_hco_get_info(hcobject, hcalgorithm)                           \
do {                                                                          \
     *(hcalgorithm) = (hcobject)->algorithm;                                  \
} while (0)

/* REQ-FSL-SHW-PINTFC-API-BASIC-HASH-003 */
/* REQ-FSL-SHW-PINTFC-API-BASIC-HASH-004 */
/*!
 * Set the current hash value and message length in the hash context object.
 *
 * The algorithm must have already been specified.  See #fsl_shw_hco_init().
 *
 * @param      hcobject  The hash context to operate upon.
 * @param      hccontext Pointer to buffer of appropriate length to copy into
 *                       the hash context object.
 * @param      hcmsglen  The number of octets of the message which have
 *                        already been hashed.
 *
 */
#define fsl_shw_hco_set_digest(hcobject, hccontext, hcmsglen)                 \
do {                                                                          \
     memcpy((hcobject)->context, hccontext, (hcobject)->context_length);      \
     if (((hcobject)->algorithm == FSL_HASH_ALG_SHA224)                       \
         || ((hcobject)->algorithm == FSL_HASH_ALG_SHA256)) {                 \
         (hcobject)->context[8] = hcmsglen;                                   \
     } else {                                                                 \
         (hcobject)->context[5] = hcmsglen;                                   \
     }                                                                        \
} while (0)

/*!
 * Set flags in a Hash Context Object.
 *
 * Turns on the flags specified in @a flags.  Other flags are untouched.
 *
 * @param hcobject   The hash context to be operated on.
 * @param hcflags    The flags to be set in the context.  These can be ORed
 *                   members of #fsl_shw_hash_ctx_flags_t.
 */
#define fsl_shw_hco_set_flags(hcobject, hcflags)                              \
      (hcobject)->flags |= (hcflags)

/*!
 * Clear flags in a Hash Context Object.
 *
 * Turns off the flags specified in @a flags.  Other flags are untouched.
 *
 * @param hcobject   The hash context to be operated on.
 * @param hcflags    The flags to be reset in the context.  These can be ORed
 *                   members of #fsl_shw_hash_ctx_flags_t.
 */
#define fsl_shw_hco_clear_flags(hcobject, hcflags)                            \
      (hcobject)->flags &= ~(hcflags)

/*!
 * Initialize an HMAC Context Object.
 *
 * This function must be called before performing any other operation with the
 * Object.  It sets the current message length and hash algorithm in the HMAC
 * context object.
 *
 * @param      hcobject    The HMAC context to operate upon.
 * @param      hcalgorithm The hash algorithm to be used (#FSL_HASH_ALG_MD5,
 *                         #FSL_HASH_ALG_SHA256, etc).
 *
 */
#define fsl_shw_hmco_init(hcobject, hcalgorithm)                              \
    fsl_shw_hco_init(hcobject, hcalgorithm)

/*!
 * Set flags in an HMAC Context Object.
 *
 * Turns on the flags specified in @a flags.  Other flags are untouched.
 *
 * @param hcobject   The HMAC context to be operated on.
 * @param hcflags    The flags to be set in the context.  These can be ORed
 *                   members of #fsl_shw_hmac_ctx_flags_t.
 */
#define fsl_shw_hmco_set_flags(hcobject, hcflags)                             \
      (hcobject)->flags |= (hcflags)

/*!
 * Clear flags in an HMAC Context Object.
 *
 * Turns off the flags specified in @a flags.  Other flags are untouched.
 *
 * @param hcobject   The HMAC context to be operated on.
 * @param hcflags    The flags to be reset in the context.  These can be ORed
 *                   members of #fsl_shw_hmac_ctx_flags_t.
 */
#define fsl_shw_hmco_clear_flags(hcobject, hcflags)                           \
      (hcobject)->flags &= ~(hcflags)

/*!
 * Initialize a Symmetric Cipher Context Object.
 *
 * This function must be called before performing any other operation with the
 * Object.  This will set the @a mode and @a algorithm and initialize the
 * Object.
 *
 * @param scobject  The context object to operate on.
 * @param scalg     The cipher algorithm this context will be used with.
 * @param scmode    #FSL_SYM_MODE_CBC, #FSL_SYM_MODE_ECB, etc.
 *
 */
#define fsl_shw_scco_init(scobject, scalg, scmode)                            \
do {                                                                          \
      register uint32_t bsb;   /* block-size bytes */                         \
                                                                              \
      switch (scalg) {                                                        \
      case FSL_KEY_ALG_AES:                                                   \
          bsb = 16;                                                           \
          break;                                                              \
      case FSL_KEY_ALG_DES:                                                   \
          /* fall through */                                                  \
      case FSL_KEY_ALG_TDES:                                                  \
          bsb = 8;                                                            \
          break;                                                              \
      case FSL_KEY_ALG_ARC4:                                                  \
          bsb = 259;                                                          \
          break;                                                              \
      case FSL_KEY_ALG_HMAC:                                                  \
          bsb = 1;  /* meaningless */                                         \
          break;                                                              \
      default:                                                                \
          bsb = 00;                                                           \
      }                                                                       \
      (scobject)->block_size_bytes = bsb;                                     \
      (scobject)->mode = scmode;                                              \
      (scobject)->flags = 0;                                                  \
} while (0)

/*!
 * Set the flags for a Symmetric Cipher Context.
 *
 * Turns on the flags specified in @a flags.  Other flags are untouched.
 *
 * @param scobject The context object to operate on.
 * @param scflags  The flags to reset (one or more values from
 *                 #fsl_shw_sym_ctx_flags_t ORed together).
 *
 */
#define fsl_shw_scco_set_flags(scobject, scflags)                             \
       (scobject)->flags |= (scflags)

/*!
 * Clear some flags in a Symmetric Cipher Context Object.
 *
 * Turns off the flags specified in @a flags.  Other flags are untouched.
 *
 * @param scobject The context object to operate on.
 * @param scflags  The flags to reset (one or more values from
 *                 #fsl_shw_sym_ctx_flags_t ORed together).
 *
 */
#define fsl_shw_scco_clear_flags(scobject, scflags)                           \
       (scobject)->flags &= ~(scflags)

/*!
 * Set the Context (IV) for a Symmetric Cipher Context.
 *
 * This is to set the context/IV for #FSL_SYM_MODE_CBC mode, or to set the
 * context (the S-Box and pointers) for ARC4.  The full context size will
 * be copied.
 *
 * @param scobject  The context object to operate on.
 * @param sccontext A pointer to the buffer which contains the context.
 *
 */
#define fsl_shw_scco_set_context(scobject, sccontext)                         \
       memcpy((scobject)->context, sccontext,                                 \
                  (scobject)->block_size_bytes)

/*!
 * Get the Context for a Symmetric Cipher Context.
 *
 * This is to retrieve the context/IV for #FSL_SYM_MODE_CBC mode, or to
 * retrieve context (the S-Box and pointers) for ARC4.  The full context
 * will be copied.
 *
 * @param      scobject  The context object to operate on.
 * @param sccontext Pointer to location where context will be stored.
 */
#define fsl_shw_scco_get_context(scobject, sccontext)                         \
       memcpy(sccontext, (scobject)->context, (scobject)->block_size_bytes)

/*!
 * Set the Counter Value for a Symmetric Cipher Context.
 *
 * This will set the Counter Value for CTR mode.
 *
 * @param scobject  The context object to operate on.
 * @param sccounter The starting counter value.  The number of octets.
 *                  copied will be the block size for the algorithm.
 * @param scmodulus The modulus for controlling the incrementing of the
 *                  counter.
 *
 */
#define fsl_shw_scco_set_counter_info(scobject, sccounter, scmodulus)        \
do {                                                                         \
           if ((sccounter) != NULL) {                                        \
               memcpy((scobject)->context, sccounter,                        \
                          (scobject)->block_size_bytes);                     \
           }                                                                 \
           (scobject)->modulus_exp = scmodulus;                              \
} while (0)

/*!
 * Get the Counter Value for a Symmetric Cipher Context.
 *
 * This will retrieve the Counter Value is for CTR mode.
 *
 * @param     scobject    The context object to query.
 * @param sccounter  Pointer to location to store the current counter
 *                        value.  The number of octets copied will be the
 *                        block size for the algorithm.
 * @param scmodulus  Pointer to location to store the modulus.
 *
 */
#define fsl_shw_scco_get_counter_info(scobject, sccounter, scmodulus)        \
do {                                                                         \
           if ((sccounter) != NULL) {                                        \
               memcpy(sccounter, (scobject)->context,                        \
                          (scobject)->block_size_bytes);                     \
           }                                                                 \
           if ((scmodulus) != NULL) {                                        \
               *(scmodulus) = (scobject)->modulus_exp;                       \
           }                                                                 \
} while (0)

/*!
 * Initialize a Authentication-Cipher Context.
 *
 * @param acobject  Pointer to object to operate on.
 * @param acmode    The mode for this object (only #FSL_ACC_MODE_CCM
 *                  supported).
 */
#define fsl_shw_acco_init(acobject, acmode)                                   \
do {                                                                          \
       (acobject)->flags = 0;                                                 \
       (acobject)->mode = (acmode);                                           \
} while (0)

/*!
 * Set the flags for a Authentication-Cipher Context.
 *
 * Turns on the flags specified in @a flags.  Other flags are untouched.
 *
 * @param acobject  Pointer to object to operate on.
 * @param acflags   The flags to set (one or more from
 *                  #fsl_shw_auth_ctx_flags_t ORed together).
 *
 */
#define fsl_shw_acco_set_flags(acobject, acflags)                             \
       (acobject)->flags |= (acflags)

/*!
 * Clear some flags in a Authentication-Cipher Context Object.
 *
 * Turns off the flags specified in @a flags.  Other flags are untouched.
 *
 * @param acobject  Pointer to object to operate on.
 * @param acflags   The flags to reset (one or more from
 *                  #fsl_shw_auth_ctx_flags_t ORed together).
 *
 */
#define fsl_shw_acco_clear_flags(acobject, acflags)                           \
       (acobject)->flags &= ~(acflags)

/*!
 * Set up the Authentication-Cipher Object for CCM mode.
 *
 * This will set the @a auth_object for CCM mode and save the @a ctr,
 * and @a mac_length.  This function can be called instead of
 * #fsl_shw_acco_init().
 *
 * The paramater @a ctr is Counter Block 0, (counter value 0), which is for the
 * MAC.
 *
 * @param acobject  Pointer to object to operate on.
 * @param acalg     Cipher algorithm.  Only AES is supported.
 * @param accounter The initial counter value.
 * @param acmaclen  The number of octets used for the MAC.  Valid values are
 *                  4, 6, 8, 10, 12, 14, and 16.
 */
/* Do we need to stash the +1 value of the CTR somewhere? */
#define fsl_shw_acco_set_ccm(acobject, acalg, accounter, acmaclen)            \
 do {                                                                         \
      (acobject)->flags = 0;                                                  \
      (acobject)->mode = FSL_ACC_MODE_CCM;                                    \
      (acobject)->auth_info.CCM_ctx_info.block_size_bytes = 16;               \
      (acobject)->cipher_ctx_info.block_size_bytes = 16;                      \
      (acobject)->mac_length = acmaclen;                                      \
      fsl_shw_scco_set_counter_info(&(acobject)->cipher_ctx_info, accounter,  \
            FSL_CTR_MOD_128);                                                 \
} while (0)

/*!
 * Format the First Block (IV) & Initial Counter Value per NIST CCM.
 *
 * This function will also set the IV and CTR values per Appendix A of NIST
 * Special Publication 800-38C (May 2004).  It will also perform the
 * #fsl_shw_acco_set_ccm() operation with information derived from this set of
 * parameters.
 *
 * Note this function assumes the algorithm is AES.  It initializes the
 * @a auth_object by setting the mode to #FSL_ACC_MODE_CCM and setting the
 * flags to be #FSL_ACCO_NIST_CCM.
 *
 * @param acobject  Pointer to object to operate on.
 * @param act       The number of octets used for the MAC.  Valid values are
 *                  4, 6, 8, 10, 12, 14, and 16.
 * @param acad      Number of octets of Associated Data (may be zero).
 * @param acq       A value for the size of the length of @a q field.  Valid
 *                  values are 1-8.
 * @param acN       The Nonce (packet number or other changing value). Must
 *                  be (15 - @a q_length) octets long.
 * @param acQ       The value of Q (size of the payload in octets).
 *
 */
#define fsl_shw_ccm_nist_format_ctr_and_iv(acobject, act, acad, acq, acN, acQ)\
 do {                                                                         \
        uint64_t Q = acQ;                                                     \
        uint8_t bflag = ((acad)?0x40:0) | ((((act)-2)/2)<<3) | ((acq)-1);     \
        unsigned i;                                                           \
        uint8_t* qptr = (acobject)->auth_info.CCM_ctx_info.context + 15;      \
        (acobject)->auth_info.CCM_ctx_info.block_size_bytes = 16;             \
        (acobject)->cipher_ctx_info.block_size_bytes = 16;                    \
        (acobject)->mode = FSL_ACC_MODE_CCM;                                  \
        (acobject)->flags  = FSL_ACCO_NIST_CCM;                               \
                                                                              \
        /* Store away the MAC length (after calculating actual value */       \
        (acobject)->mac_length = (act);                                       \
        /* Set Flag field in Block 0 */                                       \
        *((acobject)->auth_info.CCM_ctx_info.context) = bflag;                \
        /* Set Nonce field in Block 0 */                                      \
        memcpy((acobject)->auth_info.CCM_ctx_info.context+1, acN,             \
                   15-(acq));                                                 \
        /* Set Flag field in ctr */                                           \
        *((acobject)->cipher_ctx_info.context) = (acq)-1;                     \
        /* Update the Q (payload length) field of Block0 */                   \
        (acobject)->q_length = acq;                                           \
        for (i = 0; i < (acq); i++) {                                         \
            *qptr-- = Q & 0xFF;                                               \
            Q >>= 8;                                                          \
        }                                                                     \
        /* Set the Nonce field of the ctr */                                  \
        memcpy((acobject)->cipher_ctx_info.context+1, acN, 15-(acq));         \
        /* Clear the block counter field of the ctr */                        \
        memset((acobject)->cipher_ctx_info.context+16-(acq), 0, (acq)+1);     \
   } while (0)

/*!
 * Update the First Block (IV) & Initial Counter Value per NIST CCM.
 *
 * This function will set the IV and CTR values per Appendix A of NIST Special
 * Publication 800-38C (May 2004).
 *
 * Note this function assumes that #fsl_shw_ccm_nist_format_ctr_and_iv() has
 * previously been called on the @a auth_object.
 *
 * @param acobject  Pointer to object to operate on.
 * @param acN       The Nonce (packet number or other changing value). Must
 *                  be (15 - @a q_length) octets long.
 * @param acQ       The value of Q (size of the payload in octets).
 *
 */
/* Do we need to stash the +1 value of the CTR somewhere? */
#define fsl_shw_ccm_nist_update_ctr_and_iv(acobject, acN, acQ)                \
  do {                                                                        \
        uint64_t Q = acQ;                                                     \
        unsigned i;                                                           \
        uint8_t* qptr = (acobject)->auth_info.CCM_ctx_info.context + 15;      \
                                                                              \
        /* Update the Nonce field field of Block0 */                          \
        memcpy((acobject)->auth_info.CCM_ctx_info.context+1, acN,             \
               15 - (acobject)->q_length);                                    \
        /* Update the Q (payload length) field of Block0 */                   \
        for (i = 0; i < (acobject)->q_length; i++) {                          \
            *qptr-- = Q & 0xFF;                                               \
            Q >>= 8;                                                          \
        }                                                                     \
        /* Update the Nonce field of the ctr */                               \
        memcpy((acobject)->cipher_ctx_info.context+1, acN,                    \
               15 - (acobject)->q_length);                                    \
   } while (0)

/*!****************************************************************************
 * Library functions
 *****************************************************************************/
/* REQ-FSL-SHW-PINTFC-COA-UCO */
/* REQ-FSLSHW-PINTFC-API-GEN-003 */
/*!
 * Determine the hardware security capabilities of this platform.
 *
 * Though a user context object is passed into this function, it will always
 * act in a non-blocking manner.
 *
 * @param  user_ctx   The user context which will be used for the query.
 *
 * @return  A pointer to the capabilities object.
 */
extern fsl_shw_pco_t *fsl_shw_get_capabilities(fsl_shw_uco_t * user_ctx);

/* REQ-FSL-SHW-PINTFC-COA-UCO */
/* REQ-FSLSHW-PINTFC-API-GEN-004 */
/*!
 * Create an association between the the user and the provider of the API.
 *
 * @param  user_ctx   The user context which will be used for this association.
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
extern fsl_shw_return_t fsl_shw_register_user(fsl_shw_uco_t * user_ctx);

/* REQ-FSL-SHW-PINTFC-COA-UCO */
/* REQ-FSLSHW-PINTFC-API-GEN-006 */
/*!
 * Destroy the association between the the user and the provider of the API.
 *
 * @param  user_ctx   The user context which is no longer needed.
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
extern fsl_shw_return_t fsl_shw_deregister_user(fsl_shw_uco_t * user_ctx);

/*!
 * Retrieve results from earlier operations.
 *
 * @param         user_ctx     The user's context.
 * @param         result_size  The number of array elements of @a results.
 * @param results      Pointer to first of the (array of) locations to
 *                             store results.
 * @param    result_count Pointer to store the number of results which
 *                             were returned.
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
extern fsl_shw_return_t fsl_shw_get_results(fsl_shw_uco_t * user_ctx,
					    unsigned result_size,
					    fsl_shw_result_t results[],
					    unsigned *result_count);

/*!
 * Place a key into a protected location for use only by cryptographic
 * algorithms.
 *
 * This only needs to be used to a) unwrap a key, or b) set up a key which
 * could be wrapped with a later call to #fsl_shw_extract_key().  Normal
 * cleartext keys can simply be placed into #fsl_shw_sko_t key objects with
 * #fsl_shw_sko_set_key() and used directly.
 *
 * The maximum key size supported for wrapped/unwrapped keys is 32 octets.
 * (This is the maximum reasonable key length on Sahara - 32 octets for an HMAC
 * key based on SHA-256.)  The key size is determined by the @a key_info.  The
 * expected length of @a key can be determined by
 * #fsl_shw_sko_calculate_wrapped_size()
 *
 * The protected key will not be available for use until this operation
 * successfully completes.
 *
 * This feature is not available for all platforms, nor for all algorithms and
 * modes.
 *
 * @param      user_ctx         A user context from #fsl_shw_register_user().
 * @param      key_info         The information about the key to be which will
 *                              be established.  In the create case, the key
 *                              length must be set.
 * @param      establish_type   How @a key will be interpreted to establish a
 *                              key for use.
 * @param      key              If @a establish_type is #FSL_KEY_WRAP_UNWRAP,
 *                              this is the location of a wrapped key.  If
 *                              @a establish_type is #FSL_KEY_WRAP_CREATE, this
 *                              parameter can be @a NULL.  If @a establish_type
 *                              is #FSL_KEY_WRAP_ACCEPT, this is the location
 *                              of a plaintext key.
 */
extern fsl_shw_return_t fsl_shw_establish_key(fsl_shw_uco_t * user_ctx,
					      fsl_shw_sko_t * key_info,
					      fsl_shw_key_wrap_t establish_type,
					      const uint8_t * key);

/*!
 * Wrap a key and retrieve the wrapped value.
 *
 * A wrapped key is a key that has been cryptographically obscured.  It is
 * only able to be used with #fsl_shw_establish_key().
 *
 * This function will also release the key (see #fsl_shw_release_key()) so
 * that it must be re-established before reuse.
 *
 * This feature is not available for all platforms, nor for all algorithms and
 * modes.
 *
 * @param      user_ctx         A user context from #fsl_shw_register_user().
 * @param      key_info         The information about the key to be deleted.
 * @param covered_key      The location to store the wrapped key.
 *                              (This size is based upon the maximum key size
 *                              of 32 octets).
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
extern fsl_shw_return_t fsl_shw_extract_key(fsl_shw_uco_t * user_ctx,
					    fsl_shw_sko_t * key_info,
					    uint8_t * covered_key);

/*!
 * De-establish a key so that it can no longer be accessed.
 *
 * The key will need to be re-established before it can again be used.
 *
 * This feature is not available for all platforms, nor for all algorithms and
 * modes.
 *
 * @param      user_ctx         A user context from #fsl_shw_register_user().
 * @param      key_info         The information about the key to be deleted.
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
extern fsl_shw_return_t fsl_shw_release_key(fsl_shw_uco_t * user_ctx,
					    fsl_shw_sko_t * key_info);

/* REQ-FSL-SHW-PINTFC-COA-UCO */
/* REQ-FSL-SHW-PINTFC-COA-SKO */
/* REQ-FSL-SHW-PINTFC-COA-SCCO */
/* REQ-FSLSHW-PINTFC-API-BASIC-SYM-001 */
/* PINTFC-API-BASIC-SYM-ARC4-001 */
/* PINTFC-API-BASIC-SYM-ARC4-002 */
/*!
 * Encrypt a stream of data with a symmetric-key algorithm.
 *
 * In ARC4, and also in #FSL_SYM_MODE_CBC and #FSL_SYM_MODE_CTR modes, the
 * flags of the @a sym_ctx object will control part of the operation of this
 * function.  The #FSL_SYM_CTX_INIT flag means that there is no context info in
 * the object.  The #FSL_SYM_CTX_LOAD means to use information in the
 * @a sym_ctx at the start of the operation, and the #FSL_SYM_CTX_SAVE flag
 * means to update the object's context information after the operation has
 * been performed.
 *
 * All of the data for an operation can be run through at once using the
 * #FSL_SYM_CTX_INIT or #FSL_SYM_CTX_LOAD flags, as appropriate, and then using
 * a @a length for the whole of the data.
 *
 * If a #FSL_SYM_CTX_SAVE flag were added, an additional call to the function
 * would "pick up" where the previous call left off, allowing the user to
 * perform the larger function in smaller steps.
 *
 * In #FSL_SYM_MODE_CBC and #FSL_SYM_MODE_ECB modes, the @a length must always
 * be a multiple of the block size for the algorithm being used.  For proper
 * operation in #FSL_SYM_MODE_CTR mode, the @a length must be a multiple of the
 * block size until the last operation on the total octet stream.
 *
 * Some users of ARC4 may want to compute the context (S-Box and pointers) from
 * the key before any data is available.  This may be done by running this
 * function with a @a length of zero, with the init & save flags flags on in
 * the @a sym_ctx.  Subsequent operations would then run as normal with the
 * load and save flags.  Note that they key object is still required.
 *
 * @param         user_ctx  A user context from #fsl_shw_register_user().
 * @param         key_info  Key and algorithm  being used for this operation.
 * @param sym_ctx   Info on cipher mode, state of the cipher.
 * @param         length   Length, in octets, of the pt (and ct).
 * @param         pt       pointer to plaintext to be encrypted.
 * @param    ct       pointer to where to store the resulting ciphertext.
 *
 * @return    A return code of type #fsl_shw_return_t.
 *
 */
extern fsl_shw_return_t fsl_shw_symmetric_encrypt(fsl_shw_uco_t * user_ctx,
						  fsl_shw_sko_t * key_info,
						  fsl_shw_scco_t * sym_ctx,
						  uint32_t length,
						  const uint8_t * pt,
						  uint8_t * ct);

/* REQ-FSL-SHW-PINTFC-COA-UCO */
/* REQ-FSL-SHW-PINTFC-COA-SKO */
/* REQ-FSL-SHW-PINTFC-COA-SCCO */
/* PINTFC-API-BASIC-SYM-002 */
/* PINTFC-API-BASIC-SYM-ARC4-001 */
/* PINTFC-API-BASIC-SYM-ARC4-002 */
/*!
 * Decrypt a stream of data with a symmetric-key algorithm.
 *
 * In ARC4, and also in #FSL_SYM_MODE_CBC and #FSL_SYM_MODE_CTR modes, the
 * flags of the @a sym_ctx object will control part of the operation of this
 * function.  The #FSL_SYM_CTX_INIT flag means that there is no context info in
 * the object.  The #FSL_SYM_CTX_LOAD means to use information in the
 * @a sym_ctx at the start of the operation, and the #FSL_SYM_CTX_SAVE flag
 * means to update the object's context information after the operation has
 * been performed.
 *
 * All of the data for an operation can be run through at once using the
 * #FSL_SYM_CTX_INIT or #FSL_SYM_CTX_LOAD flags, as appropriate, and then using
 * a @a length for the whole of the data.
 *
 * If a #FSL_SYM_CTX_SAVE flag were added, an additional call to the function
 * would "pick up" where the previous call left off, allowing the user to
 * perform the larger function in smaller steps.
 *
 * In #FSL_SYM_MODE_CBC and #FSL_SYM_MODE_ECB modes, the @a length must always
 * be a multiple of the block size for the algorithm being used.  For proper
 * operation in #FSL_SYM_MODE_CTR mode, the @a length must be a multiple of the
 * block size until the last operation on the total octet stream.
 *
 * Some users of ARC4 may want to compute the context (S-Box and pointers) from
 * the key before any data is available.  This may be done by running this
 * function with a @a length of zero, with the #FSL_SYM_CTX_INIT &
 * #FSL_SYM_CTX_SAVE flags on in the @a sym_ctx.  Subsequent operations would
 * then run as normal with the load & save flags.  Note that they key object is
 * still required.
 *
 * @param      user_ctx  A user context from #fsl_shw_register_user().
 * @param      key_info The key and algorithm being used in this operation.
 * @param sym_ctx Info on cipher mode, state of the cipher.
 * @param      length   Length, in octets, of the ct (and pt).
 * @param      ct       pointer to ciphertext to be decrypted.
 * @param pt       pointer to where to store the resulting plaintext.
 *
 * @return    A return code of type #fsl_shw_return_t
 *
 */
extern fsl_shw_return_t fsl_shw_symmetric_decrypt(fsl_shw_uco_t * user_ctx,
						  fsl_shw_sko_t * key_info,
						  fsl_shw_scco_t * sym_ctx,
						  uint32_t length,
						  const uint8_t * ct,
						  uint8_t * pt);

/* REQ-FSL-SHW-PINTFC-COA-UCO */
/* REQ-FSL-SHW-PINTFC-COA-HCO */
/* REQ-FSLSHW-PINTFC-API-BASIC-HASH-005 */
/*!
 * Hash a stream of data with a cryptographic hash algorithm.
 *
 * The flags in the @a hash_ctx control the operation of this function.
 *
 * Hashing functions work on 64 octets of message at a time.  Therefore, when
 * any partial hashing of a long message is performed, the message @a length of
 * each segment must be a multiple of 64.  When ready to
 * #FSL_HASH_FLAGS_FINALIZE the hash, the @a length may be any value.
 *
 * With the #FSL_HASH_FLAGS_INIT and #FSL_HASH_FLAGS_FINALIZE flags on, a
 * one-shot complete hash, including padding, will be performed.  The @a length
 * may be any value.
 *
 * The first octets of a data stream can be hashed by setting the
 * #FSL_HASH_FLAGS_INIT and #FSL_HASH_FLAGS_SAVE flags.  The @a length must be
 * a multiple of 64.
 *
 * The flag #FSL_HASH_FLAGS_LOAD is used to load a context previously saved by
 * #FSL_HASH_FLAGS_SAVE.  The two in combination will allow a (multiple-of-64
 * octets) 'middle sequence' of the data stream to be hashed with the
 * beginning.  The @a length must again be a multiple of 64.
 *
 * Since the flag #FSL_HASH_FLAGS_LOAD is used to load a context previously
 * saved by #FSL_HASH_FLAGS_SAVE, the #FSL_HASH_FLAGS_LOAD and
 * #FSL_HASH_FLAGS_FINALIZE flags, used together, can be used to finish the
 * stream.  The @a length may be any value.
 *
 * If the user program wants to do the padding for the hash, it can leave off
 * the #FSL_HASH_FLAGS_FINALIZE flag.  The @a length must then be a multiple of
 * 64 octets.
 *
 * @param      user_ctx  A user context from #fsl_shw_register_user().
 * @param hash_ctx Hashing algorithm and state of the cipher.
 * @param      msg       Pointer to the data to be hashed.
 * @param      length    Length, in octets, of the @a msg.
 * @param result    If not null, pointer to where to store the hash
 *                       digest.
 * @param      result_len Number of octets to store in @a result.
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
extern fsl_shw_return_t fsl_shw_hash(fsl_shw_uco_t * user_ctx,
				     fsl_shw_hco_t * hash_ctx,
				     const uint8_t * msg,
				     uint32_t length,
				     uint8_t * result, uint32_t result_len);

/* REQ-FSL-SHW-PINTFC-COA-UCO */
/* REQ-FSL-SHW-PINTFC-API-BASIC-HMAC-001 */
/*!
 * Precompute the Key hashes for an HMAC operation.
 *
 * This function may be used to calculate the inner and outer precomputes,
 * which are the hash contexts resulting from hashing the XORed key for the
 * 'inner hash' and the 'outer hash', respectively, of the HMAC function.
 *
 * After execution of this function, the @a hmac_ctx will contain the
 * precomputed inner and outer contexts, so that they may be used by
 * #fsl_shw_hmac().  The flags of @a hmac_ctx will be updated with
 * #FSL_HMAC_FLAGS_PRECOMPUTES_PRESENT to mark their presence.  In addtion, the
 * #FSL_HMAC_FLAGS_INIT flag will be set.
 *
 * @param      user_ctx  A user context from #fsl_shw_register_user().
 * @param      key_info  The key being used in this operation.  Key must be
 *                       1 to 64 octets long.
 * @param hmac_ctx The context which controls, by its flags and
 *                         algorithm, the operation of this function.
 * @return    A return code of type #fsl_shw_return_t.
 */
extern fsl_shw_return_t fsl_shw_hmac_precompute(fsl_shw_uco_t * user_ctx,
						fsl_shw_sko_t * key_info,
						fsl_shw_hmco_t * hmac_ctx);

/* REQ-FSL-SHW-PINTFC-COA-UCO */
/* REQ-FSLSHW-PINTFC-API-BASIC-HMAC-002 */
/*!
 * Continue, finalize, or one-shot an HMAC operation.
 *
 * There are a number of ways to use this function.  The flags in the
 * @a hmac_ctx object will determine what operations occur.
 *
 * If #FSL_HMAC_FLAGS_INIT is set, then the hash will be started either from
 * the @a key_info, or from the precomputed inner hash value in the
 * @a hmac_ctx, depending on the value of #FSL_HMAC_FLAGS_PRECOMPUTES_PRESENT.
 *
 * If, instead, #FSL_HMAC_FLAGS_LOAD is set, then the hash will be continued
 * from the ongoing inner hash computation in the @a hmac_ctx.
 *
 * If #FSL_HMAC_FLAGS_FINALIZE are set, then the @a msg will be padded, hashed,
 * the outer hash will be performed, and the @a result will be generated.
 *
 * If the #FSL_HMAC_FLAGS_SAVE flag is set, then the (ongoing or final) digest
 * value will be stored in the ongoing inner hash computation field of the @a
 * hmac_ctx.
 *
 * @param      user_ctx  A user context from #fsl_shw_register_user().
 * @param key_info       If #FSL_HMAC_FLAGS_INIT is set in the @a hmac_ctx,
 *                       this is the key being used in this operation, and the
 *                       IPAD.  If #FSL_HMAC_FLAGS_INIT is set in the @a
 *                       hmac_ctx and @a key_info is NULL, then
 *                       #fsl_shw_hmac_precompute() has been used to populate
 *                       the @a inner_precompute and @a outer_precompute
 *                       contexts.  If #FSL_HMAC_FLAGS_INIT is not set, this
 *                       parameter is ignored.

 * @param hmac_ctx The context which controls, by its flags and
 *                       algorithm, the operation of this function.
 * @param      msg               Pointer to the message to be hashed.
 * @param      length            Length, in octets, of the @a msg.
 * @param result            Pointer, of @a result_len octets, to where to
 *                               store the HMAC.
 * @param      result_len        Length of @a result buffer.
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
extern fsl_shw_return_t fsl_shw_hmac(fsl_shw_uco_t * user_ctx,
				     fsl_shw_sko_t * key_info,
				     fsl_shw_hmco_t * hmac_ctx,
				     const uint8_t * msg,
				     uint32_t length,
				     uint8_t * result, uint32_t result_len);

/* REQ-FSL-SHW-PINTFC-COA-UCO */
/* REQ-FSLSHW-PINTFC-API-BASIC-RNG-002 */
/*!
 * Get random data.
 *
 * @param      user_ctx  A user context from #fsl_shw_register_user().
 * @param      length    The number of octets of @a data being requested.
 * @param data      A pointer to a location of @a length octets to where
 *                       random data will be returned.
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
extern fsl_shw_return_t fsl_shw_get_random(fsl_shw_uco_t * user_ctx,
					   uint32_t length, uint8_t * data);

/* REQ-FSL-SHW-PINTFC-COA-UCO */
/* REQ-FSLSHW-PINTFC-API-BASIC-RNG-003 */
/*!
 * Add entropy to random number generator.
 *
 * @param      user_ctx  A user context from #fsl_shw_register_user().
 * @param      length    Number of bytes at @a data.
 * @param      data      Entropy to add to random number generator.
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
extern fsl_shw_return_t fsl_shw_add_entropy(fsl_shw_uco_t * user_ctx,
					    uint32_t length, uint8_t * data);

/* REQ-FSL-SHW-PINTFC-COA-UCO */
/* REQ-FSL-SHW-PINTFC-COA-SKO */
/*!
 * Perform Generation-Encryption by doing a Cipher and a Hash.
 *
 * Generate the authentication value @a auth_value as well as encrypt the @a
 * payload into @a ct (the ciphertext).  This is a one-shot function, so all of
 * the @a auth_data and the total message @a payload must passed in one call.
 * This also means that the flags in the @a auth_ctx must be #FSL_ACCO_CTX_INIT
 * and #FSL_ACCO_CTX_FINALIZE.
 *
 * @param      user_ctx         A user context from #fsl_shw_register_user().
 * @param      auth_ctx         Controlling object for Authenticate-decrypt.
 * @param      cipher_key_info  The key being used for the cipher part of this
 *                              operation.  In CCM mode, this key is used for
 *                              both parts.
 * @param      auth_key_info    The key being used for the authentication part
 *                              of this operation.  In CCM mode, this key is
 *                              ignored and may be NULL.
 * @param      auth_data_length Length, in octets, of @a auth_data.
 * @param      auth_data        Data to be authenticated but not encrypted.
 * @param      payload_length   Length, in octets, of @a payload.
 * @param      payload          Pointer to the plaintext to be encrypted.
 * @param ct               Pointer to the where the encrypted @a payload
 *                              will be stored.  Must be @a payload_length
 *                              octets long.
 * @param auth_value       Pointer to where the generated authentication
 *                              field will be stored. Must be as many octets as
 *                              indicated by MAC length in the @a function_ctx.
 * @return    A return code of type #fsl_shw_return_t.
 */
extern fsl_shw_return_t fsl_shw_gen_encrypt(fsl_shw_uco_t * user_ctx,
					    fsl_shw_acco_t * auth_ctx,
					    fsl_shw_sko_t * cipher_key_info,
					    fsl_shw_sko_t * auth_key_info,
					    uint32_t auth_data_length,
					    const uint8_t * auth_data,
					    uint32_t payload_length,
					    const uint8_t * payload,
					    uint8_t * ct, uint8_t * auth_value);

/* REQ-FSL-SHW-PINTFC-COA-UCO */
/* REQ-FSL-SHW-PINTFC-COA-SKO */
/*!
 * Perform Authentication-Decryption in Cipher + Hash.
 *
 * This function will perform a one-shot decryption of a data stream as well as
 * authenticate the authentication value.  This is a one-shot function, so all
 * of the @a auth_data and the total message @a payload must passed in one
 * call.  This also means that the flags in the @a auth_ctx must be
 * #FSL_ACCO_CTX_INIT and #FSL_ACCO_CTX_FINALIZE.
 *
 * @param      user_ctx         A user context from #fsl_shw_register_user().
 * @param      auth_ctx         Controlling object for Authenticate-decrypt.
 * @param      cipher_key_info  The key being used for the cipher part of this
 *                              operation.  In CCM mode, this key is used for
 *                              both parts.
 * @param      auth_key_info    The key being used for the authentication part
 *                              of this operation.  In CCM mode, this key is
 *                              ignored and may be NULL.
 * @param      auth_data_length Length, in octets, of @a auth_data.
 * @param      auth_data        Data to be authenticated but not decrypted.
 * @param      payload_length   Length, in octets, of @a ct and @a pt.
 * @param      ct               Pointer to the encrypted input stream.
 * @param      auth_value       The (encrypted) authentication value which will
 *                              be authenticated.  This is the same data as the
 *                              (output) @a auth_value argument to
 *                              #fsl_shw_gen_encrypt().
 * @param payload          Pointer to where the plaintext resulting from
 *                              the decryption will be stored.
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
extern fsl_shw_return_t fsl_shw_auth_decrypt(fsl_shw_uco_t * user_ctx,
					     fsl_shw_acco_t * auth_ctx,
					     fsl_shw_sko_t * cipher_key_info,
					     fsl_shw_sko_t * auth_key_info,
					     uint32_t auth_data_length,
					     const uint8_t * auth_data,
					     uint32_t payload_length,
					     const uint8_t * ct,
					     const uint8_t * auth_value,
					     uint8_t * payload);

/*!***************************************************************************
 *
 * Functions available to other SHW-family drivers.
 *
*****************************************************************************/

#ifdef __KERNEL__
/*!
 * Add an entry to a work/result queue.
 *
 * @param pool  Pointer to list structure
 * @param entry Entry to place at tail of list
 *
 * @return void
 */
inline static void SHW_ADD_QUEUE_ENTRY(shw_queue_t * pool,
				       shw_queue_entry_t * entry)
{
	os_lock_context_t lock_context;

	entry->next = NULL;
	os_lock_save_context(shw_queue_lock, lock_context);

	if (pool->tail != NULL) {
		pool->tail->next = entry;
	} else {
		/* Queue was empty, so this is also the head. */
		pool->head = entry;
	}
	pool->tail = entry;

	os_unlock_restore_context(shw_queue_lock, lock_context);

	return;

}

/*!
 * Get first entry on the queue and remove it from the queue.
 *
 * @return Pointer to first entry, or NULL if none.
 */
inline static shw_queue_entry_t *SHW_POP_FIRST_ENTRY(shw_queue_t * queue)
{
	shw_queue_entry_t *entry;
	os_lock_context_t lock_context;

	os_lock_save_context(shw_queue_lock, lock_context);

	entry = queue->head;

	if (entry != NULL) {
		queue->head = entry->next;
		entry->next = NULL;
		/* If this was only entry, clear the tail. */
		if (queue->tail == entry) {
			queue->tail = NULL;
		}
	}

	os_unlock_restore_context(shw_queue_lock, lock_context);

	return entry;
}

/*!
 * Remove an entry from the list.
 *
 * If the entry not on the queue, no error will be returned.
 *
 * @param pool Pointer to work queue
 * @param entry Entry to remove from queue
 *
 * @return void
 *
 */
inline static void SHW_QUEUE_REMOVE_ENTRY(shw_queue_t * pool,
					  shw_queue_entry_t * entry)
{
	os_lock_context_t lock_context;

	os_lock_save_context(shw_queue_lock, lock_context);

	/* Check for quick case. */
	if (pool->head == entry) {
		pool->head = entry->next;
		entry->next = NULL;
		if (pool->tail == entry) {
			pool->tail = NULL;
		}
	} else {
		register shw_queue_entry_t *prev = pool->head;

		/* We know it is not the head, so start looking at entry after head. */
		while (prev->next) {
			if (prev->next != entry) {
				prev = prev->next;	/* Try another */
				continue;
			} else {
				/* Unlink from chain. */
				prev->next = entry->next;
				entry->next = NULL;
				/* If last in chain, update tail. */
				if (pool->tail == entry) {
					pool->tail = prev;
				}
				break;
			}
		}		/* while */
	}

	os_unlock_restore_context(shw_queue_lock, lock_context);

	return;
}
#endif				/* __KERNEL__ */

/*!***************************************************************************
 *
 * Functions available to User-Mode API functions
 *
 ****************************************************************************/
#ifndef __KERNEL__

 /*!
  * Sanity checks the user context object fields to ensure that they make some
  * sense before passing the uco as a parameter.
  *
  * @brief Verify the user context object
  *
  * @param  uco  user context object
  *
  * @return    A return code of type #fsl_shw_return_t.
  */
fsl_shw_return_t validate_uco(fsl_shw_uco_t * uco);

/*!
 * Initialize a request block to go to the driver.
 *
 * @param hdr      Pointer to request block header
 * @param user_ctx Pointer to user's context
 *
 * @return void
 */
inline static void init_req(struct shw_req_header *hdr,
			    fsl_shw_uco_t * user_ctx)
{
	hdr->flags = user_ctx->flags;
	hdr->user_ref = user_ctx->user_ref;
	hdr->code = FSL_RETURN_ERROR_S;

	return;
}

/*!
 * Send a request block off to the driver.
 *
 * If this is a non-blocking request, then req will be freed.
 *
 * @param type  The type of request being sent
 * @param req   Pointer to the request block
 * @param ctx   Pointer to user's context
 *
 * @return code from driver if ioctl() succeeded, otherwise
 *              FSL_RETURN_INTERNAL_ERROR_S.
 */
inline static fsl_shw_return_t send_req(shw_user_request_t type,
					struct shw_req_header *req,
					fsl_shw_uco_t * ctx)
{
	fsl_shw_return_t ret = FSL_RETURN_INTERNAL_ERROR_S;
	unsigned blocking = ctx->flags & FSL_UCO_BLOCKING_MODE;
	int code;

	code = ioctl(ctx->openfd, SHW_IOCTL_REQUEST + type, req);

	if (code == 0) {
		if (blocking) {
			ret = req->code;
		} else {
			ret = FSL_RETURN_OK_S;
		}
	} else {
#ifdef FSL_DEBUG
		fprintf(stderr, "SHW: send_req failed with (%d), %s\n", code,
			strerror(code));
#endif
	}

	if (blocking) {
		free(req);
	}

	return ret;
}

#endif				/* no __KERNEL__ */

#endif				/* SHW_DRIVER_H */
