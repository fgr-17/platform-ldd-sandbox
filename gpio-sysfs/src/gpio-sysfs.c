#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio/machine.h>

#undef pr_fmt
#define pr_fmt(fmt) "%s : " fmt,__func__

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Federico Roux");
MODULE_DESCRIPTION("A simple GPIO sysfs module");
MODULE_VERSION("1.0");

/* Device private data structure */
struct gpiodev_private_data {
    char label[20];
    struct gpio_desc *desc;
};

/* Driver private data structure */
struct gpiodrv_private_data {
    int total_devices;
    struct class* class_gpio;
    struct device **dev;
};

struct gpiodrv_private_data gpio_drv_data;

/*
 * Runtime mapping for gpio-sim.
 * setup-gpio-sim uses label "sandbox-gpio" by default.
 */
static struct gpiod_lookup_table bone_gpio_lookup_table = {
    .dev_id = "4000000.bone_gpio_devs",
    .table = {
        GPIO_LOOKUP_IDX("sandbox-gpio", 0, "bone", 0, GPIO_ACTIVE_HIGH),
        GPIO_LOOKUP_IDX("sandbox-gpio", 1, "bone", 1, GPIO_ACTIVE_HIGH),
        GPIO_LOOKUP_IDX("sandbox-gpio", 2, "bone", 2, GPIO_ACTIVE_HIGH),
        GPIO_LOOKUP_IDX("sandbox-gpio", 3, "bone", 3, GPIO_ACTIVE_HIGH),
        GPIO_LOOKUP_IDX("sandbox-gpio", 4, "bone", 4, GPIO_ACTIVE_HIGH),
        { },
    },
};


/* ---------------- Device attributes ---------------- */


// show/store method for device attributes
static ssize_t direction_show(struct device *dev, struct device_attribute *attr, char *buf) {
    struct gpiodev_private_data *dev_data = dev_get_drvdata(dev);
    if(!dev_data) {
        dev_err(dev, "Failed to get device data");
        return -EINVAL;
    }

    int dir;
    dir = gpiod_get_direction(dev_data->desc);
    if(dir < 0) {
        dev_err(dev, "Failed to get GPIO direction: %d\n", dir);
        return dir;
    }

    return sysfs_emit(buf, "%s\n", dir ? "out" : "in");

}

static ssize_t direction_store(struct device *dev, struct device_attribute *attr,
                               const char *buf, size_t size) {
    
    int ret = 0;
    struct gpiodev_private_data* dev_data = dev_get_drvdata(dev);
    
    if(!dev_data) {
        dev_err(dev, "Failed to get device data");
        return -EINVAL;
    }

    if(sysfs_streq(buf, "in")) {
        ret = gpiod_direction_input(dev_data->desc);
    }
    else if(sysfs_streq(buf, "out")) {
        ret = gpiod_direction_output(dev_data->desc, 0);
    }
    else {
        dev_err(dev, "Invalid direction: %s\n", buf);
        ret = -EINVAL;
    }

    return ret ? : size;
}

static ssize_t value_show(struct device *dev, struct device_attribute *attr, char *buf) {
    struct gpiodev_private_data* dev_data = dev_get_drvdata(dev);
    int value;
    value = gpiod_get_value_cansleep(dev_data->desc);
    if(value < 0) {
        dev_err(dev, "Failed to get GPIO value: %d\n", value);
        return value;
    }
    return sysfs_emit(buf, "%d\n", value);
}

static ssize_t value_store(struct device *dev, struct device_attribute *attr,
                           const char *buf, size_t size) {
    struct gpiodev_private_data* dev_data = dev_get_drvdata(dev);
    int ret = 0;
    long value;

    ret = kstrtol(buf, 0, &value);
    if(ret < 0) {
        dev_err(dev, "Failed to convert value: %s\n", buf);
        return ret;
    }

    gpiod_set_value_cansleep(dev_data->desc, value);
    return size;
}

static ssize_t label_show(struct device *dev, struct device_attribute *attr, char *buf) {
    struct gpiodev_private_data* dev_data = dev_get_drvdata(dev);
    return sysfs_emit(buf, "%s\n", dev_data->label);
}

static DEVICE_ATTR_RW(direction);
static DEVICE_ATTR_RW(value);
static DEVICE_ATTR_RO(label);

/** NULL terminated array of attributes */
static struct attribute *gpio_attrs[] = {
    &dev_attr_direction.attr,
    &dev_attr_value.attr,
    &dev_attr_label.attr,
    NULL,
};

/** Attribute group */
static const struct attribute_group gpio_attr_group = {
    .attrs = gpio_attrs,
};

/** NULL terminated array of attribute groups */
static const struct attribute_group *gpio_attr_groups[] = {
    &gpio_attr_group,
    NULL,
};

/* -------------------------------------------- */


