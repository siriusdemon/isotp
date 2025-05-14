#include "rkcanfd.h"
#include <stdio.h>


int can_recv() {
    printf("Can recv\n");
    int phycan = 0;
    int bitrate = 500000;
    int dbitrate = 2000000;
    int can_id = 0x7E1;
    int canfd = 0;
    int link, res;
    int filters[] = {can_id};
    link_frame frame;
    linkdown(phycan);
    linkset(phycan, bitrate, dbitrate);
    linkup(phycan);
    
    res = linkconnect(phycan, &link, 0);
    if (res < 0) {
        printf("connect fail\n");
        return -1;
    }
    //res = linkfilter_canid(link, filters, 1);
    while (1) {
        res = linkrecv(link, &frame, 1000);
        if (res < 0) {
            printf("recv fail\n");
        } else if (res == 0) {
            // nothing just wait
            continue;
        } else {
            for (int i = 0; i < frame.len; i++) {
                printf("%02X ", frame.data[i]);
            }
            printf("\n");
        }
    }
    linkdisconnect(link);
    return 0;
}


int can_send() {
    printf("can send\n");
    int phycan = 0;
    int bitrate = 500000;
    int dbitrate = 2000000;
    int can_id = 0x7E1;
    int canfd = 0;
    int link, res;
    int filters[] = {can_id};
    link_frame frame;
    res = linkconnect(phycan, &link, 0);

    frame.can_id = can_id;
    frame.canfd = canfd;
    frame.data[0] = 0x02;
    frame.data[1] = 0x10;
    frame.data[2] = 0x03;
    frame.len = 3;

    res = linksend(link, &frame, 1000);
    linkdisconnect(link);
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        can_recv();
    }
    else {
        can_send();
    }
}