/*
 *  mt9v112 camera driver functions.
 *
 *  Copyright (C) 2006 - 2010 Sagemcom All rights reserved
 *
 *  File name: mt9v112.c
 *  Creation date: 06/08/2007
 *
 *  This program is free software; you can redistribute it and/or modify it under the terms of the GNU General
 *  Public License as published by the Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *  Write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA to
 *  receive a copy of the GNU General Public License.
 *  This Copyright notice should not be removed
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/clk.h>
#include <asm/arch/mxc_i2c.h>
#include "mxc_v4l2_capture.h"
#include "mt9v112.h"

//#undef pr_debug
//#define pr_debug printk

static sensor_interface *interface_param = NULL;
static mt9v112_conf mt9v112_device;
//static unsigned long master_clock;
extern void gpio_i2c_active(int i2c_num);
static void ic_select(void);
static void ifp_cpr_select(void);
static void ifp_ccr_select(void);
static int i2c_read(u8 reg, u16 *val);
static int i2c_write(u8 reg, u16 val);
static int mt9v112_attach(struct i2c_adapter *adapter);
static int mt9v112_detach(struct i2c_client *client);
static struct proc_dir_entry *proc_entry;
static void mt9v112_power_up(void);
static void mt9v112_power_down(void);

static struct i2c_driver mt9v112_i2c_driver = {
	.driver = {
	.owner = THIS_MODULE,
	.name = "MT9V112 Client",
		   },
	.attach_adapter = mt9v112_attach,
	.detach_client = mt9v112_detach,
};

static struct i2c_client mt9v112_i2c_client = {
	.name = "mt9v112 I2C dev",
	.addr = MT9V112_I2C_ADDRESS,
	.driver = &mt9v112_i2c_driver,
};


/*
 * Function definitions
 */

static u16 mt9v112_endian_swap16(u16 data)
{
	u16 temp;

	temp = data;
	temp = ((data >> 8) & 0xff) | ((data << 8) & 0xff00);

	return temp;
}

static int mt9v112_i2c_client_xfer(unsigned int addr, char *reg, int reg_len,
				   char *buf, int num, int tran_flag)
{
	struct i2c_msg msg[2];
	int ret;

	msg[0].addr = addr;
	msg[0].len = reg_len;
	msg[0].buf = reg;
	msg[0].flags = tran_flag;
	msg[0].flags &= ~I2C_M_RD;

	msg[1].addr = addr;
	msg[1].len = num;
	msg[1].buf = buf;
	msg[1].flags = tran_flag;

	if (tran_flag & MXC_I2C_FLAG_READ) {
		msg[1].flags |= I2C_M_RD;
	} else {
		msg[1].flags &= ~I2C_M_RD;
	}

	ret = i2c_transfer(mt9v112_i2c_client.adapter, msg, 2);
	if (ret >= 0)
		return 0;
	else
	{
		printk("%s - ERROR return by i2c_transfer\n",__FUNCTION__);
	}

	return ret;
}

int mt9v112_read_register(struct register_set * mt9v112_reg)
{

  u16 value;
  u8 *reg = &mt9v112_reg->reg;
  u8 *page = &mt9v112_reg->page;
  int vl_ret;

  if (*page==0){
    ic_select();
  }
  else if (*page==1){
    ifp_cpr_select();
  }
  else if (*page==2){
    ifp_ccr_select();
  }
  else {
    printk(KERN_ERR "Wrong page number of camera registers: Must be between 0-2\n");
  }

  vl_ret=i2c_read(*reg,&value);
  value = mt9v112_endian_swap16(value);
  mt9v112_reg->value = value;

  return vl_ret;
}

int mt9v112_set_register(struct register_set * mt9v112_reg)
{

  u16 value = mt9v112_reg->value;
  u8 reg = mt9v112_reg->reg;
  u8 page = mt9v112_reg->page;
  int vl_ret;

  if (page==0){
    ic_select();
  }
  else if (page==1){
    ifp_cpr_select();
  }
  else if (page==2){
    ifp_ccr_select();
  }
  else {
    printk(KERN_ERR "Wrong page number of camera registers: Must be betwwen 0-2\n");
  }

  vl_ret=i2c_write(reg,value);
  pr_debug("%s: page %d, register 0x%x Value 0x%x\n",__FUNCTION__,mt9v112_reg->page, mt9v112_reg->reg, mt9v112_reg->value );

  return vl_ret;
}

static int mt9v112_read_reg(u8 * reg, u16 * val)
{
  u16 temp;

  temp = mt9v112_i2c_client_xfer(MT9V112_I2C_ADDRESS, reg, 1,(u8 *) val, 2, MXC_I2C_FLAG_READ);

  return temp;


}


static int mt9v112_write_reg(u8 reg, u16 val)
{
	u8 temp1;
	u16 temp2;
	temp1 = reg;
	temp2 = mt9v112_endian_swap16(val); 
	//pr_debug("%s - %x val %x.\n",__FUNCTION__, reg, val);
	return mt9v112_i2c_client_xfer(MT9V112_I2C_ADDRESS, &temp1, 1,
				       (u8 *) & temp2, 2, 0);
}

static int i2c_write(u8 reg, u16 val)
{
  return mt9v112_write_reg(reg, val);
}


/*!
 * Initialize mt9v112_sensor_lib
 * Libarary for Sensor configuration through I2C
 *
 * @param       coreReg       Core Registers
 * @param       ifpReg        IFP Register
 *
 * @return status
 */
static u8 mt9v112_sensor_lib(mt9v112_coreReg * coreReg, mt9v112_IFPCPReg * ifpCPReg,mt9v112_IFPCRReg * ifpCRReg )
{
	struct register_set reg;

	pr_debug("%s\n",__FUNCTION__);

	reg.page  =  2;
	reg.reg   =  MT9V112ICR_AE_GAIN_ZONE_LIMITS;
	reg.value =  ifpCRReg->ShutterWidth;
        mt9v112_set_register(&reg);

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_H_PAN;
	reg.value =  ifpCPReg->HPan;
        mt9v112_set_register(&reg);

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_V_PAN;
	reg.value =  ifpCPReg->VPan;
        mt9v112_set_register(&reg);

        reg.page  =  1;
	reg.reg   =  MT9V112ICP_H_ZOOM;
	reg.value =  ifpCPReg->HZoom;
        mt9v112_set_register(&reg);

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_V_ZOOM;
	reg.value =  ifpCPReg->VZoom;
        mt9v112_set_register(&reg);
	
	reg.page  =  1;
	reg.reg   =  MT9V112ICP_H_SIZE_CTXTA;
	reg.value =  ifpCPReg->HSizeCtxtA;
        mt9v112_set_register(&reg);

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_V_SIZE_CTXTA;
	reg.value =  ifpCPReg->VSizeCtxtA;
        mt9v112_set_register(&reg);

	reg.page  =  0;
	reg.reg   =  MT9V112S_HOR_BLANKING_CTXTA;
	reg.value =  coreReg->horizontalBlankingCtxtA;
        mt9v112_set_register(&reg);

	reg.page  =  0;
	reg.reg   =  MT9V112S_VER_BLANKING_CTXTA;
	reg.value =  coreReg->verticalBlankingCtxtA;
        mt9v112_set_register(&reg);

	reg.page  =  0;
	reg.reg   =  MT9V112S_RESET;
	reg.value =  0x8;
        mt9v112_set_register(&reg);

	return 0;
}

