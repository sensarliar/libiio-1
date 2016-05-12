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

unsigned int iio_scan_context_poll(struct iio_scan_context *ctx,
		unsigned int timeout_ms)
{
	unsigned int nb_events = 0;

#if LOCAL_BACKEND
	if (ctx->print_local_context) {
		(*ctx->cb)("local:", "Local devices", true, ctx->user_data);
		ctx->print_local_context = false;
		nb_events++;
	}
#endif

#if USB_BACKEND && NETWORK_BACKEND
	/* First poll with a timeout of 0, to return immediately if an event is
	 * found; otherwise, in the case that we have an event on the network
	 * backend and none on the USB backend, we would wait unnecessarily
	 * for the timeout to expire. */
	if (ctx->usb_ctx)
		nb_events += usb_scan_poll(ctx->usb_ctx, 0);
	if (ctx->ip_ctx)
		nb_events += network_scan_poll(ctx->ip_ctx, 0);
#endif

	/* If we had events so far, without blocking, return now */
	if (nb_events)
		return nb_events;

#if USB_BACKEND
	if (ctx->usb_ctx) {
		nb_events += usb_scan_poll(ctx->usb_ctx, timeout_ms);

		/* We timed out - we don't need to wait in the other backends */
		if (!nb_events)
			timeout_ms = 0;
	}
#endif

#if NETWORK_BACKEND
	if (ctx->ip_ctx)
		nb_events += network_scan_poll(ctx->ip_ctx, timeout_ms);
#endif

	return nb_events;
}
