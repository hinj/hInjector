#include <stdio.h>
#include <time.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#define NETLINK_USER 31

#define MAX_PAYLOAD 1024

struct sockaddr_nl src_addr, dest_addr;
struct nlmsghdr *nlh = NULL;
struct iovec iov;
int sock_fd;
struct msghdr msg;

int main(int argc, char **argv){

  system("/sbin/insmod ./hInjLKM.ko");

  sock_fd=socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
  if(sock_fd<0){
    return -1;
  }

  memset(&src_addr, 0, sizeof(src_addr));
  src_addr.nl_family = AF_NETLINK;
  src_addr.nl_pid = getpid();

  bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr));

  memset(&dest_addr, 0, sizeof(dest_addr));
  memset(&dest_addr, 0, sizeof(dest_addr));
  dest_addr.nl_family = AF_NETLINK;
  dest_addr.nl_pid = 0;
  dest_addr.nl_groups = 0;

  nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
  memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
  nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
  nlh->nlmsg_pid = getpid();
  nlh->nlmsg_flags = 0;

  int counter = 1;
  printf("Activating HInjector LKM...\n");

  for(counter = 1; counter < argc; counter++){
    strcpy(NLMSG_DATA(nlh), argv[counter]);
    iov.iov_base = (void *)nlh;
    iov.iov_len = nlh->nlmsg_len;
    msg.msg_name = (void *)&dest_addr;
    msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    sendmsg(sock_fd,&msg,0);
  }
  strcpy(NLMSG_DATA(nlh), "0");

  iov.iov_base = (void *)nlh;
  iov.iov_len = nlh->nlmsg_len;
  msg.msg_name = (void *)&dest_addr;
  msg.msg_namelen = sizeof(dest_addr);
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  sendmsg(sock_fd,&msg,0);

  printf("Generating log file...\n");

  time_t rawtime;
  struct tm *timeinfo;
  time(&rawtime);
  timeinfo = localtime( &rawtime );
  char buff[20];
  strftime (buff,20,"%Y_%m_%d_%H_%M_%S",timeinfo);
  char log_file[80];
  strcpy(log_file,"./log/");
  strcat(log_file,buff);
  strcat(log_file,".log");

  counter=0;

  FILE *fp = fopen(log_file, "w");
  if(fp == NULL){
    printf("Creation of log file failed.");
    return EXIT_FAILURE;
  }

  time(&rawtime);
  timeinfo = localtime( &rawtime );
  fprintf(fp, "Experiment start: %s\n", asctime(timeinfo));
  do {
    strcpy(NLMSG_DATA(nlh), "");
    recvmsg(sock_fd, &msg, 0);
    fprintf(fp, "%s\n", (char *)NLMSG_DATA(nlh));
    counter++;
  } while(strcmp((char *)NLMSG_DATA(nlh),"-1") != 0);
  fclose(fp);
  close(sock_fd);
  
  system("/sbin/rmmod hInjLKM.ko");
  printf("HInjector terminated successfully.\n");
  return 0;
}
