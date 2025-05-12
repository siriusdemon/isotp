#include <inttypes.h>


typedef struct link_frame {
    uint32_t can_id;
    uint32_t len;
    uint8_t data[4096];
    bool canfd;
}link_frame;

int linkup(int phycan);
int linkdown(int phycan);
int linkset(int phycan, int bitrate, int dbitrate, int canfd);

int linkconnect(int phycan, int* link);

int linksend(int link, const link_frame* frame, int timeout);
int linkrecv(int link, link_frame* frame, int timeout);

int linkfilter(int link, uint32_t* filters, int size);

int linkdisconnect(int link);