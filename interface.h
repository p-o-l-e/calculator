#pragma once
#include "hardware/clocks.h"
#include "hardware/structs/clocks.h"
#include "cell/midi_uart.h"
#include "SSD1306/ssd1306.h"
#include "CD74HC4067.h"
#include "littlefs/fio.h"

#define PAGE_MAIN 0
#define PAGE_NOTE 1
#define PAGE_VELO 2
#define PAGE_DRTN 3
#define PAGE_FFST 4
#define PAGE_AUTO 5
#define PAGE_DRFT 6
#define PAGE_LOG_ 7
#define PAGE_NCDR 12

#define PAGES     6


sequencer esq;
uint16_t point[_tracks];
static lfs_t      lfs;
static lfs_file_t INIT;

int load_init()
{
    int err = lfs_mount(&lfs, &CFG);

    if (err) 
    {
        lfs_format(&lfs, &CFG);
        lfs_mount (&lfs, &CFG);

        lfs_file_open (&lfs, &INIT, "PRESETS", LFS_O_RDWR | LFS_O_CREAT);
        lfs_file_write(&lfs, &INIT, &esq, sizeof(esq));
        lfs_file_close(&lfs, &INIT);
    }
    lfs_file_open(&lfs, &INIT, "PRESETS", LFS_O_RDWR);
    lfs_file_read(&lfs, &INIT, &esq, sizeof(esq));

    lfs_file_close(&lfs, &INIT);
    lfs_unmount(&lfs);
}



int load_file(int n, const char* path)
{
    int err = lfs_mount(&lfs, &CFG);
    if (err) 
    {
        lfs_format(&lfs, &CFG);
        lfs_mount (&lfs, &CFG);

        lfs_file_open (&lfs, &INIT, path, LFS_O_RDWR | LFS_O_CREAT);
        lfs_file_write(&lfs, &INIT, &esq, sizeof(esq));
        lfs_file_close(&lfs, &INIT);
    }

    if(lfs_file_open (&lfs, &INIT, path, LFS_O_RDWR) == 0)
    {
        lfs_file_read (&lfs, &INIT, &esq, sizeof(esq));
        lfs_file_close(&lfs, &INIT);
    }

    lfs_unmount(&lfs);
}


int save_file(int n, const char* path)
{
    int err = lfs_mount(&lfs, &CFG);
    lfs_file_open (&lfs, &INIT, path, LFS_O_RDWR | LFS_O_CREAT);
    err = lfs_file_write(&lfs, &INIT, &esq, sizeof(esq));
    lfs_file_close(&lfs, &INIT);
    lfs_unmount(&lfs);
    return err;
}


int save_init()
{
    int err = lfs_mount(&lfs, &CFG);
    lfs_file_open (&lfs, &INIT, "PRESETS", LFS_O_RDWR | LFS_O_CREAT);
    lfs_file_write(&lfs, &INIT, &esq, sizeof(esq));
    lfs_file_close(&lfs, &INIT);
    lfs_unmount(&lfs);
    return err;
}