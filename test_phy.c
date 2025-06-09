#include <linux/module.h>
#include <linux/phy.h>
#include <linux/netdevice.h>
#include <net/net_namespace.h>
#include <linux/kallsyms.h>
#include <linux/ethtool_netlink.h>

static void print_phy_dev_info(struct phy_device *phydev);
static int my_phy_start_cable_test(struct phy_device *phydev,
			 struct netlink_ext_ack *extack);

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
    } else if(phydev->drv->cable_test_start) {
        pr_err("Failed to start cable test: %d\n", ret);
        if (extack._msg)
            pr_err("Error message: %s\n", extack._msg);
    } else {
	my_phy_start_cable_test(phydev, &extack);
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


static inline void phy_led_trigger_change_speed(struct phy_device *phy) { }
static void phy_link_up(struct phy_device *phydev)
{
	phydev->phy_link_change(phydev, true);
	phy_led_trigger_change_speed(phydev);
}

static void phy_link_down(struct phy_device *phydev)
{
	phydev->phy_link_change(phydev, false);
	phy_led_trigger_change_speed(phydev);
	WRITE_ONCE(phydev->link_down_events, phydev->link_down_events + 1);
}

static int private_cable_test_start(struct phy_device *phydev)
{
	int bmcr;

	pr_info("private_cable_test_start ...\n");

	bmcr = phy_read(phydev, MII_BMCR);
	if (bmcr < 0)
		return bmcr;
	//TODO:

	return 0;
}

static int my_phy_start_cable_test(struct phy_device *phydev,
			 struct netlink_ext_ack *extack)
{
	struct net_device *dev = phydev->attached_dev;
	int err = -ENOMEM;

	mutex_lock(&phydev->lock);
	if (phydev->state == PHY_CABLETEST) {
		pr_info("PHY already performing a test");
		err = -EBUSY;
		goto out;
	}

	if (phydev->state < PHY_UP ||
	    phydev->state > PHY_CABLETEST) {
		pr_info("PHY not configured. Try setting interface up");
		err = -EBUSY;
		goto out;
	}

	err = ethnl_cable_test_alloc(phydev, ETHTOOL_MSG_CABLE_TEST_NTF);
	if (err) {
		pr_info("fail to alloc ethnl");
		goto out;
	}

	/* Mark the carrier down until the test is complete */
	phy_link_down(phydev);

	netif_testing_on(dev);
	//err = phydev->drv->cable_test_start(phydev);
	err = private_cable_test_start(phydev);
	pr_info("private_cable_test_start end (err: %d!)", err);
	if (err) {
		netif_testing_off(dev);
		phy_link_up(phydev);
		goto out_free;
	}

	//phydev->state = PHY_CABLETEST;

	if (phy_polling_mode(phydev)) {
		//TODO: trigger phy_state_machine()
		pr_info("TODO: trigger phy_state_machine()");
		//phy_trigger_machine(phydev);
	}

	mutex_unlock(&phydev->lock);

	return 0;

out_free:
	ethnl_cable_test_free(phydev);
out:
	mutex_unlock(&phydev->lock);

    	pr_info("exit err %d\n", err);
	return err;
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
