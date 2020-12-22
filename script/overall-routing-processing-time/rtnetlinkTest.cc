#include <asm/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <net/if.h>
#include <netinet/in.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <sched.h>
#include <ifaddrs.h>

static int testTimes=0;
static char testNICName[100]={0};
struct LinkEventRecord
{
  unsigned char status;
  struct timeval time;
};
static struct LinkEventRecord *rtnetlinkEventRecords;
static struct LinkEventRecord *pollIfStatusEventRecords;
static unsigned int rtnetlinkEventID=0;
static unsigned int pollIfStatusEventID=0;

unsigned char initNICStatus;

static pthread_mutex_t mutexPrint=PTHREAD_MUTEX_INITIALIZER;//mutex for all debug print 

int readRtnetlinkEvent (int sockint, unsigned int ifIndex)
{
 //return 0 no new link status; return 1 get a new link status
  int status;
  char buf[4096];
  struct iovec iov = { buf, sizeof buf };
  struct sockaddr_nl snl;
  struct msghdr msg = { (void *) &snl, sizeof snl, &iov, 1, NULL, 0, 0 };
  struct nlmsghdr *h;
  struct ifinfomsg *ifi;

  status = recvmsg (sockint, &msg, 0);

  if (status < 0)
  {
    perror ("read_netlink: Error: ");
    exit(1);
  }

  if (status == 0)
  {
    printf ("read_netlink: EOF\n");
    return 0;
  }

  // We need to handle more than one message per 'recvmsg'
  for (h = (struct nlmsghdr *) buf; NLMSG_OK (h, (unsigned int) status);
     h = NLMSG_NEXT (h, status))
  {
    // printf("(message type %4x)\t",h->nlmsg_type);

    switch(h->nlmsg_type)// check man page for netlink(7) and rtnetlink(7)
    {
      case NLMSG_DONE:
      // printf("read_netlink: done reading\n");
      break;
      case NLMSG_NOOP:
      // printf("read_netlink: noop message\n");
      break;
      case NLMSG_ERROR:
        perror("read_netlink: message error\n");
        exit(1);
      break;
      case RTM_NEWLINK:
        ifi = (struct ifinfomsg *)NLMSG_DATA (h);
        if(ifi->ifi_index==ifIndex)
        {
          unsigned char nicStatus=(ifi->ifi_flags & IFF_RUNNING) ? 1 : 0;
          //First-time check, NIC status do not change
          if(rtnetlinkEventID==0 && nicStatus==initNICStatus)
            continue;
          //Subsequent check, NIC status do not change
          if(rtnetlinkEventID>0 && nicStatus==rtnetlinkEventRecords[rtnetlinkEventID-1].status)
            continue;

          rtnetlinkEventRecords[rtnetlinkEventID].status=nicStatus;
          gettimeofday(&(rtnetlinkEventRecords[rtnetlinkEventID].time),NULL);

          // pthread_mutex_lock(&mutexPrint);
          // printf("rtnetlink event[%5d]:\t",rtnetlinkEventID);
          // printf ("if %d(%s)\t%s\n", ifi->ifi_index, testNICName, (ifi->ifi_flags & IFF_RUNNING) ? "Up" : "Down");
          // fflush(stdout);
          // pthread_mutex_unlock(&mutexPrint);

          rtnetlinkEventID++;
          return 1;
        }
        else
        {
          char ifname[IF_NAMESIZE];
          memset(ifname,0,sizeof(char)*IF_NAMESIZE);
          if_indextoname(ifi->ifi_index, ifname);
          pthread_mutex_lock(&mutexPrint);
          printf ("Unwanted LINK!!! if %d(%s)\t%s\n", ifi->ifi_index, ifname, (ifi->ifi_flags & IFF_RUNNING) ? "Up" : "Down");
          fflush(stdout);
          pthread_mutex_unlock(&mutexPrint);
          return 0;
        }
      break;
    }
  }

  return 0;
}

