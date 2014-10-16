/*
* Tested on Xen 4.4.1, Host OS: Debian 7.0, 64 bit (amd64)
* To Do's:
*   --- Error handling;
*   --- Verification of injection scenario configuration file;
*   --- Implement non-blocking netlink socket communication to reduce performance overhead; 
*   --- Implement the gen_hypercallXarg functions as macros for efficiency; 
*   --- Improve the hypercall invocation process (e.g., reduce the number of macros used, eliminate hypercall number <-> number of hypercall parameters dependency --- see, for example, __MHYPERCALL1ARGS_IF);
*   --- Improve the method for adding the field hypercall_number in the shared_info structure (e.g., do not add hypercall_number after padding[32]);
*   --- Improve the generation of struct hypercall parameter addresses (use struct names to create struct objects instead of names of first struct members);
*        --- Create macros/functionality for storing values in an arbitrary number of members per struct (e.g., from 1 member to the maximal number of members of a given struct);
*   --- Create HInjector LKM header file(s);
*   --- Fix the memory leak related to hinjHcall_input and hinjMembers
*/

#undef __KERNEL__
#define __KERNEL__
 
#undef MODULE
#define MODULE
 
/* Linux header files */
#include <linux/module.h>    
#include <linux/kernel.h>    
#include <linux/init.h>        
#include <linux/delay.h>      
#include <linux/time.h>      

/* Xen header files */
#include <linux/slab.h>
#include <asm/xen/page.h>
#include <asm/xen/hypervisor.h>
#include <asm/xen/hypercall.h>
#include <xen/interface/memory.h>

/* Netlink header files */
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>

MODULE_LICENSE("GPL");

/*
*   Macros and defines
*/


/*
*   The macros below produce virtual memory adresses pointing to struct objects. 
*   The macros are used for passing structs to injected hypercalls. 
*/

#define FILL_STRUCTARGS0(arg0) \
        struct arg0 ob; \
        hinjStructAddr = (unsigned long int)&ob;

#define FILL_STRUCTARGS1(arg0, arg1, arg2) \
        FILL_STRUCTARGS0(arg0) \
        ob.arg1 = arg2;

#define FILL_STRUCTARGS2(arg0, arg1, arg2, arg3, arg4) \
        FILL_STRUCTARGS1(arg0, arg1, arg2) \
        ob.arg3 = arg4;

#define FILL_STRUCTARGS3(arg0, arg1, arg2, arg3, arg4, arg5, arg6) \
        FILL_STRUCTARGS2(arg0, arg1, arg2, arg3, arg4) \
        ob.arg5 = arg6;

#define FILL_STRUCTARGS4(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8) \
        FILL_STRUCTARGS3(arg0, arg1, arg2, arg3, arg4, arg5, arg6) \
        ob.arg7 = arg8; 

#define FILL_STRUCTARGS5(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10) \
        FILL_STRUCTARGS4(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8) \
        ob.arg9 = arg10;

#define FILL_STRUCTARGS6(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12) \
        FILL_STRUCTARGS5(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10) \
        ob.arg11 = arg12;

#define FILL_STRUCTARGS7(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14) \
        FILL_STRUCTARGS6(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12) \
        ob.arg13 = arg14;

#define FILL_STRUCTARGS8(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15, arg16) \
        FILL_STRUCTARGS7(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14) \
        ob.arg15 = arg16;
   
#define NETLINK_USER 31

#define HCALL_ARRAY_MAX 10 /* The maximal number of <hcall> tags in scenario configuration files */

#define HCALL_ARRAY_LEN 270 /* The maximal number of characters used for specifying hypercalls in configuration */

#define LOG_MAX 250 /* The maximal length of a log entry */

/* The maximal number of struct parameter members (and maximal length of the value of a member, in string format) defined in configuration */
#define STRUCT_MEMBERS_MAX 10
#define STRUCT_MEMBERS_LEN 31

/* Hypercall invocation */
#define __MHYPERCALL_ENTRY(x)                                            \
        [offset] "i" ( x * sizeof(hypercall_page[0]))

