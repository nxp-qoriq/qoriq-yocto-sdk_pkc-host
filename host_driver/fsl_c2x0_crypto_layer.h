/* Copyright 2013 Freescale Semiconductor, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * Neither the name of Freescale Semiconductor nor the
 * names of its contributors may be used to endorse or promote products
 * derived from this software without specific prior written permission.
 *
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") as published by the Free Software
 * Foundation, either version 2 of that License or (at your option) any
 * later version.
 *
 * THIS SOFTWARE IS PROVIDED BY Freescale Semiconductor ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Freescale Semiconductor BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE)ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef FSL_PKC_CRYPTO_LAYER_H
#define FSL_PKC_CRYPTO_LAYER_H

#include "memmgr.h"
#include "types.h"

extern int napi_poll_count;

/* times in milliseconds */
#define HS_TIMEOUT	500
#define HS_LOOP_BREAK	5

/* the number of context pools is arbitrary and NR_CPUS is a good default
 * considering that worker threads using the contexts are local to a CPU.
 * However we set a conservative default until we fix malloc issues for x86 */
#define NR_CTX_POOLS 2

/* Identifies different states of the device */
typedef enum handshake_state {
	DEFAULT,
	FIRMWARE_UP = 10,
	FW_INIT_CONFIG_COMPLETE,
	FW_GET_SEC_INFO_COMPLETE,
	FW_INIT_RING_PAIR_COMPLETE,
	FW_INIT_MSI_INFO_COMPLETE,
	FW_INIT_IDX_MEM_COMPLETE,
	FW_INIT_COUNTERS_MEM_COMPLETE,
	FW_INIT_RNG,
	FW_RNG_COMPLETE
} handshake_state_t;

/* Identifies different commands to be sent to the firmware */
typedef enum fw_handshake_commands {
	FW_GET_SEC_INFO,
	FW_INIT_CONFIG,
	FW_INIT_RING_PAIR,
	FW_INIT_MSI_INFO,
	FW_INIT_IDX_MEM,
	FW_INIT_COUNTERS_MEM,
	FW_HS_COMPLETE,
	FW_WAIT_FOR_RNG,
	FW_RNG_DONE
} fw_handshake_commands_t;

/* Different types of memory between driver and ep */
typedef enum crypto_dev_mem_type {
	MEM_TYPE_CONFIG,
	MEM_TYPE_SRAM,
	MEM_TYPE_DRIVER,
	MEM_TYPE_MAX
} crypto_dev_mem_type_t;

/*******************************************************************************
Description :	Contains the configuration read from the file.
Fields      :	dev_no    : Number of the device to which this config applies.
		ring_id   : Identifies the ring Command/App
		flags     : Useful only for App to identify its properties
			0-4 : Priority level 32- priority levels
			5-7 : SEC engine affinity
			8   : Ordered/Un-ordered
		list      : To maintain list of config structures per device
*******************************************************************************/
struct crypto_dev_config {
	uint32_t dev_no;
/*  int8_t      *name;  We may not need this field  */
#define FIRMWARE_FILE_DEFAULT_PATH  "/etc/crypto/pkc-firmware.bin"
#define FIRMWARE_FILE_PATH_LEN  100
	uint8_t fw_file_path[FIRMWARE_FILE_PATH_LEN];

	uint8_t *firmware;

	uint8_t num_of_rps;

/* Safe MAX number of ring pairs -
 * Only required for some static data structures. */
#define FSL_CRYPTO_MAX_RING_PAIRS   6

	struct ring_info {
		uint32_t ring_id;
		uint32_t depth;
		uint32_t msi_addr_l;
		uint32_t msi_addr_h;
		uint16_t msi_data;
	} ring[FSL_CRYPTO_MAX_RING_PAIRS];

	struct list_head list;
};

