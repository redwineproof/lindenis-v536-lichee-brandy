/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Jerry Wang <wangflord@allwinnertech.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>
#include <malloc.h>
#include "../../nand_interface/nand_bsp.h"


#define OOB_BUF_SIZE					64
#define NAND_BOOT0_BLK_START			0
#define NAND_BOOT0_BLK_CNT				2
#define NAND_UBOOT_BLK_START			(NAND_BOOT0_BLK_START+NAND_BOOT0_BLK_CNT)
#define NAND_UBOOT_BLK_CNT				6
#define NAND_BOOT0_PAGE_CNT_PER_COPY	64

static char nand_para_store[256];
static int flash_scaned;
static struct _nand_info *g_nand_info;
static int nand_partition_num;

int mbr_burned_flag;
PARTITION_MBR nand_mbr = {0};

extern void *NAND_Malloc(unsigned int Size);
extern void NAND_Free(void *pAddr, unsigned int Size);
extern int NAND_Print(const char *str, ...);
extern int NAND_Print_DBG(const char *str, ...);
extern int NAND_set_boot_mode(__u32 boot);
__u32 NAND_GetNandCapacityLevel(void);
extern int get_uboot_start_block(void);
extern int get_uboot_next_block(void);
extern __u32 get_storage_type(void);
extern int rawnand_is_blank(void);

int __NAND_UpdatePhyArch(void)
{
	NAND_Print("call null __NAND_UpdatePhyArch()!!!\n");
	return 0;
}
int NAND_UpdatePhyArch(void)
    __attribute__((weak, alias("__NAND_UpdatePhyArch")));

int msg(const char *str, ...)
{
	NAND_Print(str);
	return 0;
}

int NAND_PhyInit(void)
{
	struct _nand_info *nand_phy_info;

	NAND_Print("NB1 : enter phy init\n");

	nand_phy_info = NandHwInit();
	if (nand_phy_info == NULL) {
		NAND_Print("NB1 : nand phy init fail\n");
		return -1;
	}

	NAND_Print("NB1 : nand phy init ok\n");
	return 0;
}

int NAND_PhyExit(void)
{
	NAND_Print("NB1 : enter phy Exit\n");
	NandHwExit();
	return 0;
}

int NAND_LogicWrite(uint nSectNum, uint nSectorCnt, void *pBuf)
{
	return nftl_write(nSectNum, nSectorCnt, pBuf);
}

int NAND_LogicRead(uint nSectNum, uint nSectorCnt, void *pBuf)
{
	return nftl_read(nSectNum, nSectorCnt, pBuf);
}

extern void do_nand_interrupt(u32 no);
int NAND_LogicInit(int boot_mode)
{
	__s32 result = 0;
	__s32 ret = -1;
	__s32 i, nftl_num, capacity_level;
	struct _nand_info *nand_info;
	 /*char* mbr;*/

	NAND_Print("NB1: enter NAND_LogicInit\n");

	nand_info = NandHwInit();
	capacity_level = NAND_GetNandCapacityLevel();
	set_capacity_level(nand_info, capacity_level);

	g_nand_info = nand_info;
	if (nand_info == NULL) {
		NAND_Print("NB1: nand phy init fail\n");
		return ret;
	}

	if ((!boot_mode) && (nand_mbr.PartCount != 0) &&
	    (mbr_burned_flag == 0)) {
		NAND_Print(
		    "burn nand partition table! mbr tbl: 0x%x, part_count:%d\n",
		    (__u32)(&nand_mbr), nand_mbr.PartCount);
		result = nand_info_init(nand_info, 0, 8, (uchar *)&nand_mbr);
		mbr_burned_flag = 1;
	} else {
		NAND_Print("not burn nand partition table!\n");
		result = nand_info_init(nand_info, 0, 8, NULL);
	}

	if (result != 0) {
		NAND_Print("NB1: nand_info_init fail\n");
		return -5;
	}

	if (boot_mode) {
		nftl_num = get_phy_partition_num(nand_info);
		NAND_Print("NB1: nftl num: %d\n", nftl_num);
		if ((nftl_num < 1) || (nftl_num > 5)) {
			NAND_Print("NB1: nftl num: %d error\n", nftl_num);
			return -1;
		}

		nand_partition_num = 0;
		for (i = 0; i < ((nftl_num > 1) ? (nftl_num - 1) : 1); i++) {
			nand_partition_num++;
			NAND_Print("init nftl: %d\n", i);
			result = nftl_build_one(nand_info, i);
		}
	} else {
		result = nftl_build_all(nand_info);
		nand_partition_num = get_phy_partition_num(nand_info);
	}

	if (result != 0) {
		NAND_Print("NB1: nftl_build_all fail\n");
		return -5;
	}

	NAND_Print("NB1: NAND_LogicInit ok, result = 0x%x\n", result);

	return result;
}

