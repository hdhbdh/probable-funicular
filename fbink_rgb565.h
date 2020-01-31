/*
* C Header for use with https://github.com/NiLuJe/FBInk
* c.f., FBInk's tools/rgb565_lut.py
*/

static const uint16_t y8ToRGB565[256] = {
	0x0000u, 0x0000u, 0x0000u, 0x0000u, 0x0020u, 0x0020u, 0x0020u, 0x0020u, 0x0841u, 0x0841u, 0x0841u, 0x0841u,
	0x0861u, 0x0861u, 0x0861u, 0x0861u, 0x1082u, 0x1082u, 0x1082u, 0x1082u, 0x10a2u, 0x10a2u, 0x10a2u, 0x10a2u,
	0x18c3u, 0x18c3u, 0x18c3u, 0x18c3u, 0x18e3u, 0x18e3u, 0x18e3u, 0x18e3u, 0x2104u, 0x2104u, 0x2104u, 0x2104u,
	0x2124u, 0x2124u, 0x2124u, 0x2124u, 0x2945u, 0x2945u, 0x2945u, 0x2945u, 0x2965u, 0x2965u, 0x2965u, 0x2965u,
	0x3186u, 0x3186u, 0x3186u, 0x3186u, 0x31a6u, 0x31a6u, 0x31a6u, 0x31a6u, 0x39c7u, 0x39c7u, 0x39c7u, 0x39c7u,
	0x39e7u, 0x39e7u, 0x39e7u, 0x39e7u, 0x4208u, 0x4208u, 0x4208u, 0x4208u, 0x4228u, 0x4228u, 0x4228u, 0x4228u,
	0x4a49u, 0x4a49u, 0x4a49u, 0x4a49u, 0x4a69u, 0x4a69u, 0x4a69u, 0x4a69u, 0x528au, 0x528au, 0x528au, 0x528au,
	0x52aau, 0x52aau, 0x52aau, 0x52aau, 0x5acbu, 0x5acbu, 0x5acbu, 0x5acbu, 0x5aebu, 0x5aebu, 0x5aebu, 0x5aebu,
	0x630cu, 0x630cu, 0x630cu, 0x630cu, 0x632cu, 0x632cu, 0x632cu, 0x632cu, 0x6b4du, 0x6b4du, 0x6b4du, 0x6b4du,
	0x6b6du, 0x6b6du, 0x6b6du, 0x6b6du, 0x738eu, 0x738eu, 0x738eu, 0x738eu, 0x73aeu, 0x73aeu, 0x73aeu, 0x73aeu,
	0x7bcfu, 0x7bcfu, 0x7bcfu, 0x7bcfu, 0x7befu, 0x7befu, 0x7befu, 0x7befu, 0x8410u, 0x8410u, 0x8410u, 0x8410u,
	0x8430u, 0x8430u, 0x8430u, 0x8430u, 0x8c51u, 0x8c51u, 0x8c51u, 0x8c51u, 0x8c71u, 0x8c71u, 0x8c71u, 0x8c71u,
	0x9492u, 0x9492u, 0x9492u, 0x9492u, 0x94b2u, 0x94b2u, 0x94b2u, 0x94b2u, 0x9cd3u, 0x9cd3u, 0x9cd3u, 0x9cd3u,
	0x9cf3u, 0x9cf3u, 0x9cf3u, 0x9cf3u, 0xa514u, 0xa514u, 0xa514u, 0xa514u, 0xa534u, 0xa534u, 0xa534u, 0xa534u,
	0xad55u, 0xad55u, 0xad55u, 0xad55u, 0xad75u, 0xad75u, 0xad75u, 0xad75u, 0xb596u, 0xb596u, 0xb596u, 0xb596u,
	0xb5b6u, 0xb5b6u, 0xb5b6u, 0xb5b6u, 0xbdd7u, 0xbdd7u, 0xbdd7u, 0xbdd7u, 0xbdf7u, 0xbdf7u, 0xbdf7u, 0xbdf7u,
	0xc618u, 0xc618u, 0xc618u, 0xc618u, 0xc638u, 0xc638u, 0xc638u, 0xc638u, 0xce59u, 0xce59u, 0xce59u, 0xce59u,
	0xce79u, 0xce79u, 0xce79u, 0xce79u, 0xd69au, 0xd69au, 0xd69au, 0xd69au, 0xd6bau, 0xd6bau, 0xd6bau, 0xd6bau,
	0xdedbu, 0xdedbu, 0xdedbu, 0xdedbu, 0xdefbu, 0xdefbu, 0xdefbu, 0xdefbu, 0xe71cu, 0xe71cu, 0xe71cu, 0xe71cu,
	0xe73cu, 0xe73cu, 0xe73cu, 0xe73cu, 0xef5du, 0xef5du, 0xef5du, 0xef5du, 0xef7du, 0xef7du, 0xef7du, 0xef7du,
	0xf79eu, 0xf79eu, 0xf79eu, 0xf79eu, 0xf7beu, 0xf7beu, 0xf7beu, 0xf7beu, 0xffdfu, 0xffdfu, 0xffdfu, 0xffdfu,
	0xffffu, 0xffffu, 0xffffu, 0xffffu
};