#include <inttypes.h>
#include <string.h>

#include "eeprom.h"
#include "param.h"
#include "uart.h"

#define PARAM_MEM_MAXLEN  512

typedef struct __lbfs_node_header {
	uint8_t inode;
	uint8_t len;
	uint8_t type;
	uint16_t ver;
} LBFS_NODE_HEADER;

// len name null data null len name null data null

void param_init()
{


}

uint8_t param_get_inode_P(PGM_P name)
{
	//FIXME hack
	char buf[18];
	strcpy_P(buf, name);
	return param_get_inode(buf);
}

uint8_t param_get_inode(const char * name)
{
	LBFS_NODE_HEADER header;
	uint16_t i = 0;
	uint8_t keylen;

	char buf[16]; // max param key len (filename)
	while (i < PARAM_MEM_MAXLEN) {
		eeprom_read(&header, i, sizeof(LBFS_NODE_HEADER));
		if (header.type > 0xf0) break; // 0xff or something

		keylen = eeprom_read_str(buf, i + sizeof(LBFS_NODE_HEADER), 15);

		if (!(strcasecmp(buf, name))) {
			return header.inode;
		} 
		i += sizeof(LBFS_NODE_HEADER) + header.len;
	}

	return 0;
}

// returnib eepromi aadressi datale
uint16_t param_get(uint8_t inode, uint8_t * type)
{
	LBFS_NODE_HEADER header;

	uint16_t latest_ver = 0;
	uint16_t latest_addr = 0;

	uint16_t i = 0;

	while (i < PARAM_MEM_MAXLEN) {
		eeprom_read(&header, i, sizeof(LBFS_NODE_HEADER));

		if (header.type > 0xf0) break; // 0xff or something

		if (header.inode == inode) {
			if (header.ver >= latest_ver) {
				latest_ver = header.ver;
				latest_addr = i;
				*type = header.type;
			}
		}

		i += sizeof(LBFS_NODE_HEADER) + header.len;
	}

	return latest_addr;
}

uint8_t param_get_str(char * dst, uint16_t src, uint8_t len)
{
	uint8_t keylen;

	keylen = eeprom_read_str(dst, src + sizeof(LBFS_NODE_HEADER), len);
	keylen += 1; // terminator
	keylen = eeprom_read_str(dst, src + keylen + sizeof(LBFS_NODE_HEADER), len);

	return keylen;
}

uint16_t param_get_u16(uint16_t src)
{
	char dst[30]; // FIXME fucking hack
	uint16_t num;
	uint8_t keylen;

	keylen = eeprom_read_str(dst, src + sizeof(LBFS_NODE_HEADER), 30);
	keylen++;
	eeprom_read(&num, src + keylen + sizeof(LBFS_NODE_HEADER), 2);

	return num;
}




