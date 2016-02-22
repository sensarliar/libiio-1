/*
 * libiio - Library for interfacing industrial I/O (IIO) devices
 *
 * Copyright (C) 2016 Analog Devices, Inc.
 * Author: Paul Cercueil <paul.cercueil@analog.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

#include "iio-private.h"

#include <errno.h>

struct iio_scan_context {
	int foo; /* avoid complaints about empty structure */
};

struct iio_scan_context * iio_create_scan_context(
		void (*cb)(const char *uri, const char *description,
			bool connected, void *user_data),
		void *user_data)
{
	struct iio_scan_context *ctx = malloc(sizeof(*ctx));

	if (!ctx) {
		errno = ENOMEM;
		return NULL;
	}

	return ctx;
}

void iio_scan_context_destroy(struct iio_scan_context *ctx)
{
	free(ctx);
}

bool iio_scan_context_poll(struct iio_scan_context *ctx)
{
	return false;
}
