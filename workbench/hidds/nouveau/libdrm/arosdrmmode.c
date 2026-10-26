/*
 * Copyright (c) 2007-2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * Copyright (c) 2007-2008 Dave Airlie <airlied@linux.ie>
 * Copyright (c) 2007-2008 Jakob Bornecrantz <wallbraker@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */

#include <libdrm/arosdrm.h>
#include <libdrm/arosdrmmode.h>

#include <string.h>
#include <errno.h>

#define U642VOID(x) ((void *)(unsigned long)(x))
#define VOID2U64(x) ((uint64_t)(unsigned long)(x))

static inline int DRM_IOCTL(int fd, unsigned long cmd, void *arg)
{
	int ret = drmIoctl(fd, cmd, arg);
	return ret;
}

/*
 * Util functions
 */

void* drmAllocCpy(void *array, int count, int entry_size)
{
	char *r;
	int i;

	if (!count || !array || !entry_size)
		return 0;

	if (!(r = drmMalloc(count*entry_size)))
		return 0;

	for (i = 0; i < count; i++)
		memcpy(r+(entry_size*i), array+(entry_size*i), entry_size);

	return r;
}

/*
 * A couple of free functions.
 */

void drmModeFreeModeInfo(drmModeModeInfoPtr ptr)
{
	if (!ptr)
		return;

	drmFree(ptr);
}

void drmModeFreeResources(drmModeResPtr ptr)
{
	if (!ptr)
		return;

	drmFree(ptr);

}

void drmModeFreeCrtc(drmModeCrtcPtr ptr)
{
	if (!ptr)
		return;

	drmFree(ptr);

}

void drmModeFreeConnector(drmModeConnectorPtr ptr)
{
	if (!ptr)
		return;

	drmFree(ptr->encoders);
	drmFree(ptr->prop_values);
	drmFree(ptr->props);
	drmFree(ptr->modes);
	drmFree(ptr);

}

void drmModeFreeEncoder(drmModeEncoderPtr ptr)
{
	drmFree(ptr);
}

/*
 * ModeSetting functions.
 */

drmModeResPtr drmModeGetResources(int fd)
{
	struct drm_mode_card_res res, counts;
	drmModeResPtr r = 0;

retry:
	memset(&res, 0, sizeof(struct drm_mode_card_res));
	if (drmIoctl(fd, DRM_IOCTL_MODE_GETRESOURCES, &res))
		return 0;

	counts = res;

	if (res.count_fbs) {
		res.fb_id_ptr = VOID2U64(drmMalloc(res.count_fbs*sizeof(uint32_t)));
		if (!res.fb_id_ptr)
			goto err_allocs;
	}
	if (res.count_crtcs) {
		res.crtc_id_ptr = VOID2U64(drmMalloc(res.count_crtcs*sizeof(uint32_t)));
		if (!res.crtc_id_ptr)
			goto err_allocs;
	}
	if (res.count_connectors) {
		res.connector_id_ptr = VOID2U64(drmMalloc(res.count_connectors*sizeof(uint32_t)));
		if (!res.connector_id_ptr)
			goto err_allocs;
	}
	if (res.count_encoders) {
		res.encoder_id_ptr = VOID2U64(drmMalloc(res.count_encoders*sizeof(uint32_t)));
		if (!res.encoder_id_ptr)
			goto err_allocs;
	}

	if (drmIoctl(fd, DRM_IOCTL_MODE_GETRESOURCES, &res))
		goto err_allocs;

	/* The number of available connectors and etc may have changed with a
	 * hotplug event in between the ioctls, in which case the field is
	 * silently ignored by the kernel.
	 */
	if (counts.count_fbs < res.count_fbs ||
	    counts.count_crtcs < res.count_crtcs ||
	    counts.count_connectors < res.count_connectors ||
	    counts.count_encoders < res.count_encoders)
	{
		drmFree(U642VOID(res.fb_id_ptr));
		drmFree(U642VOID(res.crtc_id_ptr));
		drmFree(U642VOID(res.connector_id_ptr));
		drmFree(U642VOID(res.encoder_id_ptr));

		goto retry;
	}

	/*
	 * return
	 */
	if (!(r = drmMalloc(sizeof(*r))))
		goto err_allocs;

	r->min_width     = res.min_width;
	r->max_width     = res.max_width;
	r->min_height    = res.min_height;
	r->max_height    = res.max_height;
	r->count_fbs     = res.count_fbs;
	r->count_crtcs   = res.count_crtcs;
	r->count_connectors = res.count_connectors;
	r->count_encoders = res.count_encoders;

	r->fbs        = drmAllocCpy(U642VOID(res.fb_id_ptr), res.count_fbs, sizeof(uint32_t));
	r->crtcs      = drmAllocCpy(U642VOID(res.crtc_id_ptr), res.count_crtcs, sizeof(uint32_t));
	r->connectors = drmAllocCpy(U642VOID(res.connector_id_ptr), res.count_connectors, sizeof(uint32_t));
	r->encoders   = drmAllocCpy(U642VOID(res.encoder_id_ptr), res.count_encoders, sizeof(uint32_t));
	if ((res.count_fbs && !r->fbs) ||
	    (res.count_crtcs && !r->crtcs) ||
	    (res.count_connectors && !r->connectors) ||
	    (res.count_encoders && !r->encoders))
	{
		drmFree(r->fbs);
		drmFree(r->crtcs);
		drmFree(r->connectors);
		drmFree(r->encoders);
		drmFree(r);
		r = 0;
	}

err_allocs:
	drmFree(U642VOID(res.fb_id_ptr));
	drmFree(U642VOID(res.crtc_id_ptr));
	drmFree(U642VOID(res.connector_id_ptr));
	drmFree(U642VOID(res.encoder_id_ptr));

	return r;
}

