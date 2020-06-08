#!/usr/local/cs/bin/python3
# NAME: Matthew Pombo
# EMAIL: mppombo5@gmail.com
# UID: 405140036
import sys
import csv

progName = "lab3b"
usage = "lab3b FILENAME"

# array of strings corresponding to the level of indirection
#          [0]       [1]             [2]                  [3]
indirStr = ["", " INDIRECT", " DOUBLE INDIRECT", " TRIPLE INDIRECT"]

def printBadBlock(badType, indirLevel, blockNum, inodeNum, offset):
    print("{}{} BLOCK {} IN INODE {} AT OFFSET {}".format(badType, indirStr[indirLevel], blockNum, inodeNum, offset))

class Inode:
    def __init__(self, inodeNum):
        self.num = int(inodeNum)    # number of the inode
        self.isFree = False         # whether or not it appears in an IFREE entry
        self.isAllocated = False    # whether or not it appears in an INODE entry
        self.blocks = {}            # dictionary mapping block numbers to tuples of (block #, indirection level, offset)
        self.iType = None           # inode type, as it appears in the INODE entry
        self.reportedLinks = 0      # number of links to inode as it appears on the INODE entry
        self.actualLinks = 0        #
        self.exists = True
        self.childInodes = set()
        self.parentInodes = set()

    def addBlock(self, blockNum, indirLevel, offset):
        blockNum = int(blockNum)
        if blockNum not in self.blocks:
            self.blocks[blockNum] = [(blockNum, indirLevel, offset)]
        else:
            self.blocks[blockNum].append((blockNum, indirLevel, offset))

    def usesBlock(self, blockNum) -> bool:
        return blockNum in self.blocks

    def getBlock(self, blockNum):
        num = int(blockNum)
        if num not in self.blocks:
            return None
        return self.blocks[num]

    # as they should be
    def setFree(self):
        self.isFree = True

    def allocate(self):
        self.isAllocated = True

    def setLinkCount(self, linkCount):
        self.reportedLinks = int(linkCount)

    def addLink(self):
        self.actualLinks += 1

    # shoddy workaround for making sure inodes in DIRENTS but not INODE aren't analyzed as such
    def doesntExist(self):
        self.exists = False

    def addChild(self, inodeNum):
        self.childInodes.add(inodeNum)

    def addParent(self, inodeNum):
        self.parentInodes.add(inodeNum)


class Block:
    def __init__(self, blockNum):
        self.num = int(blockNum)
        self.isFree = False
        self.inodeRefs = {}
        self.isAllocated = False

    def addInodeRef(self, inodeNum, indirLevel, offset):
        num = int(inodeNum)
        if num not in self.inodeRefs:
            self.inodeRefs[inodeNum] = [(num, indirLevel, offset)]
        else:
            self.inodeRefs[inodeNum].append((num, indirLevel, offset))

    def refdByInode(self, inodeNum) -> bool:
        return inodeNum in self.inodeRefs

    def getInodeRef(self, inodeNum):
        num = int(inodeNum)
        if num not in self.inodeRefs:
            return None
        return self.inodeRefs[num]

    def setFree(self):
        self.isFree = True

    def allocate(self):
        self.isAllocated = True

class DirEntry:
    def __init__(self, inodeNum, parentInodeNum, fileName):
        self.inodeNum = int(inodeNum)
        self.parentInodeNum = int(parentInodeNum)
        self.name = fileName


class FileSystem:
    numBlocks = 0
    numInodes = 0
    blockSize = 0
    inodeSize = 0
    numFreeBlocks = 0
    numFreeInodes = 0
    inodeTableBlock = 0
    firstInode = 0

    # initialize the summary with info from the superblock
    def initSuperblock(self, numBlocks, numInodes, blockSize, inodeSize, firstInode):
        self.numBlocks = int(numBlocks)
        self.numInodes = int(numInodes)
        self.blockSize = int(blockSize)
        self.inodeSize = int(inodeSize)
        self.firstInode = int(firstInode)

    def initGroup(self, numFreeBlocks, numFreeInodes, inodeBlock):
        self.numFreeBlocks = int(numFreeBlocks)
        self.numFreeInodes = int(numFreeInodes)
        self.inodeTableBlock = int(inodeBlock)