#define JR_SIZE_SHIFT   0
#define JR_SIZE_MASK    0x0000ffff
#define JR_NO_SHIFT     16
#define JR_NO_MASK      0x00ff0000
#define SEC_NO_SHIFT    24
#define SEC_NO_MASK     0xff000000

/*** HANDSHAKE RELATED DATA STRUCTURES ***/

/***********************************************************************
Description : Defines the handshake memory on the host
Fields      :
***********************************************************************/
struct host_handshake_mem {
	uint8_t state;
	uint8_t result;

	union resp_data {
		struct fw_up_data {
			uint32_t p_ib_mem_base_l;
			uint32_t p_ib_mem_base_h;
			uint32_t p_ob_mem_base_l;
			uint32_t p_ob_mem_base_h;
			uint32_t no_secs;
		} device;
		struct config_data {
			uint32_t r_s_c_cntrs;
		} config;
		struct ring_data {
			uint32_t req_r;
			uint32_t intr_ctrl_flag;
		} ring;
	} data;
};

/*******************************************************************************
Description : Defines the handshake memory on the device
Fields      :
*******************************************************************************/
struct dev_handshake_mem {
	uint32_t h_ob_mem_l;
	uint32_t h_ob_mem_h;

	uint8_t state;
	uint8_t data_len;

	union cmd_data {
		/* these are communicated by the host to the device.
		 * Addresses are dma addresses on host for data located in OB mem */
		struct c_config_data {
			uint8_t num_of_rps;  /* total number of rings, in and out */
			uint32_t r_s_cntrs;/* dma address for other shadow counters */
		} config;
		struct c_ring_data {
			uint8_t rid;
			uint16_t msi_data;
			uint32_t depth;
			uint32_t resp_ring_offset;
			uint32_t msi_addr_l;
			uint32_t msi_addr_h;
		} ring;
	} data;
};

/*******************************************************************************
Description :	Defines the input buffer pool
Fields      :	pool		: Pool pointer returned by the pool manager
		drv_pool_addr	: Address in ib mem for driver's internal use
		dev_pool_base	: Holds the address of pool inside the device,
					will be required inside the SEC desc
*******************************************************************************/
typedef struct fsl_h_rsrc_pool {
	void *pool;

	void *drv_pool_addr;
	uint32_t dev_pool_base;
	uint32_t len;
} fsl_h_rsrc_pool_t;

/*******************************************************************************
Description :	Defines the ring indexes
Fields      :	w_index		: Request ring write index
		r_index		: Response ring read index
*******************************************************************************/
struct ring_idxs_mem {
	uint32_t w_index;
	uint32_t r_index;
};

/*******************************************************************************
Description :	Contains the counters per job ring. There will two copies one
		for local usage and one shadowed for firmware
Fields      :	Local memory
		jobs_added	: Count of number of req jobs added
		jobs_processed	: Count of number of resp jobs processed

		Shadow copy memory
		jobs_added	: Count of number of resp jobs added by fw
		jobs_processed	: Count of number of req jobs processed by fw
*******************************************************************************/
struct ring_counters_mem {
	uint32_t jobs_added;
	uint32_t jobs_processed;
};

/*******************************************************************************
Description :	Contains the total counters. There will two copies one
		for local usage and one shadowed for firmware
Fields      :	Local memory
		tot_jobs_added	: Total count of req jobs added by driver
		tot_jobs_processed: Total count of resp jobs processed

		Shadow copy memory
		tot_jobs_added	: Total count of resp jobs added by fw
		tot_jobs_processed: Total count of req jobs processed by fw
*******************************************************************************/
struct counters_mem {
	uint32_t tot_jobs_added;
	uint32_t tot_jobs_processed;
};

/**** RING PAIR RELATED DATA STRUCTURES ****/

/*******************************************************************************
Description : Identifies the request ring entry
Fields      : sec_desc        : DMA address of the sec addr valid in dev domain
*******************************************************************************/
struct req_ring_entry {
	dev_dma_addr_t sec_desc;
};