int drmModeAddFB(int fd, uint32_t width, uint32_t height, uint8_t depth,
                 uint8_t bpp, uint32_t pitch, uint32_t bo_handle,
		 uint32_t *buf_id)
{
	struct drm_mode_fb_cmd f;
	int ret;

	f.width  = width;
	f.height = height;
	f.pitch  = pitch;
	f.bpp    = bpp;
	f.depth  = depth;
	f.handle = bo_handle;

	if ((ret = DRM_IOCTL(fd, DRM_IOCTL_MODE_ADDFB, &f)))
		return ret;

	*buf_id = f.fb_id;
	return 0;
}

int drmModeRmFB(int fd, uint32_t bufferId)
{
	return DRM_IOCTL(fd, DRM_IOCTL_MODE_RMFB, &bufferId);
}

/*
 * Crtc functions
 */

drmModeCrtcPtr drmModeGetCrtc(int fd, uint32_t crtcId)
{
	struct drm_mode_crtc crtc;
	drmModeCrtcPtr r;

	crtc.crtc_id = crtcId;

	if (drmIoctl(fd, DRM_IOCTL_MODE_GETCRTC, &crtc))
		return 0;

	/*
	 * return
	 */

	if (!(r = drmMalloc(sizeof(*r))))
		return 0;

	r->crtc_id         = crtc.crtc_id;
	r->x               = crtc.x;
	r->y               = crtc.y;
	r->mode_valid      = crtc.mode_valid;
	if (r->mode_valid)
		memcpy(&r->mode, &crtc.mode, sizeof(struct drm_mode_modeinfo));
	r->buffer_id       = crtc.fb_id;
	r->gamma_size      = crtc.gamma_size;
	return r;
}


int drmModeSetCrtc(int fd, uint32_t crtcId, uint32_t bufferId,
                   uint32_t x, uint32_t y, uint32_t *connectors, int count,
		   drmModeModeInfoPtr mode)
{
	struct drm_mode_crtc crtc;

	crtc.x             = x;
	crtc.y             = y;
	crtc.crtc_id       = crtcId;
	crtc.fb_id         = bufferId;
	crtc.set_connectors_ptr = VOID2U64(connectors);
	crtc.count_connectors = count;
	if (mode) {
	  memcpy(&crtc.mode, mode, sizeof(struct drm_mode_modeinfo));
	  crtc.mode_valid = 1;
	} else
	  crtc.mode_valid = 0;

	return DRM_IOCTL(fd, DRM_IOCTL_MODE_SETCRTC, &crtc);
}

