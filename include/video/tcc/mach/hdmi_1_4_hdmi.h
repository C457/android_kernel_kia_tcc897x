/****************************************************************************
 * FileName    : kernel/arch/arm/mach-tcc893x/include/mach/hdmi_1_4_hdmi.h
 * Description : hdmi driver
 *
 * Copyright (C) 2013 Telechips Inc.
 *
 * This program is free software; you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation;
 * either version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple Place,
 * Suite 330, Boston, MA 02111-1307 USA
 * ****************************************************************************/

#ifndef _LINUX_HDMI_H_
#define _LINUX_HDMI_H_

#define HDMI_IOC_MAGIC      'h'


#ifndef __HDMI_VIDEO_VIDEOFORMAT__
#define __HDMI_VIDEO_VIDEOFORMAT__
/**
 * @enum VideoFormat
 * Video format
 */
enum VideoFormat
{
    /** 640x480p\@60Hz */
    v640x480p_60Hz = 0,
    /** 720x480p\@60Hz */
    v720x480p_60Hz,
    //MVC_PROCESS
    /** 1280x700p\@60Hz for 3D(MVC)*/
    v1280x720p_60Hz_3D,
    /** 1280x700p\@60Hz */
    v1280x720p_60Hz,
    /** 1920x1080i\@60Hz */
    v1920x1080i_60Hz,
    /** 720x480i\@60Hz */
    v720x480i_60Hz,
    /** 720x240p\@60Hz */
    v720x240p_60Hz,
    /** 2880x480i\@60Hz */
    v2880x480i_60Hz,
    /** 2880x240p\@60Hz */
    v2880x240p_60Hz,
    /** 1440x480p\@60Hz */
    v1440x480p_60Hz,
    /** 1920x1080p\@60Hz */
    v1920x1080p_60Hz,
    /** 720x576p\@60Hz */
    v720x576p_50Hz,
    /** 1280x720p\@50Hz */
    v1280x720p_50Hz,
    /** 1920x1080i\@50Hz (V total = 1125) */
    v1920x1080i_50Hz,
    /** 720x576i\@50Hz */
    v720x576i_50Hz,
    /** 720x288p\@50Hz */
    v720x288p_50Hz,
    /** 2880x576i\@50Hz */
    v2880x576i_50Hz,
    /** 2880x288p\@50Hz */
    v2880x288p_50Hz,
    /** 1440x576p\@50Hz */
    v1440x576p_50Hz,
    /** 1920x1080p\@50Hz */
    v1920x1080p_50Hz,
    /** 1920x1080p\@23.976Hz */
    v1920x1080p_23_976Hz,
	//MVC_PROCESS
    /** 1920x1080p\@24Hz for 3D)MVC)*/
    v1920x1080p_24Hz_3D,
    /** 1920x1080p\@24Hz */
    v1920x1080p_24Hz,
    /** 1920x1080p\@25Hz */
    v1920x1080p_25Hz,
    /** 1920x1080p\@30Hz */
    v1920x1080p_30Hz,
    /** 2880x480p\@60Hz */
    v2880x480p_60Hz,
    /** 2880x576p\@60Hz */
    v2880x576p_50Hz,
    /** 1920x1080i\@50Hz (V total = 1250) */
    v1920x1080i_50Hz_1250,
    /** 1920x1080i\@100Hz */
    v1920x1080i_100Hz,
    /** 1280x720p\@100Hz */
    v1280x720p_100Hz,
    /** 720x576p\@100Hz */
    v720x576p_100Hz,
    /** 720x576i\@100Hz */
    v720x576i_100Hz,
    /** 1920x1080i\@120Hz */
    v1920x1080i_120Hz,
    /** 1280x720p\@120Hz */
    v1280x720p_120Hz,
    /** 720x480p\@120Hz */
    v720x480p_120Hz,
    /** 720x480i\@120Hz */
    v720x480i_120Hz,
    /** 720x576p\@200Hz */
    v720x576p_200Hz,
    /** 720x576i\@200Hz */
    v720x576i_200Hz,
    /** 720x480p\@240Hz */
    v720x480p_240Hz,
    /** 720x480i\@240Hz */
    v720x480i_240Hz,
	/** 1920x1080p\@29.97Hz */
    v1920x1080p_29_97Hz,

