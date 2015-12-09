#include "devices.h"

/**
 * Global register initialization for OV5642.
 * Taken from ArduCAM project.
 */
RegisterTuple16_8 ov5642_dvp_fmt_global_init[] = {
	{0x3103, 0x93},
	{0x3008, 0x82},
	{0x3017, 0x7f},
	{0x3018, 0xfc},
	{0x3810, 0xc2},
	{0x3615, 0xf0},
	{0x3000, 0x00},
	{0x3001, 0x00},
	{0x3002, 0x00},
	{0x3003, 0x00},
	{0x3000, 0xf8},
	{0x3001, 0x48},
	{0x3002, 0x5c},
	{0x3003, 0x02},
	{0x3004, 0x07},
	{0x3005, 0xb7},
	{0x3006, 0x43},
	{0x3007, 0x37},
	{0x3011, 0x07},

	// fps	25fps
	//{0x300F, 0x0a},
	//{0x3010 ,0x00},
	//{0x3011, 0x07},
	//{0x3012 ,0x00},

	{0x370c, 0xa0},
	{0x3602, 0xfc},
	{0x3612, 0xff},
	{0x3634, 0xc0},
	{0x3613, 0x00},
	{0x3605, 0x7c},
	{0x3622, 0x60},
	{0x3604, 0x40},
	{0x3603, 0xa7},
	{0x3603, 0x27},
	{0x4000, 0x21},
	{0x401d, 0x22},
	{0x3600, 0x54},
	{0x3605, 0x04},
	{0x3606, 0x3f},
	{0x3c01, 0x80},
	{0x5000, 0x4f},
	{0x5020, 0x04},
	{0x5500, 0x0a},
	{0x5504, 0x00},
	{0x5505, 0x7f},
	{0x5080, 0x08},
	{0x300e, 0x18},
	{0x4610, 0x00},
	{0x471d, 0x05},
	{0x4708, 0x06},
	{0x4300, 0x32},
	{0x3503, 0x07},
	{0x3501, 0x73},
	{0x3502, 0x80},
	{0x350b, 0x00},
	{0x3503, 0x07},
	//{0x3824, 0x11},
	{0x3501, 0x1e},
	{0x3502, 0x80},
	{0x350b, 0x7f},
	{0x3a0d, 0x04},
	{0x3a0e, 0x03},
	{0x3705, 0xdb},
	{0x370a, 0x81},
	{0x3801, 0x80},
	{0x3801, 0x50},
	{0x3803, 0x08},
	//{0x3827, 0x08},
	{0x3810, 0x40},
	{0x3a00, 0x78},
	{0x3a1a, 0x05},
	{0x3a13, 0x30},
	{0x3a18, 0x00},
	{0x3a19, 0x7c},
	{0x3a08, 0x12},
	{0x3a09, 0xc0},
	{0x3a0a, 0x0f},
	{0x3a0b, 0xa0},
	{0x3004, 0xff},
	{0x350c, 0x07},
	{0x350d, 0xd0},
	{0x3500, 0x00},
	{0x3501, 0x00},
	{0x3502, 0x00},
	{0x350a, 0x00},
	{0x350b, 0x00},
	{0x3503, 0x00},
	{0x3a0f, 0x3c},
	{0x3a10, 0x30},
	{0x3a1b, 0x3c},
	{0x3a1e, 0x30},
	{0x3a11, 0x70},
	{0x3a1f, 0x10},
	{0x3030, 0x0b},//2b
	{0x3a02, 0x00},
	{0x3a03, 0x7d},
	{0x3a04, 0x00},
	{0x3a14, 0x00},
	{0x3a15, 0x7d},
	{0x3a16, 0x00},
	{0x3a00, 0x78},
	{0x3a08, 0x09},
	{0x3a09, 0x60},
	{0x3a0a, 0x07},
	{0x3a0b, 0xd0},
	{0x3a0d, 0x08},
	{0x3a0e, 0x06},
	{0x5193, 0x70},
	{0x589b, 0x04},
	{0x589a, 0xc5},
	{0x401e, 0x20},
	{0x4001, 0x42},
	{0x401c, 0x04},
	{0x5300, 0x00},
	{0x5301, 0x20},
	{0x5302, 0x00},
	{0x5303, 0x7c},
	{0x530c, 0x00},
	{0x530d, 0x0c},
	{0x530e, 0x20},
	{0x530f, 0x80},
	{0x5310, 0x20},
	{0x5311, 0x80},
	{0x5308, 0x20},
	{0x5309, 0x40},
	{0x5304, 0x00},
	{0x5305, 0x30},
	{0x5306, 0x00},
	{0x5307, 0x80},
	{0x5314, 0x08},
	{0x5315, 0x20},
	{0x5319, 0x30},
	{0x5316, 0x10},
	{0x5317, 0x08},
	{0x5318, 0x02},

	//color matrix
	{0x5380, 0x1 },
	{0x5381, 0x0 },
	{0x5382, 0x0 },
	{0x5383, 0x1a},
	{0x5384, 0x0 },
	{0x5385, 0x1a},
	{0x5386, 0x0 },
	{0x5387, 0x0 },
	{0x5388, 0x1 },
	{0x5389, 0x3c},
	{0x538a, 0x0 },
	{0x538b, 0x35},
	{0x538c, 0x0 },
	{0x538d, 0x0 },
	{0x538e, 0x0 },
	{0x538f, 0x05},
	{0x5390, 0x0 },
	{0x5391, 0xe8},
	{0x5392, 0x0 },
	{0x5393, 0xa2},
	{0x5394, 0x8 },

	//gamma
	{0x5480, 0xd },
	{0x5481, 0x18},
	{0x5482, 0x2a},
	{0x5483, 0x49},
	{0x5484, 0x56},
	{0x5485, 0x62},
	{0x5486, 0x6c},
	{0x5487, 0x76},
	{0x5488, 0x80},
	{0x5489, 0x88},
	{0x548a, 0x96},
	{0x548b, 0xa2},
	{0x548c, 0xb8},
	{0x548d, 0xcc},
	{0x548e, 0xe0},
	{0x548f, 0x10},
	{0x5490, 0x3 },
	{0x5491, 0x40},
	{0x5492, 0x3 },
	{0x5493, 0x0 },
	{0x5494, 0x2 },
	{0x5495, 0xa0},
	{0x5496, 0x2 },
	{0x5497, 0x48},
	{0x5498, 0x2 },
	{0x5499, 0x26},
	{0x549a, 0x2 },
	{0x549b, 0xb },
	{0x549c, 0x1 },
	{0x549d, 0xee},
	{0x549e, 0x1 },
	{0x549f, 0xd8},
	{0x54a0, 0x1 },
	{0x54a1, 0xc7},
	{0x54a2, 0x1 },
	{0x54a3, 0xb3},
	{0x54a4, 0x1 },
	{0x54a5, 0x90},
	{0x54a6, 0x1 },
	{0x54a7, 0x62},
	{0x54a8, 0x1 },
	{0x54a9, 0x27},
	{0x54aa, 0x01},
	{0x54ab, 0x09},
	{0x54ac, 0x01},
	{0x54ad, 0x00},
	{0x54ae, 0x0 },
	{0x54af, 0x40},
	{0x54b0, 0x1 },
	{0x54b1, 0x20},
	{0x54b2, 0x1 },
	{0x54b3, 0x40},
	{0x54b4, 0x0 },
	{0x54b5, 0xf0},
	{0x54b6, 0x1 },
	{0x54b7, 0xdf},

	//saturation
	{0x5583, 0x3a}, //ken 20100106
	{0x5584, 0x3a}, //ken 20100106

	{0x5580, 0x02},
	{0x5000, 0xcf},

	//for sunny
	//lens shading
	{0x5800, 0x35},
	{0x5801, 0x20},
	{0x5802, 0x17},
	{0x5803, 0x17},
	{0x5804, 0x18},
	{0x5805, 0x1E},
	{0x5806, 0x2D},
	{0x5807, 0x47},
	{0x5808, 0x17},
	{0x5809, 0x11},
	{0x580a, 0xC },
	{0x580b, 0xB },
	{0x580c, 0xC },
	{0x580d, 0x10},
	{0x580e, 0x17},
	{0x580f, 0x23},
	{0x5810, 0x10},
	{0x5811, 0xA },
	{0x5812, 0x5 },
	{0x5813, 0x3 },
	{0x5814, 0x4 },
	{0x5815, 0x8 },
	{0x5816, 0xF },
	{0x5817, 0x19},
	{0x5818, 0xE },
	{0x5819, 0x7 },
	{0x581a, 0x2 },
	{0x581b, 0x3 },
	{0x581c, 0x3 },
	{0x581d, 0x4 },
	{0x581e, 0xD },
	{0x581f, 0x16},
	{0x5820, 0xF },
	{0x5821, 0x8 },
	{0x5822, 0x2 },
	{0x5823, 0x3 },
	{0x5824, 0x3 },
	{0x5825, 0x5 },
	{0x5826, 0xE },
	{0x5827, 0x19},
	{0x5828, 0x11},
	{0x5829, 0xB },
	{0x582a, 0x6 },
	{0x582b, 0x3 },
	{0x582c, 0x4 },
	{0x582d, 0x8 },
	{0x582e, 0x10},
	{0x582f, 0x19},
	{0x5830, 0x19},
	{0x5831, 0x11},
	{0x5832, 0xC },
	{0x5833, 0xB },
	{0x5834, 0xB },
	{0x5835, 0x10},
	{0x5836, 0x17},
	{0x5837, 0x23},
	{0x5838, 0x35},
	{0x5839, 0x1F},
	{0x583a, 0x16},
	{0x583b, 0x12},
	{0x583c, 0x14},
	{0x583d, 0x19},
	{0x583e, 0x26},
	{0x583f, 0x3D},
	{0x5840, 0xF },
	{0x5841, 0xE },
	{0x5842, 0xD },
	{0x5843, 0xD },
	{0x5844, 0xE },
	{0x5845, 0x10},
	{0x5846, 0xF },
	{0x5847, 0xE },
	{0x5848, 0xE },
	{0x5849, 0xE },
	{0x584a, 0xE },
	{0x584b, 0xD },
	{0x584c, 0xE },
	{0x584d, 0xF },
	{0x584e, 0x10},
	{0x584f, 0xF },
	{0x5850, 0xF },
	{0x5851, 0xD },
	{0x5852, 0xE },
	{0x5853, 0xF },
	{0x5854, 0xF },
	{0x5855, 0xF },
	{0x5856, 0xF },
	{0x5857, 0xD },
	{0x5858, 0x10},
	{0x5859, 0xE },
	{0x585a, 0xF },
	{0x585b, 0xF },
	{0x585c, 0xE },
	{0x585d, 0xD },
	{0x585e, 0xD },
	{0x585f, 0xC },
	{0x5860, 0xB },
	{0x5861, 0xC },
	{0x5862, 0xC },
	{0x5863, 0xD },
	{0x5864, 0x12},
	{0x5865, 0x13},
	{0x5866, 0x14},
	{0x5867, 0x14},
	{0x5868, 0x12},
	{0x5869, 0x11},
	{0x586a, 0x14},
	{0x586b, 0x11},
	{0x586c, 0x10},
	{0x586d, 0x10},
	{0x586e, 0x11},
	{0x586f, 0x14},
	{0x5870, 0x14},
	{0x5871, 0xF },
	{0x5872, 0xF },
	{0x5873, 0xF },
	{0x5874, 0xF },
	{0x5875, 0x12},
	{0x5876, 0x14},
	{0x5877, 0xF },
	{0x5878, 0xF },
	{0x5879, 0xF },
	{0x587a, 0xF },
	{0x587b, 0x13},
	{0x587c, 0x14},
	{0x587d, 0x12},
	{0x587e, 0x10},
	{0x587f, 0x10},
	{0x5880, 0x11},
	{0x5881, 0x13},
	{0x5882, 0x13},
	{0x5883, 0x13},
	{0x5884, 0x16},
	{0x5885, 0x16},
	{0x5886, 0x13},
	{0x5887, 0x13},

	{0x3710, 0x10},
	{0x3632, 0x51},
	{0x3702, 0x10},
	{0x3703, 0xb2},
	{0x3704, 0x18},
	{0x370b, 0x40},
	{0x370d, 0x03},
	{0x3631, 0x01},
	{0x3632, 0x52},
	{0x3606, 0x24},
	{0x3620, 0x96},
	{0x5785, 0x07},
	{0x3a13, 0x30},
	{0x3600, 0x52},
	{0x3604, 0x48},
	{0x3606, 0x1b},
	{0x370d, 0x0b},
	{0x370f, 0xc0},
	{0x3709, 0x01},
	{0x3823, 0x00},
	{0x5007, 0x00},
	{0x5009, 0x00},
	{0x5011, 0x00},
	{0x5013, 0x00},
	{0x519e, 0x00},
	{0x5086, 0x00},
	{0x5087, 0x00},
	{0x5088, 0x00},
	{0x5089, 0x00},
	{0x302b, 0x00},

	{0x4740, 0x20},
	{0x3c00, 0x04},
	{0x3012, 0x00},

	//denoise YUV
	{0x528a, 0x02},
	{0x528b, 0x06},
	{0x528c, 0x20},
	{0x528d, 0x30},
	{0x528e, 0x40},
	{0x528f, 0x50},
	{0x5290, 0x60},
	{0x5292, 0x00},
	{0x5293, 0x02},
	{0x5294, 0x00},
	{0x5295, 0x04},
	{0x5296, 0x00},
	{0x5297, 0x08},
	{0x5298, 0x00},
	{0x5299, 0x10},
	{0x529a, 0x00},
	{0x529b, 0x20},
	{0x529c, 0x00},
	{0x529d, 0x28},
	{0x529e, 0x00},
	{0x529f, 0x30},
	{0x5282, 0x00},

	{0x350b, 0x0f},
	{0x3a19, 0x00},
	{0x3001, 0x48},
	{0x3002, 0x5c},
	{0x3003, 0x02},
	{0x3004, 0xFF},
	{0x3005, 0xb7},
	{0x3006, 0x43},
	{0x3007, 0x37},
	{0x3a19, 0xff},
	{0x350c, 0x07},
	{0x350d, 0xd0},
	{0x3602, 0xfc},
	{0x3612, 0xff},
	{0x3613, 0x00},
	{0x3621, 0x87},
	{0x3622, 0x60},
	{0x3623, 0x01},
	{0x3604, 0x48},
	{0x3705, 0xdb},
	{0x370a, 0x81},
	{0x3801, 0x50},
	{0x3803, 0x08},
	{0x3804, 0x05},
	{0x3805, 0x00},
	{0x3806, 0x03},
	{0x3807, 0xc0},
	{0x3808, 0x01},
	{0x3809, 0x40},
	{0x380a, 0x00},
	{0x380b, 0xf0},
	{0x380c, 0x0c},
	{0x380d, 0x80},

	{0x380E, 0x03},
	{0x380F, 0xe8},

	{0x3810, 0x40},
	{0x3824, 0x11},
	{0x3827, 0x08},
	{0x3a00, 0x78},
	{0x3a0d, 0x08},
	{0x3a0e, 0x06},
	{0x3a11, 0xd0},
	{0x3a1f, 0x40},
	{0x460b, 0x37},
	{0x471d, 0x05},
	{0x4713, 0x02},
	{0x471c, 0xd0},
	{0x5001, 0xff},
	{0x589b, 0x04},
	{0x589a, 0xc5},
	{0x4407, 0x0c},
	{0x3002, 0x5c},
	{0x3002, 0x5c},
	{0x3503, 0x00},
	{0x460c, 0x22},
	{0x460b, 0x37},
	{0x471c, 0xd0},
	{0x471d, 0x05},
	{0x3818, 0xc1},
	{0x501f, 0x00},
	{0x3002, 0x5c},
	{0x3819, 0x80},
	{0x5002, 0xe0},
	{0x3503, 0x00},
	//ex weight
	{0x5688, 0x11},
	{0x5689, 0x11},
	{0x568a, 0x11},
	{0x568b, 0x11},
	{0x568c, 0x11},
	{0x568d, 0x11},
	{0x568e, 0x11},
	{0x568f, 0x11},

	{0x350b, 0x3f},
	{0x3503, 0x00},
	{0x3a19, 0x7c},
	//ex window
	{0x5680, 0x00},
	{0x5681, 0x00},
	{0x5682, 0x05},
	{0x5683, 0x00},
	{0x5684, 0x00},
	{0x5685, 0x00},
	{0x5686, 0x03},
	{0x5687, 0xc0},

	//awb 20100205 kenxu
	{0x5180, 0xff},
	{0x5181, 0x52},
	{0x5182, 0x11},
	{0x5183, 0x14},
	{0x5184, 0x25},
	{0x5185, 0x24},
	{0x5186, 0x0c},
	{0x5187, 0x16},
	{0x5188, 0x10},
	{0x5189, 0x64},
	{0x518a, 0x69},
	{0x518b, 0xff},
	{0x518c, 0x84},
	{0x518d, 0x3b},
	{0x518e, 0x41},
	{0x518f, 0x4f},
	{0x5190, 0x50},
	{0x5191, 0xf8},
	{0x5192, 0x04},
	{0x5193, 0x70},
	{0x5194, 0xf0},
	{0x5195, 0xf0},
	{0x5196, 0x03},
	{0x5197, 0x01},
	{0x5198, 0x06},
	{0x5199, 0x12},
	{0x519a, 0x04},
	{0x519b, 0x00},
	{0x519c, 0x05},
	{0x519d, 0x9b},
	{0x519e, 0x00},

	//ken modify 20091208

	{0x3010, 0x10},
	{0x3815, 0x07}, //24M DVP CLOCK

	//AEC/AGC setting
	//Make sure use AEC/AGC source before gamma (0x5025 = 0x80)
	{0x5025, 0x80},
	{0x3a0f, 0x48},
	{0x3a10, 0x40},
	{0x3a1b, 0x4a},
	{0x3a1e, 0x3e},
	{0x3a11, 0x70},
	{0x3a1f, 0x20},

	//Gamma related setting
	{0x54B0, 0x1 }, //ADD
	{0x54B1, 0x20},
	{0x54B2, 0x0},
	{0x54B3, 0x10},
	{0x54B4, 0x0},
	{0x54B5, 0xf0},
	{0x54B6, 0x0},
	{0x54B7, 0xDF},

	//De-noise setting
	{0x5317, 0x00}, //08

	//Auto Sharpness    +1
	{0x530c, 0x04},
	{0x530d, 0x18},
	{0x5312, 0x20},

	{0xffff, 0xff},
};