/*
 * Cursor manipulation
 */

int drmModeSetCursor(int fd, uint32_t crtcId, uint32_t bo_handle, uint32_t width, uint32_t height)
{
	struct drm_mode_cursor arg;

	arg.flags = DRM_MODE_CURSOR_BO;
	arg.crtc_id = crtcId;
	arg.width = width;
	arg.height = height;
	arg.handle = bo_handle;

	return DRM_IOCTL(fd, DRM_IOCTL_MODE_CURSOR, &arg);
}

int drmModeMoveCursor(int fd, uint32_t crtcId, int x, int y)
{
	struct drm_mode_cursor arg;

	arg.flags = DRM_MODE_CURSOR_MOVE;
	arg.crtc_id = crtcId;
	arg.x = x;
	arg.y = y;

	return DRM_IOCTL(fd, DRM_IOCTL_MODE_CURSOR, &arg);
}

/*
 * Encoder get
 */
drmModeEncoderPtr drmModeGetEncoder(int fd, uint32_t encoder_id)
{
	struct drm_mode_get_encoder enc;
	drmModeEncoderPtr r = NULL;

	enc.encoder_id = encoder_id;
	enc.encoder_type = 0;
	enc.possible_crtcs = 0;
	enc.possible_clones = 0;

	if (drmIoctl(fd, DRM_IOCTL_MODE_GETENCODER, &enc))
		return 0;

	if (!(r = drmMalloc(sizeof(*r))))
		return 0;

	r->encoder_id = enc.encoder_id;
	r->crtc_id = enc.crtc_id;
	r->encoder_type = enc.encoder_type;
	r->possible_crtcs = enc.possible_crtcs;
	r->possible_clones = enc.possible_clones;

	return r;
}

/*
 * Connector manipulation
 */

drmModeConnectorPtr drmModeGetConnector(int fd, uint32_t connector_id)
{
	struct drm_mode_get_connector conn, counts;
	drmModeConnectorPtr r = NULL;

retry:
	memset(&conn, 0, sizeof(struct drm_mode_get_connector));
	conn.connector_id = connector_id;

	if (drmIoctl(fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn))
		return 0;

	counts = conn;

	if (conn.count_props) {
		conn.props_ptr = VOID2U64(drmMalloc(conn.count_props*sizeof(uint32_t)));
		if (!conn.props_ptr)
			goto err_allocs;
		conn.prop_values_ptr = VOID2U64(drmMalloc(conn.count_props*sizeof(uint64_t)));
		if (!conn.prop_values_ptr)
			goto err_allocs;
	}

	if (conn.count_modes) {
		conn.modes_ptr = VOID2U64(drmMalloc(conn.count_modes*sizeof(struct drm_mode_modeinfo)));
		if (!conn.modes_ptr)
			goto err_allocs;
	}

	if (conn.count_encoders) {
		conn.encoders_ptr = VOID2U64(drmMalloc(conn.count_encoders*sizeof(uint32_t)));
		if (!conn.encoders_ptr)
			goto err_allocs;
	}

	if (drmIoctl(fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn))
		goto err_allocs;

	/* The number of available connectors and etc may have changed with a
	 * hotplug event in between the ioctls, in which case the field is
	 * silently ignored by the kernel.
	 */
	if (counts.count_props < conn.count_props ||
	    counts.count_modes < conn.count_modes ||
	    counts.count_encoders < conn.count_encoders) {
		drmFree(U642VOID(conn.props_ptr));
		drmFree(U642VOID(conn.prop_values_ptr));
		drmFree(U642VOID(conn.modes_ptr));
		drmFree(U642VOID(conn.encoders_ptr));

		goto retry;
	}

	if(!(r = drmMalloc(sizeof(*r)))) {
		goto err_allocs;
	}

	r->connector_id = conn.connector_id;
	r->encoder_id = conn.encoder_id;
	r->connection   = conn.connection;
	r->mmWidth      = conn.mm_width;
	r->mmHeight     = conn.mm_height;
	/* convert subpixel from kernel to userspace */
	r->subpixel     = conn.subpixel + 1;
	r->count_modes  = conn.count_modes;
	r->count_props  = conn.count_props;
	r->props        = drmAllocCpy(U642VOID(conn.props_ptr), conn.count_props, sizeof(uint32_t));
	r->prop_values  = drmAllocCpy(U642VOID(conn.prop_values_ptr), conn.count_props, sizeof(uint64_t));
	r->modes        = drmAllocCpy(U642VOID(conn.modes_ptr), conn.count_modes, sizeof(struct drm_mode_modeinfo));
	r->count_encoders = conn.count_encoders;
	r->encoders     = drmAllocCpy(U642VOID(conn.encoders_ptr), conn.count_encoders, sizeof(uint32_t));
	r->connector_type  = conn.connector_type;
	r->connector_type_id = conn.connector_type_id;

	if ((r->count_props && !r->props) ||
	    (r->count_props && !r->prop_values) ||
	    (r->count_modes && !r->modes) ||
	    (r->count_encoders && !r->encoders)) {
		drmFree(r->props);
		drmFree(r->prop_values);
		drmFree(r->modes);
		drmFree(r->encoders);
		drmFree(r);
		r = 0;
	}

err_allocs:
	drmFree(U642VOID(conn.prop_values_ptr));
	drmFree(U642VOID(conn.props_ptr));
	drmFree(U642VOID(conn.modes_ptr));
	drmFree(U642VOID(conn.encoders_ptr));

	return r;
}