/*!
 * mt9v112 sensor interface Initialization
 * @param param            sensor_interface *
 * @param width            u32
 * @param height           u32
 * @return  None
 */

static void ic_select(void)
{
  u8 reg;
  u16 data;
  reg = MT9V112_REGISTER_SELECT;
  data = mt9v112_device.coreReg->addrSpaceSel;
  mt9v112_write_reg(reg, data);
}


static void ifp_cpr_select(void)
{
  u8 reg;
  u16 data;
  reg = MT9V112_REGISTER_SELECT;
  data = mt9v112_device.ifpCPReg->addrSpaceSel;
  mt9v112_write_reg(reg, data);

}
static void ifp_ccr_select(void)
{
  u8 reg;
  u16 data;
  reg = MT9V112_REGISTER_SELECT;
  data = mt9v112_device.ifpCRReg->addrSpaceSel;
  mt9v112_write_reg(reg, data);


}



static void mt9v112_interface(sensor_interface * param, u32 width, u32 height)
{
	param->Vsync_pol = 0x0;
	param->clk_mode = 0x0;	//gated
	param->pixclk_pol = 0x0;
	param->data_width = 0x1;
	param->data_pol = 0x0;
	param->ext_vsync = 0x0;
	param->Vsync_pol = 0x0;
	param->Hsync_pol = 0x0;
	param->width = width - 1;
	param->height = height - 1;
	param->pixel_fmt = IPU_PIX_FMT_UYVY;
	param->mclk = 27000000;

	pr_debug("%s: w=%d h=%d desired clock=%d\n",__FUNCTION__,param->width,param->height,param->mclk);
}

static int i2c_read(u8 reg, u16 *val)
{
  return mt9v112_read_reg( &reg, (u16 *) val);
}


/*!
 * MT9V112 frame rate calculate
 *
 * @param frame_rate frame rate in frame/s
 * @param mclk       clock frequency in Hz
 * @param high_quality if equal 1, frame rate is variable with *frame_rate as minimum value ow frame rate is constant
 * @return  None
 */

static void mt9v112_rate_cal(int *frame_rate, int mclk, int high_quality)
{
	int num_clock_per_row;
	int max_rate = 0;

	mt9v112_device.coreReg->horizontalBlankingCtxtA = MT9V112_HORZBLANK;

	num_clock_per_row = (MT9V112_WINWIDTH + MT9V112_HORZBLANK + 8) * 2;
	max_rate = mclk / (num_clock_per_row *
			   (MT9V112_WINHEIGHT + MT9V112_VERTBLANK_MIN));

	if ((*frame_rate > max_rate) || (*frame_rate == 0)) {
		*frame_rate = max_rate;
	}

	mt9v112_device.frame_rate=*frame_rate;
        if (high_quality)
        {
	  mt9v112_device.coreReg->verticalBlankingCtxtA = MT9V112_VERTBLANK_DEFAULT;
        }
        else
        {
	  mt9v112_device.coreReg->verticalBlankingCtxtA = mclk / (*frame_rate * num_clock_per_row) - MT9V112_WINHEIGHT;
        }
 
	//set shutter width
	if (*frame_rate>20)
	{
		mt9v112_device.ifpCRReg->ShutterWidth=0x80;
	}
	else if (*frame_rate>10)
	{
		mt9v112_device.ifpCRReg->ShutterWidth=0x100;
	}
	else
	{
		mt9v112_device.ifpCRReg->ShutterWidth=0x300;
	}

	pr_debug("%s - frame rate=%d, max_rate=%d num_clock_per_row=%d HBlankA=0x%x VBlankA=0x%x mclk=%d Shutter Width=0x%x\n",__FUNCTION__,*frame_rate,max_rate,num_clock_per_row, mt9v112_device.coreReg->horizontalBlankingCtxtA,mt9v112_device.coreReg->verticalBlankingCtxtA,mclk,mt9v112_device.ifpCRReg->ShutterWidth);

} 

/*!
 * MT9V112 set format
 *
 * @param output_width output width in pixels, return achievable value
 * @param output_height output height in pixels, return achievable value
 */
void mt9v112_set_format(int *output_width,int *output_height)
{
  pr_debug("%s Enter - output_width=%d, output_height=%d\n",__FUNCTION__,*output_width,*output_height);

  if (*output_width>MT9V112_WINWIDTH_DEFAULT) *output_width=MT9V112_WINWIDTH_DEFAULT;
  if (*output_width<32) *output_width=32;
  if (*output_height>MT9V112_WINHEIGHT_DEFAULT) *output_height=MT9V112_WINHEIGHT_DEFAULT;
  if (*output_height<32) *output_height=32;

  

  // Let's first define, panning area, by default zoom=1
  // The pan area is computed for having the same form factor as the output image

  // Center pan area
  mt9v112_device.ifpCPReg->HPan = 0x4000;
  mt9v112_device.ifpCPReg->VPan = 0x4000;
  if ((*output_width)/(*output_height)>(MT9V112_WINWIDTH_DEFAULT/MT9V112_WINHEIGHT_DEFAULT))
  {
     mt9v112_device.ifpCPReg->HZoom=MT9V112_WINWIDTH_DEFAULT;
     mt9v112_device.max_pan_width=MT9V112_WINWIDTH_DEFAULT;
     mt9v112_device.ifpCPReg->VZoom=(mt9v112_device.ifpCPReg->HZoom*(*output_height))/(*output_width);
  }
  else
  {
     mt9v112_device.ifpCPReg->VZoom=MT9V112_WINHEIGHT_DEFAULT;
     mt9v112_device.max_pan_height=MT9V112_WINHEIGHT_DEFAULT;
     mt9v112_device.ifpCPReg->HZoom=(mt9v112_device.ifpCPReg->VZoom*(*output_width))/(*output_height);
  }

  mt9v112_device.ifpCPReg->HSizeCtxtA = *output_width;
  mt9v112_device.ifpCPReg->VSizeCtxtA = *output_height;

  pr_debug("%s Exit - output_width=%d, output_height=%d\n",__FUNCTION__,*output_width,*output_height);
  pr_debug("%s Exit - HZOOM=%d, VZOOM=%d\n",__FUNCTION__,mt9v112_device.ifpCPReg->HZoom,mt9v112_device.ifpCPReg->VZoom);
}

/*!
 * MT9V112 get format
 *
 * @param output_width return current output width
 * @param output_height return current output height
 */
void mt9v112_get_format (int *output_width,int *output_height)
{
  *output_width=mt9v112_device.ifpCPReg->HSizeCtxtA;
  *output_height=mt9v112_device.ifpCPReg->VSizeCtxtA;
}

