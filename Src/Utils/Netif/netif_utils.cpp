#include "netif_utils.h"

#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

static std::atomic<uint64_t> s_seq = 0;

// static char s_buf[32768];

namespace edm {

namespace util {

static void parse_rtattr(struct rtattr **tb, int max, struct rtattr *attr,
                         int len) {
    for (; RTA_OK(attr, len); attr = RTA_NEXT(attr, len)) {
        if (attr->rta_type <= max) {
            tb[attr->rta_type] = attr;
        }
    }
}

static void netifinfo_get_netdev_info(struct nlmsghdr *nlh, netdev_info &info) {
    int len;
    struct rtattr *tb[IFLA_MAX + 1];
    struct ifinfomsg *ifinfo;

    bzero(tb, sizeof(tb));
    ifinfo = (ifinfomsg *)NLMSG_DATA(nlh);
    len = nlh->nlmsg_len - NLMSG_SPACE(sizeof(*ifinfo));
    parse_rtattr(tb, IFLA_MAX, IFLA_RTA(ifinfo), len);

    // parse over, get data
    if (tb[IFLA_IFNAME]) {
        info.name = (char *)(RTA_DATA(tb[IFLA_IFNAME]));
    } else {
        info.name = "UNKNOWN";
    }

    info.link_up = !!(ifinfo->ifi_flags & IFF_UP);
}

std::optional<std::vector<netdev_info>> get_netdev_list() {

    class socket_wrapper {
    public:
        socket_wrapper() {
            fd_ = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
        }
        ~socket_wrapper() {
            if (fd_ >= 0) {
                (void)close(fd_);
            }
        }

        int fd() const { return fd_; }

    private:
        int fd_;
    };

    auto sk_wrapper = socket_wrapper();

    // create an rtnetlink socket
    int sk = sk_wrapper.fd();
    if (sk < 0) {
        s_logger->error(
            "get_netdev_info failed, can not create rtnetlink socket. {}", sk);
        return std::nullopt;
    }

    // request if info
    struct {
        struct nlmsghdr nh;
        struct ifinfomsg info;
        char attrbuf[512];
    } req;
    req.nh.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
    req.nh.nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST;
    req.nh.nlmsg_type = RTM_GETLINK;
    req.nh.nlmsg_seq = ++s_seq;
    req.info.ifi_family = AF_UNSPEC;

    // send request
    int send_ret = send(sk, &req, req.nh.nlmsg_len, 0);
    if (send_ret < 0) {
        s_logger->error("get_netdev_info send error : {}", send_ret);
        return std::nullopt;
    }

    sockaddr_nl addr;
    struct iovec iov;
    static socklen_t len = sizeof(addr);
    bool recv_done = false; // wait for msg done received

    std::optional<std::vector<netdev_info>> ret_v = std::nullopt;

    while (!recv_done) {

        // get msg size
        msghdr mh {
            .msg_name = &addr,
            .msg_namelen = len,
            .msg_iov = &iov,
            .msg_iovlen = 1
        };
        mh.msg_iov->iov_base = NULL;
        mh.msg_iov->iov_len = 0;

        int msg_size = recvmsg(sk, &mh, MSG_PEEK | MSG_TRUNC);
        if (msg_size < 0) {
            s_logger->error("recvmsg failed, return {}", msg_size);
            return ret_v;
        }

        if (msg_size < 32768) {
            msg_size = 32768;
        }

        // char *buf = new char[msg_size];
        std::string s; // use string to manage buffer
        s.resize(msg_size);
        char* p_raw_buf = s.data();

        // recv from kernel
        int recv_ret = recvfrom(sk, p_raw_buf, msg_size, 0, (sockaddr *)&addr, &len);
        if (recv_ret < 0) {
            s_logger->error("recvfrom failed, return {}", recv_ret);
            return ret_v;
        }

        nlmsghdr *nlh = reinterpret_cast<nlmsghdr *>(p_raw_buf);
        for (; NLMSG_OK(nlh, recv_ret); nlh = NLMSG_NEXT(nlh, recv_ret)) {
            switch (nlh->nlmsg_type) {
            case NLMSG_DONE:
                recv_done = true;
                break;
            case RTM_NEWLINK: {
                netdev_info info;
                netifinfo_get_netdev_info(nlh, info);

                if (ret_v.has_value()) {
                    ret_v.value().push_back(info);
                } else {
                    std::vector<netdev_info> vec_;
                    vec_.push_back(info);
                    ret_v.emplace(vec_);
                }
                break;
            }
            case RTM_DELLINK:
            case NLMSG_ERROR:
            default:
                break;
            }
        }
    }

    return ret_v;
}

static int _netdev_index(const std::string &netdev_name,
                         const std::vector<netdev_info> &dev_list) {
    for (int i = 0; i < dev_list.size(); ++i) {
        if (dev_list[i].name == netdev_name) {
            return i;
        }
    }

    return -1;
}

bool is_netdev_exist(const std::string &netdev_name) {
    auto netdev_list = get_netdev_list();
    if (!netdev_list) {
        return false;
    }

    int index = _netdev_index(netdev_name, netdev_list.value());
    if (index < 0) {
        return false;
    } else {
        return true;
    }
}

std::optional<netdev_info> get_netdev_info(const std::string &netdev_name) {
    auto netdev_list = get_netdev_list();
    if (!netdev_list) {
        return std::nullopt;
    }

    int index = _netdev_index(netdev_name, netdev_list.value());
    if (index < 0) {
        return std::nullopt;
    }

    return netdev_list.value()[index];
}

} // namespace util

} // namespace edm
