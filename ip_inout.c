#include <string.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include "netif/ppp/pppos.h"
#include "lwip/ip6.h"

#define BUF_SIZE 1518

static int tun_fd = -1;
static char *in_buf = NULL;
static char *out_buf = NULL;
static unsigned char ip6_header[4] = { 0, 0, 0x86, 0xdd };  // Ethernet (IPv6)
static unsigned char ip4_header[4] = { 0, 0, 0x08, 0 };     // Ethernet (IPv4)

static err_t tun_input(struct pbuf *p, unsigned char tun_header[4])
{
    struct pbuf *n;
    memcpy(in_buf, tun_header, 4);
    for (n = p; n; n = n->next) {
        memcpy(in_buf + 4, n->payload, n->len);
        if (write(tun_fd, in_buf, n->len + 4) != n->len + 4) {
            return ERR_ABRT;
        }
    }
    return ERR_OK;
}

err_t ip6_input(struct pbuf *p, struct netif *inp)
{
    return tun_input(p, ip6_header);
}

err_t ip4_input(struct pbuf *p, struct netif *inp)
{
    return tun_input(p, ip4_header);
}


int tun_init(char *devname)
{
    struct ifreq ifr;

    if ((tun_fd = open("/dev/net/tun", O_RDWR)) == -1) {
        perror("open /dev/net/tun");
        return -1;
    }
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN;
    strncpy(ifr.ifr_name, devname, IFNAMSIZ);

    if (ioctl(tun_fd, TUNSETIFF, (void *)&ifr) == -1) {
        perror("ioctl TUNSETIFF %m");
        close(tun_fd);
        tun_fd = -1;
        return -1;
    }
    ioctl(tun_fd, TUNSETNOCSUM, 1);

    in_buf = malloc(BUF_SIZE);
    out_buf = malloc(BUF_SIZE);
    if (in_buf ==  NULL || out_buf == NULL) {
        free(in_buf);
        free(out_buf);
        tun_fd = -1;
    }
    return tun_fd;
}

void tun_deinit(int fd)
{
    close(fd);
    free(in_buf);
    free(out_buf);
    tun_fd = -1;
}

int tun_read(int fd, struct netif *pppos_netif)
{
    struct pbuf *p;
    ssize_t len = read(tun_fd, out_buf, BUF_SIZE);
    if (len < 0) {
        perror("read returned -1");
        return -1;
    }

    if (len <= 4) {
        return -1;
    }
    len -= 4;
    p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
    if (p == NULL) {
        perror("Cannot allocate pbuf");
        return -1;
    }
    pbuf_take(p, out_buf + 4, len);
    if (memcmp(out_buf, ip6_header, 4) == 0) {
        pppos_netif->output_ip6(pppos_netif, p, NULL);
    } else if (memcmp(out_buf, ip4_header, 4) == 0) {
        pppos_netif->output(pppos_netif, p, NULL);
    } else {
        printf("Unknown protocol %x %x\n", out_buf[2], out_buf[3]);
        pbuf_free(p);
    }
}