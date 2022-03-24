#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>

#define NETLINK_USER 31  //netlink 协议簇，最大31

struct sock *nl_sk = NULL;

struct profile
{
    char enable;
    unsigned int ip;
} profile1;

static unsigned int
mybl_in_hook(unsigned int hook, struct sk_buff *skb, const struct net_device *in,
             const struct net_device *out, int (*okfn)(struct sk_buff *));

/* 钩子定义 */
static struct nf_hook_ops nf_test_ops[] __read_mostly = {
    {
        .hook = mybl_in_hook,
        .owner = THIS_MODULE,
        .pf = NFPROTO_IPV4,
        .hooknum = NF_INET_PRE_ROUTING,
        .priority = NF_IP_PRI_FIRST,
    },
};

void hdr_dump(struct ethhdr *ehdr)
{
    printk("[MAC_DES:%x,%x,%x,%x,%x,%x"
           "MAC_SRC: %x,%x,%x,%x,%x,%x Prot:%x]\n",
           ehdr->h_dest[0], ehdr->h_dest[1], ehdr->h_dest[2], ehdr->h_dest[3],
           ehdr->h_dest[4], ehdr->h_dest[5], ehdr->h_source[0], ehdr->h_source[1],
           ehdr->h_source[2], ehdr->h_source[3], ehdr->h_source[4],
           ehdr->h_source[5], ehdr->h_proto);
}

#define NIPQUAD(addr)                \
    ((unsigned char *)&addr)[0],     \
        ((unsigned char *)&addr)[1], \
        ((unsigned char *)&addr)[2], \
        ((unsigned char *)&addr)[3]
#define NIPQUAD_FMT "%u.%u.%u.%u"

/* 处理和用户空间的交互，例如：配置文件下发 */
static void mybl_nl_recv_msg(struct sk_buff *skb)
{

    printk(KERN_INFO "Entering: %s\n", __FUNCTION__);

    struct nlmsghdr *nlh;
    int pid;

    // 接收消息
    nlh = (struct nlmsghdr *)skb->data;
    memcpy(&profile1, nlmsg_data(nlh), sizeof(nlmsg_data(nlh)));
    int enable = profile1.enable;
    printk(KERN_INFO "Netlink received enable: %d\n", enable);
    if (enable == 1)
    {
        printk(KERN_INFO "Netlink received ip: %u\n", profile1.ip);
    }
    pid = nlh->nlmsg_pid; /*pid of sending process */

    // 回复消息
    struct sk_buff *skb_out;
    int res;
    int msg = 0;
    skb_out = nlmsg_new(4, 0);

    if (!skb_out)
    {
        printk(KERN_ERR "Failed to allocate new skb\n");
        return;
    }
    nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, 4, 0);
    NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */
    memcpy(nlmsg_data(nlh), &msg, 4);

    res = nlmsg_unicast(nl_sk, skb_out, pid);

    if (res < 0)
        printk(KERN_INFO "Error while sending bak to user\n");
}


/* 钩子函数 */
static unsigned int
mybl_in_hook(unsigned int hook, struct sk_buff *skb, const struct net_device *in,
             const struct net_device *out, int (*okfn)(struct sk_buff *))
{
    struct iphdr *ip_header;
    ip_header = (struct iphdr *)(skb_network_header(skb));
    // 如果配置文件使能，且配置文件的IP和源IP一致，则打印并DROP
    if (profile1.enable == 1 && ip_header->saddr == profile1.ip){
        printk("src IP:'" NIPQUAD_FMT "', ip int: %d \n", NIPQUAD(ip_header->saddr), ip_header->saddr);
        return NF_DROP;
    }
    return NF_ACCEPT;
}

/* 内核模块初始化 */
static int __init mybl_init(void)
{
    printk("Entering: %s\n", __FUNCTION__);
    // This is for 3.6 kernels and above.
    struct netlink_kernel_cfg cfg = {
        .input = mybl_nl_recv_msg,
    };
    // 打开netlink通道
    nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);
    if (!nl_sk)
    {
        printk(KERN_ALERT "Error creating socket.\n");
        return -10;
    }

    // 申明挂载钩子
    int ret;
    ret = nf_register_hooks(nf_test_ops, ARRAY_SIZE(nf_test_ops));
    if (ret < 0)
    {
        printk("register nf hook fail\n");
        return ret;
    }
    printk(KERN_NOTICE "register nf test hook\n");

    return 0;
}

/* 内核模块退出清理 */
static void __exit mybl_exit(void)
{

    printk(KERN_INFO "exiting mybl module\n");
    // 关闭netlink通道
    netlink_kernel_release(nl_sk);
    // 卸载钩子
    nf_unregister_hooks(nf_test_ops, ARRAY_SIZE(nf_test_ops));
}

module_init(mybl_init);
module_exit(mybl_exit);

MODULE_LICENSE("GPL");