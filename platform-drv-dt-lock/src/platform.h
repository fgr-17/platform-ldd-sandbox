#ifndef _PLATFORM_H_
#define _PLATFORM_H_

struct platform_data {
    int size;
    int perm;
    const char* serial_number;
};

struct dev_custom_config {
    int config_1;
    int config_2;
};

enum pdev_names {
    PDEV_A1X,
    PDEV_B1X
};

extern struct dev_custom_config pdev_config_table[];
extern struct of_device_id org_pdev_dt_match[];

#endif /* _PLATFORM_H_ */