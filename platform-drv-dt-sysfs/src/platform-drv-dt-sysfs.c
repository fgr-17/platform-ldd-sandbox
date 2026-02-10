#include "platform-drv-dt-sysfs.h"
#include "platform.h"
#include "pcd-syscalls.h"

#define MAX_DEVICES 4
#define CONFIG_ATTRIBUTE_GROUPS

static ssize_t max_size_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t max_size_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t serial_num_show(struct device *dev, struct device_attribute *attr, char *buf);
static int pcd_sysfs_create_files(struct device*device);

struct platform_drv_private_data pdrv_data;
struct dev_custom_config pdev_config_table[] = {
    [PDEV_A1X] = {.config_1 = 10, .config_2 = 20},
    [PDEV_B1X] = {.config_1 = 100, .config_2 = 200}
};

static struct file_operations fops = {
    .open = platform_dev_open,  
    .release = platform_dev_release,
    .read = platform_dev_read,
    .write = platform_dev_write,
    .llseek = platform_dev_lseek,
    .owner = THIS_MODULE
};

/**
 * @brief show callbacks for the device attributes
 * @param dev: the device struct
 * @param attr: the attribute struct
 * @param buf: the buffer to store the data: pointer from kernel space to user space
 * @param buf: size limited to PAGE_SIZE (4096 bytes for most architectures)
 * @return should return the number of bytes written to buf, or an error code
 */

ssize_t max_size_show(struct device *dev, struct device_attribute *attr, char *buf) {
    printk(KERN_INFO "max_size_show\n");
    struct platform_dev_private_data* dev_data = dev_get_drvdata(dev->parent);
    return sprintf(buf, "%d", dev_data->pdata.size);
}

/**
 * @brief store callbacks for the device attributes, called when you write to the attribute
 * @param dev: the device struct
 * @param attr: the attribute struct
 * @param buf: user data from user space to kernel space, NULL terminated string, limited to PAGE_SIZE (4096 bytes for most architectures)
 * @param count: the number of bytes to write
 * @return should return the number of bytes written to buf, or an error code
 */

ssize_t max_size_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
    printk(KERN_INFO "max_size_store\n");

    long result; 
    int ret;
    struct platform_dev_private_data* dev_data = dev_get_drvdata(dev->parent);
    ret = kstrtol(buf, 10, &result);
    if(ret) {
        return ret;
    }
    dev_data->pdata.size = result;
    dev_data->buf = krealloc(dev_data->buf, dev_data->pdata.size, GFP_KERNEL);
    if(!dev_data->buf) {
        return -ENOMEM;
    }

    return count; 
}

ssize_t serial_num_show(struct device *dev, struct device_attribute *attr, char *buf) {
    printk(KERN_INFO "serial_num_show\n");
    /* get access to the device private data */
    /* dev: is a pointer to the private data of the device */
    struct platform_dev_private_data* dev_data = dev_get_drvdata(dev->parent);
    return sprintf(buf, "%s", dev_data->pdata.serial_number);
}

/**
 * @brief create 2 variables of struc device_attribute
 */

 static DEVICE_ATTR(max_size, S_IRUGO|S_IWUSR, max_size_show, max_size_store);
 static DEVICE_ATTR_RO(serial_num);

 // null terminated array of attributes
 struct attribute *pcd_attrs[] = {
    &dev_attr_max_size.attr,
    &dev_attr_serial_num.attr,
    NULL
 };

 const struct attribute_group pcd_attr_group = {
    .attrs = pcd_attrs,
 };

/**
 * @brief each entry in devicetree is a `struct platform_device` that will be passed
 * to the `probe()` function of the driver. The `struct platform_device` contains a
 * `struct device` inside, that, at the same time, has an internal structure called
 * `struct device_node` that represents the details of node in dt, called `of_node`.
 * If `of_node` is not null, it means that the `probe()` function was called because
 * of a device tree node
 */

struct platform_data* pdev_get_platform_data_from_dt(struct device*dev) {
    struct device_node*dev_node = dev->of_node;
    struct platform_data*pdata;
    if(!dev_node) {
        // probe() wasn't called because of dt node
        return NULL;
    }

    pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
    if(!pdata) {
        dev_info(dev, "Cannot allocate memory");
        return ERR_PTR(-ENOMEM);
    }

    if(of_property_read_string(dev_node, "org,device-serial-num", &pdata->serial_number)) {
        dev_info(dev, "Missing serial number");
        return ERR_PTR(-EINVAL);
    }

    if(of_property_read_u32(dev_node, "org,size", &pdata->size)) {
        dev_info(dev, "Missing size number");
        return ERR_PTR(-EINVAL);
    }

    if(of_property_read_u32(dev_node, "org,perm", &pdata->perm)) {
        dev_info(dev, "Missing permission prop");
        return ERR_PTR(-EINVAL);
    }

    return pdata;
}

int pcd_sysfs_create_files(struct device*device) {
#ifndef CONFIG_ATTRIBUTE_GROUPS
    int ret = 0;
    ret = sysfs_create_file(&device->kobj, &dev_attr_max_size.attr);
    if(ret) return ret;
    return sysfs_create_file(&device->kobj, &dev_attr_serial_num.attr);
#else
    return sysfs_create_group(&device->kobj, &pcd_attr_group);
#endif
}

/**
 * @brief called when matched platform dev is found
 */
int platform_drv_probe(struct platform_device*pdev) {

    struct platform_dev_private_data* dev_data;
    struct platform_data* pdev_data;
    int ret = 0;
    int config_entry = 0;

    struct device*dev = &pdev->dev;

    /** @brief detects the matched entry of `of_device_id` list of this driver */
    const struct of_device_id* match;

    dev_info(dev, "Device detected");

    // match will be null if CONFIG_OF is disabled (device tree not supported)
    match = of_match_device(of_match_ptr(org_pdev_dt_match), dev);
    if(match) {
        pdev_data = pdev_get_platform_data_from_dt(dev);
        if(IS_ERR(pdev_data)) {
            return PTR_ERR(pdev_data);
        }
        config_entry = (uintptr_t) match->data;
    }
    else {
        pdev_data = (struct platform_data*) dev_get_platdata(dev);
        if(!pdev_data) {
            pr_err("No platform data available");
            return -EINVAL;
        }
        config_entry = pdev->id_entry->driver_data;
    }



    // instance not coming from devicetree
    // pdev_data = pdev->dev.platform_data;
    if(!pdev_data) {
        pr_err("No platform data available");
        return -EINVAL;
    }
    /** @brief if using of_match_* stuff, we don't need the following code: */
    // }
    // else {
    //     // match comes from devicetree
    //     // match = os_match_device(pdev->dev.driver->of_match_table, dev);
    //     // config_entry = (int)match->data;
    //     config_entry = (uintptr_t) of_device_get_match_data(dev);
    // }

    // dev_data = kzalloc(sizeof(*dev_data), GFP_KERNEL);
    dev_data = devm_kzalloc(&pdev->dev, sizeof(*dev_data), GFP_KERNEL);
    if(!dev_data) {
        pr_err("Cannot allocate memory for dev data");
        return -ENOMEM;
    }

    dev_set_drvdata(dev, dev_data);

    dev_data->pdata.size = pdev_data->size;
    dev_data->pdata.perm = pdev_data->perm;
    dev_data->pdata.serial_number = pdev_data->serial_number;

    pr_info("probe cb: Device detected");
    pr_info("sn: %s", dev_data->pdata.serial_number);
    pr_info("perm: %x", dev_data->pdata.perm);
    pr_info("size: %d", dev_data->pdata.size);

    pr_info("Using config index: %d", config_entry);
    pr_info("config 1: %d", pdev_config_table[config_entry].config_1);
    pr_info("config 2: %d", pdev_config_table[config_entry].config_2);


    // dev_data->buf = kzalloc(dev_data->pdata.size, GFP_KERNEL);
    dev_data->buf = devm_kzalloc(&pdev->dev, dev_data->pdata.size, GFP_KERNEL);

    if(!dev_data->buf) {
        pr_err("Cannot allocate memory for dev buffer");
        ret = -ENOMEM;
        goto dev_data_free;
    }

    dev_data->dev_num = pdrv_data.dev_num_base + pdrv_data.total_devices;

    cdev_init(&dev_data->cdev, &fops);
    dev_data->cdev.owner = THIS_MODULE;
    ret = cdev_add(&dev_data->cdev, dev_data->dev_num, 1);
    if(ret < 0) {
        pr_err("`cdev_add failed`");
        goto buffer_free;
    }

    pdrv_data.device_pcd = device_create(pdrv_data.class_pcd, dev, dev_data->dev_num, NULL, "pdev-%d", pdrv_data.total_devices);
    if(IS_ERR(pdrv_data.device_pcd)) {
        pr_err("Cannot create device");
        ret = PTR_ERR(pdrv_data.device_pcd);
        goto cdev_del;
    }

    dev_set_drvdata(&pdev->dev, dev_data);
    
    
    ret = pcd_sysfs_create_files(pdrv_data.device_pcd);
    if(ret < 0) {
        pr_err("Cannot create sysfs files");
        device_destroy(pdrv_data.class_pcd, dev_data->dev_num);
        goto cdev_del;
    }
    
    pdrv_data.total_devices++;
    pr_info("successful probe()!");
    return 0;

cdev_del:
    cdev_del(&dev_data->cdev);

buffer_free:
    // kfree(dev_data->buf);    // no need to do this if using resource managed kernel APIs
    devm_kfree(&pdev->dev, dev_data->buf);

dev_data_free:
    // kfree(dev_data);          // no need to do this if using resource managed kernel APIs
    devm_kfree(&pdev->dev, dev_data);
    return ret;
}