int drmModeAttachMode(int fd, uint32_t connector_id, drmModeModeInfoPtr mode_info)
{
	struct drm_mode_mode_cmd res;

	memcpy(&res.mode, mode_info, sizeof(struct drm_mode_modeinfo));
	res.connector_id = connector_id;

	return DRM_IOCTL(fd, DRM_IOCTL_MODE_ATTACHMODE, &res);
}

int drmModeDetachMode(int fd, uint32_t connector_id, drmModeModeInfoPtr mode_info)
{
	struct drm_mode_mode_cmd res;

	memcpy(&res.mode, mode_info, sizeof(struct drm_mode_modeinfo));
	res.connector_id = connector_id;

	return DRM_IOCTL(fd, DRM_IOCTL_MODE_DETACHMODE, &res);
}

/*
 * Planes, page flips and vblank waits: AROS additions for the display
 * compositor's scan-out planes (the kernel side comes with the DRM core).
 */
drmModePlaneResPtr drmModeGetPlaneResources(int fd)
{
	struct drm_mode_get_plane_res res, counts;
	drmModePlaneResPtr r = 0;

retry:
	memset(&res, 0, sizeof(res));
	if (drmIoctl(fd, DRM_IOCTL_MODE_GETPLANERESOURCES, &res))
		return 0;

	counts = res;

	if (res.count_planes) {
		res.plane_id_ptr = VOID2U64(drmMalloc(res.count_planes * sizeof(uint32_t)));
		if (!res.plane_id_ptr)
			goto err_allocs;
	}

	if (drmIoctl(fd, DRM_IOCTL_MODE_GETPLANERESOURCES, &res))
		goto err_allocs;

	if (counts.count_planes < res.count_planes) {
		drmFree(U642VOID(res.plane_id_ptr));
		goto retry;
	}

	if (!(r = drmMalloc(sizeof(*r))))
		goto err_allocs;

	r->count_planes = res.count_planes;
	r->planes = U642VOID(res.plane_id_ptr);
	return r;

err_allocs:
	drmFree(U642VOID(res.plane_id_ptr));
	return r;
}

void drmModeFreePlaneResources(drmModePlaneResPtr ptr)
{
	if (!ptr)
		return;
	drmFree(ptr->planes);
	drmFree(ptr);
}

