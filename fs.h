#include "littlefs/fio.h"
#include "cell/sequencer.h"


int load_file(lfs_t *lfs, lfs_file_t* file, const char* path, void* buffer)
{
    int err = lfs_mount(lfs, &CFG);
    if(err) return err;
    if(lfs_file_open(lfs, file, path, LFS_O_RDWR) == 0)
    {
        lfs_file_read(lfs, file, buffer, sizeof(*buffer));
        lfs_file_close(lfs, file);
    }
    lfs_unmount(lfs);
    return 0;
}


int save_file(lfs_t *lfs, lfs_file_t* file, const char* path, const void* buffer)
{
    int err = lfs_mount(lfs, &CFG);
    if(err) return err;
    lfs_file_open(lfs, file, path, LFS_O_RDWR | LFS_O_CREAT);
    err = lfs_file_write(lfs, file, buffer, sizeof(*buffer));
    lfs_file_close(lfs, file);
    lfs_unmount(lfs);
    return err;
}


int lfs_ls(ssd1306_t* oled, lfs_t *lfs, const char *path) 
{
    int err = lfs_mount(lfs, &CFG);
    char str[16];
    lfs_dir_t dir;
    err = lfs_dir_open(lfs, &dir, path);
    if (err) return err;

    struct lfs_info info;
    while (true) 
    {
        int res = lfs_dir_read(lfs, &dir, &info);
        if (res  < 0) return res;
        if (res == 0) break;

        sprintf(str, "%s", info.name);
        ssd1306_log(oled, str, 0, 0);
    }

    err = lfs_dir_close(lfs, &dir);
    if (err) return err;
    lfs_unmount(lfs);
    return 0;
}