    /** 1920x720@60Hz */
    v1920x720p_60Hz,

    
	max_video_formats,
};
#endif /* __HDMI_VIDEO_VIDEOFORMAT__ */
#ifndef __HDMI_VIDEO_COLORSPACE__
#define __HDMI_VIDEO_COLORSPACE__
/**
 * @enum ColorSpace
 * Color space.
 */
enum ColorSpace
{
    /** RGB color space */
    HDMI_CS_RGB,
    /** YCbCr 4:4:4 color space */
    HDMI_CS_YCBCR444,
    /** YCbCr 4:2:2 color space */
    HDMI_CS_YCBCR422
};
#endif /* __HDMI_VIDEO_COLORSPACE__ */

#ifndef __HDMI_VIDEO_COLORDEPTH__
#define __HDMI_VIDEO_COLORDEPTH__
/**
 * @enum ColorDepth
 * Color Depth per pixel.
 */
enum ColorDepth
{
    /** 36 bit color depth per pixel */
    HDMI_CD_36,
    /** 30 bit color depth per pixel */
    HDMI_CD_30,
    /** 24 bit color depth per pixel */
    HDMI_CD_24
};
#endif /* __HDMI_VIDEO_COLORDEPTH__ */

#ifndef __HDMI_VIDEO_HDMIMODE__
#define __HDMI_VIDEO_HDMIMODE__
/**
 * @enum HDMIMode
 * System mode
 */
enum HDMIMode
{
    /** DVI mode */
    DVI = 0,
    /** HDMI mode */
    HDMI
};
#endif /* __HDMI_VIDEO_HDMIMODE__ */

#ifndef __HDMI_VIDEO_PIXELLIMIT__
#define __HDMI_VIDEO_PIXELLIMIT__
/**
 * @enum PixelLimit
 * Pixel limitation.
 */
enum PixelLimit
{
    /** Full range */
    HDMI_FULL_RANGE,
    /** limit range for RGB color space */
    HDMI_RGB_LIMIT_RANGE,
    /** limit range for YCbCr color space */
    HDMI_YCBCR_LIMIT_RANGE
};
#endif /* __HDMI_VIDEO_PIXELLIMIT__ */

#ifndef __HDMI_VIDEO_COLORIMETRY__
#define __HDMI_VIDEO_COLORIMETRY__
/**
 * @enum HDMIColorimetry
 * Colorimetry of video stream
 */
enum HDMIColorimetry
{
    /** Colorimetry is not defined */
    HDMI_COLORIMETRY_NO_DATA,
    /** ITU601 colorimetry */
    HDMI_COLORIMETRY_ITU601,
    /** ITU709 colorimetry */
    HDMI_COLORIMETRY_ITU709,
    /** Extended ITU601 colorimetry */
    HDMI_COLORIMETRY_EXTENDED_xvYCC601,
    /** Extended ITU709 colorimetry */
    HDMI_COLORIMETRY_EXTENDED_xvYCC709
};
#endif /* __HDMI_VIDEO_COLORIMETRY__ */

#ifndef __HDMI_VIDEO_PIXELASPECTRATIO__
#define __HDMI_VIDEO_PIXELASPECTRATIO__
/**
 * @enum PixelAspectRatio
 * Pixel aspect ratio.
 */
enum PixelAspectRatio
{
	/** as picutre pixel ratio */
	HDMI_PIXEL_RATIO_AS_PICTURE,
    /** 4:3 pixel ratio */
    HDMI_PIXEL_RATIO_4_3,
    /** 16:9 pixel ratio */
    HDMI_PIXEL_RATIO_16_9
};
#endif /* __HDMI_VIDEO_PIXELASPECTRATIO__ */

#ifndef __HDMI_AUDIO_AUDIOSAMPLEPACKETTYPE__
#define __HDMI_AUDIO_AUDIOSAMPLEPACKETTYPE__
/**
 * @enum HDMIASPType
 * Type of HDMI audio sample packet
 */
enum HDMIASPType
{
    /** Audio Sample Packet Type */
    HDMI_ASP,
    /** One Bit Audio Packet Type */
    HDMI_DSD,
    /** High Bit Rate Packet Type */
    HDMI_HBR,
    /** DST Packet Type */
    HDMI_DST
};
#endif /* __HDMI_AUDID_AUDIOSAMPLEPACKETTYPE__ */

#ifndef __DEVICE_VIDEO_PARAMS__
#define __DEVICE_VIDEO_PARAMS__
/**
 * @struct device_video_params
 * Video timing paramters to set HDMI H/W @n
 * For more information, refer to HDMI register map doc.
 */
