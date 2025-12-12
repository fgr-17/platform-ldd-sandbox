/* Function prototypes */
int check_permission(int dev_perm, int acc_mode);
loff_t platform_dev_lseek(struct file *filep, loff_t offset, int whence);
ssize_t platform_dev_read(struct file *filep, char __user *buf, size_t count, loff_t *offset);
ssize_t platform_dev_write(struct file *filep, const char __user *buf, size_t count, loff_t *offset);
int platform_dev_open(struct inode *inodep, struct file *filep);
int platform_dev_release(struct inode *inodep, struct file *filep);
int platform_drv_probe(struct platform_device*pdev);
int platform_drv_remove(struct platform_device*pdev);
struct platform_data* pdev_get_platform_data_from_dt(struct device*dev);