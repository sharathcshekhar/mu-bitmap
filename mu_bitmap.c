/*
 * bitmap.c is a part of Mu-BitMap - a multi-level bitmap library
 * 
 * Copyright (C) 2014  Sharath Chandrashekhara <sharathcshekhar@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <features.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include "mu_bitmap.h"
#include "mu_bitmap_priv.h"

/*
 * Structure of the BitMap in disk:
 * 	0-7 bytes	: Magic Number
 *	8-39 bytes	: Reserved
 *	40-47 bytes	: size of the bitmap
 *	48 bytes+	: Bitmap 
 */

struct m_bitmap *mbm_create_bitmap(uint32_t size, char *filename, uint64_t offset)
{
	assert((size % BITMAP_WIDTH) == 0);

	struct m_bitmap *bm = malloc(sizeof(struct m_bitmap));
	assert(bm);

	bm->size = size;
	bm->low_map = malloc(size * sizeof(uint64_t));
	memset(bm->low_map, 0, size * sizeof(uint64_t));

	if (filename != NULL) {
		strncpy(bm->filename, filename, MAX_FILE_NAME);
		bm->fd = open(bm->filename, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		assert(bm->fd > 0);
		lseek(bm->fd, offset, SEEK_SET);
		
		//TODO: write the magic number and advance by 32 bytes
		//for the reserved bits. For now, just advance by 40 bytes
		lseek(bm->fd, 40, SEEK_CUR);
		
		write(bm->fd, &(bm->size), sizeof(bm->size)); 
		write(bm->fd, bm->low_map, size * sizeof(uint64_t));
	} else {
		/* non-persistent bitmap */
		bm->fd = -1;
	}

	init_bitmap(bm);
	return bm;
}

bool mbm_is_bit_set(struct m_bitmap *bm, uint64_t bit)
{
	int index = bit/BITMAP_WIDTH;
	int bit_offset = bit % BITMAP_WIDTH;
	uint64_t mask = (uint64_t)1 << bit_offset;

	if (index >= bm->size) {
		return false;
	}

	if (bm->low_map[index] & mask) {
		return true;
	}

	return false;
}

int mbm_clear_bit(struct m_bitmap *bm, uint64_t bit)
{
	int index = bit / BITMAP_WIDTH;
	int bit_offset = bit % BITMAP_WIDTH;
	uint64_t mask = (uint64_t)1 << bit_offset;
	
	if (index >= bm->size) {
		return -1;
	}

	(bm->low_map)[index] &= (~mask);
	if (bm->fd != -1)
		write_word_to_disk(bm, index);

	/* set the associated mid-level bit */
	if (bm->mid_map == NULL) {
		return 0;
	}
	int mid_index = bit / (16 * 1024);
	int mid_offset = bit % (16 * 1024);
	mask = (uint64_t)1 << mid_offset;
	(bm->mid_map)[mid_index] &= (~mask);

	/* set the associated top-level bit */
	if (bm->top_map == NULL) {
		return 0;
	}
	int top_index = (mid_index + mid_offset) / (16 * 1024);
	int top_offset = (mid_index + mid_offset) % (16 * 1024);
	
	mask = (uint64_t)1 << top_offset;
	(bm->top_map)[top_index] &= (~mask);
	
	//TODO: WRITE THIS TO FILE
	return 0;
}

uint64_t mbm_set_first_clear(struct m_bitmap *bm)
{
	int offset;
	int i;
	uint64_t blk;
	uint64_t mid_index = 0, low_index = 0;
	int n_low_ints = bm->size / BITMAP_WIDTH;
	int n_mid_ints = (n_low_ints + (LOWMAP_SZ * BITMAP_WIDTH) - 1) / (LOWMAP_SZ * BITMAP_WIDTH);
	int n_top_ints = (n_mid_ints + (MIDMAP_SZ * BITMAP_WIDTH) - 1) / (MIDMAP_SZ * BITMAP_WIDTH);
	
	/* Find first free block in top level map */
	if (bm->top_map != NULL) {
		T(("Entered top level\n"));
		for (i = 0; i < n_top_ints; i++) {
			if ((bm->top_map)[i] != MAX_VAL_64BIT) {
				offset =  ffsl(~(bm->top_map)[i]) - 1;
				mid_index = (i * BITMAP_WIDTH + offset) * 16;
				break;
			}
		}
	}
	/* Find the first free block in mid level map */
	T(("Mid index = %lu\n", mid_index));
	if (bm->mid_map != NULL) {
		for (i = mid_index; i < 16; i++) {
			if ((bm->mid_map)[mid_index+i] != MAX_VAL_64BIT) {
				offset =  ffsl(~(bm->mid_map)[mid_index+i]) - 1;
				T(("Offset = %d, iteration = %d\n", offset, i));
				low_index = ((mid_index + i) * BITMAP_WIDTH + offset) * 16;
				break;
			}
		}
	}
	T(("low index = %lu\n", low_index));
	
	/* Find the exact free block in the low level map */
	for (i = 0; i < 16; i++) {
		if ((bm->low_map)[low_index+i] != MAX_VAL_64BIT) {
			offset =  ffsl(~(bm->low_map)[low_index + i]) - 1;
			blk = set_bit(bm, low_index+i, offset);
			return blk;
		}
	}
	
	return 0;
}

void mbm_free_bitmap(struct m_bitmap *bm)
{
	assert(bm);
	if (bm->top_map) {
		free(bm->top_map);
	}
	if (bm->mid_map) {
		free(bm->mid_map);
	}
	
	free(bm->low_map);
	
	if (bm->fd != -1) { 
		close(bm->fd);
	}
	free(bm);
}


struct m_bitmap* mbm_load_bitmap(char *filename, uint64_t offset)
{
	struct m_bitmap *bm = malloc(sizeof(struct m_bitmap));
	assert(bm);
	int ret = 0;