static int gpio_sysfs_remove(struct platform_device *pdev) {

    dev_info(&pdev->dev, "gpio_sysfs_remove\n");
    for(int i = 0; i < gpio_drv_data.total_devices; i++) {
        device_unregister(gpio_drv_data.dev[i]);
    }
    return 0;
}

static int gpio_sysfs_probe(struct platform_device *pdev)
{
    pr_info("gpio_sysfs_probe\n");
    
    int i = 0;
    int ret = 0;
    const char *name = NULL;
    struct device *dev = &pdev->dev;
    struct gpiodev_private_data *dev_data = NULL;
    struct device_node *parent = pdev->dev.of_node;
    struct device_node *child = NULL;


    gpio_drv_data.total_devices = of_get_child_count(parent);

    if(gpio_drv_data.total_devices <= 0) {
        dev_err(dev, "No children found for parent %pOFn\n", parent);
        return -EINVAL;
    }

    dev_info(dev, "Total devices: %d\n", gpio_drv_data.total_devices);
    gpio_drv_data.dev = devm_kzalloc(dev, gpio_drv_data.total_devices * sizeof(struct device *), GFP_KERNEL);

    for_each_available_child_of_node(parent, child) {
        dev_data = devm_kzalloc(dev, sizeof(*dev_data), GFP_KERNEL);
        if (!dev_data) {
            pr_err("Failed to allocate memory for dev_data\n");
            return -ENOMEM;
        }

        // read label property from DT node
        if(of_property_read_string(child, "label", &name)) {
            dev_warn(dev, "No label found for child %pOFn\n", child);
            snprintf(dev_data->label, sizeof(dev_data->label), "unkngpio-%d", i);
        }
        else {
            strscpy(dev_data->label, name, sizeof(dev_data->label));
            dev_info(dev, "GPIO label = %s\n", dev_data->label);
        }

        /*
         * Use machine lookup table to bind this platform device to runtime
         * gpio-sim lines. Child DT labels are still used for sysfs names.
         */
        dev_data->desc = devm_gpiod_get_index(dev, "bone", i, GPIOD_ASIS);

        if(IS_ERR(dev_data->desc)) {
            ret = PTR_ERR(dev_data->desc);
            pr_err("Failed to get GPIO descriptor for child %pOFn: %ld\n", child, PTR_ERR(dev_data->desc));
            if(ret == -ENOENT) {
                dev_err(dev, "Failed to set GPIO direction to output for child %pOFn: %d\n",
                                                                                child, ret);
            }
            return ret;
        }

        /* set gpio direction to output */
        ret = gpiod_direction_output(dev_data->desc, 0);
        if(ret) {
            dev_err(dev, "Failed to set GPIO direction to output for child %pOFn: %d\n", child, ret);
            return ret;
        }

        /** Create devices under `/sys/class/bone_gpio */

        gpio_drv_data.dev[i] = device_create_with_groups(gpio_drv_data.class_gpio, dev, 0, dev_data, gpio_attr_groups, dev_data->label);
        if(IS_ERR(gpio_drv_data.dev[i])) {
            dev_err(dev, "Failed to create device for child");
            return PTR_ERR(gpio_drv_data.dev[i]);
        }

        i++;
    }

    return 0;
}

static const struct of_device_id gpio_device_match[] = {
    { .compatible = "org,bone-gpio-sysfs", },
    {},
};

static struct platform_driver gpiosysfs_platform_driver = {
    .probe = gpio_sysfs_probe,
    .remove = gpio_sysfs_remove,
    .driver = {
        .name = "bone-gpio-sysfs",
        .of_match_table = of_match_ptr(gpio_device_match),
    },
};

static int __init gpio_sysfs_init(void)
{
    int ret;

    gpio_drv_data.class_gpio = class_create("bone_gpio");
    if (IS_ERR(gpio_drv_data.class_gpio)) {
        pr_err("Failed to create class\n");
        return PTR_ERR(gpio_drv_data.class_gpio);
    }

    gpiod_add_lookup_table(&bone_gpio_lookup_table);

    ret = platform_driver_register(&gpiosysfs_platform_driver);
    if (ret) {
        pr_err("platform_driver_register failed: %d\n", ret);
        gpiod_remove_lookup_table(&bone_gpio_lookup_table);
        class_destroy(gpio_drv_data.class_gpio);
        return ret;
    }

    pr_info("module load success\n");
    return 0;
}

static void __exit gpio_sysfs_exit(void)
{
    platform_driver_unregister(&gpiosysfs_platform_driver);
    gpiod_remove_lookup_table(&bone_gpio_lookup_table);
    class_destroy(gpio_drv_data.class_gpio);
    pr_info("module unload success\n");
}

module_init(gpio_sysfs_init);
module_exit(gpio_sysfs_exit);