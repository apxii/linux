#include "rk_hdmi.h"

#ifdef CONFIG_HDMI_LCDC_EDID_DEBUG
#define hdmi_lcdc_debug(fmt, ...) \
        printk(KERN_INFO pr_fmt(fmt), ##__VA_ARGS__)
#else
#define hdmi_lcdc_debug(fmt, ...)
#endif

#define LCD_ACLK		800000000

static const struct hdmi_video_timing hdmi_mode [] = {
		//name				refresh		xres	yres	pixclock	h_bp	h_fp	v_bp	v_fp	h_pw	v_pw					polariry								PorI	flag	vic	pixelrepeat	interface
	{ {	"720x480p@60Hz",	60,			720,	480,	27000000,	60,		16,		30,		9,		62,		6,							0,									0,		0	},	2,  	1,		OUT_P888	},
	{ {	"720x576p@50Hz",	50,			720,	576,	27000000,	68,		12,		39,		5,		64,		5,							0,									0,		0	},	17,  	1,		OUT_P888	},
	{ {	"1280x720p@50Hz",	50,			1280,	720,	74250000,	220,	440,	20,		5,		40,		5,		FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,			0,		0	},	19,  	1,		OUT_P888	},
	{ {	"1280x720p@60Hz",	60,			1280,	720,	74250000,	220,	110,	20,		5,		40,		5,		FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,			0,		0	},	4,  	1,		OUT_P888	},
	{ {	"1920x1080p@50Hz",	50,			1920,	1080,	148500000,	148,	528,	36,		4,		44,		5,		FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,			0,		0	},	31,  	1,		OUT_P888	},
	{ {	"1920x1080p@60Hz",	60,			1920,	1080,	148500000,	148,	88,		36,		4,		44,		5,		FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,			0,		0	},	16,  	1,		OUT_P888	},		
};

static int hdmi_set_info(struct rk29fb_screen *screen, unsigned int vic)
{
    int i;
    struct fb_videomode *mode;
    if(screen == NULL)
    	return HDMI_ERROR_FALSE;
    
    if(vic == 0)
    	vic = HDMI_VIDEO_DEFAULT_MODE;
    	
    for(i = 0; i < ARRAY_SIZE(hdmi_mode); i++)
    {
    	if(hdmi_mode[i].vic == vic)
    		break;
    }
    if(i == ARRAY_SIZE(hdmi_mode))
    	return HDMI_ERROR_FALSE;
    
    memset(screen, 0, sizeof(struct rk29fb_screen));
    
    /* screen type & face */
    screen->type = SCREEN_HDMI;
    screen->face = hdmi_mode[i].interface;

    mode = (struct fb_videomode *)&(hdmi_mode[i].mode);
    
    /* Screen size */
    screen->x_res = mode->xres;
    screen->y_res = mode->yres;
    
    /* Timing */
    screen->pixclock = mode->pixclock;
	screen->lcdc_aclk = LCD_ACLK;
	screen->left_margin = mode->left_margin;
	screen->right_margin = mode->right_margin;
	screen->hsync_len = mode->hsync_len;
	screen->upper_margin = mode->upper_margin;
	screen->lower_margin = mode->lower_margin;
	screen->vsync_len = mode->vsync_len;

	/* Pin polarity */
	if(FB_SYNC_HOR_HIGH_ACT & mode->sync)
		screen->pin_hsync = 1;
	else
		screen->pin_hsync = 0;
	if(FB_SYNC_VERT_HIGH_ACT & mode->sync)
		screen->pin_vsync = 1;
	else
		screen->pin_vsync = 0;
	screen->pin_den = 0;
	screen->pin_dclk = 1;

	/* Swap rule */
    screen->swap_rb = 0;
    screen->swap_rg = 0;
    screen->swap_gb = 0;
    screen->swap_delta = 0;
    screen->swap_dumy = 0;

    /* Operation function*/
    screen->init = NULL;
    screen->standby = NULL;
    
    return 0;
}

/**
 * hdmi_find_best_mode: find the video mode nearest to input vic
 * @hdmi: 
 * @vic: input vic
 * 
 * NOTES:
 * If vic is zero, return the high resolution video mode vic.
 */
static int hdmi_find_best_mode(struct hdmi* hdmi, int vic)
{
	struct list_head *pos, *head = &hdmi->edid.modelist;
	struct display_modelist *modelist = NULL;
	int found = 0;
	
	if(vic)
	{
		list_for_each(pos, head) {
			modelist = list_entry(pos, struct display_modelist, list);
			if(modelist->vic == vic)
			{
				found = 1;	
				break;
			}
		}
	}
	if( (vic == 0 || found == 0) && head->next != head)	{
		modelist = list_entry(head->next, struct display_modelist, list);
	}
		
	if(modelist != NULL)
		return modelist->vic;
	else
		return 0;
}
/**
 * hdmi_set_lcdc: switch lcdc mode to required video mode
 * @hdmi: 
 * 
 * NOTES:
 * 
 */
int hdmi_set_lcdc(struct hdmi *hdmi)
{
	int rc = 0;
	struct rk29fb_screen screen;
//	printk("%s vic is %d\n", __FUNCTION__, hdmi->vic);
	if(hdmi->autoset)
		hdmi->vic = hdmi_find_best_mode(hdmi, 0);
	else
	hdmi->vic = hdmi_find_best_mode(hdmi, hdmi->vic);
//	printk("%s selected vic is %d\n", __FUNCTION__, hdmi->vic);
	if(hdmi->vic == 0)
		hdmi->vic = HDMI_VIDEO_DEFAULT_MODE;

	hdmi_lcdc_debug( "[HDMI-LCDC] Used CEA-Mode (hdmi->vic) = %d \n", hdmi->vic);

	rc = hdmi_set_info(&screen, hdmi->vic);

	if(rc == 0) {		
		rk_fb_switch_screen(&screen, 1, hdmi->lcdc->id);
	}
	return rc;
}

/**
 * hdmi_init_lcdc: initial lcdc panel timing
 * @screen: 
 * @lcd_info:
 * 
 * NOTES:
 * 
 */
void hdmi_init_lcdc(struct rk29fb_screen *screen, struct rk29lcd_info *lcd_info)
{
	hdmi_set_info(screen, HDMI_VIDEO_DEFAULT_MODE);
}

/**
 * hdmi_videomode_compare - compare 2 videomodes
 * @mode1: first videomode
 * @mode2: second videomode
 *
 * RETURNS:
 * 1 if mode1 > mode2, 0 if mode1 = mode2, -1 mode1 < mode2
 */
static int hdmi_videomode_compare(const struct fb_videomode *mode1,
		     const struct fb_videomode *mode2)
{
	if(mode1->xres > mode2->xres)
		return 1;
	else if(mode1->xres == mode2->xres)
	{ 
		if(mode1->yres > mode2->yres)
			return 1;
		else if(mode1->yres == mode2->yres)
		{
			if(mode1->pixclock > mode2->pixclock)	
				return 1;
			else if(mode1->pixclock == mode2->pixclock)
			{	
				if(mode1->refresh > mode2->refresh)
					return 1;
				else if(mode1->refresh == mode2->refresh) 
					return 0;
			}
		}
	}
	return -1;		
}

int hdmi_add_vic(int vic, struct list_head *head)
{
	struct list_head *pos;
	struct display_modelist *modelist;
	int found = 0, v;

//	DBG("%s vic %d", __FUNCTION__, vic);
	if(vic == 0)
		return -1;
		
	list_for_each(pos, head) {
		modelist = list_entry(pos, struct display_modelist, list);
		v = modelist->vic;
		if (v == vic) {
			found = 1;
			break;
		}
	}
	if (!found) {
		modelist = kmalloc(sizeof(struct display_modelist),
						  GFP_KERNEL);

		if (!modelist)
			return -ENOMEM;
		memset(modelist, 0, sizeof(struct display_modelist));
		modelist->vic = vic;
		list_add_tail(&modelist->list, head);
	}
	return 0;
}

/**
 * hdmi_add_videomode: adds videomode entry to modelist
 * @mode: videomode to add
 * @head: struct list_head of modelist
 *
 * NOTES:
 * Will only add unmatched mode entries
 */
static int hdmi_add_videomode(const struct fb_videomode *mode, struct list_head *head)
{
	struct list_head *pos;
	struct display_modelist *modelist, *modelist_new;
	struct fb_videomode *m;
	int i, found = 0;

	for(i = 0; i < ARRAY_SIZE(hdmi_mode); i++)
    {
    	m =(struct fb_videomode*) &(hdmi_mode[i].mode);
    	if (fb_mode_is_equal(m, mode)) {
			found = 1;
			break;
		}
    }

	if (found) {
		list_for_each(pos, head) {
			modelist = list_entry(pos, struct display_modelist, list);
			m = &modelist->mode;
			if (fb_mode_is_equal(m, mode)) {
			// m == mode	
				return 0;
			}
			else
			{ 
				if(hdmi_videomode_compare(m, mode) == -1) {
					break;
				}
			}
		}

		modelist_new = kmalloc(sizeof(struct display_modelist),
				  GFP_KERNEL);					
		if (!modelist_new)
			return -ENOMEM;	
		modelist_new->mode = hdmi_mode[i].mode;
		modelist_new->vic = hdmi_mode[i].vic;
		list_add_tail(&modelist_new->list, pos);
	}
	
	return 0;
}

#ifdef CONFIG_HDMI_LCDC_EDID_DEBUG
static void __show_info_modelist_modes(struct list_head *head)
{
	//*head = &hdmi->edid.modelist

	struct list_head *pos;
	struct display_modelist *modelist_entry;
	struct fb_videomode *m;
		
	list_for_each(pos, head) {
		modelist_entry = list_entry(pos, struct display_modelist, list);
		m = &modelist_entry->mode;
		printk( "	%s, CEA: %d.\n", m->name, modelist_entry->vic);
	}
}

static void hdmi_show_sink_info(struct hdmi *hdmi)
{
	int i;
	struct hdmi_audio *audio;

	printk( "****************************************\n");
	printk( "Show Sink Info (HDMI-LCDC)         start\n");
	printk( "****************************************\n");
	printk( "Support video mode: \n");
	__show_info_modelist_modes(&hdmi->edid.modelist);
	
	for(i = 0; i < hdmi->edid.audio_num; i++)
	{
		audio = &(hdmi->edid.audio[i]);
		switch(audio->type)
		{
			case HDMI_AUDIO_LPCM:
				printk( "Support audio type: LPCM\n");
				break;
			case HDMI_AUDIO_AC3:
				printk( "Support audio type: AC3\n");
				break;
			case HDMI_AUDIO_MPEG1:
				printk( "Support audio type: MPEG1\n");
				break;
			case HDMI_AUDIO_MP3:
				printk( "Support audio type: MP3\n");
				break;
			case HDMI_AUDIO_MPEG2:
				printk( "Support audio type: MPEG2\n");
				break;
			case HDMI_AUDIO_AAC_LC:
				printk( "Support audio type: AAC\n");
				break;
			case HDMI_AUDIO_DTS:
				printk( "Support audio type: DTS\n");
				break;
			case HDMI_AUDIO_ATARC:
				printk( "Support audio type: ATARC\n");
				break;
			case HDMI_AUDIO_DSD:
				printk( "Support audio type: DSD\n");
				break;
			case HDMI_AUDIO_E_AC3:
				printk( "Support audio type: E-AC3\n");
				break;
			case HDMI_AUDIO_DTS_HD:
				printk( "Support audio type: DTS-HD\n");
				break;
			case HDMI_AUDIO_MLP:
				printk( "Support audio type: MLP\n");
				break;
			case HDMI_AUDIO_DST:
				printk( "Support audio type: DST\n");
				break;
			case HDMI_AUDIO_WMA_PRO:
				printk( "Support audio type: WMP-PRO\n");
				break;
			default:
				printk( "Support audio type: Unkown\n");
				break;
		}
		
		printk( "Support audio sample rate: \n");
		if(audio->rate & HDMI_AUDIO_FS_32000)
			printk( "	32000\n");
		if(audio->rate & HDMI_AUDIO_FS_44100)
			printk( "	44100\n");
		if(audio->rate & HDMI_AUDIO_FS_48000)
			printk( "	48000\n");
		if(audio->rate & HDMI_AUDIO_FS_88200)
			printk( "	88200\n");
		if(audio->rate & HDMI_AUDIO_FS_96000)
			printk( "	96000\n");
		if(audio->rate & HDMI_AUDIO_FS_176400)
			printk( "	176400\n");
		if(audio->rate & HDMI_AUDIO_FS_192000)
			printk( "	192000\n");
		
		printk( "Support audio word lenght: \n");
		if(audio->rate & HDMI_AUDIO_WORD_LENGTH_16bit)
			printk( "	16bit\n");
		if(audio->rate & HDMI_AUDIO_WORD_LENGTH_20bit)
			printk( "	20bit\n");
		if(audio->rate & HDMI_AUDIO_WORD_LENGTH_24bit)
			printk( "	24bit\n");
	}
	printk( "*****************end*******************\n");
}
#endif

static const unsigned int double_aspect_vic[] = {3, 7, 9, 11, 13, 15, 18, 22, 24, 26, 28, 30, 36, 38, 43, 45, 49, 51, 53, 55, 57, 59};
static void hdmi_sort_modelist(struct hdmi_edid *edid)
{
	struct list_head *pos, *pos_new;
	struct list_head head_new, *head = &edid->modelist;
	struct display_modelist *modelist, *modelist_new, *modelist_n;
	struct fb_videomode *m, *m_new;
	int i, j, compare;
	
	INIT_LIST_HEAD(&head_new);
	list_for_each(pos, head) {
		modelist = list_entry(pos, struct display_modelist, list);
//		printk("%s vic %d\n", __FUNCTION__, modelist->vic);
		for(i = 0; i < ARRAY_SIZE(hdmi_mode); i++) {
	    	for(j = 0; j < ARRAY_SIZE(double_aspect_vic); j++) {
				if(modelist->vic == double_aspect_vic[j]) {
					modelist->vic--;
					break;
				}
			}

	    	if (modelist->vic == hdmi_mode[i].vic) {
				modelist->mode = hdmi_mode[i].mode;
				compare = 1;
				list_for_each(pos_new, &head_new) {
					modelist_new = list_entry(pos_new, struct display_modelist, list);
					m =(struct fb_videomode*) &hdmi_mode[i].mode;
					m_new = &modelist_new->mode; 
					compare = hdmi_videomode_compare(m, m_new);
					if(compare != -1) break;				
				}
				if(compare != 0) {
					modelist_n = kmalloc(sizeof(struct display_modelist), GFP_KERNEL);					
					if (!modelist_n)
						return;
					*modelist_n = *modelist;
					list_add_tail(&modelist_n->list, pos_new);
				}
				break;
			}
	    }
	}
	fb_destroy_modelist(head);

	edid->modelist = head_new;
	edid->modelist.prev->next = &edid->modelist;
}

#ifdef CONFIG_EDID_FINDBESTMODE_GEN2THOMAS_FIX //EDID fix by gen2thomas to find best mode also for not sorted lists
// calculate the ratio of a given (screen) dimension
unsigned int __get_ratio(int  max_x, int  max_y)
{
	/* Maximum vertical size (cm) */
	/* Maximum horizontal size (cm) */
	//5:4=1.25(125), 4:3=1.33(133), 16:10=1.6(160), 16:9=1.78(178)
	int ratio = 0, level;

	ratio = (max_x * 100)/max_y;

	level = HDMI_05_BY_04 + ((HDMI_04_BY_03 - HDMI_05_BY_04)/2);
	if (ratio < level) return HDMI_05_BY_04;

	level = HDMI_04_BY_03 + ((HDMI_16_BY_10 - HDMI_04_BY_03)/2);
	if (ratio < level) return HDMI_04_BY_03;

	level = HDMI_16_BY_10 + ((HDMI_16_BY_09 - HDMI_16_BY_10)/2);
	if (ratio < level) return HDMI_16_BY_10;

	return HDMI_16_BY_09;
}

// find the best entry in modedb matching the given mode
struct fb_videomode* __find_best_matching_mode(struct fb_monspecs *specs)
{
	struct fb_videomode *modedb;
	unsigned int ratio_from_specs; //g2t: 5.4=1.25(125), 4:3=1.33(133), 16:10=1.6(160), 16:9=1.78(178)
	char ratio_found;
	int i;

	ratio_from_specs = __get_ratio(specs->max_x, specs->max_y);
	hdmi_lcdc_debug("[HDMI-LCDC] check mode with ratio %d [1/100]\n", ratio_from_specs);
	ratio_found = 0;
	modedb = &specs->modedb[0];

	for (i = 0; i < specs->modedb_len; i++) {
		hdmi_lcdc_debug("[HDMI-LCDC] check mode %dx%d@%d\n", specs->modedb[i].xres, specs->modedb[i].yres, specs->modedb[i].refresh);
		if(specs->modedb[i].xres > modedb->xres) { // case 1
			if(specs->modedb[i].yres >= modedb->yres) {
				// this is always an better resolution
				modedb = &specs->modedb[i];
				ratio_found = __get_ratio(specs->modedb[i].xres, specs->modedb[i].yres) == ratio_from_specs;
			}
			else {
				// take the resolution only in case the old one has the wrong ratio and the new the right one
				if(ratio_found == 0)
					if(__get_ratio(specs->modedb[i].xres, specs->modedb[i].yres) == ratio_from_specs)	{
						ratio_found = 1;
						modedb = &specs->modedb[i];
					}
			}
		} // case 1
		else {
			if(specs->modedb[i].xres == modedb->xres) { // case 2
				if(specs->modedb[i].yres > modedb->yres) {
					modedb = &specs->modedb[i];
					ratio_found = __get_ratio(specs->modedb[i].xres, specs->modedb[i].yres) == ratio_from_specs;
				}
				else {
					if(specs->modedb[i].yres == modedb->yres)
						// check for frequency (greater is better but must be inside limits)
						if(specs->modedb[i].refresh > modedb->refresh)
							if ((specs->modedb[i].refresh >= specs->hfmin) && (specs->modedb[i].refresh >= specs->hfmax))
								modedb = &specs->modedb[i];
				}
			}
			else {
				if(specs->modedb[i].xres < modedb->xres) // case 3
					if(specs->modedb[i].yres > modedb->yres && ratio_found == 0){
						if(__get_ratio(specs->modedb[i].xres, specs->modedb[i].yres) == ratio_from_specs){
							ratio_found = 1;
							modedb = &specs->modedb[i];
						}
					}
			} // case 3
		} // case 2
	} // for modedb_len

	return modedb;
}

#endif

/**
 * hdmi_ouputmode_select - select hdmi transmitter output mode: hdmi or dvi?
 * @hdmi: handle of hdmi
 * @edid_ok: get EDID data success or not, HDMI_ERROR_SUCESS means success.
 */
int hdmi_ouputmode_select(struct hdmi *hdmi, int edid_ok)
{
	struct list_head *head = &hdmi->edid.modelist;
	struct fb_monspecs	*specs = hdmi->edid.specs;
	struct fb_videomode *modedb = NULL, *mode = NULL;
	int i, pixclock;
	
	if(edid_ok != HDMI_ERROR_SUCESS) {
		dev_err(hdmi->dev, "warning: EDID error, assume sink as HDMI !!!!\n");
		hdmi->edid.sink_hdmi = 1;
	}

	if(edid_ok != HDMI_ERROR_SUCESS) {
		hdmi->edid.ycbcr444 = 0;
		hdmi->edid.ycbcr422 = 0;
		hdmi->autoset = 0;
	}
	if(head->next == head) {
		dev_info(hdmi->dev, "warning: no CEA video mode parsed from EDID !!!!\n");
		// If EDID get error, list all system supported mode.
		// If output mode is set to DVI and EDID is ok, check
		// the output timing.
		
		if(hdmi->edid.sink_hdmi == 0 && specs && specs->modedb_len) {
			/* Get max resolution timing */
#ifdef CONFIG_EDID_FINDBESTMODE_GEN2THOMAS_FIX
			hdmi_lcdc_debug("________________________________________\n");
			hdmi_lcdc_debug("[HDMI-LCDC] Find best mode         start\n");
			modedb = __find_best_matching_mode(specs);
			hdmi_lcdc_debug("[HDMI-LCDC] Best mode found: %dx%d@%d\n", modedb->xres, modedb->yres, modedb->refresh);
			hdmi_lcdc_debug("__________________end___________________\n");
#else
			modedb = &specs->modedb[0];
			for (i = 0; i < specs->modedb_len; i++) {
				if(specs->modedb[i].xres > modedb->xres)
					modedb = &specs->modedb[i];
				else if(specs->modedb[i].yres > modedb->yres)
					modedb = &specs->modedb[i];
			}
#endif
			// For some monitor, the max pixclock read from EDID is smaller
			// than the clock of max resolution mode supported. We fix it.
			pixclock = PICOS2KHZ(modedb->pixclock);
			pixclock /= 250;
			pixclock *= 250;
			pixclock *= 1000;
			if(pixclock == 148250000)
				pixclock = 148500000;
			if(pixclock > specs->dclkmax)
				specs->dclkmax = pixclock;
		}
		
		for(i = 0; i < ARRAY_SIZE(hdmi_mode); i++) {
			mode = (struct fb_videomode *)&(hdmi_mode[i].mode);
			if(modedb) {
				if( (mode->pixclock < specs->dclkmin) || 
					(mode->pixclock > specs->dclkmax) || 
					(mode->refresh < specs->vfmin) ||
					(mode->refresh > specs->vfmax) ||
					(mode->xres > modedb->xres) ||
					(mode->yres > modedb->yres) )
				continue;
			}
			hdmi_add_videomode(mode, head);
		}
	}
	else
		hdmi_sort_modelist(&hdmi->edid);
	
	#ifdef CONFIG_HDMI_LCDC_EDID_DEBUG
	hdmi_show_sink_info(hdmi);
	#endif
	return HDMI_ERROR_SUCESS;
}

/**
 * hdmi_videomode_to_vic: transverse video mode to vic
 * @vmode: videomode to transverse
 * 
 */
int hdmi_videomode_to_vic(struct fb_videomode *vmode)
{
	struct fb_videomode *mode;
	unsigned char vic = 0;
	int i = 0;
	
	for(i = 0; i < ARRAY_SIZE(hdmi_mode); i++)
	{
		mode = (struct fb_videomode*) &(hdmi_mode[i].mode);
		if(	vmode->vmode == mode->vmode &&
			vmode->refresh == mode->refresh &&
			vmode->xres == mode->xres && 
			vmode->left_margin == mode->left_margin &&
			vmode->right_margin == mode->right_margin &&
			vmode->upper_margin == mode->upper_margin &&
			vmode->lower_margin == mode->lower_margin && 
			vmode->hsync_len == mode->hsync_len && 
			vmode->vsync_len == mode->vsync_len)
		{
			if( (vmode->vmode == FB_VMODE_NONINTERLACED && vmode->yres == mode->yres) || 
				(vmode->vmode == FB_VMODE_INTERLACED && vmode->yres == mode->yres/2))
			{								
				vic = hdmi_mode[i].vic;
				break;
			}
		}
	}
	return vic;
}

/**
 * hdmi_vic_to_videomode: transverse vic mode to video mode
 * @vmode: vic to transverse
 * 
 */
const struct fb_videomode* hdmi_vic_to_videomode(int vic)
{
	int i;
	
	if(vic == 0)
		return NULL;
	
	for(i = 0; i < ARRAY_SIZE(hdmi_mode); i++)
	{
		if(hdmi_mode[i].vic == vic)
			return &hdmi_mode[i].mode;
	}
	return NULL;
}

/**
 * hdmi_init_modelist: initial hdmi mode list
 * @hdmi: 
 * 
 * NOTES:
 * 
 */
void hdmi_init_modelist(struct hdmi *hdmi)
{
	int i;
	struct list_head *head = &hdmi->edid.modelist;
	
	INIT_LIST_HEAD(&hdmi->edid.modelist);
	for(i = 0; i < ARRAY_SIZE(hdmi_mode); i++) {
		hdmi_add_videomode(&(hdmi_mode[i].mode), head);
	}
	#ifdef CONFIG_HDMI_LCDC_EDID_DEBUG
	printk( "****************************************\n");
	printk( "Show Software Info (HDMI-LCDC)     start\n");
	printk( "****************************************\n");
	printk( "Prepared video modes: \n");
	__show_info_modelist_modes(head);
	printk( "*****************end*******************\n");
	#endif
}
