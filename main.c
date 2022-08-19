#include "dash_4115_4p45_firmware.h"
#include "dash_4300_4p45_firmware.h"
#include "dash_4320_4p45_firmware.h"
#include "dash_4510_4p45_firmware.h"
#include "n76e_firmware.h"
#include "unknown1_firmware.h"
#include "unknown2_firmware.h"
#include "unknown3_firmware.h"
#include "warp_firmware.h"
#include <sys/fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#ifndef ANDROID
#define KLOG_ERROR_LEVEL   3

void klog_write(int level, const char *fmt, ...) {
    // Stub
}

#endif

char swarp_supported = 0;

int read_31_bytes(char *path, char *buf) {
    int fd;
    ssize_t ret;

    fd = open(path, O_RDONLY, 0);
    if (fd < 0) {
        return -1;
    }

    ret = read(fd, buf, 31);
    close(fd);

    if (ret <= 0) {
        return -1;
    }

    if (buf[ret - 1] == '\n') {
        ret = ret - 1;
    }

    buf[ret] = '\0';

    return (int) ret;
}

void *dashd_thread(void *arg) {
    return NULL;
}

static void dashd_firmware_update(void) {
    int dash_fd = 0;
    unsigned char *firmware_buf = 0;
    size_t firmware_len = 0;
    char buf[32] = {0};
    int tmp = 0;
    int hw_id = 0;
    ssize_t ret = 0;
    int ret2 = 0;

    dash_fd = open("/dev/dash", O_RDWR);
    if (dash_fd < 0) {
        klog_write(KLOG_ERROR_LEVEL, "<3>dashd: failed to open %s ret=%d\n", "/dev/dash", dash_fd);
        klog_write(KLOG_ERROR_LEVEL, "<3>dashd: firmware_update failed\n");
        return;
    }

    if (access("/proc/n76e_exit", F_OK) == 0) {
        firmware_buf = n76e_firmware;
        firmware_len = 0x24fc;
    } else if (access("/proc/warp_chg_exit", F_OK) == 0 && access("/proc/dash_4300_4p45_exit", F_OK) == 0) {
        firmware_buf = dash_4300_4p45_firmware;
        firmware_len = 0x1870;
    } else if (access("/proc/warp_chg_exit", F_OK) == 0 && access("/proc/dash_4115_4p45_exit", F_OK) == 0) {
        firmware_buf = dash_4115_4p45_firmware;
        firmware_len = 0x1870;
    } else if (access("/proc/warp_chg_exit", F_OK) == 0 && access("/proc/dash_4320_4p45_exit", F_OK) == 0) {
        firmware_buf = dash_4320_4p45_firmware;
        firmware_len = 0x1870;
    } else if (access("/proc/warp_chg_exit", F_OK) == 0 && access("/proc/dash_4510_4p45_exit", F_OK) == 0) {
        firmware_buf = dash_4510_4p45_firmware;
        firmware_len = 0x1870;
    } else if (access("/proc/warp_chg_exit", F_OK) == 0) {
        firmware_buf = warp_firmware;
        firmware_len = 0x1980;
    } else if (access("/proc/swarp_chg_exist", F_OK) == 0) {
        swarp_supported = 1;
        for (tmp = 0; tmp < 5; tmp++) {
            memset(buf, 0, sizeof(buf));
            read_31_bytes("/proc/swarp_chg_exist", buf);
            hw_id = (int) strtol(buf, NULL, 0);
            if (hw_id > 0) {
                break;
            }
            klog_write(KLOG_ERROR_LEVEL, "<3>dashd: hw_id=%d, driver get hwid is not ready.", hw_id);
            sleep(1);
        }
        if (hw_id == 2) {
            firmware_buf = unknown1_firmware;
            firmware_len = 0x2538;
        } else {
            firmware_buf = unknown2_firmware;
            firmware_len = 0x2fc0;
        }
    } else {
        firmware_buf = unknown3_firmware;
        firmware_len = 0x18f8;
    }

    ret = write(dash_fd, firmware_buf, firmware_len);
    if (ret < 0) {
        klog_write(KLOG_ERROR_LEVEL, "<3>dashd: failed to write dash firmware, ret=%d\n");
        klog_write(KLOG_ERROR_LEVEL, "<3>dashd: firmware_update failed\n");
        close(dash_fd);
        return;
    }

    ret2 = ioctl(dash_fd, 0xff01);
    if (ret2 < 0) {
        klog_write(KLOG_ERROR_LEVEL, "<3>dashd: failed to notify fw update, ret =%d\n", ret2);
    }
    close(dash_fd);
}

void dashd_start_thread(void) {
    int ret;
    pthread_t thread;
    char tmp_buf[32] = {0};
    char type[32];
    int soc;
    int temp;
    int vbat;
    int ibat;

    ret = pthread_create(&thread, NULL, dashd_thread, NULL);
    if (ret) {
        klog_write(KLOG_ERROR_LEVEL, "<3>dashd: Failed to create monitor thread\n");
        return;
    }

    klog_write(KLOG_ERROR_LEVEL, "<3>dashd: THREAD is created!\n");

    memset(type, 0, sizeof(type));
    read_31_bytes("/sys/class/power_supply/usb/type", type);

    memset(tmp_buf, 0, sizeof(tmp_buf));
    read_31_bytes("/sys/class/power_supply/battery/capacity", tmp_buf);
    soc = (int) strtol(tmp_buf, NULL, 0);

    memset(tmp_buf, 0, sizeof(tmp_buf));
    read_31_bytes("/sys/class/power_supply/battery/temp", tmp_buf);
    temp = (int) strtol(tmp_buf, NULL, 0);
    temp = (temp / 10 + (temp >> 0x1f)) - (int) ((long) temp * 0x66666667 >> 0x3f);

    memset(tmp_buf, 0, sizeof(tmp_buf));
    read_31_bytes("/sys/class/power_supply/battery/voltage_now", tmp_buf);
    vbat = (int) strtol(tmp_buf, NULL, 0);
    vbat = (vbat / 1000 + (vbat >> 0x1f)) - (int) ((long) vbat * 0x10624dd3 >> 0x3f);

    memset(tmp_buf, 0, sizeof(tmp_buf));
    read_31_bytes("/sys/class/power_supply/battery/current_now", tmp_buf);
    ibat = (int) strtol(tmp_buf, NULL, 0);
    ibat = (ibat / 1000 + (ibat >> 0x1f)) - (int) ((long) ibat * 0x10624dd3 >> 0x3f);

    klog_write(KLOG_ERROR_LEVEL, "<3>dashd: type=%s, soc=%d, temp=%d, vbat=%d, ibat=%d\n", type, soc, temp, vbat, ibat);

    pthread_join(thread, NULL);
}

int main(int argc, char *argv[]) {
    klog_write(KLOG_ERROR_LEVEL, "<3>dashd: dashd_main enterd\n");
    if (argc != 0) {
        klog_write(KLOG_ERROR_LEVEL, "<3>dashd: run %s ENGINE START!\n", argv[0]);
    }

    dashd_firmware_update();
    dashd_start_thread();

    return 0;
}