#define __MYUL unsigned long

/* Invoking hypercalls with different numbers of parameters (1--5) */
#define __MHYPERCALL1ARGS_IF(x) \
        if(hinjHypNum == x){ \
          gen_hypercall1arg(x, hinjParams[0]); \
          printk(KERN_INFO "Invoked hypercall with no.: " #x"\n");} \
        else (void)0;

#define __MHYPERCALL2ARGS_IF(x) \
        if(hinjHypNum == x){ \
          gen_hypercall2arg(x, hinjParams[0], hinjParams[1]); \
          printk(KERN_INFO "Invoked hypercall with no.: " #x"\n");} \
        else (void)0;

#define __MHYPERCALL3ARGS_IF(x) \
        if(hinjHypNum == x){ \
          gen_hypercall3arg(x, hinjParams[0], hinjParams[1], hinjParams[2]); \
          printk(KERN_INFO "Invoked hypercall with no.: " #x"\n");} \
        else (void)0;

#define __MHYPERCALL4ARGS_IF(x) \
        if(hinjHypNum == x){ \
          gen_hypercall4arg(x, hinjParams[0], hinjParams[1], hinjParams[2], hinjParams[3]); \
          printk(KERN_INFO "Invoked hypercall with no.: " #x"\n");} \
        else (void)0;

#define __MHYPERCALL5ARGS_IF(x) \
        if(hinjHypNum == x){ \
          gen_hypercall5arg(x, hinjParams[0], hinjParams[1], hinjParams[2], hinjParams[3], hinjParams[4]); \
          printk(KERN_INFO "Invoked hypercall with no.: " #x"\n");} \
        else (void)0;


/* 
* timersub helps the conversion of system time (format s:msec) to HInjector internal time 
*   --- taken from the BSD kernel.
*/

#define timersub(a, b, result)\
do{\
(result).tv_sec = (a).tv_sec - (b).tv_sec;\
(result).tv_usec = (a).tv_usec - (b).tv_usec;\
if((result).tv_usec < 0){\
--(result.tv_sec);\
(result).tv_usec += 1000000;\
}}while(0)


/*
*   Global variables  
*/

unsigned long int hinjStructAddr; /* A virtual memory adress pointing to a struct object */

struct sock *hinjNl_sk = NULL; /* Netlink socket structure */

int hinjI_rcv_msg = 0; /* Counter for received messages */

/* 
*   Implementation of the shared_info structure in the LKM.
*   shared_info is needed for passing information on injected hypercalls to the hypervisor.
*/

struct shared_info hinjXen_dummy_shared_info;
struct shared_info *HYPERVISOR_shared_info = (void *)&hinjXen_dummy_shared_info;

char *hinjHcall_input[HCALL_ARRAY_MAX]; /* Configuration information for injected hypercalls */

char hinjLog[LOG_MAX]; /* Logging information (per injected hypercall) */

unsigned long hinjRate = 0; /* Rate of injecting hypercalls */

char *hinjMembers[STRUCT_MEMBERS_MAX]; /* Array where values of members of struct hypercall parameters are stored */

unsigned long hinjRepeatScen = 0; /* Counter for repeating injection scenarios */

struct nlmsghdr *hinjNlh; /* Netlink header for sending messages to userland  */

struct nlmsghdr *hinjNlh_in; /* Netlink header for received messages from userland */

int hinjPid; /* Process id of the userland Netlink process  */

struct sk_buff *hinjSkb_out; /* Netlink structure containing the content of a message that is to be sent to userland  */

int hinjMsg_size; /* The size of a Netlink message to be sent to userland  */

char hinjMsg[LOG_MAX + 1]; /* The Netlink message that is to be sent to userland  */

struct timeval hinjTimeSave; /* Timing structure */




/*
*   C Functions
*/

/*
* Hypercall invoking functions:
* --- execute any hypercall by its number;
* --- maximum 5 arguments allowed. 
*/

static inline unsigned long
gen_hypercall1arg(unsigned long number, unsigned long param1) {
register unsigned long __mres  asm(__HYPERCALL_RETREG);
register unsigned long __marg1 asm(__HYPERCALL_ARG1REG) = param1;
asm volatile (__HYPERCALL
     : "=r" (__mres), "+r" (__marg1)
     : __MHYPERCALL_ENTRY(number)
     : __HYPERCALL_CLOBBER1
     );
return (__MYUL)__mres;
}

static inline unsigned long
gen_hypercall2arg(unsigned long number, unsigned long param1, unsigned long param2) {
register unsigned long __mres  asm(__HYPERCALL_RETREG);
register unsigned long __marg1 asm(__HYPERCALL_ARG1REG) = param1;
register unsigned long __marg2 asm(__HYPERCALL_ARG2REG) = param2;
asm volatile (__HYPERCALL
     : "=r" (__mres), "+r" (__marg1), "+r" (__marg2)
     : __MHYPERCALL_ENTRY(number)
     : __HYPERCALL_CLOBBER2
     );
return (__MYUL)__mres;
}

static inline unsigned long
gen_hypercall3arg(unsigned long number, unsigned long param1, unsigned long param2, unsigned long param3) {
register unsigned long __mres  asm(__HYPERCALL_RETREG);
register unsigned long __marg1 asm(__HYPERCALL_ARG1REG) = param1;
register unsigned long __marg2 asm(__HYPERCALL_ARG2REG) = param2;
register unsigned long __marg3 asm(__HYPERCALL_ARG3REG) = param3;
asm volatile (__HYPERCALL
     : "=r" (__mres), "+r" (__marg1), "+r" (__marg2), "+r" (__marg3)
     : __MHYPERCALL_ENTRY(number)
     : __HYPERCALL_CLOBBER3
     );
return (__MYUL)__mres;
}

static inline unsigned long
gen_hypercall4arg(unsigned long number, unsigned long param1, unsigned long param2, unsigned long param3, unsigned long param4) {
register unsigned long __mres  asm(__HYPERCALL_RETREG);
register unsigned long __marg1 asm(__HYPERCALL_ARG1REG) = param1;
register unsigned long __marg2 asm(__HYPERCALL_ARG2REG) = param2;
register unsigned long __marg3 asm(__HYPERCALL_ARG3REG) = param3;
register unsigned long __marg4 asm(__HYPERCALL_ARG4REG) = param4;
asm volatile (__HYPERCALL
     : "=r" (__mres), "+r" (__marg1), "+r" (__marg2), "+r" (__marg3), "+r" (__marg4)
     : __MHYPERCALL_ENTRY(number)
     : __HYPERCALL_CLOBBER4
     );
return (__MYUL)__mres;
}

static inline unsigned long
gen_hypercall5arg(unsigned long number, unsigned long param1, unsigned long param2, unsigned long param3, unsigned long param4, unsigned long param5) {
register unsigned long __mres  asm(__HYPERCALL_RETREG);
register unsigned long __marg1 asm(__HYPERCALL_ARG1REG) = param1;
register unsigned long __marg2 asm(__HYPERCALL_ARG2REG) = param2;
register unsigned long __marg3 asm(__HYPERCALL_ARG3REG) = param3;
register unsigned long __marg4 asm(__HYPERCALL_ARG4REG) = param4;
register unsigned long __marg5 asm(__HYPERCALL_ARG5REG) = param5;
asm volatile (__HYPERCALL
     : "=r" (__mres), "+r" (__marg1), "+r" (__marg2), "+r" (__marg3), "+r" (__marg4), "+r" (__marg5)
     : __MHYPERCALL_ENTRY(number)
     : __HYPERCALL_CLOBBER5
     );
return (__MYUL)__mres;
}

/* Casts parameter value as written in configuration to unsigned long */
unsigned long get_single_para(char *in){

  unsigned long hinjRet = 0;
  const char* hinjV = in;

  kstrtol(hinjV, 10, &hinjRet);

  return hinjRet;
}

/* Generates a parameter value inside a given range */
unsigned long get_range_para(char *in){

  unsigned long hinjRet = 0;
  int hinjCounter = 0;
  char *hinjToken;
  unsigned long hinjLower = 0; /* Lower boundary */
  unsigned long hinjUpper = 0; /* Upper boundary */
  unsigned long hinjGen = 0; /* The generated value */


  while((hinjToken=strsep(&in,"-")) != NULL){
    hinjCounter++;
    /* When hinjCounter is 1 or 2, in does not store the lower or upper boundary*/
    if(hinjCounter==3){const char* hinjV = hinjToken;kstrtol(hinjV, 10, &hinjLower);}
    if(hinjCounter==4){const char* hinjV = hinjToken;kstrtol(hinjV, 10, &hinjUpper);break;}
  }

  get_random_bytes(&hinjGen, sizeof(hinjGen));
  hinjGen = hinjGen % (hinjUpper-hinjLower);
  hinjGen = hinjGen + hinjLower;
  hinjRet = hinjGen;

  return hinjRet;
}

/* Generates a random parameter value that is not equal to a set of user-specified values */
unsigned long get_irregular_para(char *in){

  unsigned long hinjRet = 0;
  int hinjCounter = 0;
  char *hinjToken;
  unsigned long hinjValue[10] = {0}; /* User-specified values */
  unsigned long hinjGen = 0; /* The generated value */

  while((hinjToken=strsep(&in,"-")) != NULL){
    hinjCounter++;
    if(hinjCounter!=1){ /* When hinjCounter is 1, hinjToken does not store a user-specified value */
      const char* v = hinjToken; kstrtol(v, 10, &hinjValue[hinjCounter-2]);}
  }

  get_random_bytes(&hinjGen, sizeof(hinjGen));

  while(hinjGen == hinjValue[0] || hinjGen == hinjValue[1] || hinjGen == hinjValue[2] || hinjGen == hinjValue[3] || hinjGen == hinjValue[4] || hinjGen == hinjValue[5] || hinjGen == hinjValue[6] || hinjGen == hinjValue[7] || hinjGen == hinjValue[8] || hinjGen == hinjValue[9]){
    get_random_bytes(&hinjGen, sizeof(hinjGen));
  }

  hinjRet = hinjGen;

  return hinjRet;
}

/* Generating a hypercall parameter value (user-defined value, random value within range, random value that is not equal to a set of user-specified values)  */
unsigned long getPara(char* in){

  unsigned long hinjRet = 0;

  if(in == NULL)
    return -1; 

  if(strstr(in, "r-r") != NULL){
    hinjRet = get_range_para(in);
  }else if(strstr(in, "r-") != NULL){
    hinjRet = get_irregular_para(in);
  }else{
    hinjRet = get_single_para(in);
  }

  return hinjRet;
}

/* Generating hypercall parameter values, stored in a struct or primitive variables. See different value generation modes above. */

unsigned long generate_parameter(char *in){

  unsigned long hinjRetParam;
  char *hinjNext;
  char *hinjToken;
  int hinjCounter = 0;
  int hinjCounter2 = 0;
  unsigned long hinjValue[10] = {0}; /* User-specified values in configuration (e.g., minimal and maximal range values, fixed parameter values) for struct parameter members, pro hypercall */
  char hinjToAppend[LOG_MAX]; /* String used for logging purposes */

  hinjNext = in;

  if(in == NULL)
  {
    return -1;
  }

  if(strstr(hinjNext, "!") != NULL)
  { /* Processing user-defined struct hypercall parameter */
    hinjCounter = 0;
    strcpy(hinjToAppend, ":struct:");strcat(hinjLog, hinjToAppend);
    while((hinjToken=strsep(&hinjNext,"!")) != NULL)
    {
      hinjCounter++;
      
      if (hinjCounter!=1 && hinjCounter!=2)
      { /* If hinjCounter is 1 or 2, hinjToken does not store struct member value (hinjCounter == 2 --- the name of the struct passed as a parameter) */
        if((hinjCounter%2) == 1)
        {
          strncpy(hinjMembers[hinjCounter-3-hinjCounter2],hinjToken,20);
          strcpy(hinjToAppend, hinjToken);strcat(hinjToAppend, ":");strcat(hinjLog, hinjToAppend);
          hinjCounter2++;
        }
        else
        {
          hinjValue[hinjCounter-3-hinjCounter2] = getPara(hinjToken);
          snprintf(hinjToAppend,LOG_MAX+1,"%lu:",hinjValue[hinjCounter-3-hinjCounter2]);
          strcat(hinjLog, hinjToAppend);
        }
      }
    }

    /* Constructing struct parameters for a given hypercall */ 
    /* TODO: strcmp with struct name, i.e., hinjToken when hinjcCounter ==2 */

    /* struct trap_info */
    if(strcmp(hinjMembers[0],"vector") == 0 && strcmp(hinjMembers[1],"flags") == 0 && strcmp(hinjMembers[2],"cs") == 0 && strcmp(hinjMembers[3],"address") == 0)
    {
      FILL_STRUCTARGS4(trap_info, vector, hinjValue[0], flags, hinjValue[1], cs, hinjValue[2], address, hinjValue[3])
    }

    /* struct physdev_get_free_pirq */
    else if(strcmp(hinjMembers[0],"type") == 0)
    {
      FILL_STRUCTARGS1(physdev_get_free_pirq, type, hinjValue[0])
    }

    /* struct gnttab_set_version */
    else if(strcmp(hinjMembers[0],"version") == 0)
    {
      FILL_STRUCTARGS1(gnttab_set_version, version, hinjValue[0])
    }

    /* struct xen_memory_exchange */
    else if(strcmp(hinjMembers[0],"in.nr_extents") == 0 && strcmp(hinjMembers[1],"in.extent_order") == 0 && strcmp(hinjMembers[2],"in.extent_start") == 0 && strcmp(hinjMembers[3],"in.domid") == 0 && strcmp(hinjMembers[4],"out.nr_extents") == 0 && strcmp(hinjMembers[5],"out.extent_order") == 0 && strcmp(hinjMembers[6],"out.extent_start") == 0 && strcmp(hinjMembers[7],"out.domid") == 0)
    {
      FILL_STRUCTARGS8(xen_memory_exchange, in.nr_extents, hinjValue[0], in.extent_order, hinjValue[1], in.extent_start, hinjValue[2], in.domid, hinjValue[3], out.nr_extents, hinjValue[4], out.extent_order, hinjValue[5], out.extent_start, hinjValue[6], out.domid, hinjValue[7])
    }
    
    else
    {
      printk(KERN_ERR "Failed to allocate struct as hypercall parameter. Probably invalid user input.\n");
      printk(KERN_ERR "0 member: %s value: %lu\n", hinjMembers[0], hinjValue[0]);
      printk(KERN_ERR "1 member: %s value: %lu\n", hinjMembers[1], hinjValue[1]);
      printk(KERN_ERR "2 member: %s value: %lu\n", hinjMembers[2], hinjValue[2]);
      printk(KERN_ERR "3 member: %s value: %lu\n", hinjMembers[3], hinjValue[3]);
      hinjStructAddr = 0; /* 0x0 virtual memory address in case struct allocation has failed */
    }

    hinjRetParam = hinjStructAddr;
  }

  else
  { /* Processing user-defined hypercall parameter (not a struct) */
      hinjRetParam = getPara(hinjNext); 
      snprintf(hinjToAppend,LOG_MAX+1,"%lu",hinjRetParam);
      strcat(hinjLog, ":Para:");
      strcat(hinjLog, hinjToAppend);
  }

  return hinjRetParam;
}

/* Sending loging information to userland */ 
static void send_log(void){
  
  strcpy(hinjMsg, "");
  strcpy(hinjMsg,hinjLog);
  hinjMsg[LOG_MAX] = '\0';
  hinjMsg_size=strlen(hinjMsg);

  hinjPid = hinjNlh_in->nlmsg_pid; 

  hinjSkb_out = nlmsg_new(hinjMsg_size,0);

  if(!hinjSkb_out)
  {
    printk(KERN_ERR "Failed to allocate Netlink message header\n");
    return;
  } 

  hinjNlh=nlmsg_put(hinjSkb_out,0,0,NLMSG_DONE,hinjMsg_size,0);  
  NETLINK_CB(hinjSkb_out).dst_group = 0; 
  strncpy(nlmsg_data(hinjNlh),hinjMsg,hinjMsg_size);

  if(nlmsg_unicast(hinjNl_sk,hinjSkb_out,hinjPid)<0){
    printk(KERN_INFO "Error while sending logging information to userland\n");
  }
  strcpy(hinjLog, "");
}

/* Function for invoking hypercalls with parameters as defined in the configuration */
void execute_single_hcall(char *in){

  char *hinjToken;
  char hinjNexta[HCALL_ARRAY_LEN];
  char *hinjNext = hinjNexta;
  unsigned long hinjParams[5] = {0}; /* Array for storing the parameters of the invoked hypercall (max. 5) */
  long int hinjHypNum = 1; /* The number of the invoked hypercall */
  int hinjCounter = 0;
  int hinjCounterParam = 0;

  char hinjToAppend[LOG_MAX];

  strncpy(hinjNext, in, HCALL_ARRAY_LEN);

  while((hinjToken=strsep(&hinjNext,"/")) != NULL){
    hinjCounter++;
    if(hinjCounter==1) /* Extracting the hypercall number */
    {
      const char* v = hinjToken;
      kstrtol(v, 10, &hinjHypNum);
      strcpy(hinjToAppend, ":hypnum:");
      strcat(hinjToAppend, hinjToken);
      strcat(hinjLog, hinjToAppend);
    }
    
    else /* Generating hypercall parameters */
    {
      hinjParams[hinjCounterParam] = generate_parameter(hinjToken);
      hinjCounterParam++;
    }
  }

  HYPERVISOR_shared_info->arch.hypercall_number = hinjHypNum + 10; /* Information used by the Filter for detecting injected hypercalls */

/*
*   Invoking the appropriate assembly code for a hypercall with a given number.
*   Currently we pass the maximal number of parameters that a hypercall (and its suboperations) may need. 
*   To simplify the code, in the future we may use only __MHYPERCALL5ARGS_IF, since Xen filters the value read from vCPU registers with respect to the number of an invoked hypercall. 
*   WARNING: This code is virtualization-mode specific (i.e., different for full PV VMs, PVH VMs, and so on)
*/

__MHYPERCALL1ARGS_IF(0);
__MHYPERCALL4ARGS_IF(1);
__MHYPERCALL2ARGS_IF(2);
__MHYPERCALL2ARGS_IF(3);
__MHYPERCALL3ARGS_IF(4);
__MHYPERCALL1ARGS_IF(5);
__MHYPERCALL2ARGS_IF(6);
__MHYPERCALL1ARGS_IF(7);
__MHYPERCALL2ARGS_IF(8);
__MHYPERCALL1ARGS_IF(9);
__MHYPERCALL4ARGS_IF(10);
__MHYPERCALL2ARGS_IF(12);
__MHYPERCALL2ARGS_IF(13);
__MHYPERCALL4ARGS_IF(14);
__MHYPERCALL2ARGS_IF(15);
__MHYPERCALL2ARGS_IF(16);
__MHYPERCALL2ARGS_IF(17);
__MHYPERCALL3ARGS_IF(18);
__MHYPERCALL2ARGS_IF(19);
__MHYPERCALL3ARGS_IF(20);
__MHYPERCALL2ARGS_IF(21);
__MHYPERCALL5ARGS_IF(22);
__MHYPERCALL2ARGS_IF(23);
__MHYPERCALL3ARGS_IF(24);
__MHYPERCALL2ARGS_IF(25);
__MHYPERCALL4ARGS_IF(26);
__MHYPERCALL2ARGS_IF(27);
__MHYPERCALL2ARGS_IF(28);
__MHYPERCALL2ARGS_IF(29);
__MHYPERCALL2ARGS_IF(30);
__MHYPERCALL2ARGS_IF(31);
__MHYPERCALL2ARGS_IF(32);
__MHYPERCALL2ARGS_IF(33);
__MHYPERCALL2ARGS_IF(34);
__MHYPERCALL1ARGS_IF(38);

  HYPERVISOR_shared_info->arch.hypercall_number = 0; /*Reset the hypercall_number field after a hypercall has been executed */	
}

/* Invokes a hypercall as defined in the configuration. Takes into account the configuration parameter "repeat" */
void execute_hcalls(char *in){

  char *hinjToken;
  char *hinjNext;
  int hinjCounter = 0;
  unsigned long hinjRepeat = 0; /* The value of the configuration parameter "repeat" */
  char hinjIna[HCALL_ARRAY_LEN];
  char *hinjIn = hinjIna;

  char hinjToAppend[LOG_MAX];
  struct timeval hinjTime;
  struct tm hinjBrokenTime;

  strncpy(hinjIn, in, HCALL_ARRAY_LEN);

  hinjCounter = 0;

  while((hinjToken=strsep(&hinjIn,":")) != NULL){
    hinjCounter++;
    if(hinjCounter==1){const char* v = hinjToken;kstrtol(v, 10, &hinjRepeat);}
    if(hinjCounter==2){hinjNext = hinjToken;}
  }

  for(hinjCounter = 0; hinjCounter < hinjRepeat; hinjCounter++)
  {
    msleep(hinjRate);
    do_gettimeofday(&hinjTime);
    timersub(hinjTime, hinjTimeSave, hinjTime);
    time_to_tm(hinjTime.tv_sec, 0, &hinjBrokenTime);
    snprintf(hinjToAppend,LOG_MAX+1,"%d:%d:%d:%ld:%ld",hinjBrokenTime.tm_hour, hinjBrokenTime.tm_min, hinjBrokenTime.tm_sec, hinjTime.tv_sec, hinjTime.tv_usec);
    strcat(hinjLog, hinjToAppend);
    execute_single_hcall(hinjNext); /* Executing a single hypercall */
    do_gettimeofday(&hinjTime);
    timersub(hinjTime, hinjTimeSave, hinjTime);
    snprintf(hinjToAppend,LOG_MAX+1,"%ld:%ld",hinjTime.tv_sec, hinjTime.tv_usec);
    strcat(hinjLog, hinjToAppend);
    send_log();
  }

}

/* The hypercall invoking function at the top of the hierarchy (scenario -> group of hypercalls -> a single hypercall). Manages the execution of hypercall injection scenarios. */

void execute_array(void){
  int hinjCounter = 0;
  int hinjCounter2 = 0;

  for(hinjCounter2 = 0;hinjCounter2 < hinjRepeatScen;hinjCounter2++){ /* Repetitive execution of a hypercall injection scenario */
    for(hinjCounter = 0;hinjCounter < hinjI_rcv_msg;hinjCounter++){ /* Invokes hypercalls as defined in the configuration. */
      execute_hcalls(hinjHcall_input[hinjCounter]);
    }
  }

  return;
}

/* Reads the configuration parameters of the scenario configuration tag (i.e., repeat and rate). */

void parse_scenario(char *in){

  int hinjCounter = 0;
  char *hinjToken;

  while((hinjToken=strsep(&in,":")) != NULL){
    hinjCounter++;
    if(hinjCounter==1){const char* v = hinjToken;kstrtol(v, 10, &hinjRate);}
    if(hinjCounter==2){const char* v = hinjToken;kstrtol(v, 10, &hinjRepeatScen);}
  }

  return;
}

/* Receives messages from userland; Stores configuration information; Commences hypercall invokation; Sends terminating signal to userland (i.e., the work of the LKM is finished) */

static void hello_nl_recv_msg(struct sk_buff *skb) {

  hinjNlh_in=(struct nlmsghdr*)skb->data;

  if(strcmp((char*)nlmsg_data(hinjNlh_in),"0") == 0){ /* When the LKM receives 0, the configuration information has been transfered to it. It commences the invokation of hypercalls */
    execute_array();
  }

  else{ /* Receiving configuration data from userland */
    if(hinjI_rcv_msg == 0){
      parse_scenario((char*)nlmsg_data(hinjNlh_in));
      hinjI_rcv_msg++;
    }
    else{
      strncpy(hinjHcall_input[hinjI_rcv_msg],(char*)nlmsg_data(hinjNlh_in),HCALL_ARRAY_LEN);
      hinjHcall_input[hinjI_rcv_msg][HCALL_ARRAY_LEN + 1]='\0';
      hinjI_rcv_msg++;
    }
    return;
  }

  /* Sends terminating signal to userland (i.e., the work of the LKM is finished) */

  strcpy(hinjMsg,"-1");
  hinjMsg_size=strlen(hinjMsg);

  hinjPid = hinjNlh_in->nlmsg_pid; 

  hinjSkb_out = nlmsg_new(hinjMsg_size,0);

  if(!hinjSkb_out){
    printk(KERN_ERR "Failed to allocate new skb\n");
    return;
  } 
  hinjNlh=nlmsg_put(hinjSkb_out,0,0,NLMSG_DONE,hinjMsg_size,0);  
  NETLINK_CB(hinjSkb_out).dst_group = 0;
  strncpy(nlmsg_data(hinjNlh),hinjMsg,hinjMsg_size);

  if(nlmsg_unicast(hinjNl_sk,hinjSkb_out,hinjPid)<0){
      printk(KERN_INFO "Error while sending back to user\n");
  }
}


/* The __init function. Sets up a Netlink socket; Maps shared_info; Initializes array for storing configuration information */

static int __init hello_init(void)
{

  int hinjCounter = 0;
  struct netlink_kernel_cfg cfg = {
    .input = hello_nl_recv_msg,
  };


  do_gettimeofday(&hinjTimeSave);

  /* Mapping shared_info */
  if (!xen_feature(XENFEAT_auto_translated_physmap)) {
        set_fixmap(FIX_PARAVIRT_BOOTMAP, xen_start_info->shared_info);
        HYPERVISOR_shared_info = (struct shared_info *)fix_to_virt(FIX_PARAVIRT_BOOTMAP);
  } else{
        HYPERVISOR_shared_info = (struct shared_info *)__va(xen_start_info->shared_info);
  }

  if(HYPERVISOR_shared_info == &hinjXen_dummy_shared_info){
    printk(KERN_ALERT "shared_info not mapped.\n");
  }

  /* Setting up a Netlink socket*/
  hinjNl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);
  
  if(!hinjNl_sk){
    printk(KERN_ALERT "Error creating socket.\n");
    return -10;
  }

  /*Initializing array for storing configuration information*/
  for(hinjCounter = 0; hinjCounter < STRUCT_MEMBERS_MAX; ++hinjCounter){
    hinjMembers[hinjCounter] = (char *)kcalloc(STRUCT_MEMBERS_LEN +1, sizeof(char), __GFP_ZERO); /*Configuration information for struct hypercall parameters*/
  }

  for(hinjCounter = 0; hinjCounter < HCALL_ARRAY_MAX; ++hinjCounter){
    hinjHcall_input[hinjCounter] = (char *)kcalloc(HCALL_ARRAY_LEN +1, sizeof(char), __GFP_ZERO); /*Configuration information specified in hcall tags*/
  }

    return 0;
}


/* The __exit function. */
static void __exit hello_cleanup(void)
{
    printk(KERN_INFO "HInjector LKM terminated.\n");
    netlink_kernel_release(hinjNl_sk);
}

module_init(hello_init);
module_exit(hello_cleanup);
