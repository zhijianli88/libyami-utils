#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <xf86drm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>
#include <limits.h>
#define __user

#include "drm.h"
#include "i915_drm.h"
#include "dmabuf-alone.h"

#define DRM_I915_GEM_VGTBUFFER          0x36
#define DRM_IOCTL_I915_GEM_VGTBUFFER    DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_GEM_VGTBUFFER, struct drm_i915_gem_vgtbuffer)

void write_ppm(uint8_t *data,  int w, int h, int bpp, const char *name)
{
	FILE *fp;
	int i;

	return ;
	fprintf(stderr, "w %d, h %d, bpp %d\n", w, h, bpp);

	if ((fp = fopen(name, "wb")) == NULL) {
		fprintf(stderr, "failed to open file\n");
		exit(1);
	}

	fprintf(fp, "%s\n%d %d\n255\n", "P6", w, h);

	for (i = 0; i < (w * h * bpp / 8); i++, data++) {
		if (i % 4 == 3)
			continue;
		putc((int) *data, fp);
	}

	fclose(fp);
	return;
}

int create_vgtbuffer_handle(int fd1, int vmid, struct drm_i915_gem_vgtbuffer *v)
{
	int ret;
	struct drm_i915_gem_vgtbuffer vcreate;

	memset(&vcreate, 0, sizeof(struct drm_i915_gem_vgtbuffer));
	vcreate.flags = I915_VGTBUFFER_CHECK_CAPABILITY;
	if (drmIoctl(fd1, DRM_IOCTL_I915_GEM_VGTBUFFER, &vcreate)) {
		perror("check failed\n");
		exit(1);
	}

	drmSetMaster(fd1);

	memset(&vcreate,0,sizeof(struct drm_i915_gem_vgtbuffer));
	vcreate.vmid = vmid;//xen_domid; //Gust xl number
	vcreate.plane_id = I915_VGT_PLANE_PRIMARY;
	vcreate.phys_pipe_id = UINT_MAX;//UINT_MAX;
	ret = drmIoctl(fd1, DRM_IOCTL_I915_GEM_VGTBUFFER, &vcreate);
	if (ret) {
	    perror("ioctl DRM_IOCTL_I915_GEM_VGTBUFFER error\n");
	    exit(1);
	}
#if 0
	printf("vmid=%d\n", vcreate.vmid);
	printf("handle=%d\n", vcreate.handle);
	printf("vcreate.width=%d, height=%d\n",vcreate.width,  vcreate.height);
	printf("vcreate.stride=%d\n",vcreate.stride);
	printf("bpp:%d\n", vcreate.bpp);
	printf("tiled:%d\n", vcreate.tiled);
	printf("start=%d\n", vcreate.start);
	printf("size=%d\n", vcreate.size);
	printf("fd=%d\n", fd1);
	printf("stride %d, user_ptr %llx, user_size %d, drm_format %x, hw_format %x\n",
		vcreate.stride, vcreate.user_ptr, vcreate.user_size, vcreate.drm_format,
		vcreate.hw_format);
#endif
#if 0
	struct drm_i915_gem_mmap_gtt mmap_arg;
	memset(&mmap_arg,0,sizeof(struct drm_i915_gem_mmap_gtt));
	uint8_t *addr=NULL;
	mmap_arg.handle = vcreate.handle;
	drmIoctl(fd1, DRM_IOCTL_I915_GEM_MMAP_GTT, &mmap_arg);

	addr = (uint8_t *)mmap(0, (vcreate.width*vcreate.height*vcreate.bpp)/8, PROT_READ | PROT_WRITE,
			MAP_SHARED, fd1, mmap_arg.offset);
	printf("handle=%d, offset=%llu, addr=%p\n",mmap_arg.handle, mmap_arg.offset, (unsigned int*)addr);


	write_ppm(addr, vcreate.width, vcreate.height, vcreate.bpp, "desktop.ppm");
#endif
	*v = vcreate;
	return vcreate.handle;
}

static int export_handle(int fd, uint32_t handle, int *outfd)
{
	struct drm_prime_handle args;
	int ret;

	args.handle = handle;
	args.flags = DRM_CLOEXEC;
	args.fd = -1;

	ret = drmIoctl(fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &args);
	if (ret) {
		ret = errno;
		fprintf(stderr, "DRM_IOCTL_PRIME_HANDLE_TO_FD failed\n");
	}
	*outfd = args.fd;

	return ret;
}

int test_dmabuf(int fd1, int vmid, struct drm_i915_gem_vgtbuffer *vcreate)
{
	uint32_t handle;
	int dma_buf_fd = -1;
	int ret;

	handle = create_vgtbuffer_handle(fd1, vmid, vcreate);

	ret = export_handle(fd1, handle, &dma_buf_fd);
	if (ret) {
           fprintf(stderr, "export_handle failed\n");
	   exit(1);
	}

	return dma_buf_fd;
}
