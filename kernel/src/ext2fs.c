/**
 * Ext2 ファイルシステム
 *
 * root ディレクトリのi-nodeアドレスは2。
 *
 * ## 注意
 * i-nodeアドレスは1から始まる。blockアドレスは0から始まる。
 */


//=============================================================================
// 公開ヘッダ

#ifndef HEADER_FS_EXT2
#define HEADER_FS_EXT2

#include <stdint.h>


struct DIRECTORY {
    void *head;
    struct DIRECTORY_ENTRY *cur_ent;
};


struct DIRECTORY_ENTRY {
    uint32_t inode_i;
    uint16_t total_size;
    uint8_t name_len;
    uint8_t type;
    char name[];
};


void ext2_init(void);
struct DIRECTORY *ext2_open_dir(int inode_i);
struct DIRECTORY_ENTRY *ext2_read_dir(struct DIRECTORY *dir);
void ext2_close_dir(struct DIRECTORY *dir);

#endif


//=============================================================================
// 非公開ヘッダ

#include "ata/common.h"
#include "ata/cmd.h"
#include "debug.h"
#include "memory.h"
#include "str.h"

#define MIN(x,y)  (((x) < (y)) ? (x) : (y))

#define SECTOR_SIZE 512

#define EXT2_SIGNATURE  0xEF53

#define START_LBA  1008

#define ADDR_SUPER  2
#define ADDR_BGD    4


struct SUPER_BLOCK {
    uint32_t total_inodes;
    uint32_t total_blocks;
    uint32_t blocks_for_su;
    uint32_t free_blocks;
    uint32_t free_inodes;
    uint32_t super_blocks;
    uint32_t block_size_log2_m10;
    uint32_t fragment_size_log2_m10;
    uint32_t blocks_per_group;
    uint32_t fragments_per_group;
    uint32_t inodes_per_group;
    uint32_t last_mount_time;
    uint32_t last_written_time;
    uint16_t mt1;
    uint16_t mt2;
    uint16_t signature;
    uint16_t fs_state;
    uint16_t err_handle;
    uint16_t minor_ver;
    uint32_t last_check_time;
    uint32_t check_interval;
    uint32_t os_id;
    uint32_t major_ver;
    uint16_t uid;
    uint16_t gid;

    uint32_t first_non_reserved_inode;
    uint16_t inode_size_B;
};


struct BLOCK_GROUP_DESCRIPTOR {
    uint32_t addr_block_bitmap;
    uint32_t addr_inode_bitmap;
    uint32_t addr_inode_table;
    uint16_t free_blocks;
    uint16_t free_inodes;
    uint16_t directories;
};


struct INODE {
    uint16_t type;
    uint16_t uid;
    uint32_t size_lo_B;
    uint32_t last_access_time;
    uint32_t creation_time;
    uint32_t last_modification_time;
    uint32_t deletion_time;
    uint16_t gid;
    uint16_t num_hard_links;
    uint32_t num_sectors;
    uint32_t flags;
    uint32_t val1;
    uint32_t blocks[12];
    uint32_t p1_blocks;
    uint32_t p2_blocks;
    uint32_t p3_blocks;
    uint32_t generation_no;
    uint32_t acl;
    uint32_t size_hi_B;
    uint32_t fragment;
    uint32_t val2[3];
};


static struct BLOCK_GROUP_DESCRIPTOR *get_block_group_descriptor(int group_i);
static struct INODE *get_inode(int inode_i);
static void *get_block(int group_i, int block_i);
static struct DIRECTORY_ENTRY *next_dir_ent(struct DIRECTORY_ENTRY *ent);


static char l_super_buf[SECTOR_SIZE];
static struct SUPER_BLOCK *l_super;  // super block

static int l_block_size_B = 1024;
static int l_sectors_per_group = 16384;
static int l_sectors_per_block = 2;
static int l_inodes_per_block = 8;


//=============================================================================
// 公開関数


void ext2_init(void)
{
    if (ata_cmd_read_sectors(g_ata0, START_LBA + ADDR_SUPER, l_super_buf, 1) < 0) {
        ERROR("could not read super block");
        return;
    }

    l_super = (struct SUPER_BLOCK *) l_super_buf;

    int sig = l_super->signature;
    ASSERT(sig == EXT2_SIGNATURE, "%X", sig);

    int ver = l_super->major_ver;
    ASSERT(ver >= 1, "unsupported ext2 version: %d", ver);

    l_block_size_B = 1024 << l_super->block_size_log2_m10;
    l_sectors_per_block = l_block_size_B / SECTOR_SIZE;
    l_sectors_per_group = l_super->blocks_per_group * l_sectors_per_block;
    l_inodes_per_block  = l_block_size_B / l_super->inode_size_B;
}