int NAND_LogicExit(void)
{
	NAND_Print("NB1: NAND_LogicExit\n");
	nftl_flush_write_cache();
	NandHwExit();
	g_nand_info = NULL;

	return 0;
}

int NAND_build_all_partition(void)
{
	int result, i;
	int nftl_num;

	if (g_nand_info == NULL) {
		NAND_Print("NAND_build_all_partition fail\n");
		return -1;
	}

	nftl_num = get_phy_partition_num(g_nand_info);
	if (nftl_num == nand_partition_num)
		return 0;

	if ((nand_partition_num >= nftl_num) || (nand_partition_num == 0)) {
		NAND_Print("NAND_build_all_partition fail %d\n",
			   nand_partition_num);
		return -1;
	}

	for (i = nand_partition_num; i < nftl_num; i++) {
		NAND_Print(" init nftl: %d\n", i);
		result = nftl_build_one(g_nand_info, i);
		if (result != 0) {
			NAND_Print("NAND_build_all_partition fail %d %d\n",
				   result, i);
			return -1;
		}
	}

	return 0;
}

int NAND_VersionGet(unsigned char *version)
{
	__u32 nand_version;

	nand_version = nand_get_nand_version();
	version[0] = 0xff; /* bad block flag*/
	version[1] = 0x00; /* reserved, set to 0x00*/
	version[2] = (nand_version >>
		      16); /* nand driver version 0, current vresion is 0x02*/
	version[3] = (nand_version >>
		      24); /* nand driver version 1, current vresion is 0x00*/

	return 0;
}

int NAND_VersionCheck(void)
{
	struct boot_physical_param boot0_readop_temp;
	struct boot_physical_param *boot0_readop = NULL;
	uint block_index;
	int version_match_flag = -1;
	unsigned char oob_buf[64];
	unsigned char nand_version[4];
	int uboot_start_block, uboot_next_block;

	/********************************************************************************
	*   nand_version[2] = 0xFF;          //the sequnece mode version <
	*   nand_version[2] = 0x01;          //the first interleave mode
	*version, care ecc
	*                                      2010-06-05
	*   nand_version[2] = 0x02;          //the current version, don't care
	*ecc
	*                                      2010-07-13
	*   NOTE:  need update the nand version in update_boot0 at the same time
	    ********************************************************************************/
	NAND_VersionGet(nand_version);

	uboot_start_block = get_uboot_start_block();
	uboot_next_block = get_uboot_next_block();
	NAND_Print_DBG("uboot_start_block %d uboot_next_block %d.\n",
		       uboot_start_block, uboot_next_block);

	NAND_Print_DBG("check nand version start.\n");
	NAND_Print_DBG("Current nand driver version is %x %x %x %x\n",
		       nand_version[0], nand_version[1], nand_version[2],
		       nand_version[3]);

	boot0_readop = &boot0_readop_temp;

	/*init boot0_readop*/
	boot0_readop->block = 0x0;
	boot0_readop->chip = 0;
	boot0_readop->mainbuf = (void *)NAND_Malloc(32 * 1024);
	if (!boot0_readop->mainbuf) {
		NAND_Print("malloc memory for boot0 read operation fail\n");
		return -1;
	}

	boot0_readop->oobbuf = oob_buf;
	boot0_readop->page = 0;
	boot0_readop->sectorbitmap = 0;

	/*scan boot1 area blocks*/
	for (block_index = uboot_start_block; block_index < uboot_next_block;
	     block_index++) {

		boot0_readop->block = block_index;
		boot0_readop->page = 0;
		nand_physic_read_page(
		    boot0_readop->chip, boot0_readop->block, boot0_readop->page,
		    32, boot0_readop->mainbuf, boot0_readop->oobbuf);

		/*check the current block is a bad block*/
		if (oob_buf[0] != 0xFF) {
			NAND_Print("block %u is bad block %x.\n", block_index,
				   oob_buf[0]);
			continue;
		}

		if ((oob_buf[1] == 0x00) || (oob_buf[1] == 0xFF)) {
			NAND_Print("Media version is valid in block %u, "
				   "version info is %x %x %x %x\n",
				   block_index, oob_buf[0], oob_buf[1],
				   oob_buf[2], oob_buf[3]);
			if (oob_buf[2] == nand_version[2]) {
				NAND_Print("nand driver version match ok in "
					   "block %u.\n",
					   block_index);
				version_match_flag = 0;
				break;
			} else {
				NAND_Print("nand driver version match fail in "
					   "block %u.\n",
					   block_index);
				version_match_flag = 1;
				break;
			}

		} else {
			NAND_Print("Media version is invalid in block %u, "
				   "version info is %x %x %x %x\n",
				   block_index, oob_buf[0], oob_buf[1],
				   oob_buf[2], oob_buf[3]);
		}
	}

	if (block_index >= uboot_next_block) {
		NAND_Print("can't find valid version info in boot blocks.\n");
		version_match_flag = -1;
	}

	NAND_Free(boot0_readop->mainbuf, 32 * 1024);

	return version_match_flag;
}

