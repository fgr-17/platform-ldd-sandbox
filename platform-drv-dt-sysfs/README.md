# Linux device driver model

TODO: intro for this section

## Building and Testing

### 1. Build the two modules

~~~bash
docker exec -it platform-ldd-sandbox bash
cd 10-platform-driver
make
~~~

### 2. Load in QEMU

The order of loading the two modules shows a difference in the output:

1. If loading first the driver and then the devices, it doesn't show any matching when the driver is loaded, but shows logs when the devices are registered:

~~~bash
./run-qemu.sh
insmod /modules/platform-drv-sysfs.ko
dmesg | tail
~~~

    Expected output:

~~~
platform-driver.ko  platform-drv.ko
[   16.902016] platform_drv: loading out-of-tree module taints kernel.
[   16.910718] platform_drv: module verification failed: signature and/or required key missing - tainting kernel~~~
~~~

### 3. Checking attributes

**Sysfs attribute code (lines 27–46).** The driver exposes two device attributes:

- **`max_size`** — read/write: `max_size_show()` is called when you read the attribute (e.g. `cat max_size`); `max_size_store()` when you write (e.g. `echo 1024 > max_size`). They are wired with `DEVICE_ATTR(max_size, S_IRUGO|S_IWUSR, max_size_show, max_size_store)` (read for all, write for owner). The current implementations only log to the kernel and return 0.
- **`serial_num`** — read-only: `serial_num_show()` is used for reads; `DEVICE_ATTR_RO(serial_num)` creates the attribute with no store callback.

The macros `DEVICE_ATTR` and `DEVICE_ATTR_RO` define `struct device_attribute` instances (`dev_attr_max_size`, `dev_attr_serial_num`). In `platform_drv_probe()`, `pcd_sysfs_create_files()` attaches them to the device’s kobject with `sysfs_create_file()`, so they appear under that device in sysfs.

**Where they appear in QEMU.** The class is created with `class_create("pcd_class")`, which shows up as `/sys/class/pcd-class/`. For each probed platform device, `device_create(..., "pdev-%d", ...)` creates a device directory (e.g. `pdev-0`, `pdev-1`). The attributes are created on that device’s kobject, so you see them at:

- `/sys/class/pcd-class/pdev-0/max_size`
- `/sys/class/pcd-class/pdev-0/serial_num`

So inside QEMU, `ls /sys/class/pcd-class/pdev-0/` lists `max_size` and `serial_num`; reading or writing those files triggers the corresponding show/store callbacks in the driver. This is how the kernel connects with the user space *through the VFS*.


