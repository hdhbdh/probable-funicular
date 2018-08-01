/*
	FBInk: FrameBuffer eInker, a tool to print text & images on eInk devices (Kobo/Kindle)
	Copyright (C) 2018 NiLuJe <ninuje@gmail.com>

	----

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Affero General Public License as
	published by the Free Software Foundation, either version 3 of the
	License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Affero General Public License for more details.

	You should have received a copy of the GNU Affero General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// Build w/ ${CROSS_TC}-gcc ${RICE_CFLAGS} -Wall -Wextra -s tools/button_scan.c -o button_scan
// (After a setopt sh_word_split w/ my ZSH setup).
// (or ${FLAGS[@]} after FLAGS=("${(s: :)RICE_CFLAGS}") or FLAGS=("${(z)RICE_CFLAGS}"))

// NOTE: Don't do this at home. This is a quick and rough POC to have some fun w/
//       https://www.mobileread.com/forums/showpost.php?p=3731967&postcount=12
//       No-one should ever, ever, ever include internal headers/code, I'm just re-using bits of private API to isolate this POC.
#include "../fbink.c"
#include "../fbink_device_id.c"

// FBInk always returns negative values on failure
#define ERRCODE(e) (-(e))

#include <linux/input.h>

// c.f., https://github.com/koreader/koreader-base/pull/468/files
#define SEND_INPUT_EVENT(t, c, v)                                                                                        \
	({                                                                                                               \
		gettimeofday(&ev.time, NULL);                                                                            \
		ev.type  = (t);                                                                                          \
		ev.code  = (c);                                                                                          \
		ev.value = (v);                                                                                          \
		write(ifd, &ev, sizeof(ev));                                                                             \
	})

// Application entry point
int
    main(int argc __attribute__((unused)), char* argv[] __attribute__((unused)))
{
	FBInkConfig fbink_config = { 0U };
	// Enable stderr diagnostics
	fbink_config.is_verbose = true;
	// Enable stdout results
	fbink_config.is_quiet = false;

	// Open framebuffer and keep it around, then setup globals.
	int fbfd = -1;
	if (ERRCODE(EXIT_FAILURE) == (fbfd = fbink_open())) {
		fprintf(stderr, "Failed to open the framebuffer, aborting . . .\n");
		return ERRCODE(EXIT_FAILURE);
	}
	if (fbink_init(fbfd, &fbink_config) == ERRCODE(EXIT_FAILURE)) {
		fprintf(stderr, "Failed to initialize FBInk, aborting . . .\n");
		return ERRCODE(EXIT_FAILURE);
	}

	// mmap the fb if need be...
	if (!g_fbink_isFbMapped) {
		if (memmap_fb(fbfd) != EXIT_SUCCESS) {
			return ERRCODE(EXIT_FAILURE);
		}
	}

	// Wheee! (Default to the proper value on 32bpp FW)
	FBInkColor button_color = { 0xD9, 0xD9, 0xD9 };

	// And handle yet another bit of 16bpp weirdness...
	// NOTE: There *may* be a rounding/conversion error somewhere...
	//       I can vouch for get_pixel_RGB565's accuracy,
	//       and set_pixel_RGB565 looks straightforward enough, so, err, I blame Kobo? :D.
	if (fbink_is_fb_quirky()) {
		button_color.r = 0xDE;
		button_color.g = 0xDB;
		button_color.b = 0xDE;
	}

	unsigned short int x;
	unsigned short int y;
	FBInkColor         color        = { 0U };
	FBInkCoordinates   coords       = { 0U };
	unsigned short int button_width = 0U;
	unsigned short int match_count  = 0U;
	FBInkCoordinates   match_coords = { 0U };
	bool               gotcha       = false;
	unsigned short int j;
	unsigned short int button_height = 0U;

	// DEBUG: Fake a Glo ;).
	/*
	viewWidth  = 758U;
	viewHeight = 1024U;
	*/

	// Centralize the various thresholds we use...
	// NOTE: Depending on the device's DPI & resolution, a button takes between 17% and 20% of the screen's width.
	//       Possibly less on larger resolutions, and more on smaller resolutions, so try to handle everyone in one fell swoop.
	unsigned short int min_target_pixels = (0.125 * viewWidth);
	unsigned short int max_target_pixels = (0.25 * viewWidth);

	// Recap the various settings as computed for this screen...
	ELOG("Button color is expected to be #%hhx%hhx%hhx", button_color.r, button_color.g, button_color.b);
	ELOG("We need to match two buttons each between %hu and %hu pixels wide!", min_target_pixels, max_target_pixels);

	// Only look in the area of the screen where we're likely to find the buttons, both to save some time,
	// and to lower the risk of false positives, as unlikely as that might be.
	unsigned short int min_height = (0.55 * viewHeight);
	unsigned short int max_height = (0.85 * viewHeight);
	unsigned short int min_width  = (0.05 * viewWidth);
	unsigned short int max_width  = (0.80 * viewWidth);
	ELOG("Looking for buttons in a %hux%hu rectangle, from (%hu, %hu) to (%hu, %hu)",
	     (unsigned short int) (max_width - min_width),
	     (unsigned short int) (max_height - min_height),
	     min_width,
	     min_height,
	     max_width,
	     max_height);

	for (y = min_height; y < max_height; y++) {
		if (match_count == 2) {
			// It looks like we found the top of the buttons on the previous line, we can stop looping.
			gotcha = true;
			break;
		}

		// New line, reset counters
		button_width = 0U;
		match_count  = 0U;

		for (x = min_width; x < max_width; x++) {
			coords.x = x;
			coords.y = y;

			// Handle 16bpp rotation (hopefully applies in Nickel, too ;D)
			(*fxpRotateCoords)(&coords);
			(*fxpGetPixel)(&coords, &color);

			if (color.r == button_color.r && color.g == button_color.g && color.b == button_color.b) {
				// Found a pixel of the right color for a button...
				button_width++;
			} else {
				// Pixel is no longer part of a button...
				if (button_width >= min_target_pixels && button_width <= max_target_pixels) {
					// But we've just finished matching enough pixels in a row to assume we found a button!
					match_count++;
					ELOG("End of match %hu after %hu consecutive matches @ (%hu, %hu)",
					     match_count,
					     button_width,
					     x,
					     y);
					// We only care about the second button, Connect :).
					if (match_count == 2) {
						match_coords.y = y;
						// Last good pixel was the previous one, store that one ;).
						match_coords.x = x - 1;
						// We've got the top-right corner of the Connect button, stop looping.
						break;
					}
				} else {
					if (button_width > 0U) {
						// And we only matched a few stray pixels of the right color before, not a button.
						ELOG("Failed end of match after %hu consecutive matches @ (%hu, %hu)",
						     button_width,
						     x,
						     y);
					}
				}
				// In any case, wrong color, reset the counter.
				button_width = 0U;
			}
		}
	}

	// If we've got a button corner in the previous pass, we're not quite done yet...
	if (gotcha) {
		gotcha   = false;
		coords.x = match_coords.x;
		// We're just going too scan down that final column of the button until we hit the end of it :).
		for (j = match_coords.y; j < max_height; j++) {
			coords.y = j;

			(*fxpRotateCoords)(&coords);
			(*fxpGetPixel)(&coords, &color);

			if (color.r == button_color.r && color.g == button_color.g && color.b == button_color.b) {
				// Found a pixel of the right color for a button...
				button_height++;
			} else {
				// Pixel is no longer part of a button,
				// which likely means we've now hit the bottom-right of the Connect button.
				// NOTE: No more guesses, assume we *really* got the corner of the button earlier.
				// Backtrack from half the height & half the width to get the center of the button.
				match_coords.y = j - (button_height / 2U);
				match_coords.x -= (button_width / 2U);
				// And we're done!
				gotcha = true;
				break;
			}
		}
	}

	if (gotcha) {
		ELOG("Matched on a %hux%hu button! :)", button_width, button_height);

		// The touch panel has a fixed origin that differs from the framebuffer's... >_<".
		rotate_coordinates(&match_coords);
		LOG("x=%hu, y=%hu", match_coords.x, match_coords.y);

		// NOTE: The H2O²r1 is a special snowflake, input is rotated 90° in the *other* direction
		//       (i.e., origin at the bottom-left instead of top-right).
		//       Hopefully that doesn't apply to the fb itself, too...
		LOG("H2O²r1: x=%hu, y=%hu",
		    (unsigned short int) (viewHeight - match_coords.x - 1),
		    (unsigned short int) (viewWidth - match_coords.y - 1));

		// Press it if TOUCH_ME is in the env...
		if (getenv("TOUCH_ME") != NULL) {
			ELOG("Pressing the button . . .");
			struct input_event ev;
			int                ifd = -1;
			ifd                    = open("/dev/input/event1", O_WRONLY | O_NONBLOCK);
			if (ifd == -1) {
				ELOG("Failed to open input device!");
				return ERRCODE(EXIT_FAILURE);
			}

			// NOTE: May not be completely right for every model... (OK on H2O)
			//       Double-check on your device w/ hexdump -x /dev/input/event1 (or -d if you prefer decimal).
			SEND_INPUT_EVENT(EV_ABS, ABS_MT_TRACKING_ID, 1);
			SEND_INPUT_EVENT(EV_ABS, ABS_MT_TOUCH_MAJOR, 1);
			SEND_INPUT_EVENT(EV_ABS, ABS_MT_WIDTH_MAJOR, 1);
			SEND_INPUT_EVENT(EV_ABS, ABS_MT_POSITION_X, match_coords.x);
			SEND_INPUT_EVENT(EV_ABS, ABS_MT_POSITION_Y, match_coords.y);
			SEND_INPUT_EVENT(EV_SYN, SYN_MT_REPORT, 0);
			SEND_INPUT_EVENT(EV_SYN, SYN_REPORT, 0);

			SEND_INPUT_EVENT(EV_ABS, ABS_MT_TRACKING_ID, 1);
			SEND_INPUT_EVENT(EV_ABS, ABS_MT_TOUCH_MAJOR, 0);
			SEND_INPUT_EVENT(EV_ABS, ABS_MT_WIDTH_MAJOR, 0);
			SEND_INPUT_EVENT(EV_ABS, ABS_MT_POSITION_X, match_coords.x);
			SEND_INPUT_EVENT(EV_ABS, ABS_MT_POSITION_Y, match_coords.y);
			SEND_INPUT_EVENT(EV_SYN, SYN_MT_REPORT, 0);
			SEND_INPUT_EVENT(EV_SYN, SYN_REPORT, 0);

			close(ifd);
		}
	} else {
		ELOG("No match :(");
	}

	// Cleanup
	if (g_fbink_isFbMapped) {
		munmap(g_fbink_fbp, g_fbink_screensize);
	}
	close(fbfd);

	return EXIT_SUCCESS;
}
