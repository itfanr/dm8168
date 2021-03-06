/*
 * linux/drivers/video/ti81xx/ti81xxfb/ti81xxfb_main.c
 *
 * Framebuffer driver for TI 81XX
 *
 * Copyright (C) 2009 Texas Instruments
 * Author: Yihe Hu(yihehu@ti.com)
 * Modify: phoong(UDWorks)
 *
 * Some codes and ideals are from TI OMAP2 by Tomi Valkeinen
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA	02111-1307, USA.
 */

/*----------------------------------------------------------------------------
 Defines referenced header files
-----------------------------------------------------------------------------*/
#include <linux/module.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/err.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/ti81xxfb.h>
#include <plat/ti81xx_ram.h>

#include "fbpriv.h"

/*----------------------------------------------------------------------------
 Definitions and macro
-----------------------------------------------------------------------------*/
#define MODULE_NAME "ti81xxfb"

#define RGB_KEY		((0xFF<<16) | (0x00<<8) | (0xFF))
#define STARTX		10
#define STARTY		10

/*----------------------------------------------------------------------------
 Declares variables
-----------------------------------------------------------------------------*/
/*the followign are defined to as the module or bootargs parameters */
static int def_grpx = 2;	//# default used grpx

/*----------------------------------------------------------------------------
 Declares a function prototype
-----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 Local function
-----------------------------------------------------------------------------*/
static inline u32 ti81xxfb_get_fb_paddr(struct ti81xxfb_info *tfbi)
{
	return tfbi->mreg.paddr;
}

static inline void __iomem *ti81xxfb_get_fb_vaddr(struct ti81xxfb_info *tfbi)
{
	return tfbi->mreg.vaddr;
}

