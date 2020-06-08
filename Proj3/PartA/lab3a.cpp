#include <iostream>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <ctime>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include "ext2_fs.h"

__u32 blockSize;

const char* const progName  = "lab3a";
const int SUPERBLOCK_OFFSET = 1024;

void    killProg(const char* msg, int exitStat);
__u32   getBlockOffset(__u32 blockID);
void    printSuperblockInfo(ext2_super_block* sBlock);
void    printGroupInfo(ext2_group_desc* gpDesc, ext2_super_block* sBlock);
void    printBitmap(int imgfd, __u32 blockID, __u32 numObjects, const char* header);
void    printInodes(int imgfd, __u32 blockID, __u32 numInodes, __u16 inodeSize);
void    printDirEntries(int imgfd, const __u32* blockArr, __u32 dirInode);
void    printIndirect(int imgfd, __u32 blockID, __u32 logicalOffset, __u32 ownerInode, int indirLevel);
void    timeString(__u32 time, char* str);

int main(int argc, char** argv) {
    if (argc != 2) {
        killProg("Invalid number of arguments.\nusage: \"lab3a IMG_FILENAME\"", 1);
    }

    int imgfd = open(argv[1], O_RDONLY);
    if (imgfd == -1) {
        killProg("Unable to open specified file", 1);
    }

    ext2_super_block superblock;
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
    ext2_group_desc gpDesc;
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
    __u32 blocksInGroup = (totalBlocks >= blocksPerGroup) ? blocksPerGroup : (totalBlocks % blocksPerGroup);
    __u32 inodesInGroup = (totalInodes >= inodesPerGroup) ? inodesPerGroup : (totalInodes % inodesPerGroup);

    // output the group descriptor info
    printGroupInfo(&gpDesc, &superblock);

    // print the free blocks from the block bitmap
    __u32 blockBitmap = gpDesc.bg_block_bitmap;
    printBitmap(imgfd, blockBitmap, blocksInGroup, "BFREE");

    // print the free inodes from the inode bitmap
    __u32 inodeBitmap = gpDesc.bg_inode_bitmap;
    printBitmap(imgfd, inodeBitmap, inodesInGroup, "IFREE");

    // print inode information
    printInodes(imgfd, gpDesc.bg_inode_table, inodesInGroup, superblock.s_inode_size);

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

void printSuperblockInfo(ext2_super_block* sBlock) {
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

void printGroupInfo(ext2_group_desc* gpDesc, ext2_super_block* sBlock) {
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

// we can just roll the block and inode bitmaps all into one function
void printBitmap(int imgfd, __u32 blockID, __u32 numObjects, const char *header) {
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

void printInodes(int imgfd, __u32 blockID, __u32 numInodes, __u16 inodeSize) {
    ext2_inode* inodes = new ext2_inode[numInodes];
    ssize_t bytesRead;
    for (__u32 i = 0; i < numInodes; i++) {
        bytesRead = pread(imgfd, inodes + i, inodeSize, getBlockOffset(blockID) + (i * inodeSize));
        if (bytesRead != inodeSize) {
            killProg("Error in pread() for inode structs", 1);
        }
    }

    char fileType;
    for (__u32 i = 0; i < numInodes; i++) {
        ext2_inode* cur = inodes + i;
        // only operate on allocated inodes
        if (cur->i_mode != 0 && cur->i_links_count != 0) {
            __u16 mode = cur->i_mode;
            std::cout << "INODE" << ','
                      << i + 1 << ',';  // inode number (1-indexed)
            // test if inode is a symlink
            if (S_ISLNK(mode)) {
                fileType = 's';
            }
            // directory
            else if (S_ISDIR(mode)) {
                fileType = 'd';
            }
            // regular file
            else if (S_ISREG(mode)) {
                fileType = 'f';
            }
            // anything else
            else {
                fileType = '?';
            }
            std::cout << fileType << ','            // filetype
                      << std::oct << (cur->i_mode & 0xFFFU) << std::dec << ','  // ownership mode (lower 12 bits)
                      << cur->i_uid << ','          // user id
                      << cur->i_gid << ','          // group id
                      << cur->i_links_count << ','; // link count

            // print time strings
            char ctime[20], mtime[20], atime[20];
            timeString(cur->i_ctime, ctime);
            timeString(cur->i_mtime, mtime);
            timeString(cur->i_atime, atime);
            std::cout << ctime << ','   // creation/change time
                      << mtime << ','   // modification time
                      << atime << ',';  // access time

            // get file size, taking into account the upper 64 bits for regular files
            unsigned long size = cur->i_dir_acl;
            size <<= 32U;
            size |= cur->i_size;
            std::cout << size << ','    // size of file
                      << cur->i_blocks; // number of 512-byte blocks

            // scan each inode for the
            __u32* blockArr = cur->i_block;
            bool searchIndir = false;
            switch (fileType) {
                case 'd':
                case 'f':
                    searchIndir = true;
                    for (int j = 0; j < EXT2_N_BLOCKS; j++) {
                        std::cout << ',' << blockArr[j];   // number from array of block addresses
                    }
                    std::cout << std::endl;
                    break;
                case 's':
                    // why 60? ...because the spec said so.
                    if (size >= 60) {
                        for (int j = 0; j < EXT2_N_BLOCKS; j++) {
                            std::cout << ',' << blockArr[j];    // might not even show up most of the time
                        }
                    }
                    std::cout << std::endl;
                    break;
                default:
                    std::cout << std::endl;
                    break;
            }

            // print directory entries
            if (fileType == 'd') {
                printDirEntries(imgfd, blockArr, i + 1);
            }

            // print indirect entries, if we need to (directories and regular files)
            // offsets determined by TA post on Piazza
            if (searchIndir) {
                if (blockArr[EXT2_IND_BLOCK] != 0) {
                    printIndirect(imgfd, blockArr[EXT2_IND_BLOCK], 12, i + 1, 1);
                }
                if (blockArr[EXT2_DIND_BLOCK] != 0) {
                    printIndirect(imgfd, blockArr[EXT2_DIND_BLOCK], 256 + 12, i + 1, 2);
                }
                if (blockArr[EXT2_TIND_BLOCK] != 0) {
                    printIndirect(imgfd, blockArr[EXT2_TIND_BLOCK], 65536 + 256 + 12, i + 1, 3);
                }
            }
        }
    }
    delete [] inodes;
}

void printDirEntries(int imgfd, const __u32 *blockArr, __u32 dirInode) {
    // directory entry struct to work with
    ext2_dir_entry entry;
    // indexing array to tell which directory entry we're looking at
    __u32 offset;
    // scan the first 12 (direct) blocks from blockArr
    for (int i = 0; i < EXT2_NDIR_BLOCKS; i++) {
        __u32 curBlock = blockArr[i];
        if (curBlock != 0) {
            offset = 0;
            while (offset < blockSize) {
                ssize_t bytesRead = pread(imgfd, &entry, sizeof(ext2_dir_entry), offset + getBlockOffset(curBlock));
                if (bytesRead != sizeof(ext2_dir_entry)) {
                    killProg("Error in pread() for directory entry", 1);
                }
                // only scan if the inode is non-zero
                if (entry.inode != 0) {
                    std::cout << "DIRENT,"
                              << dirInode << ','        // inode of the parent directory
                              << offset << ','          // offset within data block
                              << entry.inode << ','     // inode number of entry
                              << entry.rec_len << ','   // length of entry
                              << (unsigned short) entry.name_len << ','     // length of entry name (cast so it's not
                                                                            // interpreted as an integer)
                              << '\'' << entry.name << '\'' << std::endl;   // actual entry name
                }

                offset += entry.rec_len;
            }
        }
    }
}

void printIndirect(int imgfd, __u32 blockID, __u32 logicalOffset, __u32 ownerInode, int indirLevel) {
    // base case
    if (indirLevel == 0) {
        return;
    }

    // whole block is an array of __u32's
    __u32 numBlocks = blockSize / sizeof(__u32);
    __u32* blockArr = new __u32[numBlocks];
    ssize_t bytesRead = pread(imgfd, blockArr, blockSize, getBlockOffset(blockID));
    if (bytesRead != blockSize) {
        killProg("Error on pread() for indirect blocks", 1);
    }

    for (__u32 i = 0; i < numBlocks; i++) {
        if (blockArr[i] != 0) {
            std::cout << "INDIRECT,"
                      << ownerInode << ','      // inode of the owning file
                      << indirLevel << ','      // level of indirection
                      << logicalOffset << ','   // byte offset, wonky calculations
                      << blockID << ','         // parent block ID
                      << blockArr[i] << std::endl;  // current block ID

            printIndirect(imgfd, blockArr[i], logicalOffset, ownerInode, indirLevel - 1);
        }
        logicalOffset += (1U << (8U * (indirLevel - 1)));
    }

    delete [] blockArr;
}

// easier to just use sprintf instead of making a ton of if-statements with std::string's.
void timeString(__u32 time, char* str) {
    time_t newTime = time;
    std::tm* tm = std::gmtime(&newTime);
    if (tm == nullptr) {
        killProg("Error in gmtime()", 1);
    }

    int res = sprintf(str, "%02d/%02d/%02d %02d:%02d:%02d", tm->tm_mon + 1, tm->tm_mday, tm->tm_year % 100, tm->tm_hour, tm->tm_min, tm->tm_sec);
    if (res < 0) {
        killProg("Error in printf() for time format", 1);
    }
}




























