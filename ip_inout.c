#include <string.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include "netif/ppp/pppos.h"
#include "lwip/ip6.h"

static int tun_fd = -1;

err_t ip6_input(struct pbuf *p, struct netif *inp)
{
    char tun_header[4] = { 0,0,0x86, 0xdd }; // Ethernet (IPv6)
    // todo: IPv6
    return ERR_OK;
}

err_t ip4_input(struct pbuf *p, struct netif *inp)
{
    char buf[1518];
    char tun_header[4] = { 0,0,0x08, 0 }; // Ethernet (IPv4)
    memcpy(buf, tun_header, 4);
    memcpy(buf + 4, p->payload, p->len); // todo: handle pbuf chains
    int i = write(tun_fd, buf, p->len + 4);
    printf("Written %d\n", i);
    return ERR_OK;
}


int tun_init(char *devname)
{
    struct ifreq ifr;
    int err;
    char buf[1518];

    if ((tun_fd = open("/dev/net/tun", O_RDWR)) == -1) {
        perror("open /dev/net/tun");
        return -1;
    }
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN;
    strncpy(ifr.ifr_name, devname, IFNAMSIZ);

    if ((err = ioctl(tun_fd, TUNSETIFF, (void *)&ifr)) == -1) {
        perror("ioctl TUNSETIFF %m");
        close(tun_fd);
        tun_fd = -1;
        return -1;
    }
    ioctl(tun_fd, TUNSETNOCSUM, 1);
    return tun_fd;
}

int tun_read(int fd, struct netif *pppos_netif)
{
    char buf[1518];
    struct pbuf *p;
    ssize_t len = read(tun_fd, buf, sizeof(buf));
    if (len < 0) {
        perror("read returned -1");
        return -1;
    }

    printf("Received %zd bytes from desc %d\n", len, tun_fd);
    if (len <= 4)
        return -1;
    len -= 4;
    p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
    if (p == NULL) {
        perror("Cannot allocate pbuf");
        return -1;
    }
    pbuf_take(p, buf+4, len);
    pppos_netif->output(pppos_netif, p, NULL);
}