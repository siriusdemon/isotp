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


#define ISO_TP_MAX_PAYLOAD 4095


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

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(s);
        return -1;
    }

    *link = s;
    return 0;
}

int recv_controlflow(int link, uint8_t* bs, uint8_t* stmin) {
    fd_set readfds;
    uint8_t pci;
    for (int i = 0; i < 3; i++) {
        FD_ZERO(&readfds);
        FD_SET(link, &readfds);
        struct timeval tv2 = {.tv_sec = 1};
        if (select(link + 1, &readfds, NULL, NULL, &tv2) < 0) {
            perror("control flow");
            return -1;
        } 
        struct can_frame ccf;
        if (read(link, &ccf, sizeof(ccf)) < 0) {
            perror("recv control flow data");
            return -1;
        }
        pci = ccf.data[0];
        if (pci == 0x30) {
            *bs = ccf.data[1];
            *stmin = ccf.data[2];
            return 0;
        } else if (pci == 0x32) {
            perror("overflow");
            return -1;
        } else if (pci == 0x31) {
            continue;
        } else {
            perror("not control flow frame");
            return -1;
        }
    }
    perror("timeout");
    return -1;
}

uint32_t stmin_to_us(uint8_t stmin) {
    // prepare for usleep(stmin);
    if (stmin > 0 && stmin <= 0x7F)  {
        return stmin * 1000;   // to us
    } else if (stmin >= 0xF1 && stmin <= 0xF9) {
        return 100 * (stmin & 0xF); 
    } else {    // ignore other case
        return 0;
    }
}

int linksend(int link, const link_frame* frame, int timeout) {
    if (!frame) {return -1;}
    fd_set writefds, readfds;
    struct timeval tv;
    const uint8_t* data = frame->data;
    uint32_t can_id = frame->can_id;
    uint32_t total_len = frame->len;

    int mtu = frame->canfd ? CANFD_MAX_DLEN : CAN_MAX_DLEN;


    // single frame
    if ((frame->canfd && total_len <= 62) || (!frame->canfd && total_len <= 7)) {
        FD_ZERO(&writefds);
        FD_SET(link, &writefds);
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;
        if (select(link + 1, NULL, &writefds, NULL, NULL) <= 0) {
            perror("select");
            return -1;
        }
 
        if (frame->canfd) {
            struct canfd_frame cf = {.can_id = can_id};
            if (total_len < 8) {
                cf.data[0] = total_len & 0xF;
                memcpy(cf.data + 1, data, total_len);
                cf.len = 8;
            } else {
                cf.data[0] = 0;
                cf.data[1] = frame->len;
                memcpy(cf.data + 2, data, total_len);
                cf.len = 64;
            }
            return write(link, &cf, sizeof(cf));
        } else {
            struct can_frame cf = {.can_id = can_id, .can_dlc = 8};
            cf.data[0] = total_len & 0xF;
            memcpy(cf.data + 1, data, total_len);
            return write(link, &cf, sizeof(cf));
        }      
    }   


    // multiframe
    uint8_t firstlen = mtu - 2;   // 2 bytes used for FF header
    uint16_t len = total_len;

    // prepare first frame
    FD_ZERO(&writefds);
    FD_SET(link, &writefds);
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;
    if (select(link + 1, NULL, &writefds, NULL, NULL) <= 0) {
        perror("select");
        return -1;
    }
    // send first frame
    if (frame->canfd) {
        struct canfd_frame cf = {.can_id = can_id, .len = mtu};
        cf.data[0] = 0x10 | ((len >> 8)& 0xF);
        cf.data[1] = len & 0xFF;
        memcpy(cf.data + 2, data, firstlen);
        if (write(link, &cf, sizeof(cf)) < 0) return -1;
    } else {
        struct can_frame cf = {.can_id = can_id, .can_dlc = mtu};
        cf.data[0] = 0x10 | ((len >> 8)& 0xF);
        cf.data[1] = len & 0xFF;
        memcpy(cf.data + 2, data, firstlen);
        if (write(link, &cf, sizeof(cf)) < 0) return -1;
    }     

    // recv control flow
    uint8_t bs, stmin;
    if (recv_controlflow(link, &bs, &stmin) < 0) {
        return -1;
    }

    // compute how many us need to wait.
    uint32_t us = stmin_to_us(stmin);

    // send consecutive frame
    int offset = firstlen;
    uint8_t seq = 1;
    uint8_t c = 0;     // for count bs;
    while (offset < total_len) {
        // the data carry by this cf. either mtu - 1 (normal) or total_len - offset (last one) 
        uint8_t cf_len = ((total_len - offset) > (mtu - 1))?  (mtu - 1) : (total_len - offset);

        FD_ZERO(&writefds);
        FD_SET(link, &writefds);
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;
        if (select(link + 1, NULL, &writefds, NULL, &tv) <= 0)  return -1;
        
        if (frame->canfd) {
            struct canfd_frame cf = {.can_id = can_id, .len = 64};
            cf.data[0] = 0x20 | (seq & 0xF);
            memcpy(cf.data + 1, data + offset, cf_len);
            if (write(link, &cf, sizeof(cf)) < 0) return -1;
        } else {
            struct can_frame cf = {.can_id = can_id, .can_dlc = 8};
            cf.data[0] = 0x20 | (seq & 0xF);
            memcpy(cf.data + 1, data + offset, cf_len);
            if (write(link, &cf, sizeof(cf)) < 0) return -1;
        }

        seq = (seq + 1) & 0xF;
        offset += cf_len;
        c++;
        usleep(us);
        // bs = 0 means we can send all the frame without waiting another cf
        // bs > 0 means we must wait for another cf when we have send bs frames
        if (bs > 0 && c == bs) {
            if (recv_controlflow(link, &bs, &stmin) < 0) return -1;
            us = stmin_to_us(stmin);
            c = 0;
        }
    }

    return 0;
}

