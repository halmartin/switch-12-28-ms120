/*
 *	BIRD Internet Routing Daemon -- Linux Multicasting and Network Includes
 *
 *	(c) 1998--2000 Martin Mares <mj@ucw.cz>
 *	(c) 2014--2017 Cisco Systems, Inc.
 *
 *	Can be freely distributed and used under the terms of the GNU GPL.
 */

#ifndef _BIRD_SYSIO_H_
#define _BIRD_SYSIO_H_

#include <netinet/tcp.h>
#include <sockproxy.h>
#include "lib/ip.h"

#ifdef IPV6

#ifndef IPV6_UNICAST_HOPS
/* Needed on glibc 2.0 systems */
#include <linux/in6.h>
#define CONFIG_IPV6_GLIBC_20
#endif

static inline void
set_inaddr(struct in6_addr *ia, ip_addr a)
{
  ipa_hton(a);
  memcpy(ia, &a, sizeof(a));
}

static inline void
get_inaddr(ip_addr *a, struct in6_addr *ia)
{
  memcpy(a, ia, sizeof(*a));
  ipa_ntoh(*a);
}

#else

static inline void
set_inaddr(struct in_addr *ia, ip_addr a)
{
  ipa_hton(a);
  memcpy(&ia->s_addr, &a, sizeof(a));
}

static inline void
get_inaddr(ip_addr *a, struct in_addr *ia)
{
  memcpy(a, &ia->s_addr, sizeof(*a));
  ipa_ntoh(*a);
}

#endif /* IPV6 */


#ifndef HAVE_STRUCT_IP_MREQN
/* Several versions of glibc don't define this structure, so we have to do it ourselves */
struct ip_mreqn
{
  struct in_addr imr_multiaddr;			/* IP multicast address of group */
  struct in_addr imr_address;			/* local IP address of interface */
  int		 imr_ifindex;			/* Interface index */
};
#endif

#ifndef IP_MINTTL
#define IP_MINTTL 21
#endif

#ifndef IPV6_TCLASS
#define IPV6_TCLASS 67
#endif

#ifndef IPV6_MINHOPCOUNT
#define IPV6_MINHOPCOUNT 73
#endif


#ifndef TCP_MD5SIG

#define TCP_MD5SIG  14
#define TCP_MD5SIG_MAXKEYLEN 80

struct tcp_md5sig {
  struct  sockaddr_storage tcpm_addr;             /* address associated */
  u16   __tcpm_pad1;                              /* zero */
  u16   tcpm_keylen;                              /* key length */
  u32   __tcpm_pad2;                              /* zero */
  u8    tcpm_key[TCP_MD5SIG_MAXKEYLEN];           /* key (binary) */
};

#endif


/* Linux does not care if sa_len is larger than needed */
#define SA_LEN(x) sizeof(sockaddr)


/*
 *	Linux IPv4 multicast syscalls
 */

#define INIT_MREQ4(maddr,ifa) \
  { .imr_multiaddr = ipa_to_in4(maddr), .imr_ifindex = ifa->index }

