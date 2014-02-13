/*
 * bitmap.h is a part of Mu-BitMap - a multi-level bitmap library
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
 *
 */

#ifndef _MU_BITMAP_H_
#define _MU_BITMAP_H_

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <features.h>

#define MAX_VAL_64BIT	(0xFFFFFFFFFFFFFFFF)
#define BITMAP_WIDTH 	(sizeof(uint64_t) * 8)
#define MAX_FILE_NAME 	255
#define MIDMAP_SZ	16
#define LOWMAP_SZ	16

/*
 * At the lowest level of the bitmap is a 64 bit integer. The state of each
 * bit in the bitmap is tracked by the lowest level bitmap called 'low_map'
 *
 * 1024 bits, consisting of 16 integers is grouped to a 'bit-group'. The bit
 * groups are tracked by a mid level bitmap called 'mid_map'
 * 
 * 1024 bit-groups, consisting of 16 integers is grouped into a bit-cluster. The
 * bit-cluster is tracked by the top most bitmap caled 'top_map'
 *
 * The bitmap consists of a number of bit-clusters. The number of bit-cluster
 * is equal to (size / (1024 * 1024))
 *
 * size - number of bits in the bitmap
 */

typedef struct m_bitmap {
	/* user sets passes fields at the time of init */
	uint32_t size;
	uint64_t offset;
	char filename[MAX_FILE_NAME];
	
	uint64_t *low_map;
	uint64_t *mid_map;
	uint64_t *top_map;
	int fd;
} m_bitmap_t;

/*
 * mbm_create_bitmap - creates a new bitmap
 *
 * Takes:
 * 	size		- size of the bitmap to be created
 * 	filename	- filename to which the map has to be flushed. If filename is NULL,
 * 			 	  bitmap is not written to disk
 * 	offset		- offset in the file where the bitmap is to be written
 *
 * Returns: pointer to the newly created bitmap structure
 *
 */ 
struct m_bitmap *mbm_create_bitmap(uint32_t size, char *filename, uint64_t offset);

/*
 * mbm_load_bitmap - Loads an existing bitmap to memory
 * 
 * Takes:
 * 	filename	- filename to which the map has to be flushed. If filename is NULL,
 * 			 	  bitmap is not written to disk
 * 	offset		- offset in the file where the bitmap was to be written
 *
 * Returns: pointer to the loaded bitmap structure
 */
struct m_bitmap* mbm_load_bitmap(char *filename, uint64_t offset);

/*
 * mbm_flush_to_disk - flush the bitmap to disk. The application has to explicitly
 * 					   call this function to make sure the contents are flushed to
 * 					   disk
 */
int mbm_flush_to_disk(struct m_bitmap *bm);

/*
 * free the bitmap
 */
void mbm_free_bitmap(struct m_bitmap *bm);

/*
 * get the status of the bit
 */
bool mbm_is_bit_set(struct m_bitmap *bm, uint64_t bit);

/*
 * clear the indicated bit
 */
int mbm_clear_bit(struct m_bitmap *bm, uint64_t blk);

/*
 * set the first clear bit and return that bit
 */
uint64_t mbm_set_first_clear(struct m_bitmap *bm);

/* 
 * Planned APIs in future version
 */
int mbm_get_size_on_disk(struct m_bitmap *bm);
uint64_t mbm_get_first_clear(struct m_bitmap *bm);
int mbm_set_bit(struct m_bitmap *bm, uint64_t blk);

#endif /* _MU_BITMAP_H_ */
