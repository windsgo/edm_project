#include <arpa/inet.h>
#include <linux/can.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Logger/LogMacro.h"

#include <QTimer>
#include <QCoreApplication>

EDM_STATIC_LOGGER(logger, EDM_LOGGER_ROOT());

#include <linux/netlink.h>
#include <linux/rtnetlink.h>
char buff[4096];

void parse_rtattr(struct rtattr **tb, int max, struct rtattr *attr, int len) {
    for (; RTA_OK(attr, len); attr = RTA_NEXT(attr, len)) {
        if (attr->rta_type <= max) {
            tb[attr->rta_type] = attr;
        }
    }
}

void nl_netifinfo_handle(struct nlmsghdr *nlh) {
    int len;
    struct rtattr *tb[IFLA_MAX + 1];
    struct ifinfomsg *ifinfo;

    bzero(tb, sizeof(tb));
    ifinfo = (ifinfomsg *)NLMSG_DATA(nlh);
    len = nlh->nlmsg_len - NLMSG_SPACE(sizeof(*ifinfo));
    parse_rtattr(tb, IFLA_MAX, IFLA_RTA(ifinfo), len);

    printf("%s: %s ", (nlh->nlmsg_type == RTM_NEWLINK) ? "NEWLINK" : "DELLINK",
           (ifinfo->ifi_flags & IFF_UP) ? "up" : "down");
    if (tb[IFLA_IFNAME]) {
        printf("%s", (char*)(RTA_DATA(tb[IFLA_IFNAME])));
    }
    printf("\n");
}

static void test_netlink() {
    logger->info("test netlink");

    struct {
        struct nlmsghdr nh;
        struct ifinfomsg info;
        char attrbuf[512];
    } req;

    int rtnetlink_sk = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
    if (rtnetlink_sk < 0) {
        logger->error("get rtnetlink_sk error: {}", rtnetlink_sk);
        return;
    }

    // struct timeval tv;
    // tv.tv_sec = 0; //  注意防core
    // tv.tv_usec = 100000;
    // setsockopt(rtnetlink_sk, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    memset(&req, 0, sizeof(req));
    req.nh.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
    req.nh.nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST;
    req.nh.nlmsg_type = RTM_GETLINK;
    req.nh.nlmsg_seq = 2;
    req.info.ifi_family = AF_UNSPEC;

    int send_ret = send(rtnetlink_sk, &req, req.nh.nlmsg_len, 0);
    logger->info("send_ret : {}", send_ret);

    sockaddr_nl addr;
    socklen_t len = sizeof(addr);
    bool recv_done = false;

    while (!recv_done) {
        int recv_ret = recvfrom(rtnetlink_sk, &buff, sizeof(buff), 0,
                           (sockaddr *)&addr, &len);
        logger->info("recv_ret: {}", recv_ret);
        if (recv_ret < 0) {
            logger->error("recv err: {}", recv_ret);
            break;
        }

        nlmsghdr *nlh = (nlmsghdr *)buff;

        for (; NLMSG_OK(nlh, recv_ret); nlh = NLMSG_NEXT(nlh, recv_ret)) {
            logger->debug("type: {}, len: {}", nlh->nlmsg_type, nlh->nlmsg_len);
            switch (nlh->nlmsg_type) {
            case NLMSG_DONE:
                logger->trace("recv done");
                recv_done = true;
                break;
            case NLMSG_ERROR:
                break;
            case RTM_NEWLINK: /* */
            case RTM_DELLINK:
                nl_netifinfo_handle(nlh);
                break;
            default:
                break;
            }
        }
    }
}



#include "Utils/Netif/netif_utils.h"

static void test_util() {
    auto list = edm::util::get_netdev_list();

    puts("1");
    if (!list) {
        logger->error("get_netdev_list error");
        return;
    }

    puts("2");

    for (auto& i : list.value()) {
        logger->debug("{}", i.name);
    }
}

int main(int argc, char** argv) {

    // test_netlink();

    // test_libnetlink();

    test_util();

    QCoreApplication app(argc, argv);

    QTimer t;
    QObject::connect(&t, &QTimer::timeout, [&](){
        logger->info("can0 exists: {}", edm::util::is_netdev_exist("can0"));
    });

    t.start(1000);

    return app.exec();
}