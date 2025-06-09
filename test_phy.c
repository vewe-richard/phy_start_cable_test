#include <linux/module.h>
#include <linux/phy.h>
#include <linux/netdevice.h>
#include <net/net_namespace.h>
#include <linux/kallsyms.h>

static void print_phy_dev_info(struct phy_device *phydev);
static int __init test_init(void)
{
    struct net_device *dev;
    struct phy_device *phydev;
    int ret;
    
    dev = dev_get_by_name(&init_net, "eth0");
    if (!dev) {
        pr_err("Device eth0 not found\n");
        return -ENODEV;
    }
    
    phydev = dev->phydev;
    if (!phydev) {
        pr_err("PHY device for eth0 not found\n");
        dev_put(dev);
        return -ENODEV;
    }
    
    struct netlink_ext_ack extack = {};
    
    print_phy_dev_info(phydev);

    pr_info("Attempting to start cable test on %s (PHY: %s)\n",
            dev->name, phydev->drv ? phydev->drv->name : "unknown");
    
    ret = phy_start_cable_test(phydev, &extack);
    
    if (ret == 0) {
        pr_info("Cable test started successfully.");
    } else {
        pr_err("Failed to start cable test: %d\n", ret);
        if (extack._msg)
            pr_err("Error message: %s\n", extack._msg);
    }
    
    dev_put(dev);
    return 0;
}

static void print_phy_dev_info(struct phy_device *phydev) {
    pr_info("cable_test_start: %pF\n", phydev->drv->cable_test_start);
    pr_info("probe: %pF..\n", phydev->drv->probe);
    pr_info("get_stats: %pF..\n", phydev->drv->get_stats);

    char namebuf[KSYM_NAME_LEN];
    sprint_symbol(namebuf, (unsigned long)phydev->drv->probe);
    pr_info("Function name: %s\n", namebuf);

    pr_info("-EOPNOTSUPP is %d\n", -EOPNOTSUPP);
}

static void __exit test_exit(void)
{
    pr_info("Cable test module unloaded\n");
}

module_init(test_init);
module_exit(test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Kernel module to trigger PHY cable test");
MODULE_VERSION("1.0");
