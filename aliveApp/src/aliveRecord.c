/* aliveRecord.c */
/* Example record support module */
  
#ifdef vxWorks
  #include <version.h>
  #include <bootLib.h>
  #if defined(_WRS_VXWORKS_MAJOR) && (_WRS_VXWORKS_MAJOR >= 6) 
    #include <errnoLib.h>
    #include <netinet/in.h>
    #include <sockLib.h>
    #include <ioLib.h>
  #else
    #include <stddef.h>
    #include <stdarg.h>

    #include <sockLib.h>
    #include <inetLib.h>
    #include <hostLib.h>
  #endif
#else
  #include <netinet/in.h>
  #include <arpa/inet.h>
#endif

#if defined (linux) || defined (darwin)
  #include <unistd.h>
  #include <pwd.h>
  #include <grp.h>
  #include <sys/types.h>
#endif

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <errlog.h>
#include <epicsTime.h>
//#include <epicsMath.h>
#include <alarm.h>
#include <dbAccess.h>
#include <recGbl.h>
#include <dbEvent.h>
//#include <dbDefs.h>
//#include <dbAccess.h>
#include <errMdef.h>
#include <recSup.h>
#include <special.h>
//#include <callback.h>
#define GEN_SIZE_OFFSET
#include "aliveRecord.h"
#undef  GEN_SIZE_OFFSET
#include "epicsExport.h"

#include <epicsVersion.h>
#ifndef EPICS_VERSION_INT
#define VERSION_INT(V,R,M,P) ( ((V)<<24) | ((R)<<16) | ((M)<<8) | (P))
#define EPICS_VERSION_INT VERSION_INT(EPICS_VERSION, EPICS_REVISION, EPICS_MODIFICATION, EPICS_PATCH_LEVEL)
#endif
#define LT_EPICSBASE(V,R,M,P) (EPICS_VERSION_INT < VERSION_INT((V),(R),(M),(P)))


/* Create RSET - Record Support Entry Table */
#define report NULL
#define initialize NULL
static long init_record();
static long process();
static long special();
#define get_value NULL
#define cvt_dbaddr NULL
#define get_array_info NULL
#define put_array_info NULL
#define get_units NULL
#define get_precision NULL
#define get_enum_str NULL
#define get_enum_strs NULL
#define put_enum_str NULL
#define get_graphic_double NULL
#define get_control_double NULL
#define get_alarm_double NULL
 
rset aliveRSET =
  {
    RSETNUMBER,
    report,
    initialize,
    init_record,
    process,
    special,
    get_value,
    cvt_dbaddr,
    get_array_info,
    put_array_info,
    get_units,
    get_precision,
    get_enum_str,
    get_enum_strs,
    put_enum_str,
    get_graphic_double,
    get_control_double,
    get_alarm_double
  };
epicsExportAddress(rset,aliveRSET);

#define PROTOCOL_VERSION (4)

#define ENV_CNT (10)

static void checkAlarms(aliveRecord *prec);
static void monitor(aliveRecord *prec);
static void monitor_itrig(aliveRecord *prec);

typedef struct rpvtStruct 
{
  struct alive_t *alive;

  char fault_flag; // in case of unrecoverable error
  char ready_flag; // in case host doesn't make sense

  int socket;
  struct sockaddr_in h_addr;

  uint32_t incarnation;
  uint16_t lport;
  uint16_t flags;

  char ioc_name[41];
  
  // 40 character limit enforced by EPICS string type
  // otherwise, can be very big
  char env[ENV_CNT][41];

  epicsThreadId listen_thread;
} rpvtStruct;


#ifdef vxWorks
static void bootparam_write( int sock, char *string)
{
  uint8_t len8;

  len8 = strlen( string);
  write( sock, (void *) &len8, sizeof(uint8_t) );
  if( len8)
    write( sock, string, len8 );
}
#endif