struct device_video_params
{
	/** [H Total] */
	unsigned int HTotal;

	/** [H Blank] */
	unsigned int HBlank;

	/** [V Total] */
	unsigned int VTotal;

	/** [V Blank] : in interlaced mode, this is VBlank - 0.5
	*               but, VIC ID = 39, this is VBlank
	*/
	unsigned int VBlank;

	/** [HFront] */
	unsigned int HFront;

	/** [HSync] */
	unsigned int HSync;

	/** H Sync polarity */
	unsigned int HPol;

	/** [VFront] */
	unsigned int VFront;

	/** [VSync] */
	unsigned int VSync;

	/** V Sync polarity */
	unsigned int VPol;

	/** CEA VIC */
	unsigned int AVI_VIC;

	/** CEA VIC for 16:9 pixel ratio */
	unsigned int AVI_VIC_16_9;

	/** 0 - progresive, 1 - interlaced */
	unsigned int interlaced;

	/** Pixel repetition if double, set 1 */
	unsigned int repetition;
};
#endif /* __DEVICE_VIDEO_PARAMS__ */

#ifndef __HDMI_DEVICE_LCDC_TIMING_PARAMS__
#define __HDMI_DEVICE_LCDC_TIMING_PARAMS__
/**
 * @struct device_lcdc_timing_params
 * Video timing paramters to set LCDC H/W @n
 * For more information, refer to LCDC register map doc.
 */
struct device_lcdc_timing_params
{
	unsigned int id;
	unsigned int iv;
	unsigned int ih;
	unsigned int ip;
	unsigned int dp;
	unsigned int ni;
	unsigned int tv;
	unsigned int tft;
	unsigned int stn;

	//LHTIME1
	unsigned int lpw;
	unsigned int lpc;
	//LHTIME2
	unsigned int lswc;
	unsigned int lewc;
	//LVTIME1
	unsigned int vdb;
	unsigned int vdf;
	unsigned int fpw;
	unsigned int flc;
	//LVTIME2
	unsigned int fswc;
	unsigned int fewc;
	//LVTIME3
	unsigned int fpw2;
	unsigned int flc2;
	//LVTIME4
	unsigned int fswc2;
	unsigned int fewc2;

	unsigned int framepacking;

};

#endif /* __HDMI_DEVICE_VIDEO_PARAMS__ */

#if 0 //not use sys3
typedef enum{
	LCDC_IMG_FMT_1BPP,
	LCDC_IMG_FMT_2BPP,
	LCDC_IMG_FMT_4BPP,
	LCDC_IMG_FMT_8BPP,
	LCDC_IMG_FMT_RGB332 = 8,
	LCDC_IMG_FMT_RGB444 = 9,
	LCDC_IMG_FMT_RGB565 = 10,
	LCDC_IMG_FMT_RGB555 = 11,
	LCDC_IMG_FMT_RGB888 = 12,
	LCDC_IMG_FMT_RGB666 = 13,
	LCDC_IMG_FMT_YUV420SP = 24,	
	LCDC_IMG_FMT_YUV422SP = 25, 
	LCDC_IMG_FMT_YUV422SQ = 26, 
	LCDC_IMG_FMT_YUV420ITL0 = 28, 
	LCDC_IMG_FMT_YUV420ITL1 = 29, 
	LCDC_IMG_FMT_YUV422ITL0 = 30, 
	LCDC_IMG_FMT_YUV422ITL1 = 31, 
	LCDC_IMG_FMT_MAX
}LCDC_IMG_FMT_TYPE;

typedef enum{
	LCDC_LAYER_0, 
	LCDC_LAYER_1, 
	LCDC_LAYER_2, 
	LCDC_LAYER_MAX
}LCD_IMG_LAYER_TYPE;

struct device_lcdc_image_params
{
	unsigned int LCD_layer;
	
	unsigned int Frame_width;
	unsigned int Frame_height;

	unsigned int Image_width;
	unsigned int Image_height;

	unsigned int addr0;
	unsigned int addr1;
	unsigned int addr2;
	unsigned int fmt;	//LCDC_IMG_FMT_TYPE
};
#endif//

/**
 * @struct hdmi_dbg
 * Contains offset and value to set.
 */
struct hdmi_dbg{
	/** offset */
	unsigned int offset;
	/** value */
	unsigned int value;
};



#ifndef __HDMI_PHY_PIXELFREQUENCY__
#define __HDMI_PHY_PIXELFREQUENCY__

