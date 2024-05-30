/*
 *	hdq_bq27000.c - hdq bq27000 driver
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/delay.h>


#include "hdq.h"
#include "hdq_io.h"
#include "hdq_int.h"
#include "hdq_family.h"
#include "hdq_bq27000.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Laurent Isenegger");
MODULE_DESCRIPTION("hdq bq27000 driver");

static u8 hdq_bq27000_read_register(struct hdq_master *dev, int bq27000_register)
{
  u8 final_value = 0;
  
  hdq_bq27000_send_break(dev);

  hdq_bq27000_send_read(dev,bq27000_register);      
  // Add delay of 150 so that timing requirements between MX27 and battery are matched
  hdq_delay(150);
  final_value = hdq_read_8(dev);
  return final_value;
}


static ssize_t hdq_bq27000_read_generic(struct device *dev, char *buf, int reg)
{
  struct hdq_slave *sl = dev_to_hdq_slave(dev);
  u8 read_value_h1=0, read_value_h0=0,read_value_l1=0,read_value_l0=0; 
  ssize_t count;
  
  // 2 different cases: first is used to read 8 bits registers / second for 16 bits registers

  if ( (reg==DEVICE_CONTROL_REGISTER) || (reg==DEVICE_MODE_REGISTER) || (reg==STATUS_FLAGS_REGISTER) || (reg==RELATIVE_STATE_OF_CHARGE_REGISTER) || (reg==COMPENSATED_STATE_OF_CHARGE_REGISTER) || (reg==INITIAL_LAST_MEASURED_DISCHARGE_REGISTER) || (reg==SCALED_EDVF_THRESHOLD_REGISTER) || (reg==SCALED_EDV1_THRESHOLD_REGISTER) || (reg==INITIAL_STANDBY_LOAD_CURRENT_REGISTER) || (reg==DIGITAL_MAGNITUDE_FILTER_REGISTER) || (reg==AGING_ESTIMATE_ENABLE_REGISTER) || (reg==PACK_CONFIGURATION_REGISTER) || (reg==INITIAL_MAX_LOAD_CURRENT_REGISTER) || (reg==SAGEM_1_REGISTER) || (reg==SAGEM_2_REGISTER)) 
    {
      u8 final_value;
      atomic_inc(&sl->refcnt);
      if (down_interruptible(&sl->master->mutex)) {
	count = 0;
	printk("can't get mutex 1\n");
	goto out_dec;
      }
      final_value = hdq_bq27000_read_register(sl->master,reg);

      count = sprintf(buf, "0x%x\n", final_value);
      
    }
  else
    {
      u16 final_value;
      atomic_inc(&sl->refcnt);
      if (down_interruptible(&sl->master->mutex)) {
	count = 0;
	printk("can't get mutex 2\n");
	goto out_dec;
      }

      // First read the high byte of the 16 bits to read
      read_value_h0 = hdq_bq27000_read_register(sl->master,reg+1);

      // Then read the low byte of the 16 bits to read
      read_value_l0 = hdq_bq27000_read_register(sl->master,reg);

      // Then re-read the high byte of the 16 bits to read to check if this byte has changed when we were reading the low byte
      read_value_h1 = hdq_bq27000_read_register(sl->master,reg+1);

      if (read_value_h1 == read_value_h0)
	{
	  final_value = (read_value_h0<<8) | read_value_l0;
	}
      // if the two high bytes read are different re-read the low byte
      else 
	{
	  read_value_l1 = hdq_bq27000_read_register(sl->master,reg);
	  final_value = (read_value_h1<<8) | read_value_l1;
	}

      count = sprintf(buf, "0x%x\n", final_value);

    }
  up(&sl->master->mutex);  
 out_dec:
  atomic_dec(&sl->refcnt);
  return count;
}


#define HDQ_BQ27000_ATTR_RO(_name, _mode)				\
        ssize_t hdq_bq27000_attribute_show_##_name (struct device *dev, char *buf)\
	{ \
	  return hdq_bq27000_read_generic(dev, buf, _name##_REGISTER);	\
	} \
	struct device_attribute hdq_bq27000_attribute_##_name =	\
		__ATTR(hdq_bq27000_##_name, _mode,		\
		       hdq_bq27000_attribute_show_##_name, NULL)


static HDQ_BQ27000_ATTR_RO(DEVICE_CONTROL, S_IRUGO);
static HDQ_BQ27000_ATTR_RO(DEVICE_MODE, S_IRUGO);
static HDQ_BQ27000_ATTR_RO(AT_RATE, S_IRUGO);
static HDQ_BQ27000_ATTR_RO(AT_RATE_TIME_TO_EMPTY, S_IRUGO);
static HDQ_BQ27000_ATTR_RO(REPORTED_TEMPERATURE, S_IRUGO);
static HDQ_BQ27000_ATTR_RO(REPORTED_VOLTAGE, S_IRUGO);
static HDQ_BQ27000_ATTR_RO(STATUS_FLAGS, S_IRUGO);
static HDQ_BQ27000_ATTR_RO(RELATIVE_STATE_OF_CHARGE, S_IRUGO);
static HDQ_BQ27000_ATTR_RO(NOMINAL_AVAILABLE_CAPACITY, S_IRUGO);
static HDQ_BQ27000_ATTR_RO(DISCHARGE_COMPENSATED_NAC, S_IRUGO);
static HDQ_BQ27000_ATTR_RO(TEMPERATURE_COMPENSATED, S_IRUGO);
static HDQ_BQ27000_ATTR_RO(LAST_MEASURED_DISCHARGE, S_IRUGO);
static HDQ_BQ27000_ATTR_RO(AVERAGE_CURRENT, S_IRUGO);
static HDQ_BQ27000_ATTR_RO(TIME_TO_EMPTY, S_IRUGO);
static HDQ_BQ27000_ATTR_RO(TIME_TO_FULL, S_IRUGO);
static HDQ_BQ27000_ATTR_RO(STANDBY_CURRENT, S_IRUGO);
static HDQ_BQ27000_ATTR_RO(STANDBY_TIME_TO_EMPTY, S_IRUGO);
static HDQ_BQ27000_ATTR_RO(MAX_LOAD_CURRENT, S_IRUGO);
static HDQ_BQ27000_ATTR_RO(MAX_LOAD_TIME_TO_EMPTY, S_IRUGO);
static HDQ_BQ27000_ATTR_RO(AVAILABLE_ENERGY, S_IRUGO);
static HDQ_BQ27000_ATTR_RO(AVERAGE_POWER, S_IRUGO);
static HDQ_BQ27000_ATTR_RO(TIME_TO_EMPTY_AT_CONSTANT_POWER, S_IRUGO);
static HDQ_BQ27000_ATTR_RO(CYCLE_COUNT_SINCE_LEARNING_CYCLE, S_IRUGO);
static HDQ_BQ27000_ATTR_RO(CYCLE_COUNT_TOTAL, S_IRUGO);
static HDQ_BQ27000_ATTR_RO(COMPENSATED_STATE_OF_CHARGE, S_IRUGO);
static HDQ_BQ27000_ATTR_RO(PACK_CONFIGURATION, S_IRUGO);
static HDQ_BQ27000_ATTR_RO(INITIAL_LAST_MEASURED_DISCHARGE, S_IRUGO);
static HDQ_BQ27000_ATTR_RO(SCALED_EDVF_THRESHOLD, S_IRUGO);
static HDQ_BQ27000_ATTR_RO(SCALED_EDV1_THRESHOLD, S_IRUGO);
static HDQ_BQ27000_ATTR_RO(INITIAL_STANDBY_LOAD_CURRENT, S_IRUGO);
static HDQ_BQ27000_ATTR_RO(DIGITAL_MAGNITUDE_FILTER, S_IRUGO);
static HDQ_BQ27000_ATTR_RO(AGING_ESTIMATE_ENABLE, S_IRUGO);
static HDQ_BQ27000_ATTR_RO(INITIAL_MAX_LOAD_CURRENT, S_IRUGO);
static HDQ_BQ27000_ATTR_RO(SAGEM_1, S_IRUGO);
static HDQ_BQ27000_ATTR_RO(SAGEM_2, S_IRUGO);

static struct attribute *hdq_bq27000_default_attrs[] = {
	&hdq_bq27000_attribute_DEVICE_CONTROL.attr,
	&hdq_bq27000_attribute_DEVICE_MODE.attr,
	&hdq_bq27000_attribute_AT_RATE.attr,
	&hdq_bq27000_attribute_AT_RATE_TIME_TO_EMPTY.attr,
	&hdq_bq27000_attribute_REPORTED_TEMPERATURE.attr,
	&hdq_bq27000_attribute_REPORTED_VOLTAGE.attr,
	&hdq_bq27000_attribute_STATUS_FLAGS.attr,
	&hdq_bq27000_attribute_RELATIVE_STATE_OF_CHARGE.attr,
	&hdq_bq27000_attribute_NOMINAL_AVAILABLE_CAPACITY.attr,
	&hdq_bq27000_attribute_DISCHARGE_COMPENSATED_NAC.attr,
	&hdq_bq27000_attribute_TEMPERATURE_COMPENSATED.attr,
	&hdq_bq27000_attribute_LAST_MEASURED_DISCHARGE.attr,
	&hdq_bq27000_attribute_AVERAGE_CURRENT.attr,
	&hdq_bq27000_attribute_TIME_TO_EMPTY.attr,
	&hdq_bq27000_attribute_TIME_TO_FULL.attr,
	&hdq_bq27000_attribute_STANDBY_CURRENT.attr,
	&hdq_bq27000_attribute_STANDBY_TIME_TO_EMPTY.attr,
	&hdq_bq27000_attribute_MAX_LOAD_CURRENT.attr,
	&hdq_bq27000_attribute_MAX_LOAD_TIME_TO_EMPTY.attr,
	&hdq_bq27000_attribute_AVAILABLE_ENERGY.attr,
	&hdq_bq27000_attribute_AVERAGE_POWER.attr,
	&hdq_bq27000_attribute_TIME_TO_EMPTY_AT_CONSTANT_POWER.attr,
	&hdq_bq27000_attribute_CYCLE_COUNT_SINCE_LEARNING_CYCLE.attr,
	&hdq_bq27000_attribute_CYCLE_COUNT_TOTAL.attr,
	&hdq_bq27000_attribute_COMPENSATED_STATE_OF_CHARGE.attr,
	&hdq_bq27000_attribute_PACK_CONFIGURATION.attr,
	&hdq_bq27000_attribute_INITIAL_LAST_MEASURED_DISCHARGE.attr,
	&hdq_bq27000_attribute_SCALED_EDVF_THRESHOLD.attr,
	&hdq_bq27000_attribute_SCALED_EDV1_THRESHOLD.attr,
	&hdq_bq27000_attribute_INITIAL_STANDBY_LOAD_CURRENT.attr,
	&hdq_bq27000_attribute_DIGITAL_MAGNITUDE_FILTER.attr,
	&hdq_bq27000_attribute_AGING_ESTIMATE_ENABLE.attr,
	&hdq_bq27000_attribute_INITIAL_MAX_LOAD_CURRENT.attr,
	&hdq_bq27000_attribute_SAGEM_1.attr,
	&hdq_bq27000_attribute_SAGEM_2.attr,
	NULL
};

static struct attribute_group hdq_bq27000_defattr_group = {
	.attrs = hdq_bq27000_default_attrs,
};

static int hdq_bq27000_add_slave(struct hdq_slave *sl)
{
	int err=0;
	err = sysfs_create_group(&sl->dev.kobj, &hdq_bq27000_defattr_group);
	return err;
}

static void hdq_bq27000_remove_slave(struct hdq_slave *sl)
{
  sysfs_remove_group(&sl->dev.kobj, &hdq_bq27000_defattr_group);
}

static struct hdq_family_ops hdq_bq27000_fops = {
  .add_slave = hdq_bq27000_add_slave,
  .remove_slave = hdq_bq27000_remove_slave,
};

static struct hdq_family hdq_family_bq27000 = {
  .fid = HDQ_BATTERY_BQ27000,
  .fops = &hdq_bq27000_fops,
};


static int __init hdq_bq27000_init(void)
{
  return hdq_register_family(&hdq_family_bq27000);
}


static void __exit hdq_bq27000_fini(void)
{
  hdq_unregister_family(&hdq_family_bq27000);
}

module_init(hdq_bq27000_init);
module_exit(hdq_bq27000_fini);