static struct ti81xxfb_datamode tfb_datamodes[] = {
	{
		.dataformat = FVID2_DF_RGB16_565,
		.nonstd = 0, /*TI81XXFB_RGB565,*/
		.red    = {.offset = 11, .length = 5, .msb_right = 0},
		.green  = {.offset = 5,  .length = 6, .msb_right = 0},
		.blue   = {.offset = 0,  .length = 5, .msb_right = 0},
		.transp = {.offset = 0,  .length = 0, .msb_right = 0},
		.bpp    = 16,
	}, {
		.dataformat = FVID2_DF_ARGB16_1555,
		.nonstd = 0,/*TI81XXFB_ARGB1555,*/
		.red    = {.offset = 10, .length = 5, .msb_right = 0},
		.green  = {.offset = 5,  .length = 5, .msb_right = 0},
		.blue   = {.offset = 0,  .length = 5, .msb_right = 0},
		.transp = {.offset = 15, .length = 1, .msb_right = 0},
		.bpp    = 16,

	}, {
		.dataformat = FVID2_DF_ARGB16_4444,
		.nonstd = 0,/*TI81XXFB_ARGB4444,*/
		.red    = {.offset = 8,  .length = 4, .msb_right = 0},
		.green  = {.offset = 4,  .length = 4, .msb_right = 0},
		.blue   = {.offset = 0,  .length = 4, .msb_right = 0},
		.transp = {.offset = 12, .length = 4, .msb_right = 0},
		.bpp    = 16,
	}, {
		.dataformat = FVID2_DF_RGBA16_5551,
		.nonstd = 0, /*TI81XXFB_RGBA5551,*/
		.red    = {.offset = 11, .length = 5, .msb_right = 0},
		.green  = {.offset = 6,  .length = 5, .msb_right = 0},
		.blue   = {.offset = 1,  .length = 5, .msb_right = 0},
		.transp = {.offset = 0,  .length = 1, .msb_right = 0},
		.bpp    = 16,

	}, {
		.dataformat = FVID2_DF_RGBA16_4444,
		.nonstd = 0, /*TI81XXFB_RGBA4444,*/
		.red    = {.offset = 12, .length = 4, .msb_right = 0},
		.green  = {.offset = 8,  .length = 4, .msb_right = 0},
		.blue   = {.offset = 4,  .length = 4, .msb_right = 0},
		.transp = {.offset = 0,  .length = 4, .msb_right = 0},
		.bpp    = 16,
	}, {

		.dataformat = FVID2_DF_ARGB24_6666,
		.nonstd = 0, /*ETRAFB_ARGB6666,*/
		.red    = {.offset = 12, .length = 6, .msb_right = 0},
		.green  = {.offset = 6,  .length = 6, .msb_right = 0},
		.blue   = {.offset = 0,  .length = 6, .msb_right = 0},
		.transp = {.offset = 18, .length = 6, .msb_right = 0},
		.bpp = 24,
	}, {
		.dataformat = FVID2_DF_RGB24_888,
		.nonstd = 0, /*TI81XXFB_RGB888,*/
		.red    = {.offset = 16, .length = 8, .msb_right = 0},
		.green  = {.offset = 8,  .length = 8, .msb_right = 0},
		.blue   = {.offset = 0,  .length = 8, .msb_right = 0},
		.transp = {.offset = 0,  .length = 0, .msb_right = 0},
		.bpp = 24,
	}, {
		.dataformat = FVID2_DF_RGB24_888,
		.nonstd = 0, /*TI81XXFB_RGB888,*/
		.red    = {.offset = 0, .length = 8, .msb_right = 0},
		.green  = {.offset = 8,  .length = 8, .msb_right = 0},
		.blue   = {.offset = 16,  .length = 8, .msb_right = 0},
		.transp = {.offset = 0,  .length = 0, .msb_right = 0},
		.bpp = 24,

	}, {
		.dataformat = FVID2_DF_ARGB32_8888,
		.nonstd = 0, /*TI81XXFB_ARGB8888,*/
		.red    = {.offset = 16, .length = 8, .msb_right = 0},
		.green  = {.offset = 8,  .length = 8, .msb_right = 0},
		.blue   = {.offset = 0,  .length = 8, .msb_right = 0},
		.transp = {.offset = 24, .length = 8, .msb_right = 0},
		.bpp    = 32,
	}, {
		.dataformat = FVID2_DF_RGBA24_6666,
		.nonstd = 0, /*TI81XXFB_RGBA6666,*/
		.red    = {.offset = 18, .length = 6, .msb_right = 0},
		.green  = {.offset = 12, .length = 6, .msb_right = 0},
		.blue   = {.offset = 6,  .length = 6, .msb_right = 0},
		.transp = {.offset = 0,  .length = 6, .msb_right = 0},
		.bpp    = 24,
	}, {
		.dataformat = FVID2_DF_RGBA32_8888,
		.nonstd =  0, /*TI81XXFB_RGBA8888,*/
		.red    = {.offset = 24, .length = 8, .msb_right = 0},
		.green  = {.offset = 16, .length = 8, .msb_right = 0},
		.blue   = {.offset = 8,  .length = 8, .msb_right = 0},
		.transp = {.offset = 0,  .length = 8, .msb_right = 0},
		.bpp    = 32,
	}, {
		.dataformat = FVID2_DF_BITMAP8,
		.nonstd = 0, /*TI81XXFB_BMP8,*/
		.bpp    = 8,
	}, {
		.dataformat = FVID2_DF_BITMAP4_LOWER,
		.nonstd = 0,/*TI81XXFB_BMP4_L.*/
		.bpp = 4,
	}, {
		.dataformat = FVID2_DF_BITMAP4_UPPER,
		.nonstd = TI81XXFB_BMP4_U,
		.bpp    = 4,
	}, {
		.dataformat = FVID2_DF_BITMAP2_OFFSET0,
		.nonstd = 0, /*TI81XXFB_BMP2_OFF0,*/
		.bpp = 2,
	}, {
		.dataformat = FVID2_DF_BITMAP2_OFFSET1,
		.nonstd = TI81XXFB_BMP2_OFF1,
		.bpp = 2,
	}, {
		.dataformat = FVID2_DF_BITMAP2_OFFSET2,
		.nonstd = TI81XXFB_BMP2_OFF2,
		.bpp = 2,
	}, {

		.dataformat = FVID2_DF_BITMAP2_OFFSET3,
		.nonstd = TI81XXFB_BMP2_OFF3,
		.bpp = 2,
	}, {
		.dataformat = FVID2_DF_BITMAP1_OFFSET0,
		.nonstd = 0, /*TI81XXFB_BMP1_OFF0,*/
		.bpp = 1,
	}, {
		.dataformat = FVID2_DF_BITMAP1_OFFSET1,
		.nonstd = TI81XXFB_BMP1_OFF1,
		.bpp = 1,
	}, {
		.dataformat = FVID2_DF_BITMAP1_OFFSET2,
		.nonstd = TI81XXFB_BMP1_OFF2,
		.bpp = 1,
	}, {
		.dataformat = FVID2_DF_BITMAP1_OFFSET3,
		.nonstd = TI81XXFB_BMP1_OFF3,
		.bpp = 1,
	}, {
		.dataformat = FVID2_DF_BITMAP1_OFFSET4,
		.nonstd = TI81XXFB_BMP1_OFF4,
		.bpp = 1,
	}, {
		.dataformat = FVID2_DF_BITMAP1_OFFSET5,
		.nonstd = TI81XXFB_BMP1_OFF5,
		.bpp = 1,
	}, {
		.dataformat = FVID2_DF_BITMAP1_OFFSET6,
		.nonstd = TI81XXFB_BMP1_OFF6,
		.bpp = 1,
	}, {
		.dataformat = FVID2_DF_BITMAP1_OFFSET6,
		.nonstd = TI81XXFB_BMP1_OFF6,
		.bpp = 1,
	}, {
		.dataformat = FVID2_DF_BITMAP1_OFFSET7,
		.nonstd = TI81XXFB_BMP1_OFF7,
		.bpp = 1,
	},
};

static bool cmp_var_to_vpssmode(struct fb_var_screeninfo *var,
		struct ti81xxfb_datamode *dmode)
{
	bool cmp_component(struct fb_bitfield *f1, struct fb_bitfield *f2)
	{
		return f1->length == f2->length &&
			f1->offset == f2->offset &&
			f1->msb_right == f2->msb_right;
	}

	if (var->bits_per_pixel == 0 ||
			var->red.length == 0 ||
			var->blue.length == 0 ||
			var->green.length == 0)
		return 0;