drmModePlanePtr drmModeGetPlane(int fd, uint32_t plane_id)
{
	struct drm_mode_get_plane f, ovf;
	drmModePlanePtr r = 0;

retry:
	memset(&f, 0, sizeof(f));
	f.plane_id = plane_id;

	if (drmIoctl(fd, DRM_IOCTL_MODE_GETPLANE, &f))
		return 0;

	ovf = f;

	if (f.count_format_types) {
		f.format_type_ptr = VOID2U64(drmMalloc(f.count_format_types * sizeof(uint32_t)));
		if (!f.format_type_ptr)
			goto err_allocs;
	}

	if (drmIoctl(fd, DRM_IOCTL_MODE_GETPLANE, &f))
		goto err_allocs;

	if (ovf.count_format_types < f.count_format_types) {
		drmFree(U642VOID(f.format_type_ptr));
		goto retry;
	}

	if (!(r = drmMalloc(sizeof(*r))))
		goto err_allocs;

	r->count_formats = f.count_format_types;
	r->plane_id = f.plane_id;
	r->crtc_id = f.crtc_id;
	r->fb_id = f.fb_id;
	r->possible_crtcs = f.possible_crtcs;
	r->gamma_size = f.gamma_size;
	r->formats = U642VOID(f.format_type_ptr);
	return r;

err_allocs:
	drmFree(U642VOID(f.format_type_ptr));
	return r;
}

void drmModeFreePlane(drmModePlanePtr ptr)
{
	if (!ptr)
		return;
	drmFree(ptr->formats);
	drmFree(ptr);
}

int drmModeSetPlane(int fd, uint32_t plane_id, uint32_t crtc_id, uint32_t fb_id, uint32_t flags,
		    int32_t crtc_x, int32_t crtc_y, uint32_t crtc_w, uint32_t crtc_h,
		    uint32_t src_x, uint32_t src_y, uint32_t src_w, uint32_t src_h)
{
	struct drm_mode_set_plane s;

	memset(&s, 0, sizeof(s));
	s.plane_id = plane_id;
	s.crtc_id = crtc_id;
	s.fb_id = fb_id;
	s.flags = flags;
	s.crtc_x = crtc_x;
	s.crtc_y = crtc_y;
	s.crtc_w = crtc_w;
	s.crtc_h = crtc_h;
	s.src_x = src_x;
	s.src_y = src_y;
	s.src_w = src_w;
	s.src_h = src_h;

	return drmIoctl(fd, DRM_IOCTL_MODE_SETPLANE, &s);
}

int drmModePageFlip(int fd, uint32_t crtc_id, uint32_t fb_id, uint32_t flags, void *user_data)
{
	struct drm_mode_crtc_page_flip flip;

	memset(&flip, 0, sizeof(flip));
	flip.fb_id = fb_id;
	flip.crtc_id = crtc_id;
	flip.user_data = VOID2U64(user_data);
	flip.flags = flags;

	return drmIoctl(fd, DRM_IOCTL_MODE_PAGE_FLIP, &flip);
}

int drmWaitVBlank(int fd, drmVBlankPtr vbl)
{
	/* drmVBlank has the layout of the kernel's union drm_wait_vblank. */
	return drmIoctl(fd, DRM_IOCTL_WAIT_VBLANK, vbl);
}

/*
 * Properties, atomic requests and fourcc framebuffers: the same semantics as
 * libdrm's xf86drmMode.c on the kernel's ioctl structures. The item list of
 * an atomic request is sorted by object then property id and duplicates are
 * dropped, as the kernel expects one entry per object.
 */
int drmModeAddFB2(int fd, uint32_t width, uint32_t height, uint32_t pixel_format,
		  const uint32_t bo_handles[4], const uint32_t pitches[4],
		  const uint32_t offsets[4], uint32_t *buf_id, uint32_t flags)
{
	struct drm_mode_fb_cmd2 f;
	int ret;

	memset(&f, 0, sizeof(f));
	f.width  = width;
	f.height = height;
	f.pixel_format = pixel_format;
	f.flags = flags;
	memcpy(f.handles, bo_handles, 4 * sizeof(bo_handles[0]));
	memcpy(f.pitches, pitches, 4 * sizeof(pitches[0]));
	memcpy(f.offsets, offsets, 4 * sizeof(offsets[0]));

	if ((ret = DRM_IOCTL(fd, DRM_IOCTL_MODE_ADDFB2, &f)))
		return ret;

	*buf_id = f.fb_id;
	return 0;
}

