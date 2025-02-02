/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Jerry Wang <wangflord@allwinnertech.com>
 */
#include <common.h>

#define  SYSTEM_PAGE_SIZE        (512)
#define  SPINOR_PAGE_SIZE        (256)
#define  NPAGE_IN_1SYSPAGE       (SYSTEM_PAGE_SIZE/SPINOR_PAGE_SIZE)
#define  SPINOR_BLOCK_BYTES      (64 * 1024)
#define  SPINOR_BLOCK_SECTORS    (SPINOR_BLOCK_BYTES/512)

/* instruction definition */
#define SPINOR_READ        0x03
#define SPINOR_FREAD       0x0b
#define SPINOR_WREN        0x06
#define SPINOR_WRDI        0x04
#define SPINOR_RDSR        0x05
#define SPINOR_WRSR        0x01
#define SPINOR_PP          0x02
#define SPINOR_SE          0xd8
#define SPINOR_BE          0x60
#define SPINOR_RDID        0x9f

int spinor_read_id(void)
{
	uint id = 0;
	uint sdata = SPINOR_RDID;

	int txnum;
	int rxnum;

	txnum = 1;
	rxnum = 3;
	spic_config_dual_mode(0, 0, 0, txnum);
	spic_rw(txnum, (void *)&sdata, rxnum, (void *)(&id));
	printf("spinor id is %x\n", id);

	return 0;
}

int spinor_init(int stage)
{
	if (spic_init(0))
		return -1;

	spinor_read_id();

	return 0;
}

int spinor_exit(int stage)
{
	return 0;
}

int spinor_read(uint start, uint sector_cnt, void *buffer)
{
	__u32 page_addr;
	__u32 rbyte_cnt;
	__u8  sdata[4] = {0};
	int   ret = 0;
	__u32 tmp_cnt, tmp_offset = 0;
	void  *tmp_buf;
	__u32 txnum, rxnum;
	__u8  *buf = (__u8 *)buffer;

	txnum = 4;

	while (sector_cnt) {
		if (sector_cnt > 127)
			tmp_cnt = 127;
		else
			tmp_cnt = sector_cnt;

		page_addr = (start + tmp_offset) * SYSTEM_PAGE_SIZE;
		rbyte_cnt = tmp_cnt * SYSTEM_PAGE_SIZE;
		sdata[0]  =  SPINOR_READ;
		sdata[1]  = (page_addr >> 16) & 0xff;
		sdata[2]  = (page_addr >> 8) & 0xff;
		sdata[3]  =  page_addr        & 0xff;

		rxnum   = rbyte_cnt;
		tmp_buf = (__u8 *)buf + (tmp_offset << 9);
		spic_config_dual_mode(0, 0, 0, txnum);
		if (spic_rw(txnum, (void *)sdata, rxnum, tmp_buf)) {
			ret = -1;
			break;
		}

		sector_cnt -= tmp_cnt;
		tmp_offset += tmp_cnt;
	}

	return ret;
}

