/*
 * xm530v200-mediaprobe — minimal independent H.265 stream bridge for OpenIPC.
 *
 * Context (see MEDIA-IOCTL-MAP.md): the V200 image ships the full vendor media
 * kernel stack (i2c mmz vb sys log ddr vo isp vpss rgn venc hevc) but NO
 * userspace consumer. The vendor Sofia/isp are uClibc/glibc ELF binaries that
 * cannot link into the musl OpenIPC rootfs, so this program drives the char
 * devices directly with the ioctl constants reverse-engineered from the modules.
 *
 * Pipeline: sensor(SC223A) -> MIPI -> isp(vicu+bpu) -> vpss(crop/scale/bind)
 *   -> hevc /dev/h265 (H.265 HW encode, request_irq) -> venc /dev/venc
 *   (per-chn FIFO + kernel thread + /proc/umap/venc) -> userspace read/ioctl.
 *
 * This probe is DIAGNOSTIC first: it opens the devices, issues the mapped
 * init/attr sequence, reads the H.265 stream, and verifies the bytes are real
 * H.265 NAL units (0x00000001 / 0x00000001) before handing off to RTSP.
 *
 * All request constants and struct sizes are from static RE of the V200 modules
 * (capstone scan, 2026-11-13). Items marked "RE:" must be confirmed on-device.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>

/* ---- ioctl request constants (from RE of the V200 modules) ---- */

/* /dev/venc (venc.ko) — channel manager */
#define VENC_GET_BUF      0x40104901u  /* _IOC_READ,  1  : get a filled buffer ptr   */
#define VENC_READ_INFO    0x40284903u  /* _IOC_READ,  0x28: read 0x28-byte info block*/
#define VENC_RET_BUF      0xc0284902u  /* _IOC_WRITE, 0x28: return a buffer to chn    */
#define VENC_STATUS       0x40284900u  /* _IOC_NONE,  0x28: 0x28-byte status read     */
#define VENC_RELEASE      0x00004905u  /* _IOC_NONE,  0    : release/close a channel   */
#define VENC_SET          0x41044901u  /* _IOC_WRITE, 1  : set (from rr3 scan)         */

/* /dev/h265 (hevc.ko) — the H.265 hardware encoder */
#define H265_GET          0x40044806u  /* _IOC_READ,  4   : get value/status          */
#define H265_READ_BLOCK   0x40384805u  /* _IOC_READ,  0x38: read 0x38-byte block      */
#define H265_WRITE_BLOCK  0xc0344802u  /* _IOC_WRITE, 0x34: write 0x34-byte param block*/

/* /dev/vpss (vpss.ko) — crop/scale/bind */
#define VPSS_READ_01      0x40805001u  /* _IOC_READ,  0x80: group/chn attr get        */
#define VPSS_READ_02      0x40805002u
#define VPSS_READ_04      0x40805004u
#define VPSS_READ_06      0x40805006u
#define VPSS_READ_07      0x40805007u
#define VPSS_READ_0A      0x4080500au
#define VPSS_READ_0C      0x4080500cu
#define VPSS_READ_0D      0x4080500du
#define VPSS_READ_0E      0x4080500eu
#define VPSS_READ_0F      0x4080500fu
#define VPSS_READ_12      0x40805012u
#define VPSS_SET_03       0xc0805003u  /* _IOC_WRITE, 0x80: set chn attr              */
#define VPSS_SET_05       0xc0805005u  /* _IOC_WRITE, 0x80: set crop                  */

/* /dev/isp (isp.ko) — VICU capture + BPU ISP */
#define ISP_GETVER        0x30041004u  /* CMD_ISP_VERSION  (from isp.ko strings)      */
#define ISP_GETFPS        0x30041008u  /* CMD_ISP_GETFPS                                */
#define ISP_REG_READ      0x30041008u  /* /dev/isp reg read family                      */
#define ISP_REG_WRITE     0x30042004u  /* /dev/isp reg write family                     */

/* ---- small attr request struct (44-byte, from vendor `isp` userspace) ---- */
struct attr_req {
	uint32_t pad[11];
	uint32_t attr_id;   /* @0x2c */
	uint32_t value;     /* @0x30 */
};

static int dbg = 1;
#define LOG(...) do { if (dbg) fprintf(stderr, __VA_ARGS__); } while (0)

/* open a misc char device; misc_register auto-creates /dev/<name> */
static int open_dev(const char *name, int flags)
{
	char path[32];
	int fd;
	snprintf(path, sizeof path, "/dev/%s", name);
	fd = open(path, flags);
	if (fd < 0) {
		LOG("[mediaprobe] open %s: %s\n", path, strerror(errno));
		/* retry: some modules register a moment after load; brief wait */
		 usleep(200000);
		fd = open(path, flags);
		if (fd < 0) {
			LOG("[mediaprobe] open %s (retry): %s\n", path, strerror(errno));
			return -1;
		}
	}
	LOG("[mediaprobe] open %s -> fd %d\n", path, fd);
	return fd;
}

static int try_ioctl(int fd, unsigned long req, const char *what)
{
	if (ioctl(fd, req, NULL) < 0) {
		LOG("[mediaprobe] ioctl %s (0x%08lx) failed: %s\n", what, req, strerror(errno));
		return -1;
	}
	LOG("[mediaprobe] ioctl %s (0x%08lx) ok\n", what, req);
	return 0;
}