	return var->bits_per_pixel == dmode->bpp &&
		cmp_component(&var->red, &dmode->red) &&
		cmp_component(&var->green, &dmode->green) &&
		cmp_component(&var->blue, &dmode->blue) &&
		cmp_component(&var->transp, &dmode->transp);
}

/*get the var from the predefine table*/
static void get_tfb_datamode(struct ti81xxfb_datamode *dmode,
			struct fb_var_screeninfo *var)
{
	var->bits_per_pixel = dmode->bpp;
	var->red = dmode->red;
	var->green = dmode->green;
	var->blue = dmode->blue;
	var->transp = dmode->transp;
	var->nonstd = dmode->nonstd;
}

/*get the ti81xx vpss mode from fb var infor*/
static enum fvid2_dataformat tfb_datamode_to_vpss_datamode(
				struct fb_var_screeninfo *var)
{
	int i;
	enum fvid2_dataformat  df;
	if (var->nonstd) {
		for (i = 0; i < ARRAY_SIZE(tfb_datamodes); i++) {
			struct ti81xxfb_datamode *dmode = &tfb_datamodes[i];
			if (var->nonstd == dmode->nonstd) {
				get_tfb_datamode(dmode, var);
				return dmode->dataformat;
			}
		}
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(tfb_datamodes); i++) {
		struct ti81xxfb_datamode *dmode = &tfb_datamodes[i];
		if (cmp_var_to_vpssmode(var, dmode)) {
			get_tfb_datamode(dmode, var);
			return dmode->dataformat;
		}
	}

	/* if user do not specify var color infor,
	  use the default setting based on the bpp, which
	  may not be right*/

	switch (var->bits_per_pixel) {
	case 32:
		df = FVID2_DF_ARGB32_8888;
		break;
	case 24:
		df = FVID2_DF_RGB24_888;
		break;
	case 16:
		df = FVID2_DF_RGB16_565;
		break;
	case 8:
		df = FVID2_DF_BITMAP8;
		break;
	case 4:
		df = FVID2_DF_BITMAP4_LOWER;
		break;
	case 2:
		df = FVID2_DF_BITMAP2_OFFSET0;
		break;
	case 1:
		df = FVID2_DF_BITMAP1_OFFSET0;
		break;
	default:
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(tfb_datamodes); i++) {
		struct ti81xxfb_datamode *dmode = &tfb_datamodes[i];
		if (df == dmode->dataformat) {
			get_tfb_datamode(dmode, var);
			return df;
		}
	}
	return -EINVAL;

}

/*get fb mode from ti81xx data format*/
static int vpss_datamode_to_tfb_datamode(enum fvid2_dataformat df,
					 struct fb_var_screeninfo *var)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(tfb_datamodes); i++) {
		struct ti81xxfb_datamode *dmode = &tfb_datamodes[i];
		if (df == dmode->dataformat) {
			get_tfb_datamode(dmode, var);
			return 0;
		}
	}
	return -ENOENT;
}

/* check new var and modify it if possible */
static int check_fb_var(struct fb_info *fbi, struct fb_var_screeninfo *var)
{
	struct ti81xxfb_info		*tfbi = FB2TFB(fbi);
	struct vps_grpx_ctrl		*gctrl = (struct vps_grpx_ctrl *)tfbi->gctrl;
	struct vps_grpxregionparams	regp;
	enum   fvid2_dataformat		df;
	int		i;
	u8		scfmt;

	dprintk("--- check_fb_var\n");

	if (tfbi->mreg.size == 0)
		return 0;

	/*get the data format first*/
	df = tfb_datamode_to_vpss_datamode(var);
	if ((int)df < 0) {
		eprintk("var info has invalid data format.\n");
		return -EINVAL;
	}

	for(i=0; i<tfbi->num_use_grpx; i++)
	{
		gctrl = tfbi->gctrl[i];

		/* check whether the new size out of frame*/
		gctrl->get_regparams(gctrl, &regp);

		/*get the new output format if applicable*/
		gctrl->get_resolution(gctrl, &regp.regionwidth, &regp.regionheight, &scfmt);

		if (gctrl->check_params(gctrl, &regp, 0)) {
			dev_err(tfbi->fbdev->dev,
				"var failed params checking.\n");
			return -EINVAL;
		}

		//# phoong - add default transparency
		//regp.blendtype = TI81XXFB_BLENDING_PIXEL;
		regp.transenable = TI81XXFB_FEATURE_ENABLE;
		regp.transcolorrgb24 = RGB_KEY;

		regp.regionposx = STARTX;
		regp.regionposy = STARTY;

		if (gctrl->set_regparams(gctrl, &regp)) {
			dev_err(tfbi->fbdev->dev,
				"var failed params setting.\n");
			return -EINVAL;
		}
	}

	var->height = -1;
	var->width = -1;
	var->grayscale = 0;

	/*FIX ME timing information should got from media controller
	through FVID2 interface */
	if (gctrl->get_timing) {
		struct fvid2_modeinfo tinfo;
		gctrl->get_timing(gctrl, &tinfo);

		var->pixclock = KHZ2PICOS(tinfo.pixelclock);
		dprintk("tinfo.pixelclock %d, var->pixclock %d\n", tinfo.pixelclock, var->pixclock);
		var->left_margin = tinfo.hbackporch;
		var->right_margin = tinfo.hfrontporch;
		var->upper_margin = tinfo.vbackporch;
		var->lower_margin = tinfo.vfrontporch;
		var->hsync_len = tinfo.hsynclen;
		var->vsync_len = tinfo.vsynclen;
		if (tinfo.scanformat)
			var->vmode = FB_VMODE_NONINTERLACED;
		else
			var->vmode = FB_VMODE_INTERLACED;

	} else {
		var->pixclock = 0;
		var->left_margin = 0;
		var->right_margin = 0;
		var->upper_margin = 0;
		var->lower_margin = 0;
		var->hsync_len = 0;
		var->vsync_len = 0;

		var->sync = 0;
		/*FIX ME should vmode set by others*/
		if (tfbi->idx == 2)
			var->vmode = FB_VMODE_INTERLACED;
		else
			var->vmode = FB_VMODE_NONINTERLACED;
	}

	return 0;
}

