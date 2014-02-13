#ifndef _MU_BITMAP_PRIV_H
#define _MU_BITMAP_PRIV_H

#define BEBUG 0 

#if DEBUG
	#define T(X) printf X
#else
	#define T(X)
#endif 

#define BITMAP_OFFSET	48
#define BITMAP_INDEX_TO_OFFSET(index) 	(((index) * 8) + BITMAP_OFFSET)

static uint64_t set_bit(struct m_bitmap *bm, int index, int offset);
static bool is_block_group_full(uint64_t *bm, int index);
static void write_word_to_disk(struct m_bitmap *bm, uint64_t word_idx);
static void init_bitmap(struct m_bitmap *bm);

#endif /* _MU_BITMAP_PRIV_H */