drmModePropertyPtr drmModeGetProperty(int fd, uint32_t property_id)
{
	struct drm_mode_get_property prop;
	drmModePropertyPtr r;

	memset(&prop, 0, sizeof(prop));
	prop.prop_id = property_id;

	if (drmIoctl(fd, DRM_IOCTL_MODE_GETPROPERTY, &prop))
		return 0;

	if (prop.count_values)
		prop.values_ptr = VOID2U64(drmMalloc(prop.count_values * sizeof(uint64_t)));

	if (prop.count_enum_blobs && (prop.flags & (DRM_MODE_PROP_ENUM | DRM_MODE_PROP_BITMASK)))
		prop.enum_blob_ptr = VOID2U64(drmMalloc(prop.count_enum_blobs * sizeof(struct drm_mode_property_enum)));

	if (prop.count_enum_blobs && (prop.flags & DRM_MODE_PROP_BLOB)) {
		prop.values_ptr = VOID2U64(drmMalloc(prop.count_enum_blobs * sizeof(uint32_t)));
		prop.enum_blob_ptr = VOID2U64(drmMalloc(prop.count_enum_blobs * sizeof(uint32_t)));
	}

	if (drmIoctl(fd, DRM_IOCTL_MODE_GETPROPERTY, &prop)) {
		r = NULL;
		goto err_allocs;
	}

	if (!(r = drmMalloc(sizeof(*r))))
		goto err_allocs;

	r->prop_id = prop.prop_id;
	r->count_values = prop.count_values;

	r->flags = prop.flags;
	if (prop.count_values)
		r->values = drmAllocCpy(U642VOID(prop.values_ptr), prop.count_values, sizeof(uint64_t));
	if (prop.flags & (DRM_MODE_PROP_ENUM | DRM_MODE_PROP_BITMASK)) {
		r->count_enums = prop.count_enum_blobs;
		r->enums = drmAllocCpy(U642VOID(prop.enum_blob_ptr), prop.count_enum_blobs, sizeof(struct drm_mode_property_enum));
	} else if (prop.flags & DRM_MODE_PROP_BLOB) {
		r->values = drmAllocCpy(U642VOID(prop.values_ptr), prop.count_enum_blobs, sizeof(uint32_t));
		r->blob_ids = drmAllocCpy(U642VOID(prop.enum_blob_ptr), prop.count_enum_blobs, sizeof(uint32_t));
		r->count_blobs = prop.count_enum_blobs;
	}
	strncpy(r->name, prop.name, DRM_PROP_NAME_LEN);
	r->name[DRM_PROP_NAME_LEN - 1] = 0;

err_allocs:
	drmFree(U642VOID(prop.values_ptr));
	drmFree(U642VOID(prop.enum_blob_ptr));

	return r;
}

void drmModeFreeProperty(drmModePropertyPtr ptr)
{
	if (!ptr)
		return;

	drmFree(ptr->values);
	drmFree(ptr->enums);
	drmFree(ptr->blob_ids);
	drmFree(ptr);
}