/*!
 * MT9V112 set zoom
 *
 * @param width Width of zoom area in pixels, return achievable width
 * @param height Height of zoom area in pixels, return achievable height
 */
void mt9v112_set_zoom (int *width,int *height)
{
   struct register_set reg;

   pr_debug("%s Enter - width=%d, height=%d\n",__FUNCTION__,*width,*height);

   //check validity of *width
   if (*width>mt9v112_device.max_pan_width) 
   {
     *width=mt9v112_device.max_pan_width;
   }
   if (*width<mt9v112_device.ifpCPReg->HSizeCtxtA)
   {
     *width=mt9v112_device.ifpCPReg->HSizeCtxtA;
   }

   mt9v112_device.ifpCPReg->HZoom=*width;   

   //set correct form factor
   *height=((*width)*mt9v112_device.ifpCPReg->HSizeCtxtA)/mt9v112_device.ifpCPReg->VSizeCtxtA;
 
   //check validity of height for rounding issues
   if (*height>mt9v112_device.max_pan_height) 
   {
     *height=mt9v112_device.max_pan_height;
   }
   if (*height<mt9v112_device.ifpCPReg->VSizeCtxtA)
   {
     *height=mt9v112_device.ifpCPReg->VSizeCtxtA;
   }

   mt9v112_device.ifpCPReg->VZoom=*height;

   reg.page  =  1;
   reg.reg   =  MT9V112ICP_H_ZOOM;
   reg.value =  mt9v112_device.ifpCPReg->HZoom;
   mt9v112_set_register(&reg);

   reg.page  =  1;
   reg.reg   =  MT9V112ICP_V_ZOOM;
   reg.value =  mt9v112_device.ifpCPReg->VZoom;
   mt9v112_set_register(&reg);

   pr_debug("%s Exit - width=%d, height=%d\n",__FUNCTION__,*width,*height);     
}

/*!
 * MT9V112 get zoom
 *
 * @param width Return current zoom area width in pixels
 * @param height Return current zoom area height in pixels
 */
void mt9v112_get_zoom (int *width,int *height)
{
  *width=mt9v112_device.ifpCPReg->HZoom;
  *height=mt9v112_device.ifpCPReg->VZoom;
}

/*!
 * MT9V112 get zoom capabilities
 *
 * @param width_min return minimum zoom area width in pixels
 * @param height_min return minimum zoom area height in pixels
 * @param width_max return maximum zoom area width in pixels
 * @param height_max return maximum zoom area height in pixels
 */
void mt9v112_get_zoom_caps(int *width_min,int *height_min,int *width_max,int *height_max)
{
   *width_min=mt9v112_device.ifpCPReg->HSizeCtxtA;
   *height_min=mt9v112_device.ifpCPReg->VSizeCtxtA;
   *width_max=mt9v112_device.ifpCPReg->HZoom;
   *height_max=mt9v112_device.ifpCPReg->VZoom;
}	

/*!
 * MT9V112 sensor configuration
 *
 * @param frame_rate       frame rate in frame/s, return achievable frame rate
 * @param high_quality     select high quality setting
 * @param output width     width in pixels, return achievable width in pixels
 * @param output height    height in pixels, return achievable height in pixels
 * 
 * @return  sensor_interface *
 */
sensor_interface *mt9v112_config(int *frame_rate, int high_quality,int *output_width,int *output_height)
{
	pr_debug("%s - Enter\n",__FUNCTION__);

	mt9v112_set_format(output_width,output_height);
	mt9v112_interface(interface_param,*output_width,*output_height);
	set_mclk_rate(&interface_param->mclk); 
	mt9v112_rate_cal(frame_rate, interface_param->mclk,high_quality); 
	mt9v112_sensor_lib(mt9v112_device.coreReg, mt9v112_device.ifpCPReg, mt9v112_device.ifpCRReg); 

  return interface_param;
}

/*!   SAGEM NEED TO PORT THIS FUNCTION TO ITS OWN CAMERA
 * mt9v111 sensor set color configuration
 *
 * @param bright       int
 * @param saturation   int
 * @param red          int
 * @param green        int
 * @param blue         int
 * @return  None
 */
/*static void
mt9v111_set_color(int bright, int saturation, int red, int green, int blue)
{
	u8 reg;
	u16 data;

	switch (saturation) {
	case 100:
		mt9v111_device.ifpReg->awbSpeed = 0x4514;
		break;
	case 150:
		mt9v111_device.ifpReg->awbSpeed = 0x6D14;
		break;
	case 75:
		mt9v111_device.ifpReg->awbSpeed = 0x4D14;
		break;
	case 50:
		mt9v111_device.ifpReg->awbSpeed = 0x5514;
		break;
	case 37:
		mt9v111_device.ifpReg->awbSpeed = 0x5D14;
		break;
	case 25:
		mt9v111_device.ifpReg->awbSpeed = 0x6514;
		break;
	default:
		mt9v111_device.ifpReg->awbSpeed = 0x4514;
		break;
	}

	reg = MT9V111I_ADDR_SPACE_SEL;
	data = mt9v111_device.ifpReg->addrSpaceSel;
	mt9v111_write_reg(reg, data);

	// Operation Mode Control
	reg = MT9V111I_AWB_SPEED;
	data = mt9v111_device.ifpReg->awbSpeed;
	mt9v111_write_reg(reg, data);
}*/

/*! SAGEM NEED TO PORT THIS FUNCTION TO ITS OWN CAMERA
 * mt9v111 sensor get color configuration
 *
 * @param bright       int *
 * @param saturation   int *
 * @param red          int *
 * @param green        int *
 * @param blue         int *
 * @return  None
 */
/*static void
mt9v111_get_color(int *bright, int *saturation, int *red, int *green, int *blue)
{
	*saturation = (mt9v111_device.ifpReg->awbSpeed & 0x3800) >> 11;
	switch (*saturation) {
	case 0:
		*saturation = 100;
		break;
	case 1:
		*saturation = 75;
		break;
	case 2:
		*saturation = 50;
		break;
	case 3:
		*saturation = 37;
		break;
	case 4:
		*saturation = 25;
		break;
	case 5:
		*saturation = 150;
		break;
	case 6:
		*saturation = 0;
		break;
	default:
		*saturation = 0;
		break;
	}
}*/

/*!
 * mt9v112 sensor set brightness configuration
 *
 * @param bright       int
 */
static void
mt9v112_set_brightness(int *bright)
{
  struct register_set reg;

  pr_debug("%s - Enter ae precision target=0x%x bright=0x%x\n", __FUNCTION__,mt9v112_device.ifpCRReg->AEPrecisionTarget,*bright);

  if (*bright<0) *bright=0;
  if (*bright>0xff) *bright=0xff;

  mt9v112_device.ifpCRReg->AEPrecisionTarget=(mt9v112_device.ifpCRReg->AEPrecisionTarget&0xff00)|(*bright);

  reg.page  =  2;
  reg.reg   =  MT9V112ICR_AE_PRECISION_TARGET;
  reg.value =  mt9v112_device.ifpCRReg->AEPrecisionTarget;
  mt9v112_set_register(&reg);
  pr_debug("%s - Exit ae precision target=0x%x\n",__FUNCTION__,mt9v112_device.ifpCRReg->AEPrecisionTarget);  
}

