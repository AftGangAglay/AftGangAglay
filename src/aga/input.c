/*
 * SPDX-License-Identifier: X11
 * Copyright (C) 2025 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <aga/input.h>

#include <mil/translate.h>
#include <mil/widget.h>

void aga_wrap_pointer(
		struct mil_ctx* mil, mil_widget_t widget, struct aga_pointer* pointer,
		int width, int height) {

	if(pointer->dirty) {
		/* TODO: Configurable. */
		static const int edge_tolerance = 10;

		pointer->dirty = ASYS_FALSE;

		if(pointer->captured &&
		   !pointer->warping) {

			int x, y;

			if(pointer->x > width - edge_tolerance) {
				pointer->warping = ASYS_TRUE;
				pointer->warp_gt = ASYS_TRUE;
				pointer->warp_x = ASYS_TRUE;

				x = pointer->warp_coord = edge_tolerance * 2;
				y = pointer->y;
			}
			else if(pointer->x < edge_tolerance) {
				pointer->warping = ASYS_TRUE;
				pointer->warp_gt = ASYS_FALSE;
				pointer->warp_x = ASYS_TRUE;

				x = pointer->warp_coord = width - (edge_tolerance * 2);
				y = pointer->y;
			}
			else if(pointer->y > height - edge_tolerance) {
				pointer->warping = ASYS_TRUE;
				pointer->warp_gt = ASYS_TRUE;
				pointer->warp_x = ASYS_FALSE;

				x = pointer->x;
				y = pointer->warp_coord = edge_tolerance * 2;
			}
			else if(pointer->y < edge_tolerance) {
				pointer->warping = ASYS_TRUE;
				pointer->warp_gt = ASYS_FALSE;
				pointer->warp_x = ASYS_FALSE;

				x = pointer->x;
				y = pointer->warp_coord = height - (edge_tolerance * 2);
			}

			if(pointer->warping) {
				mil_widget_move_pointer(mil, widget, x, y);
			}
		}

		if(pointer->dx > width / 2 || pointer->dx < -width / 2) {
			pointer->dx = 0;
		}

		if(pointer->dy > height / 2 || pointer->dy < -height / 2) {
			pointer->dy = 0;
		}
	}
	else {
		pointer->dx = 0;
		pointer->dy = 0;
	}
}

void aga_translate_mil_input(
		struct mil_input_data* input, struct aga_input_pack* out) {

	switch(input->type) {
		default: return;

		case MIL_INPUT_MOTION: {
			struct mil_motion_input* motion = &input->data.motion;

			int old_x = out->pointer.x;
			int old_y = out->pointer.y;

			out->pointer.x = motion->x;
			out->pointer.y = motion->y;

			if(out->pointer.warping) {
				if(out->pointer.warp_x) {
					if((out->pointer.warp_gt &&
						motion->x >= out->pointer.warp_coord) ||
						motion->x <= out->pointer.warp_coord) {

						out->pointer.warping = ASYS_FALSE;
						break;
					}
				}
				else {
					if((out->pointer.warp_gt &&
						motion->y >= out->pointer.warp_coord) ||
						motion->y <= out->pointer.warp_coord) {

						out->pointer.warping = ASYS_FALSE;
						break;
					}
				}
			}

			out->pointer.dx = motion->x - old_x;
			out->pointer.dy = motion->y - old_y;

			out->pointer.dirty = ASYS_TRUE;

			break;
		}

		case MIL_INPUT_BUTTON: {
			struct mil_button_input* button = &input->data.button;

			enum aga_button out_button;

			switch(button->button) {
				default: return;

				case MIL_BUTTON_MIDDLE: out_button = AGA_BUTTON_MIDDLE; break;
				case MIL_BUTTON_RIGHT: out_button = AGA_BUTTON_RIGHT; break;
				case MIL_BUTTON_LEFT: out_button = AGA_BUTTON_LEFT; break;
			}

			if(button->state == MIL_PRESSED) {
				out->buttons.states[out_button] = AGA_BUTTON_CLICK;
			}
			else {
				out->buttons.states[out_button] = AGA_BUTTON_UP;
			}

			break;
		}

		case MIL_INPUT_KEY: {
			struct mil_key_input* key = &input->data.key;

			if(key->key > AGA_KEY_MAX) break; /* Key out of range. */

			out->keymap.states[key->key] = !!key->state;

			break;
		}
	}
}