void *rtnetlinkThread(void *)
{
  fd_set rfds, wfds;
  struct timeval tv;
  int retval;
  struct sockaddr_nl addr;

  int nl_socket = socket (AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
  if (nl_socket < 0)
    {
      perror ("Socket Open Error!");
      exit (1);
    }

  memset ((void *) &addr, 0, sizeof (addr));

  addr.nl_family = AF_NETLINK;
  addr.nl_pid = getpid ();
  // addr.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR;
  addr.nl_groups = RTMGRP_LINK;

  if (bind (nl_socket, (struct sockaddr *) &addr, sizeof (addr)) < 0)
    {
      perror ("Socket bind failed!");
      exit (1);
    }

  int epoll_fd;
  epoll_fd = epoll_create1(0);
  if (epoll_fd == -1)
    {
      perror ("epoll_create");
      exit(1);
    }
   
    //Add master socket recv monitor events
  struct epoll_event event;
  event.events = EPOLLIN;
  event.data.fd=nl_socket;
  if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, nl_socket, &event) == -1)
  {
    perror ("EPOLL_CTL_ADD nl_socket");
    exit(1);
  }

  struct epoll_event events[100];
  int received_socks_num=0;
  int recvSock;
  unsigned int ifIndex;
  ifIndex=if_nametoindex(testNICName);

  rtnetlinkEventRecords=(LinkEventRecord *)malloc(sizeof(LinkEventRecord)*testTimes);

  pthread_mutex_lock(&mutexPrint);
  printf("rtnetlinkThread ready to roll! Monitor NIC (name: %s|index: %d|initNICStatus: %d)! Running on CPU core %d\n"
    ,testNICName, ifIndex, initNICStatus, sched_getcpu());
  fflush(stdout);
  pthread_mutex_unlock(&mutexPrint);

  while(1)
  {
    received_socks_num=epoll_wait(epoll_fd, events, 100, -1);
    for(int i=0;i<received_socks_num;i++)
    {
      recvSock=events[i].data.fd;//The socks having coming data
      if ((events[i].events & EPOLLERR) ||
              (events[i].events & EPOLLHUP) ||
              (!(events[i].events & EPOLLIN)))
      {
        fprintf(stderr,"Epoll event: Recv sock(%d) closed... Stop the recv thread.\n",recvSock);
        shutdown(recvSock,SHUT_RDWR);
        close(recvSock);
        return 0;
      }
      readRtnetlinkEvent(recvSock,ifIndex);
      if(rtnetlinkEventID>=testTimes)
      {
        fprintf(stderr, "rtnetlinkThread done & exit! rtnetlinkEventID(%d)>=testTimes(%d)\n", rtnetlinkEventID,testTimes);
        shutdown(nl_socket,SHUT_RDWR);
        close(nl_socket);
        close(epoll_fd);
        pthread_exit(NULL);
      }
    }
  }
}

void *pollIfStatusThread(void *)
{
  struct ifaddrs *ifaddr, *ifa;
  unsigned int ifIndex;
  ifIndex=if_nametoindex(testNICName);

  pollIfStatusEventRecords=(LinkEventRecord *)malloc(sizeof(LinkEventRecord)*testTimes);

  pthread_mutex_lock(&mutexPrint);
  printf("pollIfStatusThread ready to roll! Monitor NIC (name: %s|index: %d|initNICStatus :%d)! Running on CPU core %d\n"
    ,testNICName, ifIndex, initNICStatus, sched_getcpu());
  fflush(stdout);
  pthread_mutex_unlock(&mutexPrint);

  while (1)//循环检测网络接口状态
  {
    if (0!=getifaddrs(&ifaddr))
    {
      fprintf(stderr, "getifaddrs error\n");
      exit(1);
    }
    //返回0表示成功，获取本地网络接口的信息，getifaddrs创建一个链表，链表上的每个节点都是一个struct ifaddrs结构

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
      if(ifa->ifa_addr == NULL)
        continue;
      if(if_nametoindex(ifa->ifa_name)!=ifIndex)//Not the NIC I want to monitor
        continue;

      if(pollIfStatusEventID>=testTimes)
      {
        fprintf(stderr, "pollIfStatusThread done & exit! pollIfStatusEventID(%d)>=testTimes(%d)\n", pollIfStatusEventID,testTimes);
        freeifaddrs(ifaddr);
        pthread_exit(NULL);
      }

      unsigned char nicStatus=(ifa->ifa_flags & IFF_RUNNING) ? 1 : 0;   
      //First-time check, NIC status do not change
      if(pollIfStatusEventID==0 && nicStatus==initNICStatus)
        continue;
      //Subsequent check, NIC status do not change
      if(pollIfStatusEventID>0 && nicStatus==pollIfStatusEventRecords[pollIfStatusEventID-1].status)
        continue;

      pollIfStatusEventRecords[pollIfStatusEventID].status=nicStatus;
      gettimeofday(&(pollIfStatusEventRecords[pollIfStatusEventID].time),NULL);

      // pthread_mutex_lock(&mutexPrint);
      // printf("pollIfStatus event[%5d]:\t",pollIfStatusEventID);
      // printf ("if %d(%s)\t%s\n", ifIndex, testNICName, (ifa->ifa_flags & IFF_RUNNING) ? "Up" : "Down");
      // fflush(stdout);
      // pthread_mutex_unlock(&mutexPrint);

      pollIfStatusEventID++;
    }
  }
}

