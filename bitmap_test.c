#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "bitmap.h"
#define  BITMAP_SZ 	(64 * 1024 * 1024)

#define METADATA_FILE "./metadata.dat"

int main()
{
	struct m_bitmap *bitmap;
	uint64_t free_blk;
	uint64_t i;
	bitmap = mbm_create_bitmap(BITMAP_SZ, NULL, 0);
	for (i = 0; i < 1024; i++) {
		free_blk = get_first_free_blk(bitmap);
		if (free_blk != i) {
			printf("freeblk %lu expected, recieved %lu\n", i, free_blk);
			return -1;
		}
	}
	
	free_blk = get_first_free_blk(bitmap);
	if (free_blk != 1024) {
		printf("freeblk 1024 expected, recieved %lu\n", free_blk);
		return -1;
	}
	
	int ret = mbm_clear_bit(bitmap, 100);
	if (ret != 0) {
		printf("clear_blk failed\n");
			return -1;
	}
	ret = mbm_clear_bit(bitmap, 512);
	if (ret != 0) {
		printf("clear_blk failed\n");
			return -1;
	}
	ret = mbm_clear_bit(bitmap, 1);
	if (ret != 0) {
		printf("clear_blk failed\n");
			return -1;
	}
	
	free_blk = get_first_free_blk(bitmap);
	if (free_blk != 1) {
		printf("freeblk 1 expected, recieved %lu\n", free_blk);
		return -1;
	}
	
	ret = mbm_clear_bit(bitmap, 1002);
	if (ret != 0) {
		printf("clear_blk failed\n");
			return -1;
	}
	ret = mbm_clear_bit(bitmap, 800);
	if (ret != 0) {
		printf("clear_blk failed\n");
			return -1;
	}
	ret = mbm_clear_bit(bitmap, 511);
	if (ret != 0) {
		printf("clear_blk failed\n");
			return -1;
	}
	
	free_blk = get_first_free_blk(bitmap);
	if (free_blk != 100) {
		printf("freeblk 100 expected, recieved %lu\n", free_blk);
		return -1;
	}
	
	free_blk = get_first_free_blk(bitmap);
	if (free_blk != 511) {
		printf("freeblk 511 expected, recieved %lu\n", free_blk);
		return -1;
	}
	
	mbm_free_bitmap(bitmap);
	printf("Test Passed\n");
	return 0;
}