void ext2_ls(void)
{
    struct DIRECTORY *dir = ext2_open_dir(2);

    ASSERT(dir, "");

    struct DIRECTORY_ENTRY *ent;

    while ((ent = ext2_read_dir(dir)) != 0) {
        dbgf("%.*s\n", ent->name_len, ent->name);
    }

    ext2_close_dir(dir);
}


struct DIRECTORY *ext2_open_dir(int inode_i)
{
    struct INODE *inode = get_inode(inode_i);

    struct DIRECTORY *dir = (struct DIRECTORY *) mem_alloc(sizeof(struct DIRECTORY));

    int size_B = inode->size_lo_B;
    dir->head = mem_alloc(size_B);
    dir->cur_ent = (struct DIRECTORY_ENTRY *) dir->head;

    char *p = (char *) dir->head;

    for (int i = 0; i < 12 && size_B > 0; i++) {
        void *block = get_block(0, inode->blocks[i]);  // TODO
        memcpy(p, block, MIN(l_block_size_B, size_B));
        mem_free(block);

        size_B -= l_block_size_B;
        p += l_block_size_B;
    }

    if (size_B > 0) {
        ERROR("too large");
        return 0;
    }

    return dir;
}


struct DIRECTORY_ENTRY *ext2_read_dir(struct DIRECTORY *dir)
{
    if (dir->cur_ent->inode_i == 0)
        return 0;

    // TODO: ブロックまたぎでもうまくいくか確認する
    dir->cur_ent = next_dir_ent(dir->cur_ent);

    if (dir->cur_ent->inode_i == 0)
        return 0;

    return dir->cur_ent;
}


void ext2_close_dir(struct DIRECTORY *dir)
{
    mem_free(dir->head);
    mem_free(dir);
}


//=============================================================================
// 非公開関数


static struct BLOCK_GROUP_DESCRIPTOR *get_block_group_descriptor(int group_i)
{
    struct BLOCK_GROUP_DESCRIPTOR *bgd = (struct BLOCK_GROUP_DESCRIPTOR *) mem_alloc(SECTOR_SIZE);

    int lba = START_LBA + (group_i * l_sectors_per_group) + ADDR_BGD;

    if (ata_cmd_read_sectors(g_ata0, lba, bgd, 1) < 0) {
        ERROR("could not read block group descriptor");
        return 0;
    }

    dbgf("block group descriptor: %X %X\n", bgd->addr_block_bitmap, bgd->addr_inode_bitmap);

    return bgd;
}


static struct INODE *get_inode(int inode_i)
{
    struct BLOCK_GROUP_DESCRIPTOR *bgd = get_block_group_descriptor(0);  // TODO

    int group_i = (inode_i - 1) / l_super->inodes_per_group;
    int tbl_i   = (inode_i - 1) % l_super->inodes_per_group;
    int block_i = bgd->addr_inode_table + (tbl_i * l_super->inode_size_B) / l_block_size_B;

    mem_free(bgd);

    char *p = (char *) get_block(group_i, block_i);
    char *p_inode = p + (tbl_i % l_inodes_per_block) * l_super->inode_size_B;

    struct INODE *inode = (struct INODE *) mem_alloc(sizeof(struct INODE));

    memcpy(inode, p_inode, sizeof(struct INODE));

    mem_free(p);

    return inode;
}


static void *get_block(int group_i, int block_i)
{
    void *block = mem_alloc(l_block_size_B);

    int lba = START_LBA + (group_i * l_sectors_per_group) + (block_i * l_sectors_per_block);

    ata_cmd_read_sectors(g_ata0, lba, block, l_sectors_per_block);

    return block;
}


static struct DIRECTORY_ENTRY *next_dir_ent(struct DIRECTORY_ENTRY *ent)
{
    int add = ((ent->name_len + 3) / 4) * 4;
    ent++;
    char *p = (char *) ent;

    return (struct DIRECTORY_ENTRY *) (p + add);
}
