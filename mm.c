// Memory manager functions
// J Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdio.h>
#include "tm4c123gh6pm.h"
#include "uart0.h"
#include "mm.h"

#define HeapBase512  0x20001000
#define HeapBase1024 0x20004000
#define HeapLimit 0x20008000
#define MaxAllocations 40

typedef struct{
    void* ptrAddress;
    uint8_t blocksUsed;
    uint16_t blocksSize;
} HeapAllocation;

HeapAllocation allocation[MaxAllocations];      //array to store the allocation info

//Global variables
uint64_t MemUse = 0;    //Index to track the usage of memory chunks
uint8_t count = 0;      //Counter to track number of allocations

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void StoreAllocation(void *addr, uint8_t blocks, uint16_t size)
{
    if (count < MaxAllocations)
    {
        allocation[count].ptrAddress = addr;        //Points to start of the allocation
        allocation[count].blocksUsed = blocks;      //count of the blocks allocated
        allocation[count].blocksSize = size;        //size of the allocation request
        count++;
    }
    else
    {
        putsUart0("Error: Maximum number of allocations reached.\n");
    }
}

uint16_t SizeMatch(uint16_t size)
{
    if (size <= 512)    //Size < 512 is considered as 512B.
    {
        return 512;
    }
    else                //Size > 512 is considered as 1024B.
    {
        return 1024;
    }
}


// REQUIRED: add your malloc code here and update the SRD bits for the current thread
void * mallocFromHeap(uint32_t size_in_bytes)
{
    uint16_t RoundedSize = SizeMatch(size_in_bytes);
    uint8_t TotalBlocks = RoundedSize == 512 ? 24 : 16;                             //Use 24 total blocks if 512B or else use 16 blocks
    uint8_t BlocksNeeded = RoundedSize == 512 ? 1 : ((size_in_bytes + 1023)/1024);  //Use the value of 1 if just one block or use
    if(RoundedSize == 512)
    {
        uint8_t blockIndex = 0;                                                     //There are 24 blocks of 512B : (3 regions * (8 subregions of 512B))
        for(blockIndex = 0; blockIndex < TotalBlocks; blockIndex++)
        {
            if((MemUse & (1ULL << blockIndex)) == 0)                                //Checks if block is free by using a bitmask to check the position of that bit
            {
                MemUse |= (1ULL << (blockIndex));                                   //Marks the block as used
                void* BlockAddress = (void*)(HeapBase512 + (blockIndex * 512));     //Calculates the allocated block address
                StoreAllocation(BlockAddress, 1, size_in_bytes);                    //Stores the allocation info in the struct
                return BlockAddress;                                                //Returns the address and exits the function
            }
        }

        //If 512B regions are full, check for space in the 1024B region [Edge case]
        for(blockIndex = 0; blockIndex < 16; blockIndex++)                          //16 blocks of 1024B
        {
            if ((MemUse & (1ULL << (24 + blockIndex))) == 0)                        //Checks for free blocks at an offset of 24 blocks since
            {                                                                       //first 24 blocks are for 512B allocations
                MemUse |= (1ULL << (24 + blockIndex));                              //Marks the 1024B block as used
                void* BlockAddress = (void*)(HeapBase1024 + (blockIndex * 1024));
                StoreAllocation(BlockAddress, 1, size_in_bytes);                    //Store the allocation info
                return BlockAddress;                                                //Return the allocated 1024B address for a 512B allocation
            }
        }
    }
    else
    {
        uint8_t blockIndex = 0;
        uint8_t i = 0;                                                      //There are 16 blocks of 1024B : (2 regions * (8 subregions of 1024B))
        uint8_t j = 0;
        for(blockIndex = 0; blockIndex < TotalBlocks; blockIndex++)
        {
            uint8_t found = 1;
            for(i = 0; i < BlocksNeeded; i++)
            {
                uint32_t currAddress = HeapBase1024 + ((blockIndex + i) * 1024);
                if(currAddress >= HeapLimit)
                {
                    return NULL;
                }

                if(MemUse & (1ULL << (24 + blockIndex + i)))                      //Offset of 24 cuz of 512B regions
                {
                    found = 0;
                    break;
                }
            }
            if(found)
            {
                for(j = 0; j < BlocksNeeded; j++)
                {
                    MemUse |= (1ULL << (24 + (blockIndex + j)));                       //Marks the block as used
                }
                void* BlockAddress = (void*)(HeapBase1024 + (blockIndex * 1024));   //Returns the allocated block address
                StoreAllocation(BlockAddress, BlocksNeeded, size_in_bytes);         //
                return BlockAddress;
            }
        }
    }
    return 0;
}

