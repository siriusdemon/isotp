#include "rkcanfd.h"
#include <stdio.h>

void printframe(link_frame* frame) {
    printf("%02X %2d ", frame->can_id, frame->len);
    for (int i = 0; i < frame->len; i++) {
        printf("%02X ", frame->data[i]);
    }
    printf("\n");
}

int can_test() {
    int phycan = 0;
    int bitrate = 500000;
    int dbitrate = 2000000;
    int can_id = 0x7E1;
    int canfd = 0;
    int link, res;
    int filters[] = {can_id};
    link_frame frame;
    // linkdown(phycan);
    // linkset(phycan, bitrate, dbitrate);
    // linkup(phycan);
    // usleep(1000000);
    
    res = linkconnect(phycan, &link, 0);
    if (res < 0) {
        printf("connect fail\n");
        return -1;
    }
    res = linkfilter_canid(link, filters, 1);

    // send and recv
    uint8_t data[8] = {0x10, 0x03};
    frame.can_id = can_id;
    frame.canfd = canfd;
    frame.data = data;
    frame.len = 2;


    printframe(&frame);
    res = linksend(link, &frame, 1000);
    res = linkrecv(link, &frame, 1000);
    printframe(&frame);


    linkdisconnect(link);
    return 0;
}

int canfd_test() {
    int phycan = 0;
    int bitrate = 500000;
    int dbitrate = 2000000;
    int can_id = 0x723;
    int canfd = 1;
    int link, res;
    int filters[] = {can_id};
    link_frame frame;
    // linkdown(phycan);
    // linkset(phycan, bitrate, dbitrate);
    // linkup(phycan);
    // usleep(1000000);
    
    res = linkconnect(phycan, &link, 0);
    if (res < 0) {
        printf("connect fail\n");
        return -1;
    }
    res = linkfilter_canid(link, filters, 1);

    // send and recv
    uint8_t data[64] = {0x10, 0x03};
    frame.can_id = can_id;
    frame.canfd = canfd;
    frame.data = data;
    frame.len = 2;


    printframe(&frame);
    res = linksend(link, &frame, 1000);
    res = linkrecv(link, &frame, 1000);
    printframe(&frame);


    linkdisconnect(link);
    return 0;
}



int main() {
    can_test();
    canfd_test();
    return 0;
}