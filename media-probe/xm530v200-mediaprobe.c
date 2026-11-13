/*
 * xm530v200-mediaprobe — H.265 media pipeline diagnostic for OpenIPC
 *
 * This program opens the V200 media char devices and issues the reverse-engineered
 * ioctl sequence to verify the H.265 hardware encoder pipeline.
 *
 * Ioctl constants are decoded from the kernel module .text sections using
 * ARM MOVW/MOVT instruction pairs. The constants are:
 *
 * VENC (venc.ko, type=0x49):
 *   0x40284903  WRITE  size=0x028  nr=0x03  GET_INFO?
 *   0x40104904  WRITE  size=0x010  nr=0x04  GET_BUF?
 *   0x41044901  WRITE  size=0x104  nr=0x01  RET_BUF?
 *   0xc0284902  READ|WRITE  size=0x028  nr=0x02  RELEASE?
 *
 * H265 (hevc.ko, type=0x48):
 *   0x40044806  WRITE  size=0x004  nr=0x06  GET?
 *   0x40384805  WRITE  size=0x038  nr=0x05  READ_BLOCK
 *   0xc0344802  READ|WRITE  size=0x034  nr=0x02  WRITE_BLOCK
 *
 * VPSS (vpss.ko, type=0x50):
 *   0x40805001  WRITE  size=0x080  nr=0x01  READ_01
 *   0x40805002  WRITE  size=0x080  nr=0x02  READ_02
 *   0x40805004  WRITE  size=0x080  nr=0x04  READ_04
 *   0x40805006  WRITE  size=0x080  nr=0x06  READ_06
 *   0x40805007  WRITE  size=0x080  nr=0x07  READ_07
 *   0x4080500a  WRITE  size=0x080  nr=0x0a  READ_0A
 *   0x4080500c  WRITE  size=0x080  nr=0x0c  READ_0C
 *   0x4080500d  WRITE  size=0x080  nr=0x0d  READ_0D
 *   0x4080500e  WRITE  size=0x080  nr=0x0e  READ_0E
 *   0x4080500f  WRITE  size=0x080  nr=0x0f  READ_0F
 *   0x40805012  WRITE  size=0x080  nr=0x12  READ_12
 *   0x40045008  WRITE  size=0x004  nr=0x08  READ_08
 *   0x40045009  WRITE  size=0x004  nr=0x09  READ_09
 *   0x420c5011  WRITE  size=0x20c  nr=0x11  READ_11
 *   0xc0805003  READ|WRITE  size=0x080  nr=0x03  SET_03
 *   0xc0805005  READ|WRITE  size=0x080  nr=0x05  SET_05
 *
 * ISP (isp.ko, type=0x10):
 *   0x340010b4  WRITE  size=0x400  nr=0xb4  ?
 *   0x34081014  WRITE  size=0x408  nr=0x14  ?
 *   0x3408101c  WRITE  size=0x408  nr=0x1c  ?
 *   0x34081010  WRITE  size=0x408  nr=0x10  ?
 *   0x34081018  WRITE  size=0x408  nr=0x18  ?
 *
 * NOTE: The direction field (bits 31-30) describes the encoded userspace/kernel
 * transfer convention, not necessarily every side effect of the driver. A
 * request with direction=WRITE may still have read side effects.
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

/* ---- ioctl request constants (decoded from kernel module .text) ---- */

/* /dev/venc (venc.ko) — channel manager */
#define VENC_GET_INFO     0x40284903u  /* WRITE,  size=0x028: get channel info     */
#define VENC_GET_BUF      0x40104904u  /* WRITE,  size=0x010: get a filled buffer  */
#define VENC_RET_BUF      0x41044901u  /* WRITE,  size=0x104: return a buffer      */
#define VENC_RELEASE      0xc0284902u  /* R|W,    size=0x028: release a channel    */

/* /dev/h265 (hevc.ko) — the H.265 hardware encoder */
#define H265_GET          0x40044806u  /* WRITE,  size=0x004: get value/status     */
#define H265_READ_BLOCK   0x40384805u  /* WRITE,  size=0x038: read 0x38-byte block */
#define H265_WRITE_BLOCK  0xc0344802u  /* R|W,    size=0x034: write 0x34-byte block*/