/*!
 * mt9v112 sensor get brightness configuration
 *
 * @param bright       int *
 */
static void
mt9v112_get_brightness(int *bright)
{
   *bright=mt9v112_device.ifpCRReg->AEPrecisionTarget&0xff;
}

/*!
 * mt9v112 sensor get brightness configuration
 *
 * @param bright_min       int *
 * @param bright_max       int *
 */
static void
mt9v112_get_brightness_caps(int *bright_min,int *bright_max)
{
  *bright_min=0;
  *bright_max=0xff;
}

/*!
 * mt9v112 sensor set AE measurement window mode configuration
 * 
 * @param ae_mode Select Auto_Exposure mode, return achievable Auto-Exposure mode
 * @return return 0 if success, -1 if failure
 */
static int mt9v112_set_ae_mode(e_AE_mode ae_mode)
{
	struct register_set reg;

	pr_debug("%s - Enter mode %d\n",__FUNCTION__,ae_mode); 
	pr_debug("%s - Enter mode control=0x%x\n",__FUNCTION__,mt9v112_device.ifpCPReg->modeControl);     
	switch (ae_mode)
	{
	  case AE_MODE_DISABLED:
	  {
	    mt9v112_device.ifpCPReg->modeControl=(mt9v112_device.ifpCPReg->modeControl&0xFBF3);
	  }
	  break;
	  case AE_MODE_STANDARD:
	  {
	    mt9v112_device.ifpCPReg->modeControl=(mt9v112_device.ifpCPReg->modeControl&0xFBF3)|OMC_ENABLE_AE|OMC_AE_LARGE_WINDOW;
	  }
          break;
   	  case AE_MODE_BACKLIGHT_COMPENSATION:
	  {
	    mt9v112_device.ifpCPReg->modeControl=(mt9v112_device.ifpCPReg->modeControl&0xFBF3)|OMC_ENABLE_AE|OMC_AE_SMALL_WINDOW;
	  }	  
   	  case AE_MODE_AVG:
	  {
	    mt9v112_device.ifpCPReg->modeControl=(mt9v112_device.ifpCPReg->modeControl&0xFBF3)|OMC_ENABLE_AE|OMC_AE_AVG_WINDOW;
	  }
	  break;
	  default:
	     pr_debug("%s - Failed to changed AE mode",__FUNCTION__);
	     return -1;
	}

	reg.page  =  1;
        reg.reg   =  MT9V112ICP_MODE_CONTROL;
	reg.value =  mt9v112_device.ifpCPReg->modeControl;
	mt9v112_set_register(&reg);
	pr_debug("%s - Exit mode control=0x%x\n",__FUNCTION__,mt9v112_device.ifpCPReg->modeControl);

	return 0;
}

/*!
 * mt9v112 sensor get AE measurement window mode configuration
 *
 * @param ae_mode      int *
 * @return  None
 */
static void mt9v112_get_ae_mode(e_AE_mode *ae_mode)
{
	if ((mt9v112_device.ifpCPReg->modeControl&0xFBFF)==OMC_ENABLE_AE)
	{
	   switch (mt9v112_device.ifpCPReg->modeControl&0xFFF3)
	   {
	     case OMC_AE_LARGE_WINDOW: *ae_mode=AE_MODE_STANDARD;break;
	     case OMC_AE_SMALL_WINDOW: *ae_mode=AE_MODE_BACKLIGHT_COMPENSATION;break;
	     default:
		*ae_mode=AE_MODE_AVG;
           }
	}
	else
	{
	  *ae_mode=AE_MODE_DISABLED;
	}
}

/*! SAGEM NEED TO PORT THIS FUNCTION TO ITS OWN CAMERA
 * mt9v111 sensor enable/disable AE 
 *
 * @param active      int
 * @return  None
 */
/*static void mt9v111_set_ae(int active)
{
	u8 reg;
	u16 data;

	mt9v111_device.ifpReg->modeControl &= 0xfff3;
	mt9v111_device.ifpReg->modeControl |= (active & 0x01) << 14;

	reg = MT9V111I_ADDR_SPACE_SEL;
	data = mt9v111_device.ifpReg->addrSpaceSel;
	mt9v111_write_reg(reg, data);

	reg = MT9V111I_MODE_CONTROL;
	data = mt9v111_device.ifpReg->modeControl;
	mt9v111_write_reg(reg, data);
}*/

/*! SAGEM NEED TO PORT THIS FUNCTION TO ITS OWN CAMERA
 * mt9v111 sensor enable/disable auto white balance
 *
 * @param active      int
 * @return  None
 */
/*static void mt9v111_set_awb(int active)
{
	u8 reg;
	u16 data;

	mt9v111_device.ifpReg->modeControl &= 0xfff3;
	mt9v111_device.ifpReg->modeControl |= (active & 0x01) << 1;

	reg = MT9V111I_ADDR_SPACE_SEL;
	data = mt9v111_device.ifpReg->addrSpaceSel;
	mt9v111_write_reg(reg, data);

	reg = MT9V111I_MODE_CONTROL;
	data = mt9v111_device.ifpReg->modeControl;
	mt9v111_write_reg(reg, data);
}*/

/*! SAGEM NEED TO PORT THIS FUNCTION TO ITS OWN CAMERA
 * mt9v111 sensor set the flicker control 
 *
 * @param control      int
 * @return  None
 */
/*static void mt9v111_flicker_control(int control)
{
	u8 reg;
	u16 data;

	reg = MT9V111I_ADDR_SPACE_SEL;
	data = mt9v111_device.ifpReg->addrSpaceSel;
	mt9v111_write_reg(reg, data);

	switch (control) {
	case MT9V111_FLICKER_DISABLE:
		mt9v111_device.ifpReg->formatControl &= ~(0x01 << 11);

		reg = MT9V111I_FORMAT_CONTROL;
		data = mt9v111_device.ifpReg->formatControl;
		mt9v111_write_reg(reg, data);
		break;

	case MT9V111_FLICKER_MANUAL_50:
		if (!(mt9v111_device.ifpReg->formatControl & (0x01 << 11))) {
			mt9v111_device.ifpReg->formatControl |= (0x01 << 11);
			reg = MT9V111I_FORMAT_CONTROL;
			data = mt9v111_device.ifpReg->formatControl;
			mt9v111_write_reg(reg, data);
		}
		mt9v111_device.ifpReg->flickerCtrl = 0x01;
		reg = MT9V111I_FLICKER_CONTROL;
		data = mt9v111_device.ifpReg->flickerCtrl;
		mt9v111_write_reg(reg, data);
		break;

	case MT9V111_FLICKER_MANUAL_60:
		if (!(mt9v111_device.ifpReg->formatControl & (0x01 << 11))) {
			mt9v111_device.ifpReg->formatControl |= (0x01 << 11);
			reg = MT9V111I_FORMAT_CONTROL;
			data = mt9v111_device.ifpReg->formatControl;
			mt9v111_write_reg(reg, data);
		}
		mt9v111_device.ifpReg->flickerCtrl = 0x03;
		reg = MT9V111I_FLICKER_CONTROL;
		data = mt9v111_device.ifpReg->flickerCtrl;
		mt9v111_write_reg(reg, data);
		break;

	case MT9V111_FLICKER_AUTO_DETECTION:
		if (!(mt9v111_device.ifpReg->formatControl & (0x01 << 11))) {
			mt9v111_device.ifpReg->formatControl |= (0x01 << 11);
			reg = MT9V111I_FORMAT_CONTROL;
			data = mt9v111_device.ifpReg->formatControl;
			mt9v111_write_reg(reg, data);
		}
		mt9v111_device.ifpReg->flickerCtrl = 0x10;
		reg = MT9V111I_FLICKER_CONTROL;
		data = mt9v111_device.ifpReg->flickerCtrl;
		mt9v111_write_reg(reg, data);
		break;
	}
	return;

}*/