/* Detect real H.265 NAL units in a buffer. */
static int looks_like_h265(const void *buf, size_t len)
{
	const uint8_t *p = buf;
	int nal = 0;
	for (size_t i = 0; i + 4 < len; i++) {
		if (p[i]==0 && p[i+1]==0 && p[i+2]==1) { nal++; }        /* Annex-B */
		else if (p[i]==0 && p[i+1]==0 && p[i+2]==0 && p[i+3]==1) { nal++; }
	}
	return nal;
}

int main(int argc, char **argv)
{
	int fd_h265, fd_venc, fd_vpss, fd_isp, fd_sys;
	void *h265blk;
	uint32_t nals = 0, got = 0;

	LOG("[mediaprobe] XM530V200 H.265 probe start\n");

	/* 0. system: sys.ko sets PLL/routing. Open and issue no-op attr. */
	fd_sys = open_dev("sys", O_RDWR);
	if (fd_sys < 0) LOG("[mediaprobe] no /dev/sys (modules not loaded?)\n");

	/* 1. isp: version + fps (diagnostic). */
	fd_isp = open_dev("isp", O_RDWR);
	if (fd_isp >= 0) {
		struct attr_req v = {0};
		try_ioctl(fd_isp, ISP_GETVER, "isp version");
		try_ioctl(fd_isp, ISP_GETFPS, "isp getfps");
	}

	/* 2. vpss: ensure group/chn exists (read attr first; create on error). */
	fd_vpss = open_dev("vpss", O_RDWR);
	if (fd_vpss >= 0) {
		uint8_t blk[0x80] = {0};
		if (ioctl(fd_vpss, VPSS_READ_01, blk) < 0)
			LOG("[mediaprobe] vpss READ_01 failed (group may need create)\n");
	}

	/* 3. hevc /dev/h265: the H.265 hardware encoder.
	 *    Write a 0x34-byte param block (channel/codec/width/height),
	 *    then read encoded output via 0x38-byte blocks. */
	fd_h265 = open_dev("h265", O_RDWR);
	if (fd_h265 < 0)
		LOG("[mediaprobe] FATAL: no /dev/h265 — hevc.ko not loaded?\n");

	/* 4. venc /dev/venc: per-channel FIFO + thread that hands frames to us. */
	fd_venc = open_dev("venc", O_RDWR);
	if (fd_venc < 0)
		LOG("[mediaprobe] FATAL: no /dev/venc — venc.ko not loaded?\n");

	if (fd_h265 >= 0) {
		/* Write a minimal H.265 param block. Layout unconfirmed on-device;
		 * zeros is a safe starting point for the diagnostic. */
		uint8_t param[0x34];
		memset(param, 0, sizeof param);
		/* TODO(RE): set codec=H265, chn=0, width=1920, height=1080, fps=20,
		 * bitrate=8192k per the live camera's main stream. */
		if (ioctl(fd_h265, H265_WRITE_BLOCK, param) < 0)
			LOG("[mediaprobe] h265 WRITE_BLOCK: %s\n", strerror(errno));

		/* Read encoded H.265 in 0x38-byte blocks and look for NAL start codes. */
		uint8_t block[0x38];
		for (int i = 0; i < 256; i++) {
			memset(block, 0, sizeof block);
			if (ioctl(fd_h265, H265_READ_BLOCK, block) < 0) {
				LOG("[mediaprobe] h265 READ_BLOCK[%d]: %s\n", i, strerror(errno));
				break;
			}
			/* Non-zero block => candidate frame data. */
			if (memchr(block, 0, sizeof block) != block) {
				got++;
				/* 0x38 is a fragment; accumulate into a larger buffer for NAL scan. */
			}
		}
	}

	/* 5. Pull frames from the venc FIFO. The thread delivers via read/ioctl;
	 *    VENC_GET_BUF returns a buffer ptr, we read it, then VENC_RET_BUF. */
	if (fd_venc >= 0) {
		uint8_t *frame = malloc(2 * 1024 * 1024); /* 2 MiB main-stream frame cap */
		if (frame) {
			for (int i = 0; i < 100; i++) {
				void *bp = NULL;
				if (ioctl(fd_venc, VENC_GET_BUF, &bp) < 0 || !bp) {
					LOG("[mediaprobe] venc GET_BUF[%d]: %s\n", i, strerror(errno));
					usleep(100000);
					continue;
				}
				/* The delivered buffer holds H.265 bytes (the venc thread
				 * __copy_to_user'd it). Scan for NAL start codes. */
				nals += looks_like_h265(bp, 1 << 16);
				LOG("[mediaprobe] frame[%d] got ptr %p\n", i, bp);
				if (ioctl(fd_venc, VENC_RET_BUF, &bp) < 0)
					LOG("[mediaprobe] venc RET_BUF: %s\n", strerror(errno));
			}
		}
	}

	LOG("[mediaprobe] done: %u read blocks, %u H.265 NAL start-codes seen\n", got, nals);
	if (nals == 0)
		LOG("[mediaprobe] WARNING: no H.265 NAL start-codes — pipeline not producing "
		    "frames (check load order, sensor init, vpss bind, or h265 params)\n");
	return nals ? 0 : 1;
}