/**
 * JPEG capture mode with QVGA resolution
 * Taken from ArduCAM project.
 */
RegisterTuple16_8 ov5642_dvp_fmt_jpeg_qvga[] = {
	{0x3819, 0x81},
	{0x3503, 0x00},	//AWE Manual Mode Control //0x07
	{0x3002, 0x00},
	{0x3003, 0x00},
	{0x3005, 0xff},
	{0x3006, 0xff},
	{0x3007 ,0x3f},
	{0x3602 ,0xe4},
	{0x3603 ,0x27},
	{0x3604 ,0x60},
	{0x3612 ,0xac},
	{0x3613 ,0x44},
	{0x3622 ,0x08},
	{0x3623 ,0x22},
	{0x3621 ,0x27},
	{0x3705 ,0xda},
	{0x370a ,0x80},
	{0x3800 ,0x01},
	{0x3801 ,0x8a},
	{0x3802 ,0x00},
	{0x3803 ,0x0a},
	{0x3804 ,0x0a},
	{0x3805 ,0x20},
	{0x3806 ,0x07},
	{0x3807 ,0x98},

	{0x3808 ,0x01},
	{0x3809 ,0x40},
	{0x380a ,0x00},
	{0x380b ,0xf0},

	{0x380c ,0x0c},
	{0x380d ,0x80},
	{0x380e ,0x07},
	{0x380f ,0xd0},
	{0x3810 ,0xc2},
	{0x3815 ,0x44},
	{0x3818 ,0xa8},
	{0x3824 ,0x01},
	{0x3827 ,0x0a},
	{0x3a00 ,0x78},
	{0x3a0d ,0x10},
	{0x3a0e ,0x0d},
	{0x3a00 ,0x78},
	{0x460b ,0x35},
	{0x471d ,0x00},
	{0x471c ,0x50},
	{0x5682 ,0x0a},
	{0x5683 ,0x20},
	{0x5686 ,0x07},
	{0x5687 ,0x98},
	{0x589b ,0x00},
	{0x589a ,0xc0},
	{0x4407 ,0x04},
	{0x589b ,0x00},
	{0x589a ,0xc0},
	{0x3002 ,0x0c},
	{0x3002 ,0x00},
	{0x4300 ,0x32},
	{0x460b ,0x35},
	{0x3002 ,0x0c},
	{0x3002 ,0x00},
	{0x4713 ,0x02},
	{0x4600 ,0x80},
	{0x4721 ,0x02},
	{0x471c ,0x40},
	{0x4408 ,0x00},
	{0x460c ,0x22},
	{0x3815 ,0x04},
	{0x3818 ,0xe8},//c8
	{0x501f ,0x00},
	{0x5002 ,0xe0},
	{0x440a ,0x01},
	{0x4402 ,0x90},
	{0x3811 ,0xf0},		//4:3 thumbnail 				// 0x size sensor.....

	{0x471c ,0x50},
	{0x3815 ,0x44},
	{0x3818 ,0xe8},//c8
	{0x4740 ,0x20},
	{0x3030 ,0x0b},//2b
	{0x350c ,0x07},
	{0x350d ,0xd0},
	{0x5001 ,0xFF},
	{0x3010 ,0x30},

	{0x460d,0xdd},//  dummy data

	/* test color bar */
	//{0x503d , 0x80},
	//{0x503e, 0x00},

	//11 fps
	{0x300F, 0x0a},
	{0x3010 ,0x00},
	{0x3011, 0x06},
	{0x3012 ,0x00},

	//{0x4407 ,0x00},

	{0xffff, 0xff},
};

/**
 * Increase the JPEG capture resolution to 1080p.
 * Taken from ArduCAM project.
 */
RegisterTuple16_8 ov5642_res_1080P[] = {
	/* 1920x1080 */
	{0x5001, 0xff},
	{0x3808 ,0x07},
	{0x3809 ,0x80},
	{0x380a ,0x04},
	{0x380b ,0x38},

	{0x3818, 0xA8},
	{0x3621, 0x10},
	{0x3801, 0xC8},

	{0xFFFF, 0xFF}
};