static void set_fb_fix(struct fb_info *fbi)
{
	struct fb_fix_screeninfo *fix = &fbi->fix;
	struct fb_var_screeninfo *var = &fbi->var;
	struct ti81xxfb_info *tfbi = FB2TFB(fbi);
	struct ti81xxfb_mem_region *mreg = &tfbi->mreg;
	int bpp = var->bits_per_pixel;

	dprintk("--- set_fb_fix\n");

	/* init mem*/
	fbi->screen_base = (char __iomem *) ti81xxfb_get_fb_vaddr(tfbi);
	fix->line_length = (var->xres_virtual * bpp >> 3);
	/*pitch should be in 16 byte boundary*/
	if (fix->line_length & 0xF)
		fix->line_length += 16 - (fix->line_length & 0xF);

	fix->smem_start = ti81xxfb_get_fb_paddr(tfbi);
	fix->smem_len = mreg->size;

	fix->type = FB_TYPE_PACKED_PIXELS;

	fix->visual = (bpp > 8) ?
	   FB_VISUAL_TRUECOLOR : FB_VISUAL_PSEUDOCOLOR;

	fix->accel = FB_ACCEL_NONE;
	fix->xpanstep = 1;
	fix->ypanstep = 1;

}

static int ti81xxfb_grpx_delete(struct fb_info *fbi)
{
	int ret = 0;
	struct ti81xxfb_info *tfbi = FB2TFB(fbi);
	struct vps_grpx_ctrl *gctrl;
	int i;

	if (tfbi->open_cnt > 1) {
		tfbi->open_cnt--;
		dprintk("dummy close fb%d\n", tfbi->idx);
		return 0;
	} else if (tfbi->open_cnt == 0)
		return 0;

	dprintk("ti81xxfb_grpx_delete fb%d (%d)\n", tfbi->idx, tfbi->open_cnt);

	for(i=0; i<tfbi->num_use_grpx; i++)
	{
		gctrl = tfbi->gctrl[i];

		if (gctrl->handle) {
			ret = gctrl->stop(gctrl);
			if (ret == 0) {
				ret = gctrl->delete(gctrl);
				if (!ret)
					tfbi->open_cnt = 0;
				else
					dev_err(tfbi->fbdev->dev,
						  "failed to delete fvid2 handle.\n");

			} else
				dev_err(tfbi->fbdev->dev,
					"failed to stop.\n");
		}
	}

	return ret;
}

static int ti81xxfb_apply_changes(struct fb_info *fbi, int init)
{
	int				ret = 0;
	struct fb_var_screeninfo	*var = &fbi->var;
	struct ti81xxfb_info		*tfbi = FB2TFB(fbi);
	struct vps_grpx_ctrl		*gctrl;
	enum   fvid2_dataformat		df;
	u32				buf_addr;
	int				offset, i, w, h;
	u8				scfmt;

	dprintk("--- ti81xxfb apply_changes\n");

	if (tfbi->mreg.size == 0)
		return 0;

	/*get the fvid2 data format first*/
	df = tfb_datamode_to_vpss_datamode(var);
	if (df < 0) {
		dprintk("unsupported data format.\n");
		return -EINVAL;
	}

	offset = 0;
	for(i=0; i<tfbi->num_use_grpx; i++)
	{
		gctrl = tfbi->gctrl[i];
		gctrl->get_resolution(gctrl, &w, &h, &scfmt);

		/* make sure that the new changes are valid size*/
		ret = gctrl->set_format(gctrl, var->bits_per_pixel,
			df, fbi->fix.line_length);
		if(ret)
			return -EINVAL;

		offset += ((w * fbi->var.bits_per_pixel >> 3) + (h * fbi->fix.line_length)) * i;

		/* update the region size*/
		buf_addr = ti81xxfb_get_fb_paddr(tfbi) + offset;
		dprintk("bufer_addr 0x%x, gctrl->buffer_addr 0x%x\n", buf_addr, gctrl->buffer_addr);

		if (buf_addr != gctrl->buffer_addr)
			ret = gctrl->set_buffer(gctrl, buf_addr);
	}

	return ret;
}

