#include "littlefs/fio.h"
#include "cell/sequencer.h"


int load_file(lfs_t *lfs, lfs_file_t* file, const char* path, void* buffer)
{
    int err = lfs_mount(lfs, &CFG);
    if(err) return err;
    if(lfs_file_open(lfs, file, path, LFS_O_RDWR) == 0)
    {
        lfs_file_read(lfs, file, buffer, sizeof(sequencer));
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
    err = lfs_file_write(lfs, file, buffer, sizeof(sequencer));
    lfs_file_close(lfs, file);
    lfs_unmount(lfs);
    return err;
}


int format(lfs_t *lfs)
{
	int err = lfs_mount(lfs, &CFG);
	if (err) 
	{
		lfs_format(lfs, &CFG);
	    lfs_mount (lfs, &CFG);
	}
	lfs_unmount(lfs);
	return 0;
}

int lfs_ls(lfs_t *lfs, const char *path, char list[5][16], int start) 
{
    int err = lfs_mount(lfs, &CFG);
    lfs_dir_t dir;
    err = lfs_dir_open(lfs, &dir, path);
    if (err) return err;

    struct lfs_info info;
    int l = 0;
	int p = 0;
	for(int i = 0; i < 5; ++i) sprintf(list[i], "                ");
    
    while (true) 
    {
        int res = lfs_dir_read(lfs, &dir, &info);
        if(res  < 0) return res;
        if(res == 0) break;

		++l;
		if(l > start)
		{
			if(l < 3) sprintf(list[p], "        ");
	     	else sprintf(list[p], "%s", info.name);
	      	++p;
		}
    }

    err = lfs_dir_close(lfs, &dir);
    if (err) return err;
    lfs_unmount(lfs);
    return l-2;
}


int get_file_count(lfs_t *lfs, const char *path)
{
 	int err = lfs_mount(lfs, &CFG);
    lfs_dir_t dir;
    err = lfs_dir_open(lfs, &dir, path);
    if (err) return err;

    struct lfs_info info;
    int l = 0;
    
    while (true) 
    {
        int res = lfs_dir_read(lfs, &dir, &info);
        if(res  < 0) return res;
        if(res == 0) break;
		++l;
    }

    err = lfs_dir_close(lfs, &dir);
    if (err) return err;
    lfs_unmount(lfs);
    return l-2;	
}
