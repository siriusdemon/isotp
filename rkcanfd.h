#include <inttypes.h>


typedef struct link_frame {
    uint32_t can_id;
    uint32_t len;
    uint8_t* data;
    int canfd;
}link_frame;

int linkup(int phycan);
int linkdown(int phycan);
int linkset(int phycan, int bitrate, int dbitrate);

int linkconnect(int phycan, int* link, int canfd);

int linksend(int link, const link_frame* frame, int timeout);
int linkrecv(int link, link_frame* frame, int timeout);

int linkfilter_canid(int link, uint32_t* canids, int size);

int linkdisconnect(int link);