static int ti81xxfb_blank(int blank, struct fb_info *fbi)
{
	int ret = 0;
	struct ti81xxfb_info *tfbi = FB2TFB(fbi);
	struct vps_grpx_ctrl *gctrl;
	int i;

	dprintk("ti81xxfb blank 0x%x\n", blank);

	ti81xxfb_lock(tfbi);
	for(i=0; i<tfbi->num_use_grpx; i++)
	{
		gctrl = tfbi->gctrl[i];

		switch (blank) {
			/*FIX ME how to unblank the system*/
		case FB_BLANK_UNBLANK:
			ret = gctrl->start(gctrl);
			break;
		case FB_BLANK_NORMAL:
		case FB_BLANK_VSYNC_SUSPEND:
		case FB_BLANK_HSYNC_SUSPEND:
		case FB_BLANK_POWERDOWN:
			ret = gctrl->stop(gctrl);
			break;
		default:
			ret = -EINVAL;
			break;
		}
	}
	ti81xxfb_unlock(tfbi);

	return ret;
}

static int ti81xxfb_pan_display(struct fb_var_screeninfo *var,
					  struct fb_info *fbi)
{
	dprintk("--- pan_display\n");

	return 0;
}

static int ti81xxfb_setcmap(struct fb_cmap *cmap, struct fb_info *fbi)
{
	dprintk("--- setcamp\n");

	return 0;
}

static int ti81xxfb_check_var(struct fb_var_screeninfo *var,
					   struct fb_info *fbi)
{
	dprintk("--- check_var\n");

	return 0;
}

static int ti81xxfb_set_var(struct fb_info *fbi)
{
	dprintk("--- set_var\n");

	return 0;
}

static int ti81xxfb_open(struct fb_info *fbi, int user)
{
	int ret = 0;
	struct ti81xxfb_info		*tfbi = FB2TFB(fbi);
	struct vps_grpx_ctrl		*gctrl;
	int i;

	if (tfbi->mreg.size == 0) {
		dev_err(tfbi->fbdev->dev,
			"please allocate memory first.\n");
		return -EINVAL;

	}
	ti81xxfb_lock(tfbi);

	if (tfbi->open_cnt != 0) {
		tfbi->open_cnt++;
		dprintk("Dummy open fb%d\n", tfbi->idx);
		goto exit;
	}

	dprintk("Opening fb%d\n", tfbi->idx);

	for(i=0; i<tfbi->num_use_grpx; i++)
	{
		gctrl = tfbi->gctrl[i];

		ret = gctrl->create(gctrl);
		if (ret) {
			dev_err(tfbi->fbdev->dev, "fvid2(%d) create failed.\n", i);
			goto exit;
		}

		gctrl->set_buffer(gctrl, gctrl->buffer_addr);

		ret = gctrl->start(gctrl);
		if (ret) {
			/*fail to start the grpx, delete it and start over*/
			dev_err(tfbi->fbdev->dev, "failed to gctrl start.\n");
			gctrl->delete(gctrl);
			goto exit;
		}
	}

	tfbi->open_cnt++;

exit:
	ti81xxfb_unlock(tfbi);
	/*FIX ME we allocate the page and tiler memory from here.
	in the ti81xx, DMM and make the isolated physical memory into
	continuous memory pace, so we do not need allocate the memory
	at the boot time, we can allocate in the open time to save the memory*/

	return ret;
}

int ti81xxfb_release(struct fb_info *fbi, int user)
{
	int ret = 0;
	struct ti81xxfb_info  *tfbi = FB2TFB(fbi);

	dprintk("ti81xxfb_release %d\n", tfbi->open_cnt);

	/*FIXME in the page and tiler memory mode, the memory deallocate will be
	done in this function.*/
	ti81xxfb_lock(tfbi);
	ret = ti81xxfb_grpx_delete(fbi);
	ti81xxfb_unlock(tfbi);

	return ret;
}

static int ti81xxfb_mmap(struct fb_info *fbi, struct vm_area_struct *vma)
{
	struct ti81xxfb_info		*tfbi = FB2TFB(fbi);
	struct ti81xxfb_device		*fbdev = tfbi->fbdev;
	struct ti81xxfb_alloc_list	*mem = NULL;
	unsigned long			offset = vma->vm_pgoff << PAGE_SHIFT;
	unsigned long			start;
	u32				len;
	bool			found = false;

	dprintk("vram end %lx start %lx, offset %lx\n",
		vma->vm_end, vma->vm_start, offset);

	if ((vma->vm_end - vma->vm_start) == 0)
		return 0;

	if (offset < fbi->fix.smem_len) {
		/* mapping framebuffer memory*/
		len = fbi->fix.smem_len - offset;
		start = ti81xxfb_get_fb_paddr(tfbi);
		offset += start;
		if (vma->vm_pgoff > (~0UL > PAGE_SHIFT))
			return -EINVAL;

		vma->vm_pgoff = offset >> PAGE_SHIFT;

	} else {
		/* mapping stenciling memory*/
		list_for_each_entry(mem, &tfbi->alloc_list, list) {
			if (offset == mem->offset) {
				found = true;
				len = mem->size;
				offset = mem->phy_addr;
				vma->vm_pgoff = offset >> PAGE_SHIFT;
				break;
			}
		}

		if (false == found)
			return -EINVAL;
	}

	len = PAGE_ALIGN(len);
	if ((vma->vm_end - vma->vm_start) > len)
		return -EINVAL;

	dprintk("user mmap regions start %lx, len %d\n",
				offset, len);
	vma->vm_flags |= VM_IO | VM_RESERVED;
	/* make buffers bufferable*/
	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
	if (io_remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff,
		vma->vm_end - vma->vm_start, vma->vm_page_prot)) {
		dev_dbg(fbdev->dev, "mmap remap_pfn_range failed.\n");
		return -ENOBUFS;
	}

	return 0;
}