// REQUIRED: add your free code here and update the SRD bits for the current thread
void freeToHeap(void *pMemory)
{
    // Search the allocation array for the given pointer
    uint8_t i = 0;
    for (i = 0; i < count; i++)
    {
        if (allocation[i].ptrAddress == pMemory)  // Found the matching allocation
        {
            // Determine whether it's in the 512B or 1024B region
            if ((uintptr_t)pMemory >= HeapBase512 && (uintptr_t)pMemory < HeapBase1024)
            {
                // Free the blocks in the 512B region
                uint8_t j = 0;
                for (j = 0; j < allocation[i].blocksUsed; j++)
                {
                    uint16_t BlockIndex = ((uintptr_t)pMemory - HeapBase512) / 512;
                    MemUse &= ~(1ULL << (BlockIndex + j));  // Mark each block as free
                }
            }
            else if ((uintptr_t)pMemory >= HeapBase1024 && (uintptr_t)pMemory < HeapLimit)
            {
                // Free the blocks in the 1024B region
                uint8_t j = 0;
                for (j = 0; j < allocation[i].blocksUsed; j++)
                {
                    uint16_t BlockIndex = ((uintptr_t)pMemory - HeapBase1024) / 1024;
                    MemUse &= ~(1ULL << (24 + BlockIndex + j));  // Mark each block as free
                }
            }

            // Reset the allocation entry
            allocation[i].ptrAddress = NULL;
            allocation[i].blocksUsed = 0;
            allocation[i].blocksSize = 0;

            count--;
            return;
        }
    }
//    putsUart0("\nInvalid pointer passed.");
}

// REQUIRED: include your solution from the mini project
void allowFlashAccess(void)
{
    //0x0000.0000 to 0x0003.FFFF => LOG2(262143B) = 18....Size = 18 - 1 => 17
    //Base address is 0x0000.0000
    NVIC_MPU_NUMBER_R = 7;
    NVIC_MPU_BASE_R = 0x00000000;
    NVIC_MPU_ATTR_R &= ~(NVIC_MPU_ATTR_XN); //Lets the Execute Never remain disabled = Instructions executed.
    //              |  RW access all |   Cacheable   |   size of 17   |
    NVIC_MPU_ATTR_R |= (0b011 << 24) | (0b010 << 16) | (0b10001 << 1) | NVIC_MPU_ATTR_ENABLE;
}

void allowPeripheralAccess(void)
{
    //0x4000.0000 to 0x43FF.FFFF => LOG2(67108863) = 26... Size = 26 - 1 => 25
    NVIC_MPU_NUMBER_R = 1;   //???? what region #
    NVIC_MPU_BASE_R = 0x40000000;
    //                  Execute Never   | RW access all | Cache & Buff. |   size of 25   |
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_XN | (0b011 << 24) | (0b101 << 16) | (0b11001 << 1) | NVIC_MPU_ATTR_ENABLE;
}

void setupSramAccess(void)
{
    //=========== 4K regions =================
   //Region 2 (4K region) for 0x20001000 to 0x20002000 => Log2(4096) = 12... Size = 11
    NVIC_MPU_NUMBER_R = 2;
    NVIC_MPU_BASE_R = 0x20001000;
    //                  Execute Never   |  RW all access | Share & Cache |   size of 11   |
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_XN | (0b011 << 24) | (0b110 << 16) | (0b01011 << 1) | NVIC_MPU_ATTR_ENABLE;

    //Region 3 (4K region) for 0x20002000 to 0x20003000 => Log2(4096) = 12... Size = 11
    NVIC_MPU_NUMBER_R = 3;
    NVIC_MPU_BASE_R = 0x20002000;
    //                  Execute Never   | RW all access | Share & Cache |   size of 11   |
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_XN | (0b011 << 24) | (0b110 << 16) | (0b01011 << 1) | NVIC_MPU_ATTR_ENABLE;

    //Region 4 (4K region) for 0x20003000 to 0x20004000 => Log2(4096) = 12... Size = 11
    NVIC_MPU_NUMBER_R = 4;
    NVIC_MPU_BASE_R = 0x20003000;
    //                   Execute Never   |  RW all access | Share & Cache |   size of 11   |
    NVIC_MPU_ATTR_R |=  NVIC_MPU_ATTR_XN | (0b011 << 24) | (0b110 << 16) | (0b01011 << 1) | NVIC_MPU_ATTR_ENABLE;

    //============= 8K regions===============
    //Region 5 (8K region) for 0x20004000 to 0x20006000 => Log2(8192) = 13... Size = 12
    NVIC_MPU_NUMBER_R = 5;
    NVIC_MPU_BASE_R = 0x20004000;
    //                  Execute Never   | RW all access | Share & Cache |   size of 12   |
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_XN | (0b011 << 24) | (0b110 << 16) | (0b01100 << 1) | NVIC_MPU_ATTR_ENABLE;

    //Region 6 (8K region) for 0x20004000 to 0x20006000 => Log2(8192) = 13... Size = 12
    NVIC_MPU_NUMBER_R = 6;
    NVIC_MPU_BASE_R = 0x20006000;
    //                   Execute Never   | RW all access | Share & Cache |   size of 12   |
    NVIC_MPU_ATTR_R |=  NVIC_MPU_ATTR_XN | (0b011 << 24) | (0b110 << 16) | (0b01100 << 1) | NVIC_MPU_ATTR_ENABLE;
}