drmModeObjectPropertiesPtr drmModeObjectGetProperties(int fd, uint32_t object_id, uint32_t object_type)
{
	struct drm_mode_obj_get_properties properties;
	drmModeObjectPropertiesPtr ret = NULL;
	uint32_t count;

retry:
	memset(&properties, 0, sizeof(properties));
	properties.obj_id = object_id;
	properties.obj_type = object_type;

	if (drmIoctl(fd, DRM_IOCTL_MODE_OBJ_GETPROPERTIES, &properties))
		return 0;

	count = properties.count_props;

	if (count) {
		properties.props_ptr = VOID2U64(drmMalloc(count * sizeof(uint32_t)));
		if (!properties.props_ptr)
			goto err_allocs;
		properties.prop_values_ptr = VOID2U64(drmMalloc(count * sizeof(uint64_t)));
		if (!properties.prop_values_ptr)
			goto err_allocs;
	}

	if (drmIoctl(fd, DRM_IOCTL_MODE_OBJ_GETPROPERTIES, &properties))
		goto err_allocs;

	if (count < properties.count_props) {
		drmFree(U642VOID(properties.props_ptr));
		drmFree(U642VOID(properties.prop_values_ptr));
		goto retry;
	}
	count = properties.count_props;

	ret = drmMalloc(sizeof(*ret));
	if (!ret)
		goto err_allocs;

	ret->count_props = count;
	ret->props = drmAllocCpy(U642VOID(properties.props_ptr), count, sizeof(uint32_t));
	ret->prop_values = drmAllocCpy(U642VOID(properties.prop_values_ptr), count, sizeof(uint64_t));
	if (ret->count_props && (!ret->props || !ret->prop_values)) {
		drmFree(ret->props);
		drmFree(ret->prop_values);
		drmFree(ret);
		ret = NULL;
	}

err_allocs:
	drmFree(U642VOID(properties.props_ptr));
	drmFree(U642VOID(properties.prop_values_ptr));
	return ret;
}

void drmModeFreeObjectProperties(drmModeObjectPropertiesPtr ptr)
{
	if (!ptr)
		return;
	drmFree(ptr->props);
	drmFree(ptr->prop_values);
	drmFree(ptr);
}

int drmModeObjectSetProperty(int fd, uint32_t object_id, uint32_t object_type,
			     uint32_t property_id, uint64_t value)
{
	struct drm_mode_obj_set_property prop;

	memset(&prop, 0, sizeof(prop));
	prop.value = value;
	prop.prop_id = property_id;
	prop.obj_id = object_id;
	prop.obj_type = object_type;

	return DRM_IOCTL(fd, DRM_IOCTL_MODE_OBJ_SETPROPERTY, &prop);
}

typedef struct _drmModeAtomicReqItem {
	uint32_t object_id;
	uint32_t property_id;
	uint64_t value;
	uint32_t cursor;
} drmModeAtomicReqItem, *drmModeAtomicReqItemPtr;

struct _drmModeAtomicReq {
	uint32_t cursor;
	uint32_t size_items;
	drmModeAtomicReqItemPtr items;
};

#define ATOMIC_ITEMS_GROW 32

drmModeAtomicReqPtr drmModeAtomicAlloc(void)
{
	drmModeAtomicReqPtr req;

	req = drmMalloc(sizeof *req);
	if (!req)
		return NULL;

	req->items = NULL;
	req->cursor = 0;
	req->size_items = 0;

	return req;
}

void drmModeAtomicFree(drmModeAtomicReqPtr req)
{
	if (!req)
		return;

	if (req->items)
		drmFree(req->items);
	drmFree(req);
}

int drmModeAtomicGetCursor(const drmModeAtomicReqPtr req)
{
	if (!req)
		return -EINVAL;
	return req->cursor;
}

void drmModeAtomicSetCursor(drmModeAtomicReqPtr req, int cursor)
{
	if (req)
		req->cursor = cursor;
}

int drmModeAtomicAddProperty(drmModeAtomicReqPtr req, uint32_t object_id,
			     uint32_t property_id, uint64_t value)
{
	if (!req)
		return -EINVAL;

	if (object_id == 0 || property_id == 0)
		return -EINVAL;

	if (req->cursor >= req->size_items) {
		drmModeAtomicReqItemPtr new;

		new = drmMalloc((req->size_items + ATOMIC_ITEMS_GROW) * sizeof(*req->items));
		if (!new)
			return -ENOMEM;
		if (req->items) {
			memcpy(new, req->items, req->cursor * sizeof(*req->items));
			drmFree(req->items);
		}
		req->items = new;
		req->size_items += ATOMIC_ITEMS_GROW;
	}

	req->items[req->cursor].object_id = object_id;
	req->items[req->cursor].property_id = property_id;
	req->items[req->cursor].value = value;
	req->items[req->cursor].cursor = req->cursor;
	req->cursor++;

	return req->cursor;
}

