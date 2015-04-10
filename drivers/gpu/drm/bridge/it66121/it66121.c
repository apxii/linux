/*
 *  ITE IT66121 HDMI transmitter driver
 *
 *  Copyright (c) 2015, Andrey Panov <xn--41a@xn----7sbbihg2a3afkii.xn--p1ai>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/of.h>
#include <linux/of_gpio.h>

#define DRV_NAME "it66121"

struct it66121_priv {
	struct gpio_desc *reset_gpio;
	struct gpio_desc *interrupt_gpio;
};

static int it66121_probe(struct i2c_client *client, const struct i2c_device_id *i2c_id)
{
	struct device *dev = &client->dev;
	int ret = 0;
	struct it66121_priv *priv;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->reset_gpio = devm_gpiod_get(&client->dev, "it66121-reset");
	if (IS_ERR(priv->reset_gpio)) {
		ret = PTR_ERR(priv->reset_gpio);
		dev_err(dev, "%s Failed to get reset-gpio %d\n", __func__, ret);
		goto exit;
	}

	priv->interrupt_gpio = devm_gpiod_get(&client->dev, "it66121-interrupt");
	if (IS_ERR(priv->interrupt_gpio)) {
		ret = PTR_ERR(priv->interrupt_gpio);
		dev_err(dev, "%s Failed to get interrupt-gpio %d\n", __func__, ret);
		goto exit;
	}

	ret = gpiod_direction_output(priv->reset_gpio, 0);
	if (ret) {
		dev_err(dev, "%s Failed to set reset-gpio to output %d\n", __func__, ret);
		goto exit;
	}

	ret = gpiod_direction_input(priv->interrupt_gpio);
	if (ret) {
		dev_err(dev, "%s Failed to set interrupt-gpio to input %d\n", __func__, ret);
		goto exit;
	}

	/* Enable device */
	gpiod_set_value(priv->reset_gpio, 1);

	i2c_set_clientdata(client, priv);
exit:
	return ret;
}

static int it66121_remove(struct i2c_client *client)
{
	int ret = 0;
	struct it66121_priv *priv;

	priv = i2c_get_clientdata(client);

	if(priv->reset_gpio){
		gpiod_set_value(priv->reset_gpio, 0);
		devm_gpiod_put(&client->dev, priv->reset_gpio);
	}

	if(priv->interrupt_gpio)
		devm_gpiod_put(&client->dev, priv->interrupt_gpio);

	return ret;
}

static const struct of_device_id it66121_dt_ids[] = {
	{.compatible = "ite,it66121",},
	{}
};
MODULE_DEVICE_TABLE(of, it66121_dt_ids);

static const struct i2c_device_id it66121_i2c_ids[] = {
	{"ite,it66121", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, it66121_i2c_ids);

static struct i2c_driver it66121_driver = {
	.id_table = it66121_i2c_ids,
	.probe = it66121_probe,
	.remove = it66121_remove,
	.driver = {
		.name = DRV_NAME,
		.of_match_table  = it66121_dt_ids,
	},
};

module_i2c_driver(it66121_driver);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrey Panov <xn--41a@xn----7sbbihg2a3afkii.xn--p1ai>");
MODULE_DESCRIPTION("ITE IT66121 HDMI transmitter driver");