void *ioc_alive_listen(void *data)
{
  aliveRecord *prec = (aliveRecord *) data;
  rpvtStruct *prpvt;

  int tcp_sockfd;
  int sflag;

  struct sockaddr_in r_addr;
#ifdef vxWorks
  int r_len;
#else
  socklen_t r_len;
#endif
  int client_sockfd;

  struct sockaddr_in l_addr;

  char *q;
  
  // key and value lengths, key length of zero means it doesn't exist
  int env_len[ENV_CNT][2]; 

  uint32_t msg32;
  uint16_t msg16, len16;
  uint8_t  len8;

  int length;
  int number;
  int type;

#if defined (linux) || defined (darwin)
  char *user;
  char *group;
  char *hostname;
  char hostname_buffer[129];
#endif
#ifdef vxWorks
  BOOT_PARAMS bootparams;
#endif

  int i;

  prpvt = prec->rpvt;

#if defined (vxWorks)
  type = 1;
#elif defined (linux)
  type = 2;
#elif defined (darwin)
  type = 3;
#else
  type = 0;
#endif
  
#ifdef vxWorks
  bzero( (char *) &l_addr, sizeof( struct sockaddr_in) );
  l_addr.sin_len = sizeof( struct sockaddr_in);
#else
  bzero( &l_addr, sizeof( struct sockaddr_in) );
#endif
  l_addr.sin_family = AF_INET;
  l_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  l_addr.sin_port = htons(prpvt->lport);

#ifdef vxWorks
  if( (tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == ERROR )
#else
  if( (tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0) ) == -1)
#endif
    {
      perror("socket");
      return NULL;
    }

  sflag = 1;
  // not the end of the world if this option doesn't work
  setsockopt(tcp_sockfd, SOL_SOCKET, SO_REUSEADDR, (void *) &sflag, 
             sizeof(sflag));

#ifdef vxWorks
  if( bind(tcp_sockfd, (struct sockaddr *) &l_addr, 
           sizeof(struct sockaddr_in)) == ERROR)
#else
  if( bind(tcp_sockfd, (struct sockaddr *) &l_addr, 
           sizeof(struct sockaddr_in)) )
#endif
    {
      perror("TCP bind");
      return NULL;
    }

#ifdef vxWorks
  if( listen(tcp_sockfd, 5) == ERROR )
#else
  if( listen(tcp_sockfd, 5) )
#endif
    {
      perror("TCP listen");
      return NULL;
    }

  while(1)
    {
      r_len = sizeof(r_addr);
      client_sockfd = accept( tcp_sockfd, (struct sockaddr *)&r_addr, 
                              &r_len);

      // fault flag can't happen, but just in case
      if( prec->isup || prpvt->fault_flag || !prpvt->ready_flag || 
          ( r_addr.sin_addr.s_addr != prpvt->h_addr.sin_addr.s_addr) )
        {
          close(client_sockfd);
          continue;
        }

      // TCP protocol version
      msg16 = htons(PROTOCOL_VERSION);
      write( client_sockfd, (void *) &msg16, sizeof(uint16_t) );

      // IOC type, currently 1 = vxworks, 2 = linux, 3 = darwin
      msg16 = htons(type);
      write( client_sockfd, (void *) &msg16, sizeof(uint16_t) );

      number = 0;
      length = 10;  // from version + flag + number + length
#ifdef vxWorks
      //      flag == 1, vxworks

      memset( &bootparams, 0, sizeof( BOOT_PARAMS) );
      bootStringToStruct( sysBootLine, &bootparams);
      // don't check to see if it returns EOS, as it's zeroed out

      length += 12; // bootparams.unitNum, bootparams.procNum, bootparams.flags
      length += 12; // 8-bit lengths below
      length += ((uint8_t) strlen(bootparams.bootDev) );
      length += ((uint8_t) strlen(bootparams.hostName) );
      length += ((uint8_t) strlen(bootparams.bootFile) );
      length += ((uint8_t) strlen(bootparams.ead) );
      length += ((uint8_t) strlen(bootparams.bad) );
      length += ((uint8_t) strlen(bootparams.had) );
      length += ((uint8_t) strlen(bootparams.gad) );
      length += ((uint8_t) strlen(bootparams.usr) );
      length += ((uint8_t) strlen(bootparams.passwd) );
      length += ((uint8_t) strlen(bootparams.targetName) );
      length += ((uint8_t) strlen(bootparams.startupScript) );
      length += ((uint8_t) strlen(bootparams.other) );
#endif
#if defined (linux) || defined (darwin)
      {
        uid_t user_id;
        gid_t group_id;
        struct passwd *passwd_entry;
        struct group *group_entry;

        user_id = getuid();
        passwd_entry = getpwuid(user_id);
        if( passwd_entry != NULL)
          user = passwd_entry->pw_name;
        else
          user = NULL;

        group_id = getgid();
        group_entry = getgrgid(group_id);
        if( group_entry != NULL)
          group = group_entry->gr_name;
        else
          group = NULL;

        if( gethostname( hostname_buffer, 128) )
          hostname = NULL;
        else
          {
            hostname_buffer[128] = '\0';
            hostname = hostname_buffer;
          }
      }

      //      flag == 2 or 3
      length += 1; // 8-bit string length
      if( user != NULL)
        length += ((uint8_t) strlen(user) );
      length += 1; // 8-bit string length
      if( group != NULL)
        length += ((uint8_t) strlen(group) );
      length += 1; // 8-bit string length
      if( hostname != NULL)
        length += ((uint8_t) strlen(hostname) );
#endif
      for( i = 0; i < ENV_CNT; i++)
        {
          if( prpvt->env[i][0] == '\0')
            env_len[i][0] = 0;
          else
            {
              number++;
              length += 3; // 8-bit key & 16-bit value string lengths
              env_len[i][0] = strlen(prpvt->env[i]);
              length += env_len[i][0];
              q = getenv(prpvt->env[i]);
              if( q == NULL)
                env_len[i][1] = 0;
              else
                {
                  env_len[i][1] = strlen(q);
                  // if size is greater that 16-bit max, truncate to zero
                  if( env_len[i][1] > 65535)
                    env_len[i][1] = 0;
                  length += env_len[i][1];
                }
            }
        }

      /* printf("%d\n", length); fflush(stdout); */

      msg32 = htonl( length);
      write( client_sockfd, (void *) &msg32, sizeof(uint32_t) );
      msg16 = htons( number);
      write( client_sockfd, (void *) &msg16, sizeof(uint16_t) );

      for( i = 0; i < ENV_CNT; i++)
        {
          if( env_len[i][0] == 0)
            continue;

          len8 = env_len[i][0];
          write( client_sockfd, (void *) &len8, sizeof(uint8_t) );
          write( client_sockfd, prpvt->env[i], len8);

          q = getenv(prpvt->env[i]);
          len16 = env_len[i][1];
          msg16 = htons( len16);
          write( client_sockfd, (void *) &msg16, sizeof(uint16_t) );
          if( len16)
            write( client_sockfd, q, len16 );
        }

#ifdef vxWorks
      bootparam_write( client_sockfd, bootparams.bootDev);
      msg32 = htonl(bootparams.unitNum);
      write( client_sockfd, (void *) &msg32, sizeof(uint32_t) );
      msg32 = htonl(bootparams.procNum);
      write( client_sockfd, (void *) &msg32, sizeof(uint32_t) );
      bootparam_write( client_sockfd, bootparams.hostName);
      bootparam_write( client_sockfd, bootparams.bootFile);
      bootparam_write( client_sockfd, bootparams.ead);
      bootparam_write( client_sockfd, bootparams.bad);
      bootparam_write( client_sockfd, bootparams.had);
      bootparam_write( client_sockfd, bootparams.gad);
      bootparam_write( client_sockfd, bootparams.usr);
      bootparam_write( client_sockfd, bootparams.passwd);
      msg32 = htonl( bootparams.flags);
      write( client_sockfd, (void *) &msg32, sizeof(uint32_t) );
      bootparam_write( client_sockfd, bootparams.targetName);
      bootparam_write( client_sockfd, bootparams.startupScript);
      bootparam_write( client_sockfd, bootparams.other);
#endif
#if defined (linux) || defined (darwin)
      if( user == NULL)
        len8 = 0;
      else
        len8 = strlen( user);
      write( client_sockfd, (void *) &len8, sizeof(uint8_t) );
      if( user != NULL)
        write( client_sockfd, user, len8 );

      if( group == NULL)
        len8 = 0;
      else
        len8 = strlen( group);
      write( client_sockfd, (void *) &len8, sizeof(uint8_t) );
      if( group != NULL)
        write( client_sockfd, group, len8 );

      if( hostname == NULL)
        len8 = 0;
      else
        len8 = strlen( hostname);
      write( client_sockfd, (void *) &len8, sizeof(uint8_t) );
      if( hostname != NULL)
        write( client_sockfd, hostname, len8 );
#endif
      close( client_sockfd);

      // turn off request flag
      prpvt->flags &= ~((uint16_t) 1);
      // if itrigger was set, unset it
      if( prec->itrig)
        {
          prec->itrig = 0; 

          monitor_itrig(prec);
        }
    }

  return NULL;
}