/*! SAGEM NEED TO PORT THIS FUNCTION TO ITS OWN CAMERA
 * mt9v111 Get mode&flicker control parameters 
 *
 * @return  None
 */
/*static void mt9v111_get_control_params(int *ae, int *awb, int *flicker)
{
	if ((ae != NULL) && (awb != NULL) && (flicker != NULL)) {
		*ae = (mt9v111_device.ifpReg->modeControl & 0x4000) >> 14;
		*awb = (mt9v111_device.ifpReg->modeControl & 0x02) >> 1;
		*flicker = (mt9v111_device.ifpReg->formatControl & 0x800) >> 9;
		if (*flicker) {
			*flicker = (mt9v111_device.ifpReg->flickerCtrl & 0x03);
			switch (*flicker) {
			case 1:
				*flicker = MT9V111_FLICKER_MANUAL_50;
				break;
			case 3:
				*flicker = MT9V111_FLICKER_MANUAL_60;
				break;
			default:
				*flicker = MT9V111_FLICKER_AUTO_DETECTION;
				break;
			}
		} else
			*flicker = MT9V111_FLICKER_DISABLE;
	}
	return;
}*/


/*!
 * mt9v112 Reset function
 *
 * @return  None
 */
static sensor_interface *mt9v112_reset(void)
{
   	struct register_set reg;
	sensor_interface *interface_param;
	pr_debug("%s\n",__FUNCTION__);
	
	//Intialize default settings
	mt9v112_device.coreReg->addrSpaceSel = MT9V112S_SEL;
        mt9v112_device.ifpCPReg->addrSpaceSel = MT9V112ICP_SEL_IFP;
        mt9v112_device.ifpCRReg->addrSpaceSel = MT9V112ICR_SEL_IFP;

        mt9v112_device.ifpCPReg->GlobalContextControl = 0;
	mt9v112_device.ifpCRReg->AEPrecisionTarget = (0x08<<8)|0x3B;
	mt9v112_device.ifpCPReg->modeControl=OMC_ENABLE_AE | OMC_ENABLE_DEFECT_CORR|OMC_ENABLE_LENS_SHADING_CORR| OMC_AE_AVG_WINDOW | OMC_ENABLE_AWB;
	mt9v112_device.ifpCPReg->TestPattern = 0x0;

        mt9v112_device.coreReg->windowHeight = MT9V112_WINHEIGHT;
        mt9v112_device.coreReg->windowWidth = MT9V112_WINWIDTH;
        mt9v112_device.coreReg->verticalBlankingCtxtA = MT9V112_VERTBLANK_DEFAULT; 
        mt9v112_device.coreReg->horizontalBlankingCtxtA = MT9V112_HORZBLANK_MIN; 
        mt9v112_device.coreReg->verticalBlankingCtxtA = MT9V112_VERTBLANK_DEFAULT; 
        mt9v112_device.coreReg->horizontalBlankingCtxtA = MT9V112_HORZBLANK_MIN;
	
        mt9v112_device.frame_rate=MT9V112_DEFAULT_FRAMERATE;
        mt9v112_device.output_width=MT9V112_WINWIDTH_DEFAULT;
        mt9v112_device.output_height=MT9V112_WINHEIGHT_DEFAULT;
	
        reg.page  =  2;
	reg.reg   =  MT9V112ICR_BLUE_GAIN_AWB_LIMIT;
	reg.value =  0x8860;
        mt9v112_set_register(&reg);
	
        reg.page  =  2;
	reg.reg   =  MT9V112ICR_RED_GAIN_AWB_LIMIT;
	reg.value =  0xD980;
        mt9v112_set_register(&reg);

        reg.page  =  0;
	reg.reg   =  MT9V112S_ROW_NOISE;
	reg.value =  0x0460;
        mt9v112_set_register(&reg);

        reg.page  =  2;
	reg.reg   =  MT9V112ICR_AE_SHUTT_60HZ_WIDTH_CTXA;
	reg.value =  0x01e8;
        mt9v112_set_register(&reg);

        reg.page  =  1;
	reg.reg   =  MT9V112ICP_BLACK_SUBSTRACTION;
	reg.value =  0x0460;
        mt9v112_set_register(&reg);

        reg.page  =  2;
	reg.reg   =  MT9V112ICR_WB_ZONE_VALIDITY_LIMITS;
	reg.value =  0x0080;
        mt9v112_set_register(&reg);

        reg.page  =  2;
	reg.reg   =  MT9V112ICR_AWB_ADVANCED_CTRL;
	reg.value =  0xef3a;
        mt9v112_set_register(&reg);

        //Initialize camera registers

        reg.page  =  0;
	reg.reg   =  MT9V112S_ROW_WIDTH;
	reg.value =  mt9v112_device.coreReg->windowHeight;
        mt9v112_set_register(&reg);

        reg.page  =  0;
	reg.reg   =  MT9V112S_COL_WIDTH;
	reg.value =  mt9v112_device.coreReg->windowWidth;
        mt9v112_set_register(&reg);

        reg.page  =  1;
        reg.reg   =  MT9V112ICP_MODE_CONTROL;
	reg.value =  mt9v112_device.ifpCPReg->modeControl;
	mt9v112_set_register(&reg);

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_GLOBAL_CONTEXT_CONTROL;
	reg.value =  mt9v112_device.ifpCPReg->GlobalContextControl;
	mt9v112_set_register(&reg);

        // configure format and frame rate, reset camera
	interface_param = mt9v112_config(&mt9v112_device.frame_rate, 0,&mt9v112_device.output_width,&mt9v112_device.output_height);		

	reg.page  =  2;
        reg.reg   =  MT9V112ICR_FLICKER_CONTROL;
        reg.value =  FM_ENABLE_FLICKER_60HZ;
        mt9v112_set_register(&reg);

	reg.page  =  1;
        reg.reg   =  MT9V112ICP_COLOR_SAT_CONTROL;
	reg.value =  CSC_150_100_COLOR_SAT | CSC_ATTENUATION_96;
	mt9v112_set_register(&reg);

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_TESTPATTERNGEN;
	reg.value =  mt9v112_device.ifpCPReg->TestPattern;
        mt9v112_set_register(&reg);

	reg.page  =  1;
        reg.reg   =  MT9V112ICP_APERTURE_GAIN;
	reg.value =  /*AC_AUTO_SHARPEN |*/ AC_100_100_SHARPEN;
	mt9v112_set_register(&reg);

	reg.page  =  1;
        reg.reg   =  MT9V112ICP_GAMMA_Y1Y2_CTXA;
	reg.value =  (0x19<<8)|0x10;
	mt9v112_set_register(&reg);

	reg.page  =  1;
        reg.reg   =  MT9V112ICP_GAMMA_Y3Y4_CTXA;
	reg.value =  (0x42<<8)|0x28;
	mt9v112_set_register(&reg);

	reg.page  =  1;
        reg.reg   =  MT9V112ICP_GAMMA_Y5Y6_CTXA;
	reg.value =  (0xA3<<8)|0x75;
	mt9v112_set_register(&reg);

	reg.page  =  1;
        reg.reg   =  MT9V112ICP_GAMMA_Y7Y8_CTXA;
	reg.value =  (0xDC<<8)|0xC4;
	mt9v112_set_register(&reg);

	reg.page  =  1;
        reg.reg   =  MT9V112ICP_GAMMA_Y9Y10_CTXA;
	reg.value =  (0xFF<<8)|0xEF;
	mt9v112_set_register(&reg);

	reg.page  =  1;
        reg.reg   =  MT9V112ICP_GAMMA_Y0_CTXA;
	reg.value =  0x00;
	mt9v112_set_register(&reg);

	reg.page  =  2;
        reg.reg   =  MT9V112ICR_AE_PRECISION_TARGET;
	reg.value =  mt9v112_device.ifpCRReg->AEPrecisionTarget;
	mt9v112_set_register(&reg);
	
	reg.page  =  1;
        reg.reg   =  MT9V112ICP_EFFECTS_MODE;
	reg.value =  (0x70<<8)|EF_NO_EFFECT;
	mt9v112_set_register(&reg);
	
	reg.page  =  0;
	reg.reg   =  MT9V112S_READ_MODE_CTXTB;
	reg.value =  RM_SHOW_BORDER;
	mt9v112_set_register(&reg);

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_LENS_CORR_CTRL;
	reg.value =  (0x0<<2)|0x3;
	mt9v112_set_register(&reg);

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_LENS_V_RED_KNEE_0;
	reg.value =  (0xE0<<8)|0x0B;
	mt9v112_set_register(&reg);

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_LENS_V_RED_KNEE_12;
	reg.value =  (0xE2<<8)|0xE0;
	mt9v112_set_register(&reg);

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_LENS_V_RED_KNEE_34;
	reg.value =  (0xFD<<8)|0xEE;
	mt9v112_set_register(&reg);

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_LENS_V_GREEN_KNEE_0;
	reg.value =  (0xE9<<8)|0x08;
	mt9v112_set_register(&reg);

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_LENS_V_GREEN_KNEE_12;
	reg.value =  (0xE8<<8)|0xEB;
	mt9v112_set_register(&reg);

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_LENS_V_GREEN_KNEE_34;
	reg.value =  (0xFE<<8)|0xF2;
	mt9v112_set_register(&reg);

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_LENS_V_BLUE_KNEE_0;
	reg.value =  (0xEC<<8)|0x07;
	mt9v112_set_register(&reg);

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_LENS_V_BLUE_KNEE_12;
	reg.value =  (0xEC<<8)|0xEC;
	mt9v112_set_register(&reg);

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_LENS_V_BLUE_KNEE_34;
	reg.value =  (0xFE<<8)|0xF4;
	mt9v112_set_register(&reg);

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_LENS_H_RED_KNEE_0;
	reg.value =  (0xE7<<8)|0x0E;
	mt9v112_set_register(&reg);

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_LENS_H_RED_KNEE_12;
	reg.value =  (0xE1<<8)|0xDC;
	mt9v112_set_register(&reg);

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_LENS_H_RED_KNEE_34;
	reg.value =  (0xF4<<8)|0xE4;
	mt9v112_set_register(&reg);

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_LENS_H_RED_KNEE_5;
	reg.value =  0x05;
	mt9v112_set_register(&reg);

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_LENS_H_GREEN_KNEE_0;
	reg.value =  (0xF0<<8)|0x0A;
	mt9v112_set_register(&reg);

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_LENS_H_GREEN_KNEE_12;
	reg.value =  (0xE8<<8)|0xE5;
	mt9v112_set_register(&reg);

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_LENS_H_GREEN_KNEE_34;
	reg.value =  (0xF7<<8)|0xE8;
	mt9v112_set_register(&reg);

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_LENS_H_GREEN_KNEE_5;
	reg.value =  0x05;
	mt9v112_set_register(&reg);	

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_LENS_H_BLUE_KNEE_0;
	reg.value =  (0xF2<<8)|0x09;
	mt9v112_set_register(&reg);

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_LENS_H_BLUE_KNEE_12;
	reg.value =  (0xEC<<8)|0xEC;
	mt9v112_set_register(&reg);

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_LENS_H_BLUE_KNEE_34;
	reg.value =  (0xF4<<8)|0xEC;
	mt9v112_set_register(&reg);

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_LENS_H_BLUE_KNEE_5;
	reg.value =  0x05;
	mt9v112_set_register(&reg);

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_LENS_V_RED_KNEE_65;
	reg.value =  (0x14<<8)|0x0B;
	mt9v112_set_register(&reg);
	
	reg.page  =  1;
	reg.reg   =  MT9V112ICP_LENS_V_RED_KNEE_87;
	reg.value =  (0x7F<<8)|0x19;
	mt9v112_set_register(&reg);

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_LENS_V_GREEN_KNEE_65;
	reg.value =  (0x11<<8)|0x09;
	mt9v112_set_register(&reg);

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_LENS_V_GREEN_KNEE_87;
	reg.value =  (0x7F<<8)|0x12;
	mt9v112_set_register(&reg);

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_LENS_V_BLUE_KNEE_65;
	reg.value =  (0x12<<8)|0x0B;
	mt9v112_set_register(&reg);

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_LENS_V_BLUE_KNEE_87;
	reg.value =  (0x50<<8)|0x0F;
	mt9v112_set_register(&reg);

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_LENS_H_RED_KNEE_76;
	reg.value =  (0x1C<<8)|0x10;
	mt9v112_set_register(&reg);

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_LENS_H_RED_KNEE_98;
	reg.value =  (0x1F<<8)|0x23;
	mt9v112_set_register(&reg);

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_LENS_H_RED_KNEE_10;
	reg.value =  0x0C;
	mt9v112_set_register(&reg);

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_LENS_H_GREEN_KNEE_76;
	reg.value =  (0x17<<8)|0x0D;
	mt9v112_set_register(&reg);
	
	reg.page  =  1;
	reg.reg   =  MT9V112ICP_LENS_H_GREEN_KNEE_98;
	reg.value =  (0x15<<8)|0x1B;
	mt9v112_set_register(&reg);
	
	reg.page  =  1;
	reg.reg   =  MT9V112ICP_LENS_H_GREEN_KNEE_10;
	reg.value =  0x06;
	mt9v112_set_register(&reg);

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_LENS_H_BLUE_KNEE_76;
	reg.value =  (0x16<<8)|0x0C;
	mt9v112_set_register(&reg);

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_LENS_H_BLUE_KNEE_98;
	reg.value =  (0x13<<8)|0x17;
	mt9v112_set_register(&reg);

	reg.page  =  1;
	reg.reg   =  MT9V112ICP_LENS_H_BLUE_KNEE_10;
	reg.value =  0xFC;
	mt9v112_set_register(&reg);

	return interface_param;
}