static struct fb_ops ti81xxfb_ops = {
	.owner = THIS_MODULE,
	.fb_open = ti81xxfb_open,
	.fb_release = ti81xxfb_release,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
	.fb_set_par = ti81xxfb_set_var,
	.fb_check_var = ti81xxfb_check_var,
	.fb_blank = ti81xxfb_blank,
	.fb_setcmap = ti81xxfb_setcmap,
	.fb_pan_display = ti81xxfb_pan_display,
	.fb_ioctl = ti81xxfb_ioctl,
	.fb_mmap = ti81xxfb_mmap,
};

/*****************************************************************************
* @brief	ti81xxfb module probe & remove function
* @section	DESC Description
*	- desc
*****************************************************************************/

static void ti81xxfb_free_fbmem(struct fb_info *fbi)
{
	struct ti81xxfb_info		*tfbi = FB2TFB(fbi);
	struct ti81xxfb_device		*fbdev = tfbi->fbdev;
	struct ti81xxfb_mem_region	*rg = &tfbi->mreg;

	if (rg->paddr) {
		if (ti81xx_vram_free(rg->paddr,
				 (void *)rg->vaddr,
				 (size_t)rg->size))
			dev_err(fbdev->dev, "VRAM FREE failed\n");
	}

	rg->vaddr = NULL;
	rg->paddr = 0;
	rg->alloc = 0;
	rg->size = 0;
}

static int ti81xxfb_free_allfbmem(struct ti81xxfb_device *fbdev)
{
	struct fb_info *fbi = fbdev->fbs;

	ti81xxfb_free_fbmem(fbi);
	memset(&fbi->fix, 0, sizeof(fbi->fix));
	memset(&fbi->var, 0, sizeof(fbi->var));

	dprintk("free all fbmem\n");

	return 0;
}

static int ti81xxfb_alloc_fbmem(struct fb_info *fbi, unsigned long size)
{
	struct ti81xxfb_info	   *tfbi = FB2TFB(fbi);
	struct ti81xxfb_device     *fbdev = tfbi->fbdev;
	struct ti81xxfb_mem_region *rg = &tfbi->mreg;
	unsigned long	paddr;
	void			*vaddr;

	size = PAGE_ALIGN(size);
	memset(rg, 0, sizeof(*rg));

	dprintk("allocating %lu bytes for fb %d\n",
		size, tfbi->idx);

	vaddr = (void *)ti81xx_vram_alloc(TI81XXFB_MEMTYPE_SDRAM,
			(size_t)size, &paddr, NULL);

	dprintk("allocated VRAM paddr %lx, vaddr %p\n", paddr, vaddr);
	if (vaddr == NULL) {
		dev_err(fbdev->dev,
			"failed to allocate framebuffer\n");
		return -ENOMEM;
	}

	//# fb memory init - phoong : temporary
	{
		int i;
		unsigned int *p = vaddr;
		for(i=0; i<size/4; i++)
			*p++ = RGB_KEY;
	}

	rg->paddr = paddr;
	rg->vaddr = vaddr;
	rg->size = size;
	rg->alloc = 1;

	fbi->screen_size = size;
	return 0;
}

static int ti81xxfb_alloc_fbmem_display(struct fb_info *fbi, unsigned long size)
{
	struct ti81xxfb_info  *tfbi = FB2TFB(fbi);
	struct vps_grpx_ctrl *gctrl;
	u8		scfmt;

	if (!size)
	{
		u32	i, w, h, tw=0, th=0;;

		for(i=0; i<tfbi->num_use_grpx; i++)
		{
			gctrl = tfbi->gctrl[i];

			gctrl->get_resolution(gctrl, &w, &h, &scfmt);
			tw = max(tw, w);
			th += h;
		}

		size = (tw * th * TI81XXFB_BPP >> 3);
	}

	return ti81xxfb_alloc_fbmem(fbi, size);
}

static int ti81xxfb_allocate_fbs(struct ti81xxfb_device *fbdev)
{
        int i=0;
        int ret;
	unsigned long vrams[TI81XX_FB_NUM];

	memset(vrams, 0, sizeof(vrams));

	if (fbdev->dev->platform_data) {
		struct ti81xxfb_platform_data *npd;
		npd = fbdev->dev->platform_data;
		for (i = 0; i < npd->mem_desc.region_cnt; ++i) {
			if (!vrams[i])
				vrams[i] = npd->mem_desc.mreg[i].size;
		}
	}

	/* allocate memory automatically only for fb0, or if
	 * excplicitly defined with vram or plat data option */
	if (i == 0 || vrams[0] != 0) {
		ret = ti81xxfb_alloc_fbmem_display(fbdev->fbs, vrams[0]);
		if (ret)
			return ret;
	}

	#if FB_DEBUG
	{
		struct ti81xxfb_info *tfbi = FB2TFB(fbdev->fbs);
		struct ti81xxfb_mem_region *rg = &tfbi->mreg;
		dprintk("region%d phys %08x virt %p size=%lu\n",
				i, rg->paddr, rg->vaddr, rg->size);
	}
	#endif

	return 0;
}

