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
#if NETWORK_BACKEND
	struct iio_scan_backend_context *ip_ctx;
#endif
#if USB_BACKEND
	struct iio_scan_backend_context *usb_ctx;
#endif
#if LOCAL_BACKEND
	void (*cb)(const char *, const char *, bool, void *);
	void *user_data;
	bool print_local_context;
#else
	int foo; /* avoid complaints about empty structure */
#endif
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

#if LOCAL_BACKEND
	ctx->cb = cb;
	ctx->user_data = user_data;
	ctx->print_local_context = local_context_possible();
#endif

#if NETWORK_BACKEND
	ctx->ip_ctx = network_scan_create(cb, user_data);
	if (!ctx->ip_ctx && errno != ENOSYS) {
		free(ctx);
		return NULL;
	}
#endif

#if USB_BACKEND
	ctx->usb_ctx = usb_scan_create(cb, user_data);
	if (!ctx->usb_ctx && errno != ENOSYS) {
#if NETWORK_BACKEND
		network_scan_destroy(ctx->ip_ctx);
#endif
		free(ctx);
		return NULL;
	}
#endif

	return ctx;
}

void iio_scan_context_destroy(struct iio_scan_context *ctx)
{
#if NETWORK_BACKEND
	if (ctx->ip_ctx)
		network_scan_destroy(ctx->ip_ctx);
#endif
#if USB_BACKEND
	if (ctx->usb_ctx)
		usb_scan_destroy(ctx->usb_ctx);
#endif
	free(ctx);
}

unsigned int iio_scan_context_poll(struct iio_scan_context *ctx)
{
	unsigned int nb_events = 0;

#if LOCAL_BACKEND
	if (ctx->print_local_context) {
		(*ctx->cb)("local:", "Local devices", true, ctx->user_data);
		ctx->print_local_context = false;
		nb_events++;
	}
#endif

#if NETWORK_BACKEND
	if (ctx->ip_ctx)
		nb_events += network_scan_poll(ctx->ip_ctx);
#endif

#if USB_BACKEND
	if (ctx->usb_ctx)
		nb_events += usb_scan_poll(ctx->usb_ctx);
#endif

	return nb_events;
}