/*! SAGEM NEED TO PORT THIS FUNCTION TO ITS OWN CAMERA
 * mt9v111 get_status function
 *
 * @return  int
 */
static int mt9v112_get_status(void)
{
	int retval = 0;
	struct register_set reg;

	if (!interface_param)
	{
		printk("%s - driver has not been opened\n",__FUNCTION__);
		return -ENODEV;
	}

	reg.page  =  0;
	reg.reg   =  MT9V112S_CHIP_VERSION;
	reg.value =  0;
        retval=mt9v112_read_register(&reg);
	if (retval<0)
	{
		printk("%s - ERROR i2c\n",__FUNCTION__);
		return -1;
	}
	if (reg.value!=MT9V112_CHIP_VERSION)
	{
		printk("%s - ERROR chip version=0x%x!=%d\n",__FUNCTION__,reg.value,MT9V112_CHIP_VERSION);
		return -ENODEV;
	}
	pr_debug("%s - chip version=0x%x\n",__FUNCTION__,reg.value);

	return 0;
}

/*!
 * Set sensor to test mode, which will generate test pattern.
 * flag If eq 1, turn on test pattern, 0 turn it off.
 * @return none
 */
static void mt9v112_test_pattern(bool flag)
{
  struct register_set reg;

  if (flag)
  {
	mt9v112_device.ifpCPReg->TestPattern=0x7;
  }
  else
  {
	mt9v112_device.ifpCPReg->TestPattern=0x0;
  }

  reg.page  =  1;
  reg.reg   =  MT9V112ICP_TESTPATTERNGEN;
  reg.value =  mt9v112_device.ifpCPReg->TestPattern;
  mt9v112_set_register(&reg);
}

