#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/mman.h>
#include "bmalloc.h"

#define BLOCK_SIZE 12
#define MIN_BLOCK_SIZE 4 // 분할할 블록의 최소 크기

bm_option bm_mode = BestFit;
bm_header_ptr bm_list_head = {0, 0, 0x0};
bm_header_ptr free_list[10]; // 각각의 크기에 맞는 블록 리스트

bm_header_ptr buddy_heap_start = NULL;
void *buddy_heap = NULL;

void init_buddy_heap()
{
	buddy_heap = mmap(NULL, pow(2, BLOCK_SIZE), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (buddy_heap == MAP_FAILED)
	{
		perror("mmap failed");
		exit(1);
	}
	bm_list_head = (bm_header_ptr)buddy_heap;
	bm_list_head->used = 0;
	bm_list_head->size = BLOCK_SIZE;
	bm_list_head->next = NULL;
}

/*
	head의 예상 sibling header 주소를 return한다.
	h와 형제의 블록 크기가 다를 경우 sibling(h)가 h의 형제가 아닐 수 있습니다.
*/
void *sibling(bm_header *h)
{
	// TODO
	// 주어진 블록의 형제 블록 주소를 반환
	int block_size = 1 << h->size;
	bm_header *parent = (bm_header *)((char *)h - block_size);
	int parent_size = h->size + 1;
	int parent_level = parent_size - 1;
	int parent_block_size = 1 << parent_size;
	int parent_index = ((char *)parent - (char *)free_list) / parent_block_size;
	int sibling_index = (parent_index % 2 == 0) ? (parent_index + 1) : (parent_index - 1);
	bm_header *sibling = (bm_header *)((char *)free_list + sibling_index * parent_block_size);
	return (void *)sibling;
}
/*
	주어진 크기 s에 맞는 가장 작은 블록의 크기 필드 값을 반환한다.
*/
int fitting(size_t s)
{
	// TODO
	// 요청한 크기 s에 맞는 가장 작은 블록의 크기 필드 값을 반환
	int size = 4;
	while (pow(2, size) < s)
	{
		size++;
	}
	return size;
}
// 블록을 반으로 분할하고, 나머지 블록을 반환
bm_header_ptr split_block(bm_header_ptr block)
{
	int block_size = pow(2, block->size);
	block->size--;
	bm_header_ptr buddy = (bm_header_ptr)((char *)block + block_size);
	buddy->size = block->size;
	buddy->used = 0;
	buddy->next = free_list[block_size - 1];
	free_list[block_size - 1] = buddy;
	return buddy;
}

/*
	주어진 s에 대해 맞는 블록이 있으면 그 블록을 사용. 만약 맞는 블록이 없다면,
	"BestFit(기본값)"인 경우, Fitting block size 보다 큰 unused block 중에서 가장 작은 블록을 선택하여 분할한다.
	"FirstFit"인 경우, select the first feasible block in the linked list, 이를 분할하여 피팅 블록을 가져온다.
*/
void *bmalloc(size_t s)
{
	// TODO
	if (buddy_heap_start == NULL)
	{
		init_buddy_heap();
		buddy_heap_start = bm_list_head;
		// TEST
		printf("%d, %d, %p\n", buddy_heap_start->used, buddy_heap_start->size, buddy_heap_start->next);
	}
	int size = fitting(s);
	int list_index = size - 1;
	if (free_list[list_index] == NULL)
	{
		// 해당 크기의 블록이 없으면 더 큰 블록에서 분할
		bm_header_ptr buddy_block = bmalloc(size);
		if (buddy_block == NULL)
		{ // 더 큰 블록에서도 사용 가능한 블록이 없는 경우
			return NULL;
		}
		bm_header_ptr split_blk = split_block(buddy_block);
		split_blk->used = 1;
		return (void *)(split_blk + 1);
	}
	else
	{
		// 해당 크기의 블록이 있으면 그 블록을 반환
		bm_header_ptr block = free_list[list_index];
		free_list[list_index] = block->next;
		block->used = 1;
		return (void *)(block + 1);
	}
}
// buddy 블록을 반환
bm_header_ptr get_buddy(bm_header_ptr block)
{
	int block_size = pow(2, block->size - 1);
	return (bm_header_ptr)((char *)block - block_size);
}
// 두 블록을 합쳐서 더 큰 블록을 만들고, 그 블록을 반환
bm_header_ptr merge_blocks(bm_header_ptr block, bm_header_ptr buddy)
{
	int block_size = pow(2, block->size);
	bm_header_ptr merged_block = (bm_header_ptr)((char *)block - block_size);
	merged_block->size++;
	merged_block->next = free_list[get_list_index(block_size * 2)];
	free_list[get_list_index(block_size * 2)] = merged_block;
	return merged_block;
}
/*
	사용 중인 블록을 사용하지 않는 상태로 전환 후, 만약 sibling이 모두 사용되고 있지 않다면 merge, merging의 경우는 가능한 경우까지 상위로 계속 반복해야 함. 전체 페이지가 모두 비었다면 페이지를 해제한다.
*/
void bfree(void *p)
{
	// TODO
	bm_header_ptr block = (bm_header_ptr)p - 1;
	block->used = 0;
	while (block->size < 4)
	{
		// 최소 크기의 블록까지 합치기
		bm_header_ptr buddy = get_buddy(block);
		if (buddy->used == 1)
		{
			break;
		}
		block = merge_blocks(block, buddy);
	}
	block->next = free_list[get_list_index(pow(2, block->size))];
	free_list[get_list_index(pow(2, block->size))] = block;
}
/*
	allocate된 메모리 버퍼 크기를 주어진 s 사이즈로 조정한다.
	이 결과로 데이터는 다른 주소로 이동될 수 있다. perform like realloc().
*/
void *brealloc(void *p, size_t s)
{
	// TODO
	bm_header_ptr block = (bm_header_ptr)p - 1;
	int old_size = pow(2, block->size);
	int new_size = get_block_size(s);
	if (new_size == old_size)
	{
		// 크기가 같으면 그대로 반환
		return p;
	}
	else if (new_size < old_size)
	{
		// 블록을 분할해서 크기를 맞춤
		while (block->size > log2(new_size))
		{
			bm_header_ptr buddy = split_block(block);
			buddy->used = 0;
		}
		return p;
	}
	else
	{
		// 새로운 블록을 할당하고 데이터를 복사한 후 이전 블록 해제
		void *new_block = bmalloc(s);
		if (new_block == NULL)
		{
			return NULL;
		}
		memcpy(new_block, p, old_size);
		bfree(p);
		return new_block;
	}
}
/*
	bestfit, firstfit 사이의 관리 방식을 설정해준다.
*/
void bmconfig(bm_option opt)
{
	// TODO
	bm_mode = opt;
}
/* 연결된 리스트의 각 블록 통계 및 상태 표시 */
void bmprint()
{
	bm_header_ptr itr;
	int i;

	printf("==================== bm_list ====================\n");
	for (itr = bm_list_head->next, i = 0; itr != 0x0; itr = itr->next, i++)
	{
		printf("%3d:%p:%1d %8d:", i, ((void *)itr) + sizeof(bm_header), (int)itr->used, (int)itr->size);

		int j;
		char *s = ((char *)itr) + sizeof(bm_header);
		for (j = 0; j < (itr->size >= 8 ? 8 : itr->size); j++)
			printf("%02x ", s[j]);
		printf("\n");
	}
	printf("=================================================\n");

	// TODO: print out the stat's.
}
