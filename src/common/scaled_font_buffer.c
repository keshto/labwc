// SPDX-License-Identifier: GPL-2.0-only
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-server-core.h>
#include <wlr/util/log.h>
#include "buffer.h"
#include "common/font.h"
#include "common/scaled_scene_buffer.h"
#include "common/scaled_font_buffer.h"
#include "common/zfree.h"

static struct lab_data_buffer *
_create_buffer(struct scaled_scene_buffer *scaled_buffer, double scale)
{
	struct lab_data_buffer *buffer;
	struct scaled_font_buffer *self = scaled_buffer->data;

	/* Buffer gets free'd automatically along the backing wlr_buffer */
	font_buffer_create(&buffer, self->max_width, self->text,
		&self->font, self->color, scale);

	self->width = buffer ? buffer->unscaled_width : 0;
	self->height = buffer ? buffer->unscaled_height : 0;
	return buffer;
}

static void
_destroy(struct scaled_scene_buffer *scaled_buffer)
{
	struct scaled_font_buffer *self = scaled_buffer->data;
	if (self->text) {
		zfree(self->text);
	}
	if (self->font.name) {
		zfree(self->font.name);
	}
	zfree(scaled_buffer->data);
}

static const struct scaled_scene_buffer_impl impl = {
	.create_buffer = _create_buffer,
	.destroy = _destroy
};

/* Public API */
struct scaled_font_buffer *
scaled_font_buffer_create(struct wlr_scene_tree *parent)
{
	assert(parent);
	struct scaled_font_buffer *self = calloc(1, sizeof(*self));
	if (!self) {
		return NULL;
	}

	struct scaled_scene_buffer *scaled_buffer
		= scaled_scene_buffer_create(parent, &impl);
	if (!scaled_buffer) {
		free(self);
		return NULL;
	}

	scaled_buffer->data = self;
	self->scaled_buffer = scaled_buffer;
	self->scene_buffer = scaled_buffer->scene_buffer;
	return self;
}

void
scaled_font_buffer_update(struct scaled_font_buffer *self,
		const char *text, int max_width, struct font *font, float *color)
{
	assert(self);
	assert(text);
	assert(font);
	assert(color);

	/* Clean up old internal state */
	if (self->text) {
		zfree(self->text);
	}
	if (self->font.name) {
		zfree(self->font.name);
	}

	/* Update internal state */
	self->text = strdup(text);
	self->max_width = max_width;
	if (font->name) {
		self->font.name = strdup(font->name);
	}
	self->font.size = font->size;
	memcpy(self->color, color, sizeof(self->color));

	/* Invalidate cache and force a new render */
	scaled_scene_buffer_invalidate_cache(self->scaled_buffer);
}