static void mt9v112_power_up()
{
	board_power_serdes(1);
	board_power_camera(1);
	//msleep(500);
}

static void mt9v112_power_down()
{
	board_power_serdes(0);
	board_power_camera(0);
}

struct camera_sensor camera_sensor_if = {
	//.set_color = mt9v111_set_color, //SAGEM TBD
	//.get_color = mt9v111_get_color, //SAGEM TBD
	.set_brightness = mt9v112_set_brightness,
	.get_brightness = mt9v112_get_brightness,
	.get_brightness_caps = mt9v112_get_brightness_caps,
	.set_ae_mode = mt9v112_set_ae_mode,
	.get_ae_mode = mt9v112_get_ae_mode,
	//.set_ae = mt9v111_set_ae, //SAGEM TBD	
	//.set_awb = mt9v111_set_awb, //SAGEM TBD
	//.flicker_control = mt9v111_flicker_control, //SAGEM TBD
	//.get_control_params = mt9v111_get_control_params, //SAGEM TBD
	.config = mt9v112_config,
	.reset = mt9v112_reset,
	.get_format = mt9v112_get_format,
	.set_zoom = mt9v112_set_zoom,
	.get_zoom = mt9v112_get_zoom,  
	.get_zoom_caps = mt9v112_get_zoom_caps,
	.test_pattern = mt9v112_test_pattern,
	.read_register = mt9v112_read_register,
	.set_register = mt9v112_set_register, //SAGEM TBD
        .get_status = mt9v112_get_status,
	.power_up = mt9v112_power_up,
	.power_down = mt9v112_power_down,
};

/*!
 * mt9v111 I2C detect_client function
 *
 * @param adapter            struct i2c_adapter *
 * @param address            int
 * @param kind               int
 * 
 * @return  Error code indicating success or failure
 */
static int mt9v112_detect_client(struct i2c_adapter *adapter, int address,
				 int kind)
{
	pr_debug("%s - Enter\n",__FUNCTION__);

	mt9v112_i2c_client.adapter = adapter;
	if (i2c_attach_client(&mt9v112_i2c_client)) {
		mt9v112_i2c_client.adapter = NULL;
		printk(KERN_ERR "mt9v112_attach: i2c_attach_client failed\n");
		return -1;
	}

	interface_param = (sensor_interface *)
	    kmalloc(sizeof(sensor_interface), GFP_KERNEL);
	if (!interface_param) {
		printk(KERN_ERR "mt9v112_attach: kmalloc failed \n");
		return -1;
	}

	printk("MT9V112 Detected\n");
	
	//now it is possible to power off camera and serdes until we really use the camera
	board_power_serdes(1);
	board_power_camera(1);


	return 0;
}

static unsigned short normal_i2c[] = { MT9V112_I2C_ADDRESS, I2C_CLIENT_END };

/* Magic definition of all other variables and things */
I2C_CLIENT_INSMOD;

/*!
 * mt9v112 I2C attach function
 *
 * @param adapter            struct i2c_adapter *
 * @return  Error code indicating success or failure
 */
static int mt9v112_attach(struct i2c_adapter *adap)
{
	uint32_t mclk = 27000000;
	struct clk *clk;
	int err;
	pr_debug("%s\n",__FUNCTION__);

	clk = clk_get(NULL, "csi_clk");
	clk_enable(clk);
	set_mclk_rate(&mclk);

	err = i2c_probe(adap, &addr_data, &mt9v112_detect_client);
	clk_disable(clk);
	clk_put(clk);

	return err;

}

/*!
 * mt9v112 I2C detach function
 *
 * @param client            struct i2c_client *
 * @return  Error code indicating success or failure
 */
static int mt9v112_detach(struct i2c_client *client)
{
	int err;

	pr_debug("%s - Enter\n",__FUNCTION__);

	if (!mt9v112_i2c_client.adapter)
		return -1;

	err = i2c_detach_client(&mt9v112_i2c_client);
	mt9v112_i2c_client.adapter = NULL;

	if (interface_param)
		kfree(interface_param);
	interface_param = NULL;

	return err;
}

extern void gpio_sensor_active(void);


/*----------------------------------------------------------------------------
 * read /proc/camera
 *
 * A read from the proc file will return the current register settings of the
 * camera.
 *
 *--------------------------------------------------------------------------*/
#define OUTPUT_MAX  PAGE_SIZE