def main():
    if len(sys.argv) != 2:
        sys.stderr.write("{}: program must be supplied exactly one argument.\n{}\n".format(progName, usage))
        exit(1)

    exitCode = 0

    blocks = {}
    inodes = {}
    dirents = []
    filesys = FileSystem()

    with open(sys.argv[1], 'r') as csvFile:
        reader = csv.reader(csvFile, delimiter=',')

        for row in reader:
            # get superblock information
            if row[0] == "SUPERBLOCK":
                filesys.initSuperblock(row[1], row[2], row[3], row[4], row[7])

            elif row[0] == "GROUP":
                filesys.initGroup(row[4], row[5], row[8])

            # set block's state to free
            elif row[0] == "BFREE":
                num = int(row[1])
                if num not in blocks:
                    blocks[num] = Block(num)
                blocks[num].setFree()

            # set inode's state to free
            elif row[0] == "IFREE":
                num = int(row[1])
                if num not in inodes:
                    inodes[num] = Inode(num)
                inodes[num].setFree()

            elif row[0] == "INODE":
                curInodeNum = int(row[1])
                rowLen = len(row)

                # insert inode in inode list, if it hasn't already been
                if curInodeNum not in inodes:
                    inodes[curInodeNum] = Inode(curInodeNum)

                curInode = inodes[curInodeNum]

                # initialize it
                curInode.iType = row[2]
                curInode.allocate()
                curInode.setLinkCount(row[6])

                # get blocks used by inode, from INODE line
                if rowLen > 12:
                    # only add blocks if there are blocks to add; indices 12-26
                    for i in range(12, rowLen):
                        curBlockNum = int(row[i])
                        if curBlockNum != 0:
                            if curBlockNum not in blocks:
                                blocks[curBlockNum] = Block(curBlockNum)

                            curBlock = blocks[curBlockNum]
                            curBlock.allocate()

                            if i < 24:
                                curInode.addBlock(curBlockNum, 0, i-12)
                                curBlock.addInodeRef(curInodeNum, 0, i-12)
                            elif i == 24:
                                curInode.addBlock(curBlockNum, 1, i-12)
                                curBlock.addInodeRef(curInodeNum, 1, i-12)
                            elif i == 25:
                                curInode.addBlock(curBlockNum, 2, 268)
                                curBlock.addInodeRef(curInodeNum, 2, 268)
                            elif i == 26:
                                curInode.addBlock(curBlockNum, 3, 65804)
                                curBlock.addInodeRef(curInodeNum, 3, 65804)

            elif row[0] == "DIRENT":
                dirents.append(DirEntry(row[3], row[1], row[6]))

            elif row[0] == "INDIRECT":
                curInodeNum = int(row[1])
                indirLevel = int(row[2])
                curBlockNum = int(row[5])
                blockOffset = int(row[3])

                # insert inode if it isn't already
                if curInodeNum not in inodes:
                    inodes[curInodeNum] = Inode(curInodeNum)

                if curBlockNum not in blocks:
                    blocks[curBlockNum] = Block(curBlockNum)

                curInode = inodes[curInodeNum]
                curBlock = blocks[curBlockNum]

                curInode.addBlock(curBlockNum, indirLevel, blockOffset)
                curBlock.addInodeRef(curInodeNum, indirLevel, blockOffset)


    # get first non-reserved block
    firstNonReserved = int(filesys.inodeTableBlock + (filesys.numInodes * filesys.inodeSize / filesys.blockSize))

    for dirent in dirents:
        if dirent.inodeNum not in inodes:
            inodes[dirent.inodeNum] = Inode(dirent.inodeNum)
            inodes[dirent.inodeNum].doesntExist()
        inodes[dirent.inodeNum].addLink()

        if dirent.inodeNum < 0 or dirent.inodeNum > filesys.numInodes:
            exitCode = 2
            print("DIRECTORY INODE {} NAME {} INVALID INODE {}".format(dirent.parentInodeNum, dirent.name, dirent.inodeNum))
        elif dirent.inodeNum not in inodes or not inodes[dirent.inodeNum].isAllocated:
            exitCode = 2
            print("DIRECTORY INODE {} NAME {} UNALLOCATED INODE {}".format(dirent.parentInodeNum, dirent.name, dirent.inodeNum))

        if dirent.name == ""'.'"":
            if dirent.inodeNum != dirent.parentInodeNum:
                print("DIRECTORY INODE {} NAME '.' LINK TO INODE {} SHOULD BE {}".format(dirent.parentInodeNum, dirent.inodeNum, dirent.parentInodeNum))
        elif dirent.name == "'..'":
            if dirent.parentInodeNum == 2 and dirent.inodeNum != 2:
                print("DIRECTORY INODE 2 NAME '..' LINK TO INODE {} SHOULD BE 2".format(dirent.inodeNum))
        else:
            inodes[dirent.parentInodeNum].addChild(dirent.inodeNum)
            inodes[dirent.inodeNum].addParent(dirent.parentInodeNum)

    # one more time to check for '..' inconsistencies
    for dirent in dirents:
        # root directory should be fine
        if dirent.inodeNum != 2:
            if dirent.name == "'..'":
                if inodes[dirent.inodeNum].exists and dirent.inodeNum not in inodes[dirent.parentInodeNum].childInodes:
                    print("DIRECTORY INODE {} NAME '..' LINK TO INODE {} SHOULD BE {}".format(dirent.parentInodeNum, dirent.inodeNum, dirent.parentInodeNum))

    # iterates over keys in dictionary
    for blockNum in blocks:
        curBlock = blocks[blockNum]
        if curBlock.isFree and curBlock.isAllocated:
            exitCode = 2
            print("ALLOCATED BLOCK {} ON FREELIST".format(curBlock.num))
        blockInodes = curBlock.inodeRefs
        if len(blockInodes) > 1:
            for inodeNum in blockInodes:
                curTupleList = blockInodes[inodeNum]
                for tup in curTupleList:
                    exitCode = 2
                    print("DUPLICATE{} BLOCK {} IN INODE {} AT OFFSET {}".format(indirStr[tup[1]], curBlock.num, tup[0], tup[2]))

    # print unreferenced blocks
    for i in range(firstNonReserved, filesys.numBlocks):
        if i not in blocks:
            exitCode = 2
            print("UNREFERENCED BLOCK {}".format(i))

    for inodeNum in inodes:
        curInode = inodes[inodeNum]
        if curInode.exists:
            if curInode.isFree and curInode.isAllocated:
                exitCode = 2
                print("ALLOCATED INODE {} ON FREELIST".format(curInode.num))

            if curInode.isAllocated and curInode.actualLinks != curInode.reportedLinks:
                exitCode = 2
                print("INODE {} HAS {} LINKS BUT LINKCOUNT IS {}".format(inodeNum, curInode.actualLinks, curInode.reportedLinks))

            if len(curInode.blocks) > 0:
                inodeBlocks = curInode.blocks
                for blockNum in inodeBlocks:
                    # gives the list of tuples within the inode
                    curTupleList = inodeBlocks[blockNum]
                    for tup in curTupleList:
                        if tup[0] < 0 or tup[0] >= filesys.numBlocks:
                            exitCode = 2
                            printBadBlock("INVALID", tup[1], tup[0], curInode.num, tup[2])
                        elif 0 < tup[0] < firstNonReserved:
                            exitCode = 2
                            printBadBlock("RESERVED", tup[1], tup[0], curInode.num, tup[2])

    # print unallocated inodes not on freelist
    for i in range(filesys.firstInode, filesys.numInodes):
        if i not in inodes:
            print("UNALLOCATED INODE {} NOT ON FREELIST".format(i))

    return exitCode


if __name__ == "__main__":
    exitCode = main()
    exit(exitCode)