/* /dev/vpss (vpss.ko) — crop/scale/bind */
#define VPSS_READ_01      0x40805001u  /* WRITE,  size=0x080: read group/chn attr  */
#define VPSS_READ_02      0x40805002u  /* WRITE,  size=0x080 */
#define VPSS_READ_04      0x40805004u  /* WRITE,  size=0x080 */
#define VPSS_READ_06      0x40805006u  /* WRITE,  size=0x080 */
#define VPSS_READ_07      0x40805007u  /* WRITE,  size=0x080 */
#define VPSS_READ_0A      0x4080500au  /* WRITE,  size=0x080 */
#define VPSS_READ_0C      0x4080500cu  /* WRITE,  size=0x080 */
#define VPSS_READ_0D      0x4080500du  /* WRITE,  size=0x080 */
#define VPSS_READ_0E      0x4080500eu  /* WRITE,  size=0x080 */
#define VPSS_READ_0F      0x4080500fu  /* WRITE,  size=0x080 */
#define VPSS_READ_12      0x40805012u  /* WRITE,  size=0x080 */
#define VPSS_READ_08      0x40045008u  /* WRITE,  size=0x004 */
#define VPSS_READ_09      0x40045009u  /* WRITE,  size=0x004 */
#define VPSS_READ_11      0x420c5011u  /* WRITE,  size=0x20c */
#define VPSS_SET_03       0xc0805003u  /* R|W,    size=0x080: set chn attr         */
#define VPSS_SET_05       0xc0805005u  /* R|W,    size=0x080: set crop             */

/* /dev/isp (isp.ko) — VICU capture + BPU ISP */
#define ISP_CMD_B4        0x340010b4u  /* WRITE,  size=0x400 */
#define ISP_CMD_14        0x34081014u  /* WRITE,  size=0x408 */
#define ISP_CMD_1C        0x3408101cu  /* WRITE,  size=0x408 */
#define ISP_CMD_10        0x34081010u  /* WRITE,  size=0x408 */
#define ISP_CMD_18        0x34081018u  /* WRITE,  size=0x408 */

