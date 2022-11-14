#pragma once
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "lfs.h"

// #define BLOCK_SIZE_BYTES (FLASH_SECTOR_SIZE)
#define HW_FLASH_STORAGE_BYTES  (1408 * 1024)
#define HW_FLASH_STORAGE_BASE   (PICO_FLASH_SIZE_BYTES - HW_FLASH_STORAGE_BYTES) // 655360

static recursive_mutex_t fs_mtx;

static int pico_lock(void) 
{
    recursive_mutex_enter_blocking(&fs_mtx);
    return LFS_ERR_OK;
}

static int pico_unlock(void) 
{
    recursive_mutex_exit(&fs_mtx);
    return LFS_ERR_OK;
}

int pico_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    uint32_t fs_start = XIP_BASE + HW_FLASH_STORAGE_BASE;
    uint32_t addr = fs_start + (block * c->block_size) + off;
    memcpy(buffer, (unsigned char *)addr, size);
    return 0;
}

int pico_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
    uint32_t fs_start = HW_FLASH_STORAGE_BASE;
    uint32_t addr = fs_start + (block * c->block_size) + off;
    uint32_t ints = save_and_disable_interrupts();
    flash_range_program(addr, (const uint8_t *)buffer, size);
    restore_interrupts(ints);
    
    return 0;
}

int pico_erase(const struct lfs_config *c, lfs_block_t block)
{           
    uint32_t fs_start = HW_FLASH_STORAGE_BASE;
    uint32_t offset = fs_start + (block * c->block_size);
    uint32_t ints = save_and_disable_interrupts();   
    flash_range_erase(offset, c->block_size);  
    restore_interrupts(ints);
    
    return 0;
}

int pico_sync(const struct lfs_config *c)
{
    return 0;
}

const struct lfs_config CFG = {

    .read           = &pico_read,
    .prog           = &pico_prog,
    .erase          = &pico_erase,
    .sync           = &pico_sync,

    .read_size      = FLASH_PAGE_SIZE, // 256
    .prog_size      = FLASH_PAGE_SIZE, // 256
    .block_size     = FLASH_SECTOR_SIZE,// 4096
    .block_count    = HW_FLASH_STORAGE_BYTES / FLASH_SECTOR_SIZE, // 352
    .block_cycles   = 16, // ?
    .cache_size     = FLASH_PAGE_SIZE, // 256
    .lookahead_size = FLASH_PAGE_SIZE, // 256    
};