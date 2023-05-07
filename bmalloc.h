typedef enum
{
	BestFit,
	FirstFit
} bm_option;

struct _bm_header
{
	unsigned int used : 1; // block 사용 시 1, if not 0
	// 2를 밑으로 하는 블록크기 지수, if size 4096 => 12
	unsigned int size : 4;
	// 다음 블록이 시작하는 주소, if not, NULL(0x0)
	struct _bm_header *next;
};

typedef struct _bm_header bm_header;
typedef struct _bm_header *bm_header_ptr;

void *bmalloc(size_t s);

void bfree(void *p);

void *brealloc(void *p, size_t s);

void bmconfig(bm_option opt);

void bmprint();
