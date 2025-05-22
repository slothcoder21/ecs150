#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

#define SIGNATURE "ECS150FS" // String to identify the unique 
#define SIGNATURE_LEN 8
#define END_OF_CHAIN 0xFFFF // end of chain marker by 0xFFFF at 8

typedef struct attribs((packed)) {

	char signature[SIGNATURE_LEN];
	uint16_t block_total;
	uint16_t root_index; 
	uint16_t data_index_start;
	u_int16_t data_blocks;
	uint8_t fat_blocks;
	uint8_t _padding[BLOCK_SIZE - SIGNATURE_LEN - sizeof(uint16_t) * 4 - sizeof(uint8_t)];

} newblock_t;


int fs_mount(const char *diskname)
{
	/* TODO */

	return 0;
}

int fs_umount(void)
{
	/* TODO */
	return 0;
}

int fs_info(void)
{
	/* TODO */
	return 0;
}