	bm->fd = open(filename, O_RDWR);
	assert(bm->fd > 0);
	lseek(bm->fd, offset, SEEK_SET);
	
	//TODO: Verify magic number, advance by 32 unused bytes.
	lseek(bm->fd, 40, SEEK_CUR);
	
	ret = read(bm->fd, &(bm->size), sizeof(uint64_t));
	bm->low_map = malloc(bm->size * sizeof(uint64_t));
	
	//TODO: write the magic number
	ret = read(bm->fd, bm->low_map, bm->size * sizeof(uint64_t));
	strncpy(bm->filename, filename, MAX_FILE_NAME);

	init_bitmap(bm);
	return bm;
}

int mbm_flush_to_disk(struct m_bitmap *bm)
{
	fsync(bm->fd);
	return 0;
}

/*
 * Private functions
 */

static uint64_t set_bit(struct m_bitmap *bm, int index, int offset)
{
	uint64_t mid_index = 0;
	uint64_t mid_offset = 0;
   	
	(bm->low_map)[index] |= ((uint64_t)1 << offset);
	if (bm->fd != -1)
		write_word_to_disk(bm, index);

	if ((bm->mid_map != NULL) && is_block_group_full(bm->low_map, index)) {
		
		/* do this if all blocks in the group are in use */
		uint64_t tmp = index / LOWMAP_SZ;
		mid_index = tmp / BITMAP_WIDTH;
		mid_offset = tmp % BITMAP_WIDTH;
		T(("Updating the mid-index = %lu, offset = %lu, index = %d, tmp = %lu\n", mid_index, mid_offset, index, tmp));
		(bm->mid_map)[mid_index] |= ((uint64_t)1 << mid_offset);

		if ((bm->top_map != NULL) && is_block_group_full(bm->mid_map, index)) {
			T(("Updating the top index\n"));
			uint64_t tmp = mid_index / MIDMAP_SZ;
			uint64_t top_index = tmp / BITMAP_WIDTH;
			uint64_t top_offset = tmp % BITMAP_WIDTH; 
			(bm->top_map)[top_index] |= ((uint64_t)1 << top_offset);
		}
	}

	//TODO: WRITE THIS TO FILE
	return ((index * BITMAP_WIDTH) + offset);
}


static bool is_block_group_full(uint64_t *bm, int index)
{
	int i;
	index = (index/16) * 16;
	for (i = 0; i < 16; i++){
		if (bm[index+i] != MAX_VAL_64BIT) {
			return false;
		}
	}
	T(("Block group is full\n"));
	return true;
}

static void init_bitmap(struct m_bitmap *bm)
{
	/* Number of 64 bit ints in lowest bitmap */
	int n_low_ints = bm->size;

	T(("Number of low ints = %d\n", n_low_ints));

	/* Number of 64 bit ints in mid bitmap */
	int n_mid_ints = 0;
	int n_mid_bits = (n_low_ints + LOWMAP_SZ - 1) / LOWMAP_SZ;

	T(("Number of mid bits = %d\n", n_mid_bits));
	if (n_mid_bits > 1) {
		n_mid_ints = (n_mid_bits + BITMAP_WIDTH - 1) / BITMAP_WIDTH;
	}
	T(("Number of mid ints = %d\n", n_mid_ints));
	
	/* Number of 64 bit ints in top bitmap */
	int n_top_ints = 0;
	int n_top_bits = (n_mid_ints + MIDMAP_SZ - 1) / MIDMAP_SZ;
	T(("Number of top bits = %d\n", n_top_bits));
	
	if (n_top_bits > 1) {
		n_top_ints = (n_top_bits + BITMAP_WIDTH - 1) / BITMAP_WIDTH;
	}
	T(("Number of top ints = %d\n", n_top_ints));

	if (n_mid_ints == 0) {
		bm->mid_map = NULL;
		bm->top_map = NULL;
	} else {
		bm->mid_map = malloc(n_mid_ints * sizeof(uint64_t));
		if (n_top_ints == 0) {
			bm->top_map = NULL;
		} else {
			bm->top_map = malloc(n_mid_ints * sizeof(uint64_t));
		}
	}
}

static void write_word_to_disk(struct m_bitmap *bm, uint64_t word_idx)
{
	int ret = 0;
	uint64_t word = (bm->low_map)[word_idx];
	uint64_t offset = bm->offset + BITMAP_INDEX_TO_OFFSET(word_idx);
	assert(bm->fd > 0);
	lseek(bm->fd, offset, SEEK_SET);
	ret = write(bm->fd, &word, sizeof(uint64_t));
	assert(ret > 0);
}
