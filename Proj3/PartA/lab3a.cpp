#include <iostream>
#include <cstdlib>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include "ext2_fs.h"

const char* const progName  = "lab3a";
unsigned int                  blockSize;
const int SUPERBLOCK_OFFSET = 1024;

void    killProg(const char* msg, int exitStat);
__u32   getBlockOffset(__u32 blockID);
void    printSuperblockInfo(struct ext2_super_block* sBlock);
void    printGroupInfo(struct ext2_group_desc* gpDesc, struct ext2_super_block* sBlock);
void    printBlockBitmap(__u32 blockID, int imgfd, __u32 numBlocks);
void    printInodeBitmap(__u32 blockID, int imgfd, __u32 numInodes);
void    printBitmap(__u32 blockID, int imgfd, __u32 numObjects, const char* header);

int main(int argc, char** argv) {
    if (argc != 2) {
        killProg("Invalid number of arguments.\nusage: \"lab3a IMG_FILENAME\"", 1);
    }

    int imgfd = open(argv[1], O_RDONLY);
    if (imgfd == -1) {
        killProg("Unable to open specified file", 1);
    }

    struct ext2_super_block superblock;
    // superblock is *always* at the 1024th byte of the file
    ssize_t bytesRead = pread(imgfd, &superblock, sizeof(struct ext2_super_block), SUPERBLOCK_OFFSET);
    // failed if we didn't read in the whole struct
    if (bytesRead != sizeof(struct ext2_super_block)) {
        killProg("Error in pread() for superblock", 1);
    }
    if (superblock.s_magic != EXT2_SUPER_MAGIC) {
        killProg("Incorrect superblock, magic number is incorrect", 1);
    }

    // save some stuff for later
    blockSize = EXT2_MIN_BLOCK_SIZE << superblock.s_log_block_size;
    __u32 totalBlocks = superblock.s_blocks_count;
    __u32 totalInodes = superblock.s_inodes_count;
    __u32 blocksPerGroup = superblock.s_blocks_per_group;
    __u32 inodesPerGroup = superblock.s_inodes_per_group;

    printSuperblockInfo(&superblock);

    // get info from the group descriptor
    struct ext2_group_desc gpDesc;
    // group descriptor will always be one block after block containing superblock
    __u32 groupBlock = superblock.s_first_data_block + 1;
    bytesRead = pread(imgfd, &gpDesc, sizeof(struct ext2_group_desc), getBlockOffset(groupBlock));
    if (bytesRead != sizeof(struct ext2_group_desc)) {
        killProg("Error in pread() for group descriptor", 1);
    }

    /*
     * all this calculation does is say whether all the system's blocks and
     * inodes are in this group, or if it has as many per group as allowed
     */
    __u32 blocksInGroup = (totalBlocks >= blocksPerGroup) ? totalBlocks : (totalBlocks % blocksPerGroup);
    __u32 inodesInGroup = (totalInodes >= inodesPerGroup) ? totalInodes : (totalInodes % inodesPerGroup);

    printGroupInfo(&gpDesc, &superblock);

    __u32 blockBitmap = gpDesc.bg_block_bitmap;
    printBitmap(blockBitmap, imgfd, blocksInGroup, "BFREE");

    __u32 inodeBitmap = gpDesc.bg_inode_bitmap;
    printBitmap(inodeBitmap, imgfd, inodesInGroup, "IFREE");


    if (close(imgfd) == -1) {
        killProg("Unable to close image file", 1);
    }

    return 0;
}

void killProg(const char* msg, int exitStat) {
    if (errno == 0) {
        std::cerr << progName << ": " << msg << std::endl;
    }
    else {
        std::cerr << progName << ": " << msg << ": " << strerror(errno) << std::endl;
    }
    exit(exitStat);
}

__u32 getBlockOffset(__u32 blockID) {
    return (blockSize * blockID);
}

void printSuperblockInfo(struct ext2_super_block* sBlock) {
    // generate csv line for superblock
    std::cout << "SUPERBLOCK,"
              << sBlock->s_blocks_count << ','       // total number of blocks
              << sBlock->s_inodes_count << ','       // total number of inodes
              << blockSize << ','                    // block size
              << sBlock->s_inode_size << ','         // inode size
              << sBlock->s_blocks_per_group << ','   // blocks per group
              << sBlock->s_inodes_per_group << ','   // inodes per group
              << sBlock->s_first_ino << std::endl;   // first free inode
}