static long init_record(void *precord,int pass)
{
  aliveRecord *prec = (aliveRecord *)precord;
  rpvtStruct  *prpvt;

  epicsTimeStamp start_time;

  int i, flag;
  char *p;
  

  if( pass == 0) 
    {
      prec->rpvt = calloc(1, sizeof(struct rpvtStruct));

      return 0;
    }

  prec->val = 0;
  prpvt = prec->rpvt;

  prpvt->fault_flag = 0;
  prpvt->ready_flag = 0;
  prpvt->lport = prec->lport;
  prpvt->flags = 1;  // first bit set, read envs

  epicsTimeGetCurrent(&start_time);
  prpvt->incarnation = (uint32_t) start_time.secPastEpoch;
  
  for( i = 0; i < ENV_CNT; i++)
    {
      flag = sscanf( *(&prec->env1 + i), "%s", prpvt->env[i]);
      if( !flag)
        prpvt->env[i][0] = '\0';
    }

  if( (prpvt->socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
      errlogSevPrintf( errlogFatal, "%s", "alive record: "
                       "Won't start, can't open DGRAM socket.\n");
      prpvt->fault_flag = 1;
      return 1;
    }
  bzero( (void *) &(prpvt->h_addr), sizeof(struct sockaddr_in) );
  prpvt->h_addr.sin_family = AF_INET;
  prpvt->h_addr.sin_port = htons(prec->rport);
#ifdef vxWorks
  prpvt->h_addr.sin_len = (u_char) sizeof (struct sockaddr_in); 
  if( (prpvt->h_addr.sin_addr.s_addr = inet_addr (prec->rhost)) != ERROR)
#else
  if( inet_aton( prec->rhost, &(prpvt->h_addr.sin_addr)) )
#endif
    prpvt->ready_flag = 1;

  p = getenv("IOC");
  if( p == NULL)
    {
      errlogSevPrintf( errlogFatal, "%s", "alive record: "
                       "Won't start, can't get IOC environment variable.\n");
      prpvt->fault_flag = 1;
      return 1;
    }
  // condition is sanity check, as the output buffer is static
  if( strlen( p) > 100)
    {
      errlogSevPrintf( errlogFatal, "%s", "alive record: "
                       "Won't start, environment variable IOC too long.\n");
      prpvt->fault_flag = 1;
      return 1;
    }
  strncpy(prpvt->ioc_name, p, 40);
  prpvt->ioc_name[40] = '\0';

  prpvt->listen_thread = 
    epicsThreadCreate("alive_tcp_thread", 50, 
                      epicsThreadGetStackSize(epicsThreadStackMedium),
                      (EPICSTHREADFUNC) ioc_alive_listen, (void *) precord);
  
  return 0;
}

static long process(aliveRecord *prec)
{
  // 200 is fine, ioc name length is limited to 100 in init
  char buffer[200];  

  uint32_t message;

  rpvtStruct *prpvt = prec->rpvt;
  epicsTimeStamp now;

  char *p;
  int len;

  if( prpvt->fault_flag || !prpvt->ready_flag)
    return 0;

  prec->pact = TRUE;
  prec->udf = FALSE;

  prec->val++;

  p = buffer;

  // magic signature, to help weed out probes
  message = htonl(prec->rmag);
  *((uint32_t *) p) = message;
  p += 4;

  // protocol version
  message = htons(PROTOCOL_VERSION);
  *((uint16_t *) p) = message;
  p += 2;
  
  // incarnation
  message = htonl(prpvt->incarnation);
  *((uint32_t *) p) = message;
  p += 4;

  // current time
  epicsTimeGetCurrent(&now);
  message = htonl( (uint32_t) now.secPastEpoch);
  *((uint32_t *) p) = message;
  p += 4;

  // heartbeat
  message = htonl(prec->val);
  *((uint32_t *) p) = message;
  p += 4;

  // flags
  message = htons( prpvt->flags);
  *((uint16_t *) p) = message;
  p += 2;

  // return port
  message = htons( prpvt->lport);
  *((uint16_t *) p) = message;
  p += 2;

  // user message
  message = htonl(prec->msg);
  *((uint32_t *) p) = message;
  p += 4;

  len = sprintf( p, "%s", prpvt->ioc_name);
  // include trailing zero
  p += (len + 1);

  if( sendto( prpvt->socket, buffer, (int) (p - buffer), 0, 
              (struct sockaddr *) &(prpvt->h_addr), 
              sizeof(struct sockaddr_in))  == -1)
    {
      errlogSevPrintf( errlogMajor, "alive record: Can't send UDP packet "
                       "(sendto errno=%d).\n", errno);
    }

  recGblGetTimeStamp(prec);
  /* check for alarms */
  checkAlarms(prec);
  /* check event list */
  monitor(prec);
  /* process the forward scan link record */
  recGblFwdLink(prec);

  prec->pact=FALSE;
  return 0;
}


static long special(DBADDR *paddr, int after)
{
  struct aliveRecord *prec = (aliveRecord *)paddr->precord;
  struct rpvtStruct   *prpvt = prec->rpvt;
  int    special_type = paddr->special;

  int   fieldIndex = dbGetFieldIndex(paddr);
  
  int relIndex;
  int f;

  if( !after) 
    return 0;

  if( special_type != SPC_MOD)
    {
      recGblDbaddrError(S_db_badChoice, paddr, "alive: special");
      return (S_db_badChoice);
    }

  switch(fieldIndex) 
    {
    case(aliveRecordRHOST):
#ifdef vxWorks
      if( (prpvt->h_addr.sin_addr.s_addr = inet_addr (prec->rhost)) == ERROR)
#else
      if( !inet_aton( prec->rhost, &(prpvt->h_addr.sin_addr)) )
#endif
        prpvt->ready_flag = 0;
      else
        prpvt->ready_flag = 1;
      break;
    case(aliveRecordRPORT):
      prpvt->h_addr.sin_port = htons(prec->rport);
      break;
    case(aliveRecordISUP):
      if( prec->isup )
        prpvt->flags |= ((uint16_t) 2);
      else
        prpvt->flags &= ~((uint16_t) 2);
      break;
    case(aliveRecordITRIG):
      //      monitor_itrig(prec);
      // falls through to trigger reread
      break;
    case(aliveRecordENV1):
    case(aliveRecordENV2):
    case(aliveRecordENV3):
    case(aliveRecordENV4):
    case(aliveRecordENV5):
    case(aliveRecordENV6):
    case(aliveRecordENV7):
    case(aliveRecordENV8):
    case(aliveRecordENV9):
    case(aliveRecordENV10):
      relIndex = fieldIndex - aliveRecordENV1;
      f = sscanf( *(&prec->env1 + relIndex), "%s", prpvt->env[relIndex]);
      if( !f)
        prpvt->env[relIndex][0] = '\0';
      break;
    default:
      recGblDbaddrError(S_db_badChoice, paddr, "alive: special");
      return(S_db_badChoice);
    }

  // everything turns on request flag
  prpvt->flags |= ((uint16_t) 1);
  
  return 0;
}



static void checkAlarms(aliveRecord *prec)
{
  if (prec->udf == TRUE) 
    {
#if LT_EPICSBASE(3,15,0,2)
      recGblSetSevr(prec,UDF_ALARM,INVALID_ALARM);
#else
      recGblSetSevr(prec,UDF_ALARM,prec->udfs);
#endif
      return;
    }
  return;
}


static void monitor(aliveRecord *prec)
{
    unsigned short  monitor_mask;

    monitor_mask = recGblResetAlarms(prec);
    monitor_mask |= DBE_VALUE|DBE_LOG;
    db_post_events(prec,&prec->val,monitor_mask);

    return;
}

static void monitor_itrig(aliveRecord *prec)
{
    unsigned short  monitor_mask;

    monitor_mask = recGblResetAlarms(prec);
    monitor_mask |= DBE_VALUE;
    db_post_events(prec,&prec->itrig,monitor_mask);

    return;
}