extern void nand_special_test(void);
int NAND_EraseBootBlocks(void)
{
	int i, boot0_block_cnt;

	NAND_Print("has cleared the boot blocks.\n");
	nand_special_test();
	boot0_block_cnt = get_uboot_start_block();

	for (i = 0; i < boot0_block_cnt; i++) {
		nand_physic_erase_block(0, i);
	}
	return 0;
}

int NAND_EraseChip(void)
{
	return nand_uboot_erase_all_chip(0);
}

int NAND_EraseChip_force(void)
{
	return nand_uboot_erase_all_chip(1);
}

int NAND_BadBlockScan(void)
{
	return 0;
}

int NAND_UbootInit(int boot_mode)
{
	int ret = 0;

	NAND_Print("NAND_UbootInit start\n");

	NAND_set_boot_mode(boot_mode);
	/* logic init */
	ret |= NAND_LogicInit(boot_mode);

	if (!flash_scaned) {
		nand_get_param((boot_nand_para_t *)nand_para_store);
		flash_scaned = 1;
	}

	NAND_Print("NAND_UbootInit end: 0x%x\n", ret);

	return ret;
}

int NAND_UbootExit(void)
{
	int ret = 0;

	NAND_Print_DBG("NAND_UbootExit\n");
	ret = NAND_LogicExit();

	return ret;
}

int NAND_UbootProbe(void)
{
	int ret = 0;

	NAND_Print_DBG("NAND_UbootProbe start\n");

	/* logic init */
	ret = NAND_PhyInit();
	NAND_PhyExit();

	NAND_Print_DBG("NAND_UbootProbe end: 0x%x\n", ret);

	return ret;
}

uint NAND_UbootRead(uint start, uint sectors, void *buffer)
{
	return NAND_LogicRead(start, sectors, buffer);
}

uint NAND_UbootWrite(uint start, uint sectors, void *buffer)
{
	return NAND_LogicWrite(start, sectors, buffer);
}

extern int nand_super_page_test(unsigned int chip, unsigned int block,
				unsigned int page);
int NAND_Uboot_Erase(int erase_flag)
{
	int version_match_flag;
	int nand_erased = 0;

	NAND_Print("erase_flag = %d\n", erase_flag);
	NAND_PhyInit();

	if (erase_flag) {
		NAND_Print("erase by flag %d\n", erase_flag);
		NAND_EraseBootBlocks();
		NAND_EraseChip();
		NAND_UpdatePhyArch();
		nand_erased = 1;
	} else {
		version_match_flag = NAND_VersionCheck();
		NAND_Print("nand version = %x\n", version_match_flag);
		NAND_EraseBootBlocks();

		if (get_storage_type() == 1) {
			if (rawnand_is_blank() == 1)
				NAND_EraseChip();
		}

		if (version_match_flag > 0) {
			NAND_Print(
			    "nand version fail,please select erase nand\n");
			nand_erased = -1;
		}
	}
	NAND_Print("NAND_Uboot_Erase\n");
	NAND_PhyExit();
	return nand_erased;
}

int NAND_Uboot_Force_Erase(void)
{
	NAND_Print("force erase\n");
	if (NAND_PhyInit()) {
		NAND_Print("phy init fail\n");
		return -1;
	}

	NAND_EraseChip_force();

	NAND_PhyExit();

	return 0;
}

int NAND_BurnBoot0(uint length, void *buffer)
{
	return nand_write_nboot_data(buffer, length);
}

int NAND_ReadBoot0(uint length, void *buffer)
{
	return nand_read_nboot_data(buffer, length);
}

int NAND_BurnUboot(uint length, void *buffer)
{
	return nand_write_uboot_data(buffer, length);
}

int NAND_GetParam_store(void *buffer, uint length)
{
	if (!flash_scaned) {
		NAND_Print("force flash init to begin hardware scanning\n");
		NAND_PhyInit();
		NAND_PhyExit();
		NAND_Print("hardware scan finish\n");
	}

	nand_get_param_for_uboottail((void *)nand_para_store);

	memcpy(buffer, nand_para_store, length);

	return 0;
}

int NAND_FlushCache(void)
{
	return nftl_flush_write_cache();
}