uint64_t createNoSramAccessMask(void)
{
    return 0xFFFFFFFFFF;  //since SRD (SubRegion Disable) = 0 means the subregion is enabled.
    //40 bits = 40 1s
}

void addSramAccessWindow(uint64_t *srdBitMask, uint32_t *baseAdd, uint32_t size_in_bytes)
{
    int SizeLeft = size_in_bytes;
    uint32_t currentAddress = (uintptr_t)baseAdd;
    uint8_t i = 0;

    while (SizeLeft > 0)
    {
        uint8_t regionNumber = 0;
        uint32_t regionBase = 0;
        uint16_t SRSize = 0;

        if (currentAddress >= 0x20001000 && currentAddress < 0x20002000)
        {
            regionNumber = 2;
            regionBase = 0x20001000;
            SRSize = 512;
        }
        else if (currentAddress >= 0x20002000 && currentAddress < 0x20003000)
        {
            regionNumber = 3;
            regionBase = 0x20002000;
            SRSize = 512;
        }
        else if (currentAddress >= 0x20003000 && currentAddress < 0x20004000)
        {
            regionNumber = 4;
            regionBase = 0x20003000;
            SRSize = 512;
        }
        else if (currentAddress >= 0x20004000 && currentAddress < 0x20006000)
        {
            regionNumber = 5;
            regionBase = 0x20004000;
            SRSize = 1024;
        }
        else if (currentAddress >= 0x20006000 && currentAddress < 0x20008000)
        {
            regionNumber = 6;
            regionBase = 0x20006000;
            SRSize = 1024;
        }
        else
        {
            putsUart0("error...\n");
        }

        //StartSR gets the index to start from.
        //Ex: 0x1400-0x1000 = 0x400 = 1024/512 = 2
        uint32_t StartSR = (currentAddress - regionBase)/SRSize;

        //EndSR gets the index to end at. '-1' for region alignment
        //Ex: (0x1400 + 0x400 - 0x1000 - 1) = 0x7FF - 1 = 2047/512 = 3
        uint32_t EndSR = (currentAddress + SizeLeft - regionBase-1)/SRSize;
        //uint16_t SRDcount = endSubregion;

        //each subregion is 0 to 7 so if more than 7 set it back to 7
        if(EndSR > 7)
        {
            EndSR = 7;
        }

        uint8_t regionOffset = (regionNumber - 2) * 8;

        //Depending on the start index, srd bits are enabled.
        for (i = StartSR; i <= EndSR; i++)
        {
            *srdBitMask &= ~(1ULL << (regionOffset + i));  // Set bit to 0
        }

        //Calculation for remaining size.
        uint32_t SizeDone = (EndSR - StartSR + 1) * SRSize;
        SizeLeft -= SizeDone;
        currentAddress += SizeDone;
    }
}

void applySramAccessMask(uint64_t srdBitMask)
{
    //Extract each 8 bits of the SRD Bit mask and apply it to each corresponding region.
    uint8_t i = 0;
    for (i = 2; i < 7; i++)
    {
        NVIC_MPU_NUMBER_R = i;

        uint8_t shiftAmount = (i - 2) * 8;
        uint64_t regionMask = (srdBitMask >> shiftAmount) & 0xFF;

        //              apply to SRD region | preserves other MPU_ATTR settings
        NVIC_MPU_ATTR_R = (regionMask << 8) | (NVIC_MPU_ATTR_R & 0xFFFF00FF);
    }
}

uint32_t RoundUp(uint32_t Bytes)
{
    if(Bytes == 512)
    {
        return 512;
    }
    else if(Bytes == 1024)
    {
        return 1024;
    }
    else
    {
        return ((Bytes + 1023) / 1024) * 1024;
    }
}
