#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include "mu_bitmap.h"
#define  BITMAP_SZ 	(64 * 1024 * 1024)

#define METADATA_FILE "./metadata.dat"

int main()
{
	struct m_bitmap *bitmap;
	uint64_t free_blk;
	uint64_t i;

	bitmap = mbm_create_bitmap(BITMAP_SZ, METADATA_FILE, 0);

	if (bitmap == NULL) {
		printf("Create test failed...\n");
		return -1;
	}
	
	printf("Create test passed...\n");

	for (i = 0; i < 1024; i++) {
		free_blk = mbm_set_first_clear(bitmap);
		if (free_blk != i) {
			printf("freeblk %lu expected, recieved %lu\n", i, free_blk);
			return -1;
		}
	}
	
	printf("set_bit test-1 passed...\n");

	free_blk = mbm_set_first_clear(bitmap);
	if (free_blk != 1024) {
		printf("freeblk 1024 expected, recieved %lu\n", free_blk);
		return -1;
	}
	
	printf("set_bit test-2 passed...\n");

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
	
	printf("clear_bit test-1 passed...\n");

	free_blk = mbm_set_first_clear(bitmap);
	if (free_blk != 1) {
		printf("freeblk 1 expected, recieved %lu\n", free_blk);
		return -1;
	}
	printf("set_bit test-3 passed...\n");

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
	printf("clear_bit test-2 passed...\n");

	free_blk = mbm_set_first_clear(bitmap);
	if (free_blk != 100) {
		printf("freeblk 100 expected, recieved %lu\n", free_blk);
		return -1;
	}

	free_blk = mbm_set_first_clear(bitmap);
	if (free_blk != 511) {
		printf("freeblk 511 expected, recieved %lu\n", free_blk);
		return -1;
	}

	printf("set_bit test-4 passed...\n");
	
	mbm_flush_to_disk(bitmap);
	mbm_free_bitmap(bitmap);

	bitmap = mbm_load_bitmap(METADATA_FILE, 0);
	
	if (bitmap == NULL) {
		printf("load test failed...\n");
		return -1;
	}	
	printf("load test passed...\n");
	
	if (!mbm_is_bit_set(bitmap, 511)) {
		printf("Wrong status in metadata file for bit 511\n");
		return -1;
	}

	if (!mbm_is_bit_set(bitmap, 100)) {
		printf("Wrong status in metadata file for bit 100\n");
		return -1;
	}

	if (mbm_is_bit_set(bitmap, 800)) {
		printf("Wrong status in metadata file for bit 800\n");
		return -1;
	}
	
	if (mbm_is_bit_set(bitmap, 1002)) {
		printf("Wrong status in metadata file for bit 1002\n");
		return -1;
	}
	
	printf("Load metadata test passed...\n");
	
	mbm_free_bitmap(bitmap);
	unlink(METADATA_FILE);
	printf("All tests Passed\n");
	return 0;
}
