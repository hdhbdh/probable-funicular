/*
	FBInk: FrameBuffer eInker, a tool to print text & images on eInk devices (Kobo/Kindle)
	Copyright (C) 2018-2019 NiLuJe <ninuje@gmail.com>

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

// NOTE: Fairly useless piece of code basically just there to test the dump/restore functionality ;).
//       We could arguably plug into stb_image_write to basically reimplement fbgrab...

// Because we're pretty much Linux-bound ;).
#ifndef _GNU_SOURCE
#	define _GNU_SOURCE
#endif

#include "../fbink.h"
#include <linux/fb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>

// We want to return negative values on failure, always
#define ERRCODE(e) (-(e))

// "Small" helper for bitdepth switch... (c.f., fbdepth.c)
static bool
    set_bpp(int fbfd, uint32_t bpp, const FBInkState* restrict fbink_state)
{
	struct fb_var_screeninfo fb_vinfo;
	if (ioctl(fbfd, FBIOGET_VSCREENINFO, &fb_vinfo)) {
		perror("ioctl GET_V");
		return false;
	}
	struct fb_fix_screeninfo fb_finfo;
	if (ioctl(fbfd, FBIOGET_FSCREENINFO, &fb_finfo)) {
		perror("ioctl GET_F");
		return false;
	}

	uint32_t expected_rota = fb_vinfo.rotate;

	fb_vinfo.bits_per_pixel = (uint32_t) bpp;
	if (bpp == 8U) {
		fb_vinfo.grayscale = (uint32_t) 1U;
	} else {
		fb_vinfo.grayscale = (uint32_t) 0U;
	}

	if (fbink_state->ntx_rota_quirk == NTX_ROTA_ALL_INVERTED) {
		// NOTE: This should cover the H2O and the few other devices suffering from the same quirk...
		fb_vinfo.rotate ^= 2;
	} else if (fbink_state->ntx_rota_quirk == NTX_ROTA_ODD_INVERTED) {
		// NOTE: This is for the Forma, which only inverts CW & CCW (i.e., odd numbers)...
		if ((fb_vinfo.rotate & 0x01) == 1) {
			fb_vinfo.rotate ^= 2;
		}
	}

	if (ioctl(fbfd, FBIOPUT_VSCREENINFO, &fb_vinfo)) {
		perror("ioctl PUT_V");
		return false;
	}

	if (fb_vinfo.rotate != expected_rota) {
		// Brute-force it until it matches...
		for (uint32_t i = fb_vinfo.rotate, j = FB_ROTATE_UR; j <= FB_ROTATE_CCW; i = (i + 1U) & 3U, j++) {
			// If we finally got the right orientation, break the loop
			if (fb_vinfo.rotate == expected_rota) {
				break;
			}
			// Do the i -> i + 1 -> i dance to be extra sure...
			// (This is useful on devices where the kernel *always* switches to the invert orientation, c.f., rota.c)
			fb_vinfo.rotate = i;
			if (ioctl(fbfd, FBIOPUT_VSCREENINFO, &fb_vinfo)) {
				perror("ioctl PUT_V");
				return false;
			}

			// Don't do anything extra if that was enough...
			if (fb_vinfo.rotate == expected_rota) {
				continue;
			}
			// Now for i + 1 w/ wraparound, since the valid rotation range is [0..3] (FB_ROTATE_UR to FB_ROTATE_CCW).
			// (i.e., a Portrait/Landscape swap to counteract potential side-effects of a kernel-side mandatory invert)
			uint32_t n      = (i + 1U) & 3U;
			fb_vinfo.rotate = n;
			if (ioctl(fbfd, FBIOPUT_VSCREENINFO, &fb_vinfo)) {
				perror("ioctl PUT_V");
				return false;
			}

			// And back to i, if need be...
			if (fb_vinfo.rotate == expected_rota) {
				continue;
			}
			fb_vinfo.rotate = i;
			if (ioctl(fbfd, FBIOPUT_VSCREENINFO, &fb_vinfo)) {
				perror("ioctl PUT_V");
				return false;
			}
		}
	}

	return true;
}

int
    main(void)
{
	// Setup FBInk
	FBInkConfig fbink_cfg = { 0 };
	FBInkDump   dump      = { 0 };
	fbink_cfg.is_verbose  = true;
	// Flash to make stuff more obvious
	fbink_cfg.is_flashing = true;

	// Assume success, until shit happens ;)
	int rv = EXIT_SUCCESS;

	// Init FBInk
	int fbfd = -1;
	// Open framebuffer and keep it around, then setup globals.
	if (ERRCODE(EXIT_FAILURE) == (fbfd = fbink_open())) {
		fprintf(stderr, "Failed to open the framebuffer, aborting . . .\n");
		rv = ERRCODE(EXIT_FAILURE);
		goto cleanup;
	}
	if (fbink_init(fbfd, &fbink_cfg) == ERRCODE(EXIT_FAILURE)) {
		fprintf(stderr, "Failed to initialize FBInk, aborting . . .\n");
		rv = ERRCODE(EXIT_FAILURE);
		goto cleanup;
	}

	// Dump
	if (fbink_dump(fbfd, &dump) != ERRCODE(EXIT_SUCCESS)) {
		fprintf(stderr, "Failed to dump fb, aborting . . .\n");
		rv = ERRCODE(EXIT_FAILURE);
		goto cleanup;
	}

	// Print random crap
	fbink_cfg.is_centered = true;
	fbink_cfg.is_padded   = true;
	fbink_cfg.is_halfway  = true;
	// Inverted to make region restore easier to spot
	fbink_cfg.is_inverted = true;
	if (fbink_print(fbfd, "Wheeee!", &fbink_cfg) < 0) {
		fprintf(stderr, "Failed to print!\n");
		rv = ERRCODE(EXIT_FAILURE);
		goto cleanup;
	}

	// Restore
	if (fbink_restore(fbfd, &fbink_cfg, &dump) != ERRCODE(EXIT_SUCCESS)) {
		fprintf(stderr, "Failed to restore fb, aborting . . .\n");
		rv = ERRCODE(EXIT_FAILURE);
		goto cleanup;
	}

	// Dump a region at the center of the screen, with a few funky offsets to test that
	fbink_cfg.halign = CENTER;
	fbink_cfg.valign = CENTER;
	if (fbink_region_dump(fbfd, -650, -50, 501, 250, &fbink_cfg, &dump) != ERRCODE(EXIT_SUCCESS)) {
		fprintf(stderr, "Failed to dump fb region, aborting . . .\n");
		rv = ERRCODE(EXIT_FAILURE);
		goto cleanup;
	}

	// Print random crap, again
	if (fbink_print(fbfd, "Wheeee!", &fbink_cfg) < 0) {
		fprintf(stderr, "Failed to print!\n");
		rv = ERRCODE(EXIT_FAILURE);
		goto cleanup;
	}

	// Restore, again
	if (fbink_restore(fbfd, &fbink_cfg, &dump) != ERRCODE(EXIT_SUCCESS)) {
		fprintf(stderr, "Failed to restore fb, aborting . . .\n");
		rv = ERRCODE(EXIT_FAILURE);
		goto cleanup;
	}

	// And now for some fun stuff, provided we're starting from a 32bpp fb...
	FBInkState fbink_state = { 0 };
	fbink_get_state(&fbink_cfg, &fbink_state);
	if (fbink_state.bpp == 32U) {
		// Switch to 8bpp (c.f., fbdepth.c)
		if (!set_bpp(fbfd, 8U, &fbink_state)) {
			fprintf(stderr, "Failed to swap bitdepth, aborting . . .\n");
			rv = ERRCODE(EXIT_FAILURE);
			goto cleanup;
		}

		// Re-init fbink so it registers the bitdepth switch
		fbink_reinit(fbfd, &fbink_cfg);

		// Print random crap, once more
		if (fbink_print(fbfd, "Wheeee!", &fbink_cfg) < 0) {
			fprintf(stderr, "Failed to print!\n");
			rv = ERRCODE(EXIT_FAILURE);
			goto cleanup;
		}

		// Watch the restore fail because of a bitdepth mismatch
		if (fbink_restore(fbfd, &fbink_cfg, &dump) != ERRCODE(EXIT_SUCCESS)) {
			fprintf(stderr, "Failed to restore fb, as expected :)\n");
		} else {
			fprintf(stderr, "Err, dump was restored *despite* a bitdepth mismatch ?!\n");
			rv = ERRCODE(EXIT_FAILURE);
			goto cleanup;
		}

		// Then try to ninja restore it via fbink_print_raw_data...
		if (fbink_print_raw_data(fbfd, dump.data, dump.w, dump.h, dump.size, dump.x, dump.y, &fbink_cfg) !=
		    ERRCODE(EXIT_SUCCESS)) {
			fprintf(stderr, "Failed to print raw data!\n");
			rv = ERRCODE(EXIT_FAILURE);
			goto cleanup;
		}

		// Switch back to 32bpp
		if (!set_bpp(fbfd, 32U, &fbink_state)) {
			fprintf(stderr, "Failed to swap bitdepth, aborting . . .\n");
			rv = ERRCODE(EXIT_FAILURE);
			goto cleanup;
		}
	}

	// Cleanup
cleanup:
	// Free potential dump data...
	free(dump.data);
	// Don't leave it dangling so it doesn't get flagged as recyclable.
	dump.data = NULL;

	if (fbink_close(fbfd) == ERRCODE(EXIT_FAILURE)) {
		fprintf(stderr, "Failed to close the framebuffer, aborting . . .\n");
		rv = ERRCODE(EXIT_FAILURE);
	}

	return rv;
}
