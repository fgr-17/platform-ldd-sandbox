# platform-drv-dt-lock

Clone of [`platform-drv-dt-sysfs`](../platform-drv-dt-sysfs/) for lock/concurrency work. Same Device Tree bindings (`pdev-A1x`, `pdev-B1x`), sysfs attributes (`max_size`, `serial_num`), and char-device layout — with distinct module and user-visible names.

| Item | `platform-drv-dt-sysfs` | `platform-drv-dt-lock` |
|------|-------------------------|------------------------|
| Module | `platform-drv-dt-sysfs.ko` | `platform-drv-dt-lock.ko` |
| Platform driver | `pseudo-char-dev` | `pseudo-char-dev-lock` |
| Sysfs class | `/sys/class/pcd_class/` | `/sys/class/pcd_lock_class/` |
| Char devices | `/dev/pdev-0`, … | `/dev/pdev-lock-0`, … |

```bash
cd /workspace/platform-drv-dt-lock && make
```

In QEMU, load **one** sysfs driver at a time (same DT `compatible`):

```sh
insmod /modules/platform-drv-dt-lock.ko
ls /sys/class/pcd_lock_class/pdev-0/
```

Details: [`platform-drv-dt-sysfs/README.md`](../platform-drv-dt-sysfs/README.md).