pthread_t createThread(void *(*start_routine) (void *), int CpuID)
{
  pthread_t thread_id;
  pthread_attr_t attr;
  cpu_set_t cpus;
  pthread_attr_init(&attr);
  CPU_ZERO(&cpus);
  CPU_SET(CpuID, &cpus);//recv thread use cores from No.useCores-1 to No.0
  if(pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus)!=0)
  {
    fprintf(stderr, "ERROR! Thread set affinity failed!\n");
    exit(1);
  }

  if(pthread_create(&thread_id,&attr,start_routine,NULL)!=0)
  {
    fprintf(stderr, "ERROR! Thread create failed!\n");
    exit(1);
  }

  return thread_id;
}

void getNICInitStatus()
{
  struct ifaddrs *ifaddr, *ifa;
  unsigned int ifIndex;
  ifIndex=if_nametoindex(testNICName);

  if (0!=getifaddrs(&ifaddr))
  {
    fprintf(stderr, "getifaddrs error\n");
    exit(1);
  }
  //返回0表示成功，获取本地网络接口的信息，getifaddrs创建一个链表，链表上的每个节点都是一个struct ifaddrs结构

  for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
  {
    if(ifa->ifa_addr == NULL)
      continue;
    if(if_nametoindex(ifa->ifa_name)!=ifIndex)//Not the NIC I want to monitor
      continue;

    initNICStatus=(ifa->ifa_flags & IFF_RUNNING) ? 1 : 0; 

    pthread_mutex_lock(&mutexPrint);
    printf ("getNICInitStatus: if %d(%s)\tinit NIC status: %s(%d)\n", ifIndex, testNICName, (ifa->ifa_flags & IFF_RUNNING) ? "Up" : "Down", initNICStatus);
    fflush(stdout);
    pthread_mutex_unlock(&mutexPrint);

    freeifaddrs(ifaddr);
    return; 
  }

  freeifaddrs(ifaddr);
}

double time_diff(struct timeval x , struct timeval y)
{
  double x_ms , y_ms , diff;
  
  x_ms = (double)x.tv_sec*1000000 + (double)x.tv_usec;
  y_ms = (double)y.tv_sec*1000000 + (double)y.tv_usec;
  
  diff = (double)y_ms - (double)x_ms;

  return diff;
}

int main (int argc, char *argv[])
{
  if(argc!=3)
  {
    printf("usage: testNICName testTimes\n");
    exit(1);
  }

  memcpy(testNICName,argv[1],strlen(argv[1]));
  testTimes=atoi(argv[2]);

  printf("testNICName: %s \t testTimes: %d\n",testNICName,testTimes);
  fflush(stdout);

  getNICInitStatus();

  pthread_t thread_id1,thread_id2;
  thread_id1=createThread(rtnetlinkThread,0);
  thread_id2=createThread(pollIfStatusThread,1);

  usleep(1000000);
  pthread_mutex_lock(&mutexPrint);
  printf ("\n\n\nTEST ready to begin...\n\n\n");
  fflush(stdout);
  pthread_mutex_unlock(&mutexPrint);


  pthread_join(thread_id1, NULL);
  pthread_join(thread_id2, NULL);

  printf ("\n\n\nTEST end!\n\n\n");

  double totalTimeDiffBetweenEpollAndPoll=0;
  for(int i=0;i<testTimes;i++)
  {
    double timeDiff=time_diff(pollIfStatusEventRecords[i].time,rtnetlinkEventRecords[i].time);
    printf("Event[%d] time diff from poll to epoll: %.0lf us\n",i,timeDiff);
    totalTimeDiffBetweenEpollAndPoll=totalTimeDiffBetweenEpollAndPoll+timeDiff;
  }
  printf("Average time diff from poll to epoll: %.0lf us\n",totalTimeDiffBetweenEpollAndPoll/testTimes);

  free(rtnetlinkEventRecords);
  free(pollIfStatusEventRecords);

  return 0;
}