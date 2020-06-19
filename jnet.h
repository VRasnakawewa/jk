#ifndef JNET_H
#define JNET_H

int jnetResolvePassive(struct addrinfo **info, const char *port);
int jnetResolveNumericHost(struct addrinfo **info,
                           const char *ip,
                           const char *port);
int jnetBind(int fd, struct addrinfo *info);
int jnetSocket(struct addrinfo *info);
int jnetGetSocketErrno(int fd);
int jnetConnect(int fd, struct addrinfo *info);

#endif