/**
 * @enum PHYFreq
 * PHY Frequency
 */
enum PHYFreq
{
	/** Not supported */
	PHY_FREQ_NOT_SUPPORTED = -1,
	/** 25.2 MHz pixel frequency */
	PHY_FREQ_25_200 = 0,
	/** 25.175 MHz pixel frequency */
	PHY_FREQ_25_175,
	/** 27 MHz pixel frequency */
	PHY_FREQ_27,
	/** 27.027 MHz pixel frequency */
	PHY_FREQ_27_027,
	/** 54 MHz pixel frequency */
	PHY_FREQ_54,
	/** 54.054 MHz pixel frequency */
	PHY_FREQ_54_054,
	/** 74.25 MHz pixel frequency */
	PHY_FREQ_74_250,
	/** 74.176 MHz pixel frequency */
	PHY_FREQ_74_176,
	/** 148.5 MHz pixel frequency */
	PHY_FREQ_148_500,
	/** 148.352 MHz pixel frequency */
	PHY_FREQ_148_352,
	/** 108.108 MHz pixel frequency */
	PHY_FREQ_108_108,
	/** 72 MHz pixel frequency */
	PHY_FREQ_72,
	/** 25 MHz pixel frequency */
	PHY_FREQ_25,
	/** 65 MHz pixel frequency */
	PHY_FREQ_65,
	/** 108 MHz pixel frequency */
	PHY_FREQ_108,
	/** 162 MHz pixel frequency */
	PHY_FREQ_162,
	/** 45 MHz pixel frequency */
	PHY_FREQ_45,
	/** 45.955 MHz pixel frequency */
	PHY_FREQ_44_955,
	/** 297 MHz pixel frequency */
	PHY_FREQ_297,
	/** 296.703 MHz pixel frequency */
	PHY_FREQ_296_703,
	/** 59.4 MHz pixel frequency */
	PHY_FREQ_59_400,
	/** 36 MHz pixel frequency */
	PHY_FREQ_36,
	/** 40 MHz pixel frequency */
	PHY_FREQ_40,
	/** 71 MHz pixel frequency */
	PHY_FREQ_71,
	/** 83.5 MHz pixel frequency */
	PHY_FREQ_83_500,
	/** 106.5 MHz pixel frequency */
	PHY_FREQ_106_500,
	/** 122.5 MHz pixel frequency */
	PHY_FREQ_122_500,
	/** 146.25 MHz pixel frequency */
	PHY_FREQ_146_250,
       /** 97.5 MHz pixel frequency */
       PHY_FREQ_97_340,
	
};

#endif /* __HDMI_PHY_PIXELFREQUENCY__ */

#ifndef __HDMI_VIDEO_SOURCE__
#define __HDMI_VIDEO_SOURCE__
/**
 * @enum HDMIVideoSource
 * Type of video source.
 */
enum HDMIVideoSource
{
	/** Internal Video Source */
	HDMI_SOURCE_INTERNAL,
	/** External Video Source */
	HDMI_SOURCE_EXTERNAL,
};
#endif /* __HDMI_VIDEO_SOURCE__ */

#ifndef __HDMI_3D_VIDEO_STRUCTURE__
#define __HDMI_3D_VIDEO_STRUCTURE__
/**
 * @enum HDMI3DVideoStructure
 * Type of 3D Video Structure
 */
enum HDMI3DVideoStructure
{
	/** 2D Video Format  */
	HDMI_2D_VIDEO_FORMAT = -1,
	/** 3D Frame Packing Structure */
	HDMI_3D_FP_FORMAT = 0,
	/** 3D Field Alternative Structure  */
	HDMI_3D_FA_FORMAT,
	/** 3D Line Alternative Structure */
	HDMI_3D_LA_FORMAT,
	/** Side-by-Side(Full)Structure */
	HDMI_3D_SSF_FORMAT,
	/** 3D L+Depth Structure */
	HDMI_3D_LD_FORMAT,
	/** 3D L+Depth+Graphics Structure */
	HDMI_3D_LDGFX_FORMAT,
	/** 3D Top-and-Bottom Structure */
	HDMI_3D_TB_FORMAT,
	/** HDMI VIC Structure (ex. 4Kx2K) */
	HDMI_VIC_FORMAT,
	/** Side-by-Side(Half)Structure */
	HDMI_3D_SSH_FORMAT,
};
#endif /* __HDMI_3D_VIDEO_STRUCTURE__ */