/* ---- 0x080-byte vpss attr struct (from 0x40805001 size) ---- */
struct vpss_attr {
    uint32_t pad[0x20];
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

/* Detect H.265 NAL units in a buffer using Annex-B start codes. */
static int count_nal_units(const void *buf, size_t len)
{
    const uint8_t *p = buf;
    int nals = 0;
    for (size_t i = 0; i + 4 <= len; i++) {
        /* 3-byte start code: 00 00 01 */
        if (p[i] == 0 && p[i+1] == 0 && p[i+2] == 1) {
            nals++;
        }
        /* 4-byte start code: 00 00 00 01 */
        if (i + 4 < len && p[i] == 0 && p[i+1] == 0 && p[i+2] == 0 && p[i+3] == 1) {
            nals++;
        }
    }
    return nals;
}

int main(int argc, char **argv)
{
    int fd_h265, fd_venc, fd_vpss, fd_isp, fd_sys;
    uint32_t nals = 0, got = 0;

    LOG("[mediaprobe] XM530V200 H.265 probe start\n");

    /* 0. system: sys.ko sets PLL/routing. Open only (no ioctl constants found). */
    fd_sys = open_dev("sys", O_RDWR);
    if (fd_sys < 0) LOG("[mediaprobe] no /dev/sys (modules not loaded?)\n");

    /* 1. isp: version + fps (diagnostic). */
    fd_isp = open_dev("isp", O_RDWR);
    if (fd_isp >= 0) {
        /* ISP has 5 ioctl constants, all WRITE with large struct sizes (0x400-0x408).
         * These are likely register access commands. Skip for now. */
        LOG("[mediaprobe] isp opened, 5 ioctl constants (register access, skipped)\n");
    }

    /* 2. vpss: ensure group/chn exists. */
    fd_vpss = open_dev("vpss", O_RDWR);
    if (fd_vpss >= 0) {
        struct vpss_attr attr = {0};
        int r = ioctl(fd_vpss, VPSS_READ_01, &attr);
        LOG("[mediaprobe] vpss READ_01(0x%08x) -> %s\n", VPSS_READ_01, r == 0 ? "ok" : strerror(errno));
    }

    /* 3. hevc /dev/h265: the H.265 hardware encoder. */
    fd_h265 = open_dev("h265", O_RDWR);
    if (fd_h265 < 0)
        LOG("[mediaprobe] FATAL: no /dev/h265 — hevc.ko not loaded?\n");

    /* 4. venc /dev/venc: per-channel FIFO + thread that hands frames to us. */
    fd_venc = open_dev("venc", O_RDWR);
    if (fd_venc < 0)
        LOG("[mediaprobe] FATAL: no /dev/venc — venc.ko not loaded?\n");

    if (fd_h265 >= 0) {
        /* Write a 0x34-byte param block (H265_WRITE_BLOCK, size=0x34).
         * Layout unconfirmed on-device; zeros is a safe starting point. */
        uint8_t param[0x34];
        memset(param, 0, sizeof param);
        int r = ioctl(fd_h265, H265_WRITE_BLOCK, param);
        LOG("[mediaprobe] h265 WRITE_BLOCK(0x%08x) -> %s\n", H265_WRITE_BLOCK, r == 0 ? "ok" : strerror(errno));

        /* Read encoded H.265 in 0x38-byte blocks (H265_READ_BLOCK, size=0x38). */
        uint8_t block[0x38];
        for (int i = 0; i < 256; i++) {
            memset(block, 0, sizeof block);
            r = ioctl(fd_h265, H265_READ_BLOCK, block);
            if (r < 0) {
                LOG("[mediaprobe] h265 READ_BLOCK[%d](0x%08x): %s\n", i, H265_READ_BLOCK, strerror(errno));
                break;
            }
            /* Non-zero block => candidate frame data. */
            if (memchr(block, 0, sizeof block) != block) {
                got++;
                /* 0x38 is a fragment; accumulate into a larger buffer for NAL scan. */
            }
        }
    }

    /* 5. Pull frames from the venc FIFO. */
    if (fd_venc >= 0) {
        /* VENC_GET_BUF returns a buffer ptr (size=0x10, likely a 16-byte descriptor).
         * We get the ptr, scan it for NAL start codes, then return it via VENC_RET_BUF. */
        uint8_t *frame = malloc(2 * 1024 * 1024); /* 2 MiB main-stream frame cap */
        if (frame) {
            for (int i = 0; i < 100; i++) {
                uint8_t buf_desc[0x10] = {0}; /* 16-byte descriptor */
                int r = ioctl(fd_venc, VENC_GET_BUF, buf_desc);
                if (r < 0 || r == 0) {
                    LOG("[mediaprobe] venc GET_BUF[%d](0x%08x): %s\n", i, VENC_GET_BUF, strerror(errno));
                    usleep(100000);
                    continue;
                }
                /* The descriptor likely contains a pointer to the frame buffer.
                 * We don't know the layout yet, so just log the raw bytes. */
                LOG("[mediaprobe] venc GET_BUF[%d] desc: ", i);
                for (int j = 0; j < 0x10; j++) LOG(" %02x", buf_desc[j]);
                LOG("\n");

                /* Try to interpret the descriptor as a pointer */
                uint32_t *ptr = (uint32_t *)buf_desc;
                for (int j = 0; j < 0x10/4; j++) {
                    uint32_t val = ptr[j];
                    /* Check if it looks like a kernel address (0x80000000-0xbfffffff) */
                    if (val >= 0x80000000 && val <= 0xbfffffff) {
                        LOG("[mediaprobe]   desc[%d] = 0x%08x (possible kernel ptr)\n", j, val);
                    }
                }

                /* Return the buffer */
                r = ioctl(fd_venc, VENC_RET_BUF, buf_desc);
                if (r < 0)
                    LOG("[mediaprobe] venc RET_BUF[%d](0x%08x): %s\n", i, VENC_RET_BUF, strerror(errno));
            }
        }
    }

    LOG("[mediaprobe] done: %u read blocks, %u H.265 NAL start-codes seen\n", got, nals);
    if (nals == 0)
        LOG("[mediaprobe] WARNING: no H.265 NAL start-codes — pipeline not producing "
            "frames (check load order, sensor init, vpss bind, or h265 params)\n");
    return nals ? 0 : 1;
}
