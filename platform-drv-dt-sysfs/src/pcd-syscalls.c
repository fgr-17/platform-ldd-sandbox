#include "platform-drv-dt-sysfs.h"
#include "pcd-syscalls.h"
#include "platform.h"

int check_permission(int dev_perm, int acc_mode) {
    if(dev_perm == O_RDWR) {
        return 0;
    }

    if((dev_perm == O_RDONLY) && (acc_mode & FMODE_READ) && !(acc_mode & FMODE_WRITE)) {
        return 0;
    }

    if((dev_perm == O_WRONLY) && (acc_mode & FMODE_WRITE) && !(acc_mode & FMODE_READ)) {
        return 0;
    }

    return -EPERM;
}

loff_t platform_dev_lseek(struct file *filep, loff_t offset, int whence) {
    return 0;
}

ssize_t platform_dev_read(struct file *filep, char __user *buf, size_t count, loff_t *offset) {
    return 0;
}

ssize_t platform_dev_write(struct file *filep, const char __user *buf, size_t count, loff_t *offset) {
    return -ENOMEM;
}

int platform_dev_open(struct inode *inodep, struct file *filep) {
    pr_info("platform_dev_open: Device opened\n");
    return 0;
}

int platform_dev_release(struct inode *inodep, struct file *filep) {
    pr_info("platform_dev_release: Device released\n");
    return 0;
}