#ifndef __HDMI_VIDEO_PARAMETERS__
#define __HDMI_VIDEO_PARAMETERS__

//! Structure for HDMI video
struct HDMIVideoParameter {
    /** Video interface */
    enum HDMIMode mode;
    /** Video format */
    enum VideoFormat resolution;
    /** Color space */
    enum ColorSpace colorSpace;
    /** Color depth */
    enum ColorDepth colorDepth;
    /** Colorimetry */
    enum HDMIColorimetry colorimetry;
    /** Pixel aspect ratio */
    enum PixelAspectRatio pixelAspectRatio;
	/** Video Source */
	enum HDMIVideoSource videoSrc;
	/** 3D Video Structure */
	enum HDMI3DVideoStructure hdmi_3d_format;
};
#endif /* __HDMI_VIDEO_PARAMETERS__ */

enum hdmi_video_format
{
	HDMI_2D,
	HDMI_VIC,
	HDMI_3D,
};

enum hdmi_3d_type
{
	FRAME_PACKING,
	TOP_AND_BOTTOM,
	SIDE_BY_SIDE,
};

struct HDMIVideoFormatCtrl{
    unsigned char video_format;
	unsigned char structure_3D;
	unsigned char ext_data_3D;
};


// IOW
/** Device request code to set color space. */
#define HDMI_IOC_SET_COLORSPACE             _IOW(HDMI_IOC_MAGIC,0,enum ColorSpace)

/** Device request code to set color depth */
#define HDMI_IOC_SET_COLORDEPTH             _IOW(HDMI_IOC_MAGIC,1,enum ColorDepth)

/** Device request code to set video system */
#define HDMI_IOC_SET_HDMIMODE               _IOW(HDMI_IOC_MAGIC,2,enum HDMIMode)

/** Device request code to set video timing parameters */
#define HDMI_IOC_SET_VIDEOMODE              _IOW(HDMI_IOC_MAGIC,3,struct device_video_params)

/**
 * Device requset code to set/clear blue screen mode @n
 * 1 to set, 0 to clear
 */
#define HDMI_IOC_SET_BLUESCREEN             _IOW(HDMI_IOC_MAGIC,4,unsigned char)

/** Device requset code to set pixel limitation */
#define HDMI_IOC_SET_PIXEL_LIMIT            _IOW(HDMI_IOC_MAGIC,5,enum PixelLimit)

/**
 * Device requset code to set/clear AVMute @n
 * 1 to set, 0 to clear
 */
#define HDMI_IOC_SET_AVMUTE                 _IOW(HDMI_IOC_MAGIC,6,unsigned char)

/** Device requset code to set packet type of HDMI audio output */
#define HDMI_IOC_SET_AUDIOPACKETTYPE        _IOW(HDMI_IOC_MAGIC,7,enum HDMIASPType)


/** Device requset code to set audio sample freqency */
#define HDMI_IOC_SET_AUDIOSAMPLEFREQ        _IOW(HDMI_IOC_MAGIC,8,enum SamplingFreq)

/** Device requset code to set number of channels */
#define HDMI_IOC_SET_AUDIOCHANNEL           _IOW(HDMI_IOC_MAGIC,9,enum ChannelNum)

/** Device requset code to set audio speaker allocation information */
#define HDMI_IOC_SET_SPEAKER_ALLOCATION     _IOW(HDMI_IOC_MAGIC,10,unsigned int)
/**
 * Device requset code to start/stop sending audio packet @n
 * 1 to start, 0 to stop
 */
#define HDMI_IOC_SET_AUDIO_ENABLE           _IOW(HDMI_IOC_MAGIC,18,unsigned char)

/** Device request code to set pixel aspect ratio */
#define HDMI_IOC_SET_PIXEL_ASPECT_RATIO     _IOW(HDMI_IOC_MAGIC,19,enum PixelAspectRatio)

/** Device requset code to set colorimetry */
#define HDMI_IOC_SET_COLORIMETRY			_IOW(HDMI_IOC_MAGIC,20,enum HDMIColorimetry)

/** Device requset code to set HDMI video source */
#define HDMI_IOC_SET_VIDEO_SOURCE			_IOW(HDMI_IOC_MAGIC,21,enum HDMIVideoSource)

/** Device requset code to fill one subpacket in Audio Sample Packet */
#define HDMI_IOC_FILL_ONE_SUBPACKET			_IOW(HDMI_IOC_MAGIC,23,unsigned char)

