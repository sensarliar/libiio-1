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

ssize_t iio_scan_contexts(struct iio_context_info ***info)
{
	struct iio_scan_result scan_result = { 0, NULL };
	int ret;

#if LOCAL_BACKEND
	ret = local_context_scan(&scan_result);
	if (ret < 0)
		goto err_free;
#endif

#if USB_BACKEND
	ret = usb_context_scan(&scan_result);
	if (ret < 0)
		goto err_free;
#endif

	*info = scan_result.info;

	return (ssize_t) scan_result.size;
err_free:
	if (scan_result.info)
		iio_context_info_list_free(scan_result.info);
	return ret;
}

const char * iio_context_info_get_description(
		const struct iio_context_info *info)
{
	return info->description;
}

const char * iio_context_info_get_uri(
		const struct iio_context_info *info)
{
	return info->uri;
}

void iio_context_info_list_free(struct iio_context_info **info)
{
	struct iio_context_info **it;

	for (it = info; *it; it++) {
		if ((*it)->free)
			(*it)->free(*it);
		free(*it);
	}

	free(info);
}

struct iio_context_info **iio_scan_result_add(
	struct iio_scan_result *scan_result, size_t num)
{
	struct iio_context_info **info;
	size_t old_size, new_size;
	size_t i;

	old_size = scan_result->size;
	new_size = old_size + num;

	info = realloc(scan_result->info, (new_size + 1) * sizeof(*info));
	if (!info) {
		scan_result->size = 0;
		free(scan_result->info);
		return NULL;
	}

	for (i = old_size; i < new_size; i++)
		info[i] = zalloc(sizeof(**info));
	info[new_size] = NULL;

	scan_result->info = info;
	scan_result->size = new_size;

	return &info[old_size];
}

struct iio_scan_context {
#if USB_BACKEND
	struct iio_scan_backend_context *usb_ctx;
#else
	int foo; /* avoid complaints about empty structure */
#endif
};

struct iio_scan_context * iio_create_scan_context(
		void (*cb)(const char *uri, bool connected))
{
	struct iio_scan_context *ctx = malloc(sizeof(*ctx));

	if (!ctx) {
		errno = ENOMEM;
		return NULL;
	}

#if USB_BACKEND
	ctx->usb_ctx = usb_scan_create(cb);
#endif

	return ctx;
}

void iio_scan_context_destroy(struct iio_scan_context *ctx)
{
#if USB_BACKEND
	usb_scan_destroy(ctx->usb_ctx);
#endif
	free(ctx);
}

int iio_scan_context_poll(struct iio_scan_context *ctx)
{
#if USB_BACKEND
	int ret = usb_scan_poll(ctx->usb_ctx);
	if (ret)
		return ret;
#endif
	return 0;
}
