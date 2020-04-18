#pragma once
#include <list>
#include <string>

enum languageStringID
{
    /*********************************************************************************
    * Meta Information                                                               *
    *********************************************************************************/
    LANGUAGE_NAME = 1,

    /*********************************************************************************
    * Config Dialog                                                                  *
    *********************************************************************************/
    TAB_VIDEO = 1000,
    TAB_EMULATION = 1001,
    TAB_FRAME_BUFFER = 1002,
    TAB_TEXTURE_ENHANCEMENT = 1003,
    TAB_OSD = 1004,
    TAB_DEBUG = 1005,
    CFG_SAVE_SETTINGS_FOR = 1010,
    CFG_SETTINGS_PROFILE = 1011,
    CFG_REMOVE = 1012,
    CFG_RESTORE_DEFAULTS = 1013,
    CFG_SAVE_AND_CLOSE = 1014,
    CFG_SAVE = 1015,
    CFG_CLOSE = 1016,

    /*********************************************************************************
    * Video Tab                                                                      *
    *********************************************************************************/
    VIDEO_GROUP = 2000,
    VIDEO_FULL_SCREEN_RES = 2001,
    VIDEO_FULL_SCREEN_RES_TOOLTIP = 2002,
    VIDEO_REFRESH_RATE = 2003,
    VIDEO_REFRESH_RATE_TOOLTIP = 2004,
    VIDEO_WINDOWED_RESOLUTION = 2005,
    VIDEO_WINDOWED_RESOLUTION_TOOLTIP = 2006,
    VIDEO_ASPECT_RATIO = 2007,
    VIDEO_ASPECT_RATIO_TOOLTIP = 2008,
    VIDEO_ASPECT_4_3 = 2009,
    VIDEO_ASPECT_16_19 = 2010,
    VIDEO_ASPECT_STRETCH = 2011,
    VIDEO_ASPECT_ADJUST = 2012,
    VIDEO_VSYNC = 2013,
    VIDEO_VSYNC_TOOLTIP = 2014,
    VIDEO_THREADED_VIDEO = 2015,
    VIDEO_THREADED_VIDEO_TOOLTIP = 2016,
    VIDEO_OVERSCAN = 2017,
    VIDEO_OVERSCAN_TOOLTIP = 2018,
    VIDEO_NTSC = 2019,
    VIDEO_PAL = 2020,
    VIDEO_ANTI_ALIASING = 2021,
    VIDEO_NO_ANTI_ALIASING = 2022,
    VIDEO_FAST_ANTI_ALIASING = 2023,
    VIDEO_MULTISAMPLE_ANTI_ALIASING = 2024,
    VIDEO_AA_OFF = 2025,
    VIDEO_AA_HIGH = 2026,
    VIDEO_AA_TOOLTIP = 2027,
    VIDEO_AA_INFO = 2028,
    VIDEO_FILTERING_GROUP = 2029,
    VIDEO_ANISOTROPIC = 2030,
    VIDEO_ANISOTROPIC_OFF = 2031,
    VIDEO_ANISOTROPIC_HIGH = 2032,
    VIDEO_BILINEAR = 2033,
    VIDEO_BILINEAR_STANDARD = 2034,
    VIDEO_BILINEAR_3POINT = 2035,
    VIDEO_BILINEAR_TOOLTIP = 2036,

    VIDEO_DITHERING_GROUP = 2037,
    VIDEO_PATTERN = 2038,
    VIDEO_DITHERING_APPLY_TO_OUTPUT = 2039,
    VIDEO_DITHERING_APPLY_TO_OUTPUT_TOOLTIP = 2040,
    VIDEO_DITHERING_5BIT_QUANTIZATION = 2041,
    VIDEO_DITHERING_5BIT_QUANTIZATION_TOOLTIP = 2042,
    VIDEO_DITHERING_HIRES_NOISE = 2043,
    VIDEO_DITHERING_HIRES_NOISE_TOOLTIP = 2044,
    VIDEO_DITHERING_MODE_TOOLTIP = 2045,
    VIDEO_DITHERING_DISABLE = 2046,
    VIDEO_DITHERING_BAYER = 2047,
    VIDEO_DITHERING_MAGIC_SQUARE = 2048,
    VIDEO_DITHERING_BLUE_NOISE = 2049,
    VIDEO_LANGUAGE = 2050,
};

struct LanguageFile
{
    std::string Filename;
    std::string LanguageName;
};
typedef std::list<LanguageFile> LanguageList;

void LoadCurrentStrings(const char * path, const std::string & lang);
LanguageList GetLanguageList(const char * path);
std::wstring wGS(languageStringID StringID);