static inline int
sk_setup_multicast4(sock *s)
{
  struct ip_mreqn mr = { .imr_ifindex = s->iface->index };
  int ttl = s->ttl;
  int n = 0;

  /* This defines where should we send _outgoing_ multicasts */
  if (CSP_setsockopt(s->fd, SOL_IP, IP_MULTICAST_IF, &mr, sizeof(mr)) < 0)
    ERR("IP_MULTICAST_IF");

  if (CSP_setsockopt(s->fd, SOL_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0)
    ERR("IP_MULTICAST_TTL");

  if (CSP_setsockopt(s->fd, SOL_IP, IP_MULTICAST_LOOP, &n, sizeof(n)) < 0)
    ERR("IP_MULTICAST_LOOP");

  return 0;
}

static inline int
sk_join_group4(sock *s, ip_addr maddr)
{
  struct ip_mreqn mr = INIT_MREQ4(maddr, s->iface);

  if (CSP_setsockopt(s->fd, SOL_IP, IP_ADD_MEMBERSHIP, &mr, sizeof(mr)) < 0)
    ERR("IP_ADD_MEMBERSHIP");

  return 0;
}

static inline int
sk_leave_group4(sock *s, ip_addr maddr)
{
  struct ip_mreqn mr = INIT_MREQ4(maddr, s->iface);

  if (CSP_setsockopt(s->fd, SOL_IP, IP_DROP_MEMBERSHIP, &mr, sizeof(mr)) < 0)
    ERR("IP_DROP_MEMBERSHIP");

  return 0;
}


/*
 *	Linux IPv4 packet control messages
 */

/* Mostly similar to standardized IPv6 code */

#define CMSG4_SPACE_PKTINFO CMSG_SPACE(sizeof(struct in_pktinfo))
#define CMSG4_SPACE_TTL CMSG_SPACE(sizeof(int))

static inline int
sk_request_cmsg4_pktinfo(sock *s)
{
  int y = 1;

  if (CSP_setsockopt(s->fd, SOL_IP, IP_PKTINFO, &y, sizeof(y)) < 0)
    ERR("IP_PKTINFO");

  return 0;
}

static inline int
sk_request_cmsg4_ttl(sock *s)
{
  int y = 1;

  if (CSP_setsockopt(s->fd, SOL_IP, IP_RECVTTL, &y, sizeof(y)) < 0)
    ERR("IP_RECVTTL");

  return 0;
}

static inline void
sk_process_cmsg4_pktinfo(sock *s, struct cmsghdr *cm)
{
  if (cm->cmsg_type == IP_PKTINFO)
  {
    struct in_pktinfo *pi = (struct in_pktinfo *) CMSG_DATA(cm);
    s->laddr = ipa_from_in4(pi->ipi_addr);
    s->lifindex = pi->ipi_ifindex;
  }
}

static inline void
sk_process_cmsg4_ttl(sock *s, struct cmsghdr *cm)
{
  if (cm->cmsg_type == IP_TTL)
    s->rcv_ttl = * (int *) CMSG_DATA(cm);
}

static inline void
sk_prepare_cmsgs4(sock *s, struct msghdr *msg, void *cbuf, size_t cbuflen)
{
  struct cmsghdr *cm;
  struct in_pktinfo *pi;
  int controllen = 0;

  msg->msg_control = cbuf;
  msg->msg_controllen = cbuflen;

  cm = CMSG_FIRSTHDR(msg);
  cm->cmsg_level = SOL_IP;
  cm->cmsg_type = IP_PKTINFO;
  cm->cmsg_len = CMSG_LEN(sizeof(*pi));
  controllen += CMSG_SPACE(sizeof(*pi));

  pi = (struct in_pktinfo *) CMSG_DATA(cm);
  pi->ipi_ifindex = s->iface ? s->iface->index : 0;
  pi->ipi_spec_dst = ipa_to_in4(s->saddr);
  pi->ipi_addr = ipa_to_in4(IPA_NONE);

  msg->msg_controllen = controllen;
}


/*
 *	Miscellaneous Linux socket syscalls
 */

static inline int
sk_set_min_ttl4(sock *s, int ttl)
{
  if (CSP_setsockopt(s->fd, SOL_IP, IP_MINTTL, &ttl, sizeof(ttl)) < 0)
  {
    if (errno == ENOPROTOOPT)
      ERR_MSG("Kernel does not support IPv4 TTL security");
    else
      ERR("IP_MINTTL");
  }

  return 0;
}

static inline int
sk_set_min_ttl6(sock *s, int ttl)
{
  if (CSP_setsockopt(s->fd, SOL_IPV6, IPV6_MINHOPCOUNT, &ttl, sizeof(ttl)) < 0)
  {
    if (errno == ENOPROTOOPT)
      ERR_MSG("Kernel does not support IPv6 TTL security");
    else
      ERR("IPV6_MINHOPCOUNT");
  }

  return 0;
}

static inline int
sk_disable_mtu_disc4(sock *s)
{
  int dont = IP_PMTUDISC_DONT;

  if (CSP_setsockopt(s->fd, SOL_IP, IP_MTU_DISCOVER, &dont, sizeof(dont)) < 0)
    ERR("IP_MTU_DISCOVER");

  return 0;
}

static inline int
sk_disable_mtu_disc6(sock *s)
{
  int dont = IPV6_PMTUDISC_DONT;

  if (CSP_setsockopt(s->fd, SOL_IPV6, IPV6_MTU_DISCOVER, &dont, sizeof(dont)) < 0)
    ERR("IPV6_MTU_DISCOVER");

  return 0;
}

extern int sk_priority_control;

static inline int
sk_set_priority(sock *s, int prio)
{
  if (CSP_setsockopt(s->fd, SOL_SOCKET, SO_PRIORITY, &prio, sizeof(prio)) < 0)
    ERR("SO_PRIORITY");

  return 0;
}

#endif /* _BIRD_SYSIO_H_ */