int linkrecv(int link, link_frame* frame, int timeout) {
    fd_set readfds, writefds;
    int ret = 0;
    int canfd = frame->canfd;
    struct canfd_frame canfd_frame; 
    struct can_frame can_frame;
    uint8_t pci;  

    struct timeval tv;

    // for receiving consecutive frame
    size_t offset, total_len;
    uint8_t mtu = canfd? CANFD_MAX_DLEN : CAN_MAX_DLEN;
    uint8_t expected_sn = 1;
    uint8_t sn = 0;

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(link, &readfds);

        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;

        ret = select(link + 1, &readfds, NULL, NULL, &tv);
        if (ret < 0) {
            perror("select");
            return -1;
        }

        if (ret == 0) return 0; // nothing recv

        if (canfd) {
            ret = read(link, &canfd_frame, sizeof(canfd_frame));
            pci = canfd_frame.data[0];       
        } else {
            ret = read(link, &can_frame, sizeof(can_frame));
            pci = can_frame.data[0];       
        }

        if (ret < 0) {
            perror("read");
            return -1;
        }


        // single frame
        if ((pci & 0xF0) == 0x00) {
            uint8_t len = pci & 0xF;
            if (canfd) {
                if (len <= 7 && len > 0) {
                    memcpy(frame->data, &canfd_frame.data[1], len);
                } else {
                    len = canfd_frame.data[1];
                    memcpy(frame->data, &canfd_frame.data[2], len);
                }
                frame->can_id = canfd_frame.can_id;
                frame->len = len;
            } else {
                memcpy(frame->data, &can_frame.data[1], len);
                frame->can_id = can_frame.can_id;
                frame->len = len;
            }
            return len;
        }
        // first frame 
        else if ((pci & 0xF0) == 0x10) {
            // the first frame contains (mtu - 2) bytes data
            offset = mtu - 2;

            // compute the total len
            if (canfd) {
                memcpy(frame->data, &canfd_frame.data[2], mtu - 2);
                frame->can_id = canfd_frame.can_id;
                total_len = ((pci & 0x0F) << 8) | canfd_frame.data[1];
            } else {
                memcpy(frame->data, &can_frame.data[2], CAN_MAX_DLEN - 2);
                frame->can_id = can_frame.can_id;
                total_len = ((pci & 0x0F) << 8) | can_frame.data[1];
            }

            // send the control flow frame. note that both bs and stmin set to 0.
            FD_ZERO(&writefds);
            FD_SET(link, &writefds);
            if (canfd) {
                memset(&canfd_frame, 0, sizeof(canfd_frame));
                canfd_frame.can_id = frame->can_id;
                canfd_frame.len = 8;
                canfd_frame.data[0] = 0x30;
            } else {
                memset(&can_frame, 0, sizeof(can_frame));
                can_frame.can_id = frame->can_id;
                can_frame.can_dlc = 8;
                can_frame.data[0] = 0x30; 
            }

            struct timeval tv2 = {.tv_sec = 1};
            if (select(link + 1, NULL, &writefds, NULL, &tv2) <= 0) {
                perror("select error when send control flow frame");
                return -1;
            }
            if (canfd) {
                ret = write(link, &canfd_frame, sizeof(canfd_frame));
            } else {
                ret = write(link, &can_frame, sizeof(can_frame));
            }

            if (ret < 0) {
                perror("send FC");
                return -1;
            } 
        } 

        // receive consecutive frame
        else if ((pci & 0xF0) == 0x20) {
            sn = pci & 0x0F;
            if (sn != expected_sn) {
                fprintf(stderr, "SN mismatch: got %d, expected %d\n", sn, expected_sn);
                return -1;
            }
            expected_sn = (expected_sn + 1) % 0x10;

            size_t remaining = total_len - offset;
            size_t copy_len = (remaining > (mtu -1))? (mtu - 1) : remaining;
            if (canfd) {
                memcpy(frame->data + offset, &canfd_frame.data[1], copy_len);
            } else {
                memcpy(frame->data + offset, &can_frame.data[1], copy_len);
            }
            offset += copy_len;

            if (offset >= total_len) {
                return total_len;
            }

        } else {
            perror("PCI unhandle");
            return -1;
        }
    }
}

int linkfilter_canid(int link, uint32_t* canids, int size) {
    struct can_filter filters[size + 1];
    // default accept all data in range 0x700 ~ 0x7FF;
    filters[0].can_id = 0x700;
    filters[0].can_mask = 0x700;

    for (int i = 0; i < size; i++) {
        filters[i+1].can_id = canids[i];
        filters[i+1].can_mask = CAN_SFF_MASK;
    }

    if (setsockopt(link, SOL_CAN_RAW, CAN_RAW_FILTER, &filters, sizeof(filters)) < 0) {
        perror("setsockopt");
        return -1;
    }
    return 0;
}

int linkdisconnect(int link) {
    close(link);
}