/**
 * @brief called when device is removed
 */
int platform_drv_remove(struct platform_device*pdev) {
    pr_info("Device removed");
    struct platform_dev_private_data*dev_data = dev_get_drvdata(&pdev->dev);
    device_destroy(pdrv_data.class_pcd, dev_data->dev_num);
    cdev_del(&dev_data->cdev);
    // no need to do this if using resource managed kernel APIs
    // kfree(dev_data->buf);
    // kfree(dev_data);
    pdrv_data.total_devices--;
    // pr_info("remove cb: device removed");
    dev_info(&pdev->dev, "remove cb: device removed");
    return 0;
}

struct platform_device_id pdevs_id[] = {
    [0] = {.name = "pdev-A1x", .driver_data = PDEV_A1X},
    [1] = {.name = "pdev-B1x", .driver_data = PDEV_B1X},
    {}          // array must be null terminated!
};

struct of_device_id org_pdev_dt_match[] = {
    {.compatible = "pdev-A1x", .data = (void*)PDEV_A1X},
    {.compatible = "pdev-B1x", .data = (void*)PDEV_B1X},
    {}
};


/**
 * @brief needs to define probe and remove cb
 * @brief also device_driver struct
 */
struct platform_driver platform_drv = {
    .probe = platform_drv_probe,
    .remove = platform_drv_remove,
    .id_table = pdevs_id,
    .driver = {
        .name = "pseudo-char-dev",
        .of_match_table = of_match_ptr(org_pdev_dt_match)
    }
};


static int __init platform_drv_init(void) {

    int ret;
    ret = alloc_chrdev_region(&pdrv_data.dev_num_base, 0, MAX_DEVICES, "pdrv");
    if(ret < 0) {
        pr_err("Alloc driver failed\n");
        return ret;
    }

    pdrv_data.class_pcd = class_create("pcd_class");
    if(IS_ERR(pdrv_data.class_pcd)) {
        pr_err("Platform driver class creation failed");
        ret = PTR_ERR(pdrv_data.class_pcd);
        unregister_chrdev_region(pdrv_data.dev_num_base, MAX_DEVICES);
        return ret;
    }

    platform_driver_register(&platform_drv);
    pr_info("platform driver loaded");
    return 0;
}

static void __exit platform_drv_cleanup(void) {
    platform_driver_unregister(&platform_drv);

    class_destroy(pdrv_data.class_pcd);
    unregister_chrdev_region(pdrv_data.dev_num_base, MAX_DEVICES);

    pr_info("platform driver unloaded");
}

module_init(platform_drv_init);
module_exit(platform_drv_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("fgr-17");