/* object id, then property id, then order of addition (the last one wins) */
static int atomic_item_before(const drmModeAtomicReqItem *a, const drmModeAtomicReqItem *b)
{
	if (a->object_id != b->object_id)
		return a->object_id < b->object_id;
	if (a->property_id != b->property_id)
		return a->property_id < b->property_id;
	return a->cursor < b->cursor;
}

int drmModeAtomicCommit(int fd, const drmModeAtomicReqPtr req, uint32_t flags, void *user_data)
{
	struct drm_mode_atomic atomic;
	drmModeAtomicReqItemPtr sorted;
	uint32_t *objs_ptr = NULL;
	uint32_t *count_props_ptr = NULL;
	uint32_t *props_ptr = NULL;
	uint64_t *prop_values_ptr = NULL;
	uint32_t last_obj_id = 0;
	uint32_t i, j, n;
	int obj_idx = -1;
	int ret = -ENOMEM;

	if (!req)
		return -EINVAL;

	if (req->cursor == 0)
		return 0;

	sorted = drmMalloc(req->cursor * sizeof(*sorted));
	if (!sorted)
		return -ENOMEM;
	memcpy(sorted, req->items, req->cursor * sizeof(*sorted));
	n = req->cursor;

	/* Sort by object id, then property id (insertion sort: the lists are short) */
	for (i = 1; i < n; i++) {
		drmModeAtomicReqItem key = sorted[i];

		for (j = i; j > 0 && atomic_item_before(&key, &sorted[j - 1]); j--)
			sorted[j] = sorted[j - 1];
		sorted[j] = key;
	}

	memset(&atomic, 0, sizeof(atomic));

	/* Drop earlier duplicates of the same object/property, count the objects */
	for (i = 0; i < n; i++) {
		if (sorted[i].object_id != last_obj_id) {
			atomic.count_objs++;
			last_obj_id = sorted[i].object_id;
		}

		if (i == n - 1)
			continue;

		if (sorted[i].object_id != sorted[i + 1].object_id ||
		    sorted[i].property_id != sorted[i + 1].property_id)
			continue;

		memmove(&sorted[i], &sorted[i + 1], (n - i - 1) * sizeof(*sorted));
		n--;
		i--;
	}

	objs_ptr = drmMalloc(atomic.count_objs * sizeof objs_ptr[0]);
	count_props_ptr = drmMalloc(atomic.count_objs * sizeof count_props_ptr[0]);
	props_ptr = drmMalloc(n * sizeof props_ptr[0]);
	prop_values_ptr = drmMalloc(n * sizeof prop_values_ptr[0]);
	if (!objs_ptr || !count_props_ptr || !props_ptr || !prop_values_ptr)
		goto out;

	for (i = 0, last_obj_id = 0; i < n; i++) {
		if (sorted[i].object_id != last_obj_id) {
			obj_idx++;
			objs_ptr[obj_idx] = sorted[i].object_id;
			last_obj_id = objs_ptr[obj_idx];
		}

		count_props_ptr[obj_idx]++;
		props_ptr[i] = sorted[i].property_id;
		prop_values_ptr[i] = sorted[i].value;
	}

	atomic.flags = flags;
	atomic.objs_ptr = VOID2U64(objs_ptr);
	atomic.count_props_ptr = VOID2U64(count_props_ptr);
	atomic.props_ptr = VOID2U64(props_ptr);
	atomic.prop_values_ptr = VOID2U64(prop_values_ptr);
	atomic.user_data = VOID2U64(user_data);

	ret = DRM_IOCTL(fd, DRM_IOCTL_MODE_ATOMIC, &atomic);

out:
	drmFree(objs_ptr);
	drmFree(count_props_ptr);
	drmFree(props_ptr);
	drmFree(prop_values_ptr);
	drmFree(sorted);

	return ret;
}
