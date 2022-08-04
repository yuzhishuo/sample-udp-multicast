#include <arpa/inet.h>
#include <configure.h>  // for CMAKE_PROJECT_VERSION_MAJOR, CMAKE_PROJECT_VERSION_MINOR
#include <netinet/in.h>
#include <spdlog/spdlog.h>  // for SPDLOG_ERROR, SPDLOG_INFO
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <thread>

#define MSGBUFSIZE 256
#define TRAM_STATUS_ADDR "239.0.0.30"
#define TRAM_STATUS_RECV_PORT 9200

int server() {
  struct sockaddr_in mcast_addr;
  int fd, cnt;
  int addrlen, num;
  char msg[32];

  sprintf(msg, "%s", "hello");

  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket");
    return -1;
  }

  memset(&mcast_addr, 0, sizeof(mcast_addr));
  mcast_addr.sin_family = AF_INET;
  mcast_addr.sin_addr.s_addr = inet_addr(TRAM_STATUS_ADDR);
  mcast_addr.sin_port = htons(TRAM_STATUS_RECV_PORT);
  using namespace std::chrono_literals;
  std::this_thread::sleep_for(3s);
  if (sendto(fd, (const char *)msg, sizeof(msg), 0,
             (struct sockaddr *)&mcast_addr, sizeof(mcast_addr)) < 0) {
    perror("sendto");
    return -1;
  }
  printf("send ok!\n");

  setsockopt(fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mcast_addr,
             sizeof(mcast_addr));
  close(fd);

  return 0;
}

int client() {
  struct sockaddr_in addr;
  int fd, nbytes, addrlen;
  struct ip_mreq mreq;
  char msgbuf[MSGBUFSIZE];
  int on;

  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket ");
    return -1;
  }
  /* 初始化地址 */
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(TRAM_STATUS_RECV_PORT);

  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    return -1;
  }

  on = 1;
  /* 设置地址复用许可, 根据具体情况判断是否增加此功能 */
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
    perror("SO_REUSEADDR");
    return -1;
  }

  /*加入多播组*/
  mreq.imr_multiaddr.s_addr = inet_addr(TRAM_STATUS_ADDR);
  mreq.imr_interface.s_addr = htonl(INADDR_ANY);
  if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
    perror("setsockopt");
    return -1;
  }

  addrlen = sizeof(addr);
  if ((nbytes = recvfrom(fd, msgbuf, MSGBUFSIZE, 0, (struct sockaddr *)&addr,
                         (socklen_t *)&addrlen)) < 0) {
    perror("recvfrom");
    return -1;
  }
  printf("recv ok!\n");

  /*退出多播组*/
  setsockopt(fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
  close(fd);

  return 0;
}

int main(int argc, char **argv) {
  std::thread t1(client);
  std::thread t2(server);
  t1.join();
  t2.join();
  return 0;
}