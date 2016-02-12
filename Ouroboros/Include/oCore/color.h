// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Common colors from http://www.codeproject.com/KB/GDI/XHtmlDraw/XHtmlDraw4.png
// Color format is argb where a is in the most significant byte and b is in the 
// least significant byte.

#pragma once
#include <cstdint>

namespace ouro {
	
namespace color
{
	static const uint32_t null                                         = 0x00000000;
	static const uint32_t mask_red                                     = 0x00FF0000;
	static const uint32_t mask_green                                   = 0x0000FF00;
	static const uint32_t mask_blue                                    = 0x000000FF;
	static const uint32_t mask_alpha                                   = 0xFF000000;
	static const uint32_t almost_black                                 = 0xFF252525;
	static const uint32_t microsoft_blue_pen                           = 0xFFFF3232;
	static const uint32_t microsoft_blue_brush                         = 0xFFA08064;
	static const uint32_t default_gray                                 = 0xFF7F7FFF;
	static const uint32_t tangent_space_normal_blue                    = 0xFF7F7FFF; // Z-Up
	static const uint32_t object_space_normal_green                    = 0xFF7FFF7F; // y-Up

	static const uint32_t alice_blue                                   = 0xFFF0F8FF;
	static const uint32_t antique_white                                = 0xFFFAEBD7;
	static const uint32_t aqua                                         = 0xFF00FFFF;
	static const uint32_t aquamarine                                   = 0xFF7FFFD4;
	static const uint32_t azure                                        = 0xFFF0FFFF;
	static const uint32_t beige                                        = 0xFFF5F5DC;
	static const uint32_t bisque                                       = 0xFFFFE4C4;
	static const uint32_t black                                        = 0xFF000000;
	static const uint32_t blanched_almond                              = 0xFFFFEBCD;
	static const uint32_t blue                                         = 0xFF0000FF;
	static const uint32_t blue_violet                                  = 0xFF8A2BE2;
	static const uint32_t brown                                        = 0xFFA52A2A;
	static const uint32_t burly_wood                                   = 0xFFDEB887;
	static const uint32_t cadet_blue                                   = 0xFF5F9EA0;
	static const uint32_t chartreuse                                   = 0xFF7FFF00;
	static const uint32_t chocolate                                    = 0xFFD2691E;
	static const uint32_t coral                                        = 0xFFFF7F50;
	static const uint32_t cornflower_blue                              = 0xFF6495ED;
	static const uint32_t cornsilk                                     = 0xFFFFF8DC;
	static const uint32_t crimson                                      = 0xFFDC143C;
	static const uint32_t cyan                                         = 0xFF00FFFF;
	static const uint32_t dark_blue                                    = 0xFF00008B;
	static const uint32_t dark_cyan                                    = 0xFF008B8B;
	static const uint32_t dark_goldenrod                               = 0xFFB8860B;
	static const uint32_t dark_gray                                    = 0xFFA9A9A9;
	static const uint32_t dark_green                                   = 0xFF006400;
	static const uint32_t dark_khaki                                   = 0xFFBDB76B;
	static const uint32_t dark_magenta                                 = 0xFF8B008B;
	static const uint32_t dark_olive_green                             = 0xFF556B2F;
	static const uint32_t dark_orange                                  = 0xFFFF8C00;
	static const uint32_t dark_orchid                                  = 0xFF9932CC;
	static const uint32_t dark_red                                     = 0xFF8B0000;
	static const uint32_t dark_salmon                                  = 0xFFE9967A;
	static const uint32_t dark_sea_green                               = 0xFF8FBC8F;
	static const uint32_t dark_slate_blue                              = 0xFF483D8B;
	static const uint32_t dark_slate_gray                              = 0xFF2F4F4F;
	static const uint32_t dark_turquoise                               = 0xFF00CED1;
	static const uint32_t dark_violet                                  = 0xFF9400D3;
	static const uint32_t deep_pink                                    = 0xFFFF1493;
	static const uint32_t deep_sky_blue                                = 0xFF00BFFF;
	static const uint32_t dim_gray                                     = 0xFF696969;
	static const uint32_t dodger_blue                                  = 0xFF1E90FF;
	static const uint32_t fire_brick                                   = 0xFFB22222;
	static const uint32_t floral_white                                 = 0xFFFFFAF0;
	static const uint32_t forest_green                                 = 0xFF228B22;
	static const uint32_t fuchsia                                      = 0xFFFF00FF;
	static const uint32_t gainsboro                                    = 0xFFDCDCDC;
	static const uint32_t ghost_white                                  = 0xFFF8F8FF;
	static const uint32_t gold                                         = 0xFFFFD700;
	static const uint32_t goldenrod                                    = 0xFFDAA520;
	static const uint32_t gray                                         = 0xFF808080;
	static const uint32_t green                                        = 0xFF008000;
	static const uint32_t green_yellow                                 = 0xFFADFF2F;
	static const uint32_t honey_dew                                    = 0xFFF0FFF0;
	static const uint32_t hot_pink                                     = 0xFFFF69B4;
	static const uint32_t indian_red                                   = 0xFFCD5C5C;
	static const uint32_t indigo                                       = 0xFF4B0082;
	static const uint32_t ivory                                        = 0xFFFFFFF0;
	static const uint32_t khaki                                        = 0xFFF0E68C;
	static const uint32_t lavender                                     = 0xFFE6E6FA;
	static const uint32_t lavender_lush                                = 0xFFFFF0F5;
	static const uint32_t lawn_green                                   = 0xFF7CFC00;
	static const uint32_t lemon_chiffon                                = 0xFFFFFACD;
	static const uint32_t light_blue                                   = 0xFFADD8E6;
	static const uint32_t light_coral                                  = 0xFFF08080;
	static const uint32_t light_cyan                                   = 0xFFE0FFFF;
	static const uint32_t light_goldenrod_yellow                       = 0xFFFAFAD2;
	static const uint32_t light_gray                                   = 0xFFD3D3D3;
	static const uint32_t light_green                                  = 0xFF90EE90;
	static const uint32_t light_pink                                   = 0xFFFFB6C1;
	static const uint32_t light_salmon                                 = 0xFFFFA07A;
	static const uint32_t light_sea_green                              = 0xFF20B2AA;
	static const uint32_t light_sky_blue                               = 0xFF87CEFA;
	static const uint32_t light_slate_gray                             = 0xFF778899;
	static const uint32_t light_steel_blue                             = 0xFFB0C4DE;
	static const uint32_t light_yellow                                 = 0xFFFFFFE0;
	static const uint32_t lime                                         = 0xFF00FF00;
	static const uint32_t lime_green                                   = 0xFF32CD32;
	static const uint32_t linen                                        = 0xFFFAF0E6;
	static const uint32_t magenta                                      = 0xFFFF00FF;
	static const uint32_t maroon                                       = 0xFF800000;
	static const uint32_t medium_aquamarine                            = 0xFF66CDAA;
	static const uint32_t medium_blue                                  = 0xFF0000CD;
	static const uint32_t medium_orchid                                = 0xFFBA55D3;
	static const uint32_t medium_purple                                = 0xFF9370D8;
	static const uint32_t medium_sea_green                             = 0xFF3CB371;
	static const uint32_t medium_slate_blue                            = 0xFF7B68EE;
	static const uint32_t medium_spring_green                          = 0xFF00FA9A;
	static const uint32_t medium_turquoise                             = 0xFF48D1CC;
	static const uint32_t medium_violet_red                            = 0xFFC71585;
	static const uint32_t midnight_blue                                = 0xFF191970;
	static const uint32_t mint_cream                                   = 0xFFF5FFFA;
	static const uint32_t misty_rose                                   = 0xFFFFE4E1;
	static const uint32_t moccasin                                     = 0xFFFFE4B5;
	static const uint32_t navajo_white                                 = 0xFFFFDEAD;
	static const uint32_t navy                                         = 0xFF000080;
	static const uint32_t old_lace                                     = 0xFFFDF5E6;
	static const uint32_t olive                                        = 0xFF808000;
	static const uint32_t olive_drab                                   = 0xFF6B8E23;
	static const uint32_t orange                                       = 0xFFFFA500;
	static const uint32_t orange_red                                   = 0xFFFF4500;
	static const uint32_t orchid                                       = 0xFFDA70D6;
	static const uint32_t pale_goldenrod                               = 0xFFEEE8AA;
	static const uint32_t pale_green                                   = 0xFF98FB98;
	static const uint32_t pale_turquoise                               = 0xFFAFEEEE;
	static const uint32_t pale_violet_red                              = 0xFFD87093;
	static const uint32_t papaya_whip                                  = 0xFFFFEFD5;
	static const uint32_t peach_puff                                   = 0xFFFFDAB9;
	static const uint32_t peru                                         = 0xFFCD853F;
	static const uint32_t pink                                         = 0xFFFFC0CB;
	static const uint32_t plum                                         = 0xFFDDA0DD;
	static const uint32_t powder_blue                                  = 0xFFB0E0E6;
	static const uint32_t purple                                       = 0xFF800080;
	static const uint32_t red                                          = 0xFFFF0000;
	static const uint32_t rosy_brown                                   = 0xFFBC8F8F;
	static const uint32_t royal_blue                                   = 0xFF4169E1;
	static const uint32_t saddle_brown                                 = 0xFF8B4513;
	static const uint32_t salmon                                       = 0xFFFA8072;
	static const uint32_t sandy_brown                                  = 0xFFF4A460;
	static const uint32_t sea_green                                    = 0xFF2E8B57;
	static const uint32_t sea_shell                                    = 0xFFFFF5EE;
	static const uint32_t sienna                                       = 0xFFA0522D;
	static const uint32_t silver                                       = 0xFFC0C0C0;
	static const uint32_t sky_blue                                     = 0xFF87CEEB;
	static const uint32_t slate_blue                                   = 0xFF6A5ACD;
	static const uint32_t slate_gray                                   = 0xFF708090;
	static const uint32_t snow                                         = 0xFFFFFAFA;
	static const uint32_t spring_green                                 = 0xFF00FF7F;
	static const uint32_t steel_blue                                   = 0xFF4682B4;
	static const uint32_t tan                                          = 0xFFD2B48C;
	static const uint32_t teal                                         = 0xFF008080;
	static const uint32_t thistle                                      = 0xFFD8BFD8;
	static const uint32_t tomato                                       = 0xFFFF6347;
	static const uint32_t turquoise                                    = 0xFF40E0D0;
	static const uint32_t violet                                       = 0xFFEE82EE;
	static const uint32_t wheat                                        = 0xFFF5DEB3;
	static const uint32_t white                                        = 0xFFFFFFFF;
	static const uint32_t white_smoke                                  = 0xFFF5F5F5;
	static const uint32_t yellow                                       = 0xFFFFFF00;
	static const uint32_t yellow_green                                 = 0xFF9ACD32;
}

namespace console_color
{
	static const uint8_t black        = 0;
	static const uint8_t blue         = 1;
	static const uint8_t green        = 2;
	static const uint8_t aqua         = 3;
	static const uint8_t red          = 4;
	static const uint8_t purple       = 5;
	static const uint8_t yellow       = 6;
	static const uint8_t white        = 7;
	static const uint8_t gray         = 8;
	static const uint8_t light_blue   = 9;
	static const uint8_t light_green  = 10;
	static const uint8_t light_aqua   = 11;
	static const uint8_t light_red    = 12;
	static const uint8_t light_purple = 13;
	static const uint8_t light_yellow = 14;
	static const uint8_t bright_white = 15;
}

union argb_channels
{
	uint32_t argb;
	struct
	{
		uint8_t b;
		uint8_t g;
		uint8_t r;
		uint8_t a;
	};
};

}
