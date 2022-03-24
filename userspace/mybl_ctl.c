#include <linux/netlink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>

#define NETLINK_USER 31

#define MAX_PAYLOAD 1024 /* maximum payload size*/
struct sockaddr_nl src_addr, dest_addr;
struct nlmsghdr *nlh = NULL;
struct iovec iov;
int sock_fd;
struct msghdr msg;

struct profile
{
    char enable;
    unsigned int ip;
};

unsigned int inet_addr(char *str)
{
    int a, b, c, d;
    char arr[4];
    sscanf(str, "%d.%d.%d.%d", &a, &b, &c, &d);
    arr[0] = a;
    arr[1] = b;
    arr[2] = c;
    arr[3] = d;
    return *(unsigned int *)arr;
}

void print_help(){
    printf("usage: ./mybl-ctl off         // 关闭IP黑名单功能\n       ./mybl-ctl on 10.0.0.2 // 打开IP黑名单功能并指定IP\n");
}

int main(int argc, char **argv)
{
    // 解析参数
    char *ipstr;
    ipstr = malloc(32);
    char enable;
    if (argc == 2) // no arguments were passed
    {
        if (strcmp("-h", argv[1]) == 0){
            print_help();
            return 0;
        }
        if (strcmp("off", argv[1]) != 0){
            printf("enable is invalid.\n");
            return -1;
        }
        enable = 0;
    }
    else if (argc == 3)
    {
        if (strcmp("on", argv[1]) != 0){
            printf("enable is invalid.\n");
            return -1;
        }
        if (sizeof(argv[2]) > 15)
        {
            printf("ip is invalid.\n");
            return -1;
        }
        enable = 1;
        strcpy(ipstr, argv[2]);
    }else{
        print_help();
        return 0;
    }

    sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
    if (sock_fd < 0)
        return -1;

    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid(); /* self pid */

    bind(sock_fd, (struct sockaddr *)&src_addr, sizeof(src_addr));

    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0;    /* For Linux Kernel */
    dest_addr.nl_groups = 0; /* unicast */

    nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
    nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
    nlh->nlmsg_pid = getpid();
    nlh->nlmsg_flags = 0;

    // 组装数据
    struct profile *profile1 = malloc(sizeof(profile1));
    profile1->enable = enable;
    if (enable == 1){
         profile1->ip = inet_addr(ipstr);
    }
    printf("config enable: %d, IP: %u\n", profile1->enable, profile1->ip);

    // 组装sock msg
    memcpy(NLMSG_DATA(nlh), profile1, sizeof(profile1));

    iov.iov_base = (void *)nlh;
    iov.iov_len = nlh->nlmsg_len;
    msg.msg_name = (void *)&dest_addr;
    msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    printf("Sending message to kernel\n");

    // 发送数据到内核
    sendmsg(sock_fd, &msg, 0);
    printf("Waiting for message from kernel\n");

    // 等待内核数据
    recvmsg(sock_fd, &msg, 0);

    // 判断结果
    int msg_res;
    memcpy(&msg_res, NLMSG_DATA(nlh), 4);
    if (msg_res == 0)
    {
        printf("config success.\n");
    }
    else
    {
        printf("config fail.\n");
    }
    close(sock_fd);
}