static void fbi_framebuffer_unreg(struct ti81xxfb_device *fbdev)
{
	unregister_framebuffer(fbdev->fbs);
}

static int ti81xxfb_alloc_clut(struct ti81xxfb_device *fbdev,
			       struct fb_info *fbi)
{
	struct ti81xxfb_info *tfbi = FB2TFB(fbi);

	/*allocate CLUT */
	tfbi->vclut = dma_alloc_writecombine(fbdev->dev,
			TI81XXFB_CLUT_SIZE, &tfbi->pclut, GFP_KERNEL);

	if (NULL == tfbi->vclut) {
		dev_err(fbdev->dev, "failed to alloca pallette memory.\n");
		return -ENOMEM;
	}

	return 0;
}

static void free_clut_ram(struct ti81xxfb_device *fbdev, struct fb_info *fbi)
{
	struct ti81xxfb_info *tfbi = FB2TFB(fbi);
	if (tfbi->vclut)
		dma_free_writecombine(fbdev->dev, TI81XXFB_CLUT_SIZE,
				     tfbi->vclut, tfbi->pclut);
	tfbi->vclut = NULL;
	tfbi->pclut = 0;
}

static void ti81xxfb_fbinfo_cleanup(struct ti81xxfb_device *fbdev,
				   struct fb_info *fbi)
{
	fb_dealloc_cmap(&fbi->cmap);
}


static void ti81xxfb_fb_cleanup(struct ti81xxfb_device *fbdev)
{
	ti81xxfb_fbinfo_cleanup(fbdev, fbdev->fbs);
	free_clut_ram(fbdev, fbdev->fbs);
	framebuffer_release(fbdev->fbs);
}

static void ti81xxfb_free_all(struct ti81xxfb_device *fbdev)
{
	dprintk("free all resources.\n");

	if (fbdev == NULL)
		return;

	/* unregister frame buffer devices first*/
	fbi_framebuffer_unreg(fbdev);

	/* free the frame buffer memory */
	ti81xxfb_free_allfbmem(fbdev);
	/* free the fbi and release framebuffer*/
	ti81xxfb_fb_cleanup(fbdev);
	dev_set_drvdata(fbdev->dev, NULL);
	/* free the device*/
	kfree(fbdev);

}

int ti81xxfb_fbinfo_init(struct ti81xxfb_device *fbdev,
			struct fb_info *fbi)
{
	struct fb_var_screeninfo	*var = &fbi->var;
	struct fb_fix_screeninfo	*fix = &fbi->fix;
	struct ti81xxfb_info		*tfbi = FB2TFB(fbi);
	struct vps_grpx_ctrl		*gctrl;
	u32				w, h, tw=0, th=0;
	u8				scfmt;

	int ret = 0;
	int i;

	fbi->fbops = &ti81xxfb_ops;
	fbi->flags = FBINFO_FLAG_DEFAULT;
	strncpy(fix->id, MODULE_NAME, sizeof(fix->id));
	fbi->pseudo_palette = tfbi->pseudo_palette;

	/*default is FVID2_DF_ARGB32_8888, FVID2_DF_RGBA32_8888 */
	vpss_datamode_to_tfb_datamode( FVID2_DF_ARGB32_8888, var);

	for(i=0; i<tfbi->num_use_grpx; i++)
	{
		gctrl = tfbi->gctrl[i];

		gctrl->get_resolution(gctrl, &w, &h, &scfmt);
		tw = max(tw, w);
		th += h;
	}
	var->xres = tw;
	var->yres = th;
	var->xres_virtual = var->xres;
	var->yres_virtual = var->yres;

	dprintk("var->xres: %d, var->yres: %d\n", var->xres, var->yres);

	ret = check_fb_var(fbi, var);
	if (0 == ret) {
		set_fb_fix(fbi);

		/*allocate the cmap with trans enable*/
		ret = fb_alloc_cmap(&fbi->cmap, 256, 1);
		if (0 != ret)
			dev_err(fbdev->dev, "unable to allocate color map memory\n");
	}

	return ret;
}

