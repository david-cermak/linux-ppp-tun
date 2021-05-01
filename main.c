#include <stdio.h>
#include <string.h>
#include "sio.h"
#include "ip_inout.h"
#include <sys/time.h>
#include <sys/param.h>
#include <lwip/tcpip.h>
#include "netif/ppp/pppos.h"
#include "lwip/dns.h"
#include "errno.h"

void ppp_init(void);

static void ppp_link_status_cb(ppp_pcb *pcb, int err_code, void *ctx)
{
    struct netif *pppif = ppp_netif(pcb);
    LWIP_UNUSED_ARG(ctx);

    switch(err_code) {
        case PPPERR_NONE:               /* No error. */
        {
#if LWIP_DNS
            const ip_addr_t *ns;
#endif /* LWIP_DNS */
            fprintf(stderr, "ppp_link_status_cb: PPPERR_NONE\n\r");
#if LWIP_IPV4
            fprintf(stderr, "   our_ip4addr = %s\n\r", ip4addr_ntoa(netif_ip4_addr(pppif)));
            fprintf(stderr, "   his_ipaddr  = %s\n\r", ip4addr_ntoa(netif_ip4_gw(pppif)));
            fprintf(stderr, "   netmask     = %s\n\r", ip4addr_ntoa(netif_ip4_netmask(pppif)));
#endif /* LWIP_IPV4 */
#if LWIP_IPV6
            fprintf(stderr, "   our_ip6addr = %s\n\r", ip6addr_ntoa(netif_ip6_addr(pppif, 0)));
#endif /* LWIP_IPV6 */

#if LWIP_DNS
            ns = dns_getserver(0);
            fprintf(stderr, "   dns1        = %s\n\r", ipaddr_ntoa(ns));
            ns = dns_getserver(1);
            fprintf(stderr, "   dns2        = %s\n\r", ipaddr_ntoa(ns));
#endif /* LWIP_DNS */
#if PPP_IPV6_SUPPORT
            fprintf(stderr, "   our6_ipaddr = %s\n\r", ip6addr_ntoa(netif_ip6_addr(pppif, 0)));
#endif /* PPP_IPV6_SUPPORT */
        }
            break;

        case PPPERR_PARAM:             /* Invalid parameter. */
            printf("ppp_link_status_cb: PPPERR_PARAM\n");
            break;

        case PPPERR_OPEN:              /* Unable to open PPP session. */
            printf("ppp_link_status_cb: PPPERR_OPEN\n");
            break;

        case PPPERR_DEVICE:            /* Invalid I/O device for PPP. */
            printf("ppp_link_status_cb: PPPERR_DEVICE\n");
            break;

        case PPPERR_ALLOC:             /* Unable to allocate resources. */
            printf("ppp_link_status_cb: PPPERR_ALLOC\n");
            break;

        case PPPERR_USER:              /* User interrupt. */
            printf("ppp_link_status_cb: PPPERR_USER\n");
            break;

        case PPPERR_CONNECT:           /* Connection lost. */
            printf("ppp_link_status_cb: PPPERR_CONNECT\n");
            break;

        case PPPERR_AUTHFAIL:          /* Failed authentication challenge. */
            printf("ppp_link_status_cb: PPPERR_AUTHFAIL\n");
            break;

        case PPPERR_PROTOCOL:          /* Failed to meet protocol. */
            printf("ppp_link_status_cb: PPPERR_PROTOCOL\n");
            break;

        case PPPERR_PEERDEAD:          /* Connection timeout. */
            printf("ppp_link_status_cb: PPPERR_PEERDEAD\n");
            break;

        case PPPERR_IDLETIMEOUT:       /* Idle Timeout. */
            printf("ppp_link_status_cb: PPPERR_IDLETIMEOUT\n");
            break;

        case PPPERR_CONNECTTIME:       /* PPPERR_CONNECTTIME. */
            printf("ppp_link_status_cb: PPPERR_CONNECTTIME\n");
            break;

        case PPPERR_LOOPBACK:          /* Connection timeout. */
            printf("ppp_link_status_cb: PPPERR_LOOPBACK\n");
            break;

        default:
            printf("ppp_link_status_cb: unknown errCode %d\n", err_code);
            break;
    }
}

static u32_t ppp_output_cb(struct ppp_pcb_s *pcb, const void *data, u32_t len, void *ctx)
{
    int fd = (long)ctx;
    return sio_write(fd, data, len);
}


int main(int argc, char *argv[])
{
    uint8_t sio_buffer[128];
    int sio_fd;
    int tun_fd;
    struct netif pppos_netif;
    ppp_pcb *ppp;

    if (argc != 3) {
        printf("Usage: uart-ppp-tun <SIO-device> <TUN-device>\n");
        printf("Example: uart-ppp-tun /dev/ttyUSB0 tun0\n");
        return 1;
    }
    sio_fd = sio_init(argv[1]); // /dev/ttyUSB0
    if (sio_fd < 0) {
        return 2;
    }
    tun_fd = tun_init(argv[2]); // tun0
    if (tun_fd < 0) {
        return 2;
    }
    sys_sem_t init_sem;
    err_t err = sys_sem_new(&init_sem, 0);
    if (err != ERR_OK) {
        printf("failed to create init_sem");
        return 2;
    }

    // Init necessary units of lwip (no need for the tcpip thread)
    sys_init();
    mem_init();
    memp_init();
    netif_init();
    dns_init();
    ppp_init();
    sys_timeouts_init();

    // init and start connection attempts on PPP interface
    ppp = pppos_create(&pppos_netif, ppp_output_cb, ppp_link_status_cb, (void*)(long)sio_fd);
    ppp_connect(ppp, 0);

    fd_set fds;
    int fmax = MAX(tun_fd, sio_fd) + 1;

    while (1) {
        FD_ZERO(&fds);
        FD_SET(tun_fd, &fds);
        FD_SET(sio_fd, &fds);
        struct timeval tv = { .tv_usec = 0, .tv_sec = 1 };

        if (select(fmax, &fds, NULL, NULL, &tv) < 0) {
            printf("select failed with errno:%i :%s\n", errno, strerror(errno));
            goto cleanup;
        }

        sys_check_timeouts();

        if (FD_ISSET(tun_fd, &fds)) {
            if (tun_read(tun_fd, &pppos_netif) < 0) {
                printf("Failed to read from tun interface\n");
                printf("errno:%i :%s\n", errno, strerror(errno));
                goto cleanup;
            }
        }

        if (FD_ISSET(sio_fd, &fds)) {
            int len = sio_read(sio_fd, sio_buffer, sizeof(sio_buffer));
            if (len < 0) {
                printf("Failed to read from SIO interface\n");
                printf("errno:%i :%s\n", errno, strerror(errno));
                goto cleanup;
            }
            pppos_input(ppp, sio_buffer, len);
        }
    }
    return 0;

cleanup:
    tun_deinit(tun_fd);
    return -1;
}