// for debugging
/** Device requset code to set HDMI debug */
#define HDMI_IOC_SET_DEBUG					_IOW(HDMI_IOC_MAGIC,24,struct hdmi_dbg)

/** Device request code to set video format information */
#define HDMI_IOC_SET_VIDEOFORMAT_INFO       _IOW(HDMI_IOC_MAGIC,26,enum VideoFormat)

#define HDMI_IOC_SET_PHY_FREQ		       _IOW(HDMI_IOC_MAGIC,27,enum PHYFreq)

#define HDMI_IOC_SET_PHY_INDEX       _IOW(HDMI_IOC_MAGIC,28,unsigned char)

/** Device request code to set lcdc timing parameters */
#define HDMI_IOC_SET_LCDC_TIMING			_IOW(HDMI_IOC_MAGIC,30,struct device_lcdc_timing_params)

/** Device request code to set/get Hdmi Device Power */
#define HDMI_IOC_SET_PWR_CONTROL			_IOW(HDMI_IOC_MAGIC,31, unsigned int)

#define HDMI_IOC_SET_PWR_V5_CONTROL			_IOW(HDMI_IOC_MAGIC,32, unsigned int)

#define HDMI_IOC_VIDEO_FORMAT_CONTROL		_IOW(HDMI_IOC_MAGIC,40, struct HDMIVideoFormatCtrl)

#define HDMI_IOC_BLANK						_IOW(HDMI_IOC_MAGIC,50, unsigned int)


/**
 * Device requset code to get status of HDMI PHY H/W @n
 * if 1, PHY is ready; if 0, PHY is not ready.
 */
#define HDMI_IOC_GET_PHYREADY               _IOR(HDMI_IOC_MAGIC,0,unsigned char)

/** Device request code to get video configuration information (video parameters) */
#define HDMI_IOC_GET_VIDEOCONFIG           _IOR(HDMI_IOC_MAGIC,8,struct HDMIVideoParameter)

/** Device request code to get hdmi start/stop status */
#define HDMI_IOC_GET_HDMISTART_STATUS      _IOR(HDMI_IOC_MAGIC,9,unsigned int)



/** Device requset code to get pixel limitation */
#define HDMI_IOC_GET_PIXEL_LIMIT            _IOR(HDMI_IOC_MAGIC,10,enum PixelLimit)

/** Device requset code to get audio speaker allocation information */
#define HDMI_IOC_GET_SPEAKER_ALLOCATION     _IOR(HDMI_IOC_MAGIC,11,unsigned int)

#define HDMI_IOC_GET_AUDIOCHANNEL           _IOR(HDMI_IOC_MAGIC,12,enum ChannelNum)

#define HDMI_IOC_GET_AUDIOSAMPLEFREQ        _IOR(HDMI_IOC_MAGIC,13,enum SamplingFreq)

#define HDMI_IOC_GET_AUDIOPACKETTYPE        _IOR(HDMI_IOC_MAGIC,14,enum HDMIASPType)

#define HDMI_IOC_GET_PWR_STATUS				_IOR(HDMI_IOC_MAGIC,31, unsigned int)

#define HDMI_IOC_GET_SUSPEND_STATUS				_IOR(HDMI_IOC_MAGIC,32, unsigned int)

/** Device requset code to start sending HDMI output */
#define HDMI_IOC_START_HDMI                 _IO(HDMI_IOC_MAGIC,0)

/** Device requset code to stop sending HDMI output */
#define HDMI_IOC_STOP_HDMI                  _IO(HDMI_IOC_MAGIC,1)

/** Device requset code to reset AUI Sampling Freq Fields  */
#define HDMI_IOC_RESET_AUISAMPLEFREQ        _IO(HDMI_IOC_MAGIC,8)

#define HDMI_IOC_SET_PHYRESET               _IO(HDMI_IOC_MAGIC,9)

/** Device requset code to print HDMI H/W registers */
#define HDMI_IOC_DEBUG_HDMI_CORE			_IO(HDMI_IOC_MAGIC,10)

/** Device requset code to print HDMI H/W Video registers */
#define HDMI_IOC_DEBUG_HDMI_CORE_VIDEO		_IO(HDMI_IOC_MAGIC,11)

#define HDMI_IOC_SET_HPD_SWITCH_STATUS      _IO(HDMI_IOC_MAGIC,12)

#endif /* _LINUX_HDMI_H_ */
