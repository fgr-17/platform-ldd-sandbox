#ifndef _PCD_SYSCALLS_H_
#define _PCD_SYSCALLS_H_

int check_permission(int dev_perm, int acc_mode);
loff_t platform_dev_lseek(struct file *filep, loff_t offset, int whence);
ssize_t platform_dev_read(struct file *filep, char __user *buf, size_t count, loff_t *offset);
ssize_t platform_dev_write(struct file *filep, const char __user *buf, size_t count, loff_t *offset);
int platform_dev_open(struct inode *inodep, struct file *filep);
int platform_dev_release(struct inode *inodep, struct file *filep);

#endif /* _PCD_SYSCALLS_H_ */