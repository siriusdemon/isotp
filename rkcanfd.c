#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/select.h>

#include "rkcanfd.h"




int linkup(int phycan) {
    if (phycan == 0) {
        return system("ip link set can0 up");
    } else if (phycan == 1) {
        return system("ip link set can1 up");
    }
    return -1;
}
int linkdown(int phycan) {
    if (phycan == 0) {
        return system("ip link set can0 down");
    } else if (phycan == 1) {
        return system("ip link set can1 down");
    }
    return -1;
}
int linkset(int phycan, int bitrate, int dbitrate) {
    if (phycan != 0 && phycan != 1) {return -1;}
    char buff[128];
    memset(buff, 0, 128);
    snprintf(buff, sizeof(buff), "ip link set can%d type can bitrate %d dbitrate %d fd on", phycan, bitrate, dbitrate);
    return system(buff);
}

int linkconnect(int phycan, int* link, int canfd) {
    if (phycan != 0 && phycan != 1) {return -1;}
    int s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    // enable canfd frame
    if (canfd == 1) {
        if (setsockopt(s, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &canfd, sizeof(canfd)) < 0) {
            perror("setsocketopt CAN_RAW_FD_FRAMES");    
            close(s);
            return -1;
        }
    }

    // bind the socket
    struct ifreq ifr;
    struct sockaddr_can addr;
    snprintf(ifr.ifr_name, IF_NAMESIZE, "can%d", phycan);
    if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) {
        perror("ioctl SIOCGIFINDEX");
        close(s);
        return -1;
    }

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(s, &addr, sizeof(addr)) < 0) {
        perror("bind");
        close(s);
        return -1;
    }

    *link = s;
    return 0;
}


int linksend(int link, const link_frame* frame, int timeout) {

}
int linkrecv(int link, link_frame* frame, int timeout) {

}

int linkfilter(int link, uint32_t* filters, int size) {

}

int linkdisconnect(int link) {
    close(link);
}