/*******************************************************************************
Description :	Identifies the response ring entry
Fields      :	sec_desc: DMA address of the sec addr valid in dev domain
		result	: Result word from sec engine
*******************************************************************************/
struct resp_ring_entry {
	dev_dma_addr_t sec_desc;
	volatile uint32_t result;
} __packed;

/*******************************************************************************
Description :	Contains the information about each ring pair
Fields      :	depth: Depth of the ring
		props: Valid only for application ring as :
			4bits : Priority level
			3bits :	Affinity level
			1bit  : Ordered/Un-ordered
		intr_ctrl_flag	: Address of intr ctrl flag on device. This will
				be used in data processing to enable/disable
				interrupt per ring.
		req_ring_addr	: Address of the request ring in ib window
		resp_ring_addr	: Response ring address in ob window
		pool		: Input buffer pool information
*******************************************************************************/
typedef struct fsl_h_rsrc_ring_pair {
	struct c29x_dev *c_dev;

	struct list_head isr_ctx_list_node;
	struct list_head bh_ctx_list_node;

	uint32_t *intr_ctrl_flag;
	struct buffer_pool *buf_pool;
	struct req_ring_entry *req_r;
	struct resp_ring_entry *resp_r;
	struct ring_idxs_mem *indexes;
	struct ring_counters_mem *counters;
	struct ring_counters_mem *r_s_cntrs;
	struct ring_counters_mem *shadow_counters;

	uint32_t depth;
	uint32_t core_no;

	spinlock_t ring_lock;
} fsl_h_rsrc_ring_pair_t;

struct pool_info {
	dma_addr_t h_dma_addr;
	void *h_v_addr;
	struct buffer_pool buf_pool;
};

/*******************************************************************************
Description :	Contains the structured layout of the driver mem - outbound mem
Fields      :	hs_mem	: Handshake memory - 64bytes
		request_rings_mem: Sequence of bytes for rings holding req ring
				mem and input buffer pool. Exact binding is
				updated in different data structure.
		idxs	: Memory of the ring pair indexes
		shadow_idxs: Memory of the shadow ring pair indexes
		counters: Memory of the counters per ring
		shadow_counters: Memory of the shadow counters per ring
*******************************************************************************/
struct host_mem_layout {
	struct host_handshake_mem hs_mem;
	struct resp_ring_entry *drv_resp_rings;
	struct ring_idxs_mem *idxs_mem;
	struct ring_counters_mem *cntrs_mem;
	struct ring_counters_mem *r_s_cntrs_mem;
	void *ip_pool;

};

struct driver_ob_mem {
	uint32_t hs_mem;
	uint32_t drv_resp_rings;
	uint32_t idxs_mem;
	uint32_t cntrs_mem;
	uint32_t r_s_cntrs_mem;
	uint32_t ip_pool;
};

typedef struct ctx_pool ctx_pool_t;

int32_t ring_enqueue(struct c29x_dev *c_dev, uint32_t jr_id,
			 dev_dma_addr_t sec_desc);

int32_t fsl_crypto_layer_add_device(struct c29x_dev *c_dev,
		struct crypto_dev_config *config);

void cleanup_crypto_device(struct c29x_dev *c_dev);
int32_t handshake(struct c29x_dev *c_dev, struct crypto_dev_config *config);
void rearrange_rings(struct c29x_dev *c_dev, struct crypto_dev_config *config);
void distribute_rings(struct c29x_dev *c_dev, struct crypto_dev_config *config);
void init_ip_pool(struct c29x_dev *c_dev);
int init_crypto_ctx_pool(struct c29x_dev *c_dev);
void init_handshake(struct c29x_dev *c_dev);
void init_ring_pairs(struct c29x_dev *c_dev);
void stop_device(struct c29x_dev *c_dev);
void start_device(struct c29x_dev *c_dev);
void response_ring_handler(struct work_struct *work);

extern int32_t rng_instantiation(struct c29x_dev *c_dev);

#endif