static int
proc_read(char *page, char **start, off_t off, int count,
		 int *eof, void *data)
{
	int len,i,j,i0,j0;
	char *buf=page;
	struct register_set setting;
	
	pr_debug("%s - page =0x%x start=%d off=%d count=%d eof=%d data=0x%x \n",__FUNCTION__,(u32)page,(u32)*start,(u32)off,count,*eof,(u32)data);
	len = 0;
	if (count<64)
	{
		goto end_proc_read;
	}
	
	if (off==0)
	{
		len += snprintf(buf+len, OUTPUT_MAX-len, "Camera register settings\n");
		len += snprintf(buf+len, OUTPUT_MAX-len, "Page Register Value\n");
	}
	
	j0=(off>>8);
	i0=(off%256);

	for (j=j0;j<3;j++)
	{
		for (i=i0;i<256;i++)
		{
			setting.page  =  j;
			setting.reg   =  i;
			setting.value =  0;
			mt9v112_read_register(&setting);
			len += snprintf(buf+len, OUTPUT_MAX-len, 
			    	   "%1d    0x%02x     0x%04x\n", 
					   setting.page, 
					   setting.reg, 
					   setting.value);
			(*start)++;
			if ((len>=OUTPUT_MAX-32)||(len>=count-32))
			{
				goto end_proc_read;
			}
		}
		i0=0;
	}
	
	*eof=1;

	end_proc_read:
	pr_debug("%s - page=0x%x start=%d off=%d count=%d eof=%d data=0x%x \n",__FUNCTION__,(u32)page,(u32)*start,(u32)off,len,*eof,(u32)data);
	return len;
}




/*----------------------------------------------------------------------------
 * Skip white space.
 *
 * Returns a pointer to character after any white space.
 * White space includes 'space', tab and any control characters.
 *--------------------------------------------------------------------------*/
static char *
skip_white_space(char *cmd)
{
	while (*cmd <= ' ' && *cmd != 0) {
		cmd++;
	}
	return cmd;
}



/*----------------------------------------------------------------------------
 * write /proc/camera
 * 
 * You can write into a caemra register
 *    echo Page Register Value > /proc/camera
 *--------------------------------------------------------------------------*/

#define COMMAND_MAX 64

static int 
proc_write(struct file* file, const char* buffer, 
         unsigned long count, void* data)
{
	char cmd[COMMAND_MAX];
	char  *cur;
	struct register_set setting;
	int nb_arg=0;	

	if (count >= COMMAND_MAX) {
		return -E2BIG;
	}
	
	if(copy_from_user(cmd, buffer, count)) {
		return -EFAULT;
	}
	
	cur=cmd;
	cmd[count] = 0;
	cur = skip_white_space(cur);
	if (*cur == 0) goto error;
	nb_arg++;
        setting.page = simple_strtoul(cur, &cur, 0);
	pr_debug("setting page=0x%x\n",setting.page);
	cur = skip_white_space(cur);
	if (*cur == 0) goto error;
	nb_arg++;
	setting.reg  = simple_strtoul(cur, &cur, 0);
	pr_debug("setting reg=0x%x\n",setting.reg);
	cur = skip_white_space(cur);
	if (*cur == 0) goto error;
	nb_arg++;
	setting.value = simple_strtoul(cur, &cur, 0);
	pr_debug("setting value=0x%x\n",setting.value);

	mt9v112_set_register(&setting);
	printk("Write Reg: Page %d Register 0x%04x Value 0x%04x\n",setting.page,setting.reg,setting.value);	


	return cur-cmd;
error:
	if (nb_arg!=2)
	{
		printk("Usage:\n");
		printk("echo Page Register > /proc/camera : Read one register\n");
		printk("echo Page Register Value > /proc/camera : Write one register\n");		
	}
	else
	{
		setting.value =  0;
		mt9v112_read_register(&setting);
		printk("Read Reg: Page %d Register 0x%04x Value 0x%04x\n",setting.page,setting.reg,setting.value);
	}
	
	return cur-cmd;
}


/*!
 * MT9V112 init function
 *
 * @return  Error code indicating success or failure
 */
static __init int mt9v112_init(void)
{
	u8 err;

	gpio_sensor_active();

	board_power_serdes(1);
	board_power_camera(1);

	mt9v112_device.coreReg = (mt9v112_coreReg *)
	    kmalloc(sizeof(mt9v112_coreReg), GFP_KERNEL);
	if (!mt9v112_device.coreReg)
		return -1;

	memset(mt9v112_device.coreReg, 0, sizeof(mt9v112_coreReg));

	mt9v112_device.ifpCPReg = (mt9v112_IFPCPReg *)
	    kmalloc(sizeof(mt9v112_IFPCPReg), GFP_KERNEL);
	if (!mt9v112_device.ifpCPReg) {
		kfree(mt9v112_device.coreReg);
		mt9v112_device.coreReg = NULL;
		return -1;
	}

	memset(mt9v112_device.ifpCPReg, 0, sizeof(mt9v112_IFPCPReg));

        mt9v112_device.ifpCRReg = (mt9v112_IFPCRReg *)
	  kmalloc(sizeof(mt9v112_IFPCRReg), GFP_KERNEL);
        if (!mt9v112_device.ifpCRReg) {
	  kfree(mt9v112_device.coreReg);
	  mt9v112_device.coreReg = NULL;
	  kfree(mt9v112_device.ifpCPReg);
          mt9v112_device.ifpCPReg = NULL;
	  return -1;
        }

        memset(mt9v112_device.ifpCRReg, 0, sizeof(mt9v112_IFPCRReg));

	err = i2c_add_driver(&mt9v112_i2c_driver);
	if (err)
	{
		printk("%s - ERROR - Cannot register camera i2c driver\n",__FUNCTION__);
	}

	proc_entry = create_proc_entry("camera", 0644, NULL);
	if (proc_entry) {
		proc_entry->write_proc = proc_write;
		proc_entry->read_proc  = proc_read;
		proc_entry->data       = NULL;
	}

	return err;
}

extern void gpio_sensor_inactive(void);
/*!
 * MT9V112 cleanup function
 *
 * @return  Error code indicating success or failure
 */
static void __exit mt9v112_clean(void)
{
	if (proc_entry) {
		remove_proc_entry("camera", NULL);
		proc_entry = NULL;
	}

	if (mt9v112_device.coreReg) {
		kfree(mt9v112_device.coreReg);
		mt9v112_device.coreReg = NULL;
	}

	if (mt9v112_device.ifpCPReg) {
		kfree(mt9v112_device.ifpCPReg);
		mt9v112_device.ifpCRReg = NULL;
	}

        if (mt9v112_device.ifpCRReg) {
	  kfree(mt9v112_device.ifpCRReg);
	  mt9v112_device.ifpCRReg = NULL;
        }

	i2c_del_driver(&mt9v112_i2c_driver);

	gpio_sensor_inactive();

}


module_init(mt9v112_init);
module_exit(mt9v112_clean);

/* Exported symbols for modules. */
EXPORT_SYMBOL(camera_sensor_if);

MODULE_AUTHOR("Sagem");
MODULE_DESCRIPTION("Mt9v112 Camera Driver");
MODULE_LICENSE("GPL");
