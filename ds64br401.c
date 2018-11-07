// SPDX-License-Identifier: GPL-2.0
/*
 * ds64br401.c - Linux kernel module for Texas Instruments DS64BR401
 *
 * Copyright (c) 2014 Ruslan Babayev
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/of.h>
#include <linux/pm_runtime.h>

#define DS64BR401_RESET			0x00
#define DS64BR401_PWDN			0x01
#define DS64BR401_PWDN_CTL		0x02
#define DS64BR401_PINCTL		0x08

#define DS64BR401_IDLERATE		0x00
#define DS64BR401_EQ			0x01
#define DS64BR401_VOD			0x02
#define DS64BR401_DEM			0x03
#define DS64BR401_IDLETHRESH		0x04

static const int ds64br401_chan_base[] = {
	0x0e, 0x15, 0x1c, 0x23, 0x2b, 0x32, 0x39, 0x40,
};

static inline int ds64br401_chan_reg(int chan, int reg)
{
	return ds64br401_chan_base[chan] + reg;
}

static inline int ds64br401_read(struct i2c_client *client, u8 reg)
{
	return i2c_smbus_read_byte_data(client, reg);
}

static inline int ds64br401_write(struct i2c_client *client, u8 reg, u8 val)
{
	return i2c_smbus_write_byte_data(client, reg, val);
}

static int ds64br401_reset(struct i2c_client *client)
{
	int ret;

	ret = ds64br401_read(client, DS64BR401_RESET);
	if (ret < 0)
		return ret;

	ret |= 0x01;

	return ds64br401_write(client, DS64BR401_RESET, ret);
}

static int ds64br401_power(struct i2c_client *client, int on)
{
	u8 val = on ? 0 : 0xff;

	return ds64br401_write(client, DS64BR401_PWDN, val);
}

static int ds64br401_probe(struct i2c_client *client,
			   const struct i2c_device_id *id)
{
	struct device_node *np = client->dev.of_node;
	int val[8], i, reg, err;

	err = ds64br401_reset(client);
	if (err)
		return err;

	err = ds64br401_power(client, 1);
	if (err)
	        return err;

	err = ds64br401_write(client, DS64BR401_PWDN_CTL, 0x1);
	if (err)
		return err;

#define	DS64BR401_INIT(prop, _reg)					\
	for (i = 0; i < 8; i++) {					\
		err = of_property_read_u32_array(np, prop, val, 8);	\
		if (err)						\
			return err;					\
		reg = ds64br401_chan_reg(i, _reg);			\
		err = ds64br401_write(client, reg, val[i]);		\
		if (err)						\
			return err;					\
	}

	DS64BR401_INIT("idle-threshold", DS64BR401_IDLETHRESH);
	DS64BR401_INIT("idle-rate", DS64BR401_IDLERATE);
	DS64BR401_INIT("eq", DS64BR401_EQ);
	DS64BR401_INIT("vod", DS64BR401_VOD);
	DS64BR401_INIT("dem", DS64BR401_DEM);
#undef	DS64BR401_INIT

	pm_runtime_enable(&client->dev);

	return 0;
}

static int ds64br401_remove(struct i2c_client *client)
{
	pm_runtime_disable(&client->dev);

	return 0;
}

static struct i2c_device_id ds64br401_id[] = {
	{ "ds64br401", 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, ds64br401_id);

#ifdef CONFIG_PM
static int ds64br401_runtime_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);

	return ds64br401_power(client, 0);
}

static int ds64br401_runtime_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);

	return ds64br401_power(client, 1);
}

static const struct dev_pm_ops ds64br401_pm_ops = {
	.runtime_suspend	= ds64br401_runtime_suspend,
	.runtime_resume		= ds64br401_runtime_resume,
};
#define DS64BR401_PM_OPS	(&ds64br401_pm_ops)
#else  /* CONFIG_PM */
#define DS64BR401_PM_OPS	NULL
#endif /* CONFIG_PM */

static struct i2c_driver ds64br401_driver = {
	.driver = {
		.name	= "ds64br401",
		.owner	= THIS_MODULE,
		.pm	= DS64BR401_PM_OPS,
	},
	.probe		= ds64br401_probe,
	.remove		= ds64br401_remove,
	.id_table	= ds64br401_id,
};

module_i2c_driver(ds64br401_driver);

MODULE_AUTHOR("Ruslan Babayev <ruslan@babayev.com>");
MODULE_DESCRIPTION("Texas Instruments DS64BR401 signal repeater");
MODULE_LICENSE("GPL");