void printGroupInfo(struct ext2_group_desc* gpDesc, struct ext2_super_block* sBlock) {
    /*
     * all this calculation does is say whether all the system's blocks and
     * inodes are in this group, or if it has as many per group as allowed
     */
    __u32 blocksInGroup = (sBlock->s_blocks_count >= sBlock->s_blocks_per_group) ?
            sBlock->s_blocks_count : (sBlock->s_blocks_count % sBlock->s_blocks_per_group);
    __u32 inodesInGroup = (sBlock->s_inodes_count >= sBlock->s_inodes_per_group) ?
            sBlock->s_inodes_count : (sBlock->s_inodes_count % sBlock->s_inodes_per_group);

    std::cout << "GROUP,"
              << "0,"                                   // group number (always 0 in this project)
              << blocksInGroup << ','                   // only one group, so it's the total number of blocks
              << inodesInGroup << ','                   // only one group, total inodes
              << sBlock->s_free_blocks_count << ','     // free blocks
              << sBlock->s_free_inodes_count << ','     // free inodes
              << gpDesc->bg_block_bitmap << ','         // block id for block bitmap
              << gpDesc->bg_inode_bitmap << ','         // block id for inode bitmap
              << gpDesc->bg_inode_table << std::endl;   // id of first block of inodes
}

void printBlockBitmap(__u32 blockID, int imgfd, __u32 numBlocks) {
    __u8* bitmapArr = new __u8[blockSize];
    ssize_t bytesRead = pread(imgfd, bitmapArr, numBlocks, getBlockOffset(blockID));
    if (bytesRead != numBlocks) {
        killProg("Error in pread() for block bitmap", 1);
    }

    // 8 blocks for every byte; TA guaranteed # blocks will be a multiple of 8
    __u32 bytesToCheck = numBlocks  >> 3U;

    for (__u32 i = 0; i < bytesToCheck; i++) {
        __u8 bitMask = 1;
        __u8 curByte = bitmapArr[i];
        for (int j = 0; j < 8; j++) {
            if (!(curByte & bitMask)) {
                // calculate the number of the block this represents
                std::cout << "BFREE," << (i*8 + j) + 1 << std::endl;
            }
            bitMask <<= 1U;
        }
    }

    delete [] bitmapArr;
}

void printInodeBitmap(__u32 blockID, int imgfd, __u32 numInodes) {
    __u8* bitmapArr = new __u8[blockSize];
    ssize_t bytesRead = pread(imgfd, bitmapArr, numInodes, getBlockOffset(blockID));
    if (bytesRead != numInodes) {
        killProg("Error in pread() for inode bitmap", 1);
    }

    // 8 inodes for every byte, same as block function
    __u32 bytesToCheck = numInodes >> 3U;

    for (__u32 i = 0; i < bytesToCheck; i++) {

    }

    delete [] bitmapArr;
}

// we can just roll the block and inode bitmaps all into one function
void printBitmap(__u32 blockID, int imgfd, __u32 numObjects, const char* header) {
    __u8* bitmapArr = new __u8[blockSize];
    ssize_t bytesRead = pread(imgfd, bitmapArr, numObjects, getBlockOffset(blockID));
    if (bytesRead != numObjects) {
        killProg("Error in pread() for bitmap", 1);
    }

    // 8 blocks for every byte, TA guaranteed they would be multiple of 8
    __u32 bytesToCheck = numObjects >> 3U;

    for (__u32 i = 0; i < bytesToCheck; i++) {
        __u8 bitMask = 1;
        __u8 curByte = bitmapArr[i];
        for (int j = 0; j < 8; j++) {
            if (!(curByte & bitMask)) {
                // calculate the number of the object this represents
                std::cout << header << ',' << (i * 8 + j) + 1 << std::endl;
            }
            bitMask <<= 1U;
        }
    }

    delete [] bitmapArr;
}




























