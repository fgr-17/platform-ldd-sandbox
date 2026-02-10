#ifndef _PLATFORM_DRV_DT_SYSFS_H_
#define _PLATFORM_DRV_DT_SYSFS_H_

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/mod_devicetable.h>
#include <linux/of.h>
#include <linux/of_device.h>

#include "platform.h"

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

/**
 * @brief platform device private data, allocated dynamically when registering devices
 */
 struct platform_dev_private_data {
    struct platform_data pdata;
    char*buf;
    dev_t dev_num;
    struct cdev cdev;
};

/**
 * @brief platform driver private data, allocated globally
 */
 struct platform_drv_private_data {
    int total_devices;
    dev_t dev_num_base;
    struct class* class_pcd;
    struct device* device_pcd;
}; 

#endif /* _PLATFORM_DRV_DT_SYSFS_H_ */