static int ti81xxfb_create_framebuffers(struct ti81xxfb_device *fbdev, int grpx_num)
{
	struct fb_info *fbi;
	struct ti81xxfb_info *tfbi;
	int ret, i;

	dprintk("--- ti81xxfb create_framebuffers\n");

	if(grpx_num == 0 || grpx_num > fbdev->max_num_grpx) {
		dev_err(fbdev->dev, "use grpx num is over max\n");
		return -EINVAL;
	}

	fbi = framebuffer_alloc(sizeof(struct ti81xxfb_info), fbdev->dev);
	if (NULL == fbi) {
		dev_err(fbdev->dev,
			"unable to allocate memory for fb %d\n", 0);
		return -ENOMEM;
	}
	fbdev->fbs = fbi;
	tfbi = FB2TFB(fbi);
	tfbi->fbdev = fbdev;
	tfbi->idx = 0;
	INIT_LIST_HEAD(&tfbi->alloc_list);
	mutex_init(&tfbi->rqueue_mutex);

	tfbi->open_cnt = 0;
	dprintk("fb_infos allocated.\n");

	/* assign grpx ctrl for the fbs*/
	tfbi->num_use_grpx = 0;
	for (i = 0; i < grpx_num; i++) {
		struct vps_grpx_ctrl *gctrl;

		gctrl = vps_grpx_get_ctrl(i);
		if (gctrl == NULL) {
			dev_err(fbdev->dev, "can't get gctrl %d\n", i);
			return -EINVAL;
		}
		tfbi->gctrl[tfbi->num_use_grpx++] = gctrl;
	}

	/* allocate frame buffer memory*/
	ret = ti81xxfb_allocate_fbs(fbdev);
	if (ret) {
		dev_err(fbdev->dev, "failed to allocate fb memory.\n");
		return ret;
	}
	dprintk("fb memory allocated.\n");

	ret = ti81xxfb_alloc_clut(fbdev, fbi);
	if (ret) {
		dev_err(fbdev->dev,
			"failed to allocate clut memory.\n");
		return ret;
	}
	dprintk("clut mem allocated.\n");

	/*setup fbs*/
	ret = ti81xxfb_fbinfo_init(fbdev, fbi);
	if (ret != 0) {
		dev_err(fbdev->dev, "fbinfo init failed.\n");
		return ret;
	}
	dprintk("fb_infos initialized\n");

	ret = ti81xxfb_apply_changes(fbi, 1);
	if (ret) {
		dev_err(fbdev->dev, "failed to change mode\n");
		return ret;
	}

	ret = register_framebuffer(fbi);
	if (ret != 0) {
		dev_err(fbdev->dev, "registering framebuffer %d failed\n", i);
		return ret;
	}
	dprintk("framebuffers registered\n");

	return 0;

}

/*----------------------------------------------------------------------------
 ti81xxfb probe function
-----------------------------------------------------------------------------*/
static int ti81xxfb_probe(struct platform_device *dev)
{
	struct ti81xxfb_device  *fbdev = NULL;
	int ret = 0;

	dprintk("--- ti81xxfb probe\n");

	if (dev->num_resources != 0) {
		dev_err(&dev->dev, "probed for an unknown device\n");
		ret = -ENODEV;
		goto cleanup;
	}

	if (dev->dev.platform_data == NULL) {
		dev_err(&dev->dev, "missing platform data\n");
		ret = -ENOENT;
		goto cleanup;
	}

	fbdev = kzalloc(sizeof(struct ti81xxfb_device), GFP_KERNEL);
	if (NULL == fbdev) {
		dev_err(&dev->dev, "unable to allocate memory for device\n");
		ret = -ENOMEM;
		goto cleanup;
	}

	fbdev->dev = &dev->dev;
	platform_set_drvdata(dev, fbdev);

	fbdev->max_num_grpx = vps_grpx_get_num_grpx();
	if (fbdev->max_num_grpx == 0) {
		dev_err(&dev->dev, "no grpxs\n");
		ret = -EINVAL;
		goto cleanup;
	}

	ret = ti81xxfb_create_framebuffers(fbdev, def_grpx);
	if (0 != ret)
		goto cleanup;

	printk("[module] ti81xxfb probe done.\n");

	return 0;

cleanup:
	ti81xxfb_free_all(fbdev);

	return ret;
}

/*----------------------------------------------------------------------------
 ti81xxfb remove function
-----------------------------------------------------------------------------*/
static int ti81xxfb_remove(struct platform_device *dev)
{
	struct ti81xxfb_device *fbdev = platform_get_drvdata(dev);
	struct ti81xxfb_info *tfbi = FB2TFB(fbdev->fbs);

	/*make sure all fb has been closed*/
	if (tfbi->open_cnt) {
		tfbi->open_cnt = 1;
		ti81xxfb_grpx_delete(fbdev->fbs);
	}

	ti81xxfb_free_all(fbdev);

	printk("[module] ti81xxfb remove done.\n");

	return 0;
}

static struct platform_driver ti81xxfb_driver = {
	.probe = ti81xxfb_probe,
	.remove = ti81xxfb_remove,
	.driver = {
		.name = MODULE_NAME,
		.owner = THIS_MODULE,
	},
};

/*****************************************************************************
* @brief	ti81xxfb module init & exit function
* @section	DESC Description
*	- dvr_netra ti81xxfb module init & exit function
*****************************************************************************/
static int __init ti81xxfb_init(void)
{
	if (platform_driver_register(&ti81xxfb_driver)) {
		printk(KERN_ERR "failed to register ti81xxfb driver\n");
		return -ENODEV;
	}
	return 0;
}

static void __exit ti81xxfb_exit(void)
{
	platform_driver_unregister(&ti81xxfb_driver);
}

module_param_named(grpx, def_grpx, int, S_IRUGO);		//# phoong
MODULE_PARM_DESC(def_grpx, "fb used grpx number");

late_initcall(ti81xxfb_init);
module_exit(ti81xxfb_exit);

MODULE_DESCRIPTION("TI TI81XX framebuffer driver");
MODULE_AUTHOR("Yihe HU<yihehu@ti.com>");
MODULE_LICENSE("GPL v2");
