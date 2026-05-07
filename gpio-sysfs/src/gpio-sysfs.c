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
};

struct gpiodrv_private_data gpio_drv_data;

static int gpio_sysfs_remove(struct platform_device *pdev)
{
    pr_info("gpio_sysfs_remove\n");
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
            strcpy(dev_data->label, name);
            dev_info(dev, "GPIO label = %s\n", dev_data->label);
        }

        dev_data->desc = devm_fwnode_gpiod_get_index(dev, &child->fwnode,
            "bone", 0,
            GPIOD_ASIS,
            dev_data->label);

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

    ret = platform_driver_register(&gpiosysfs_platform_driver);
    if (ret) {
        pr_err("platform_driver_register failed: %d\n", ret);
        class_destroy(gpio_drv_data.class_gpio);
        return ret;
    }

    pr_info("module load success\n");
    return 0;
}

static void __exit gpio_sysfs_exit(void)
{
    platform_driver_unregister(&gpiosysfs_platform_driver);
    class_destroy(gpio_drv_data.class_gpio);
    pr_info("module unload success\n");
}

module_init(gpio_sysfs_init);
module_exit(gpio_sysfs_exit);