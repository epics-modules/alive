/* aliveRecord.c */


#ifdef vxWorks
  #include <version.h>
  #include <bootLib.h>
  #include <unistd.h>
  #if defined(_WRS_VXWORKS_MAJOR) && (_WRS_VXWORKS_MAJOR >= 6) 
    #include <errnoLib.h>
    #include <netinet/in.h>
    #include <sockLib.h>
    #include <ioLib.h>
  #else
    #include <sockLib.h>
    #include <inetLib.h>
    #include <hostLib.h>
  #endif
#elif defined (linux) || defined (darwin)
  #include <unistd.h>
  #include <pwd.h>
  #include <grp.h>
  #include <sys/types.h>
#elif defined (_WIN32)
  #include <winsock2.h>
  #include <stdint.h> 
#endif

#include <stddef.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

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
#include <osiSock.h>


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


#define ALIVE_VERSION (1)
#define ALIVE_REVISION (2)
#define ALIVE_MODIFICATION (0)
#define ALIVE_DEV_VERSION (0) // 0 means not dev

#define PROTOCOL_VERSION (5)


/* Create RSET - Record Support Entry Table */
#define Report NULL
#define Initialize NULL
static long init_record();
static long process();
static long special();
#define Get_value NULL
#define Cvt_dbaddr NULL
#define Get_array_info NULL
#define Put_array_info NULL
#define Get_units NULL
#define Get_precision NULL
#define Get_enum_str NULL
#define Get_enum_strs NULL
#define Put_enum_str NULL
#define Get_graphic_double NULL
#define Get_control_double NULL
#define Get_alarm_double NULL
 
rset aliveRSET =
  {
    RSETNUMBER,
    Report,
    Initialize,
    init_record,
    process,
    special,
    Get_value,
    Cvt_dbaddr,
    Get_array_info,
    Put_array_info,
    Get_units,
    Get_precision,
    Get_enum_str,
    Get_enum_strs,
    Put_enum_str,
    Get_graphic_double,
    Get_control_double,
    Get_alarm_double
  };
epicsExportAddress(rset,aliveRSET);

#define ENV_CNT (16)

static void checkAlarms(aliveRecord *prec);
static void monitor(aliveRecord *prec);
static void monitor_field(aliveRecord *prec, void *field);

struct rpvtStruct 
{
  struct alive_t *alive;

  char fault_flag; // in case of unrecoverable error
  char ready_flag; // in case host doesn't make sense

  uint32_t incarnation;
  uint16_t flags;
  uint16_t orig_port;

  char ioc_name[41];
  
  // 40 character limit enforced by EPICS string type
  // otherwise, can be very big
  char envdef[ENV_CNT][41];
  char env[ENV_CNT][41];

  epicsThreadId listen_thread;
  epicsThreadId send_thread;

  SOCKET socket;
  struct sockaddr_in h_addr;
};

// REDO sender to have 5 second timeout and select()
static int sender( SOCKET sock, void *data, int size)
{
  int i, cnt;
  cnt = 0;
  while( cnt < size)
    {
      i = send( sock, (void *) (((char *) data) + cnt), size-cnt, 0);
      if( i == -1)
        {
          if( errno == EINTR)
            continue;
          return errno;
        }
      cnt += i;
    }
  return 0;
}

// these functions assume that the buffer length has been
// computed correctly, which is needed for header as well
/* static char *buffer_adder_8( char *bptr, uint8_t value) */
/* { */
/*   *((uint8_t *) bptr) = value; */
/*   return bptr + 1; */
/* } */
static char *buffer_adder_16( char *bptr, uint16_t value)
{
  *((uint16_t *) bptr) = htons(value);
  return bptr + 2;
}
static char *buffer_adder_32( char *bptr, uint32_t value)
{
  *((uint32_t *) bptr) = htonl(value);
  return bptr + 4;
}
static char *buffer_adder_8string( char *bptr, char *string, uint8_t length)
{
  *((uint8_t *) bptr) = length;
  memcpy( (bptr + 1), string, length);
  return bptr + 1 + length;
}
static char *buffer_adder_16string( char *bptr, char *string, uint16_t length)
{
  *((uint16_t *) bptr) = htons(length);
  memcpy( (bptr + 2), string, length);
  return bptr + 2 + length;
}
static char *buffer_adder_8string_autolen( char *bptr, char *string)
{
  uint8_t length;
  length = strlen(string);
  *((uint8_t *) bptr) = length;
  memcpy( (bptr + 1), string, length);
  return bptr + 1 + length;
}
static char *buffer_adder_8string_nullcheck( char *bptr, char *string)
{
  if( string == NULL)
    {
      *((uint8_t *) bptr) = 0;
      return bptr + 1;
    }
  else
    {
      int len;
  
      len = strlen(string);
      *((uint8_t *) bptr) = len;
      memcpy( (bptr + 1), string, len);
      return bptr + 1 + len;
    }
}

static void *ioc_alive_listen(void *data)
{
  aliveRecord *prec = (aliveRecord *) data;
  struct rpvtStruct *prpvt;

  SOCKET tcp_sockfd;
  int sflag;

  SOCKET client_sockfd;

  struct sockaddr_in l_addr;

  osiSocklen_t socklen;

  char *q;
  
  // key and value lengths, key length of zero means it doesn't exist
  int env_len[ENV_CNT][2]; 
  int envdef_len[ENV_CNT][2]; 

  // IOC type
  int type;
  // number of environment variables with non-empty strings
  int ev_number;
  // length of data to send, to allow other end to preallocate memory
  int buffer_length;
  // buffer used for sending message
  char *send_buffer;
  
#if defined (vxWorks)
  BOOT_PARAMS bootparams;
#endif
#if defined (linux) || defined (darwin)
  char *user;
  char *group;
  char *hostname;
  char hostname_buffer[129];
#endif
#if defined (_WIN32)
  char *user;
  char user_buffer[80];
  char *machine;
  char machine_buffer[20];
#endif

  int i;
  int ret;
  char *sptr;
  
  prpvt = prec->rpvt;

#if defined (vxWorks)
  type = 1;
#elif defined (linux)
  type = 2;
#elif defined (darwin)
  type = 3;
#elif defined (_WIN32)
  type = 4;
#else
  type = 0;
#endif
  
#ifdef vxWorks
  bzero( (char *) &l_addr, sizeof( struct sockaddr_in) );
  l_addr.sin_len = sizeof( struct sockaddr_in);
#else
  memset( &l_addr, 0, sizeof( struct sockaddr_in) );
#endif
  l_addr.sin_family = AF_INET;
  l_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  l_addr.sin_port = htons(prpvt->orig_port);

  if( (tcp_sockfd = epicsSocketCreate(AF_INET, SOCK_STREAM, 0))
      == INVALID_SOCKET )
    {
      perror("socket");
      prec->ipsts = aliveIPSTS_INOPERABLE;
      monitor_field(prec, (void *) &prec->ipsts);
      return NULL;
    }

  sflag = 1;
  // not the end of the world if this option doesn't work
  setsockopt(tcp_sockfd, SOL_SOCKET, SO_REUSEADDR, (void *) &sflag,
             sizeof(sflag));


  if( bind(tcp_sockfd, (struct sockaddr *) &l_addr, 
           sizeof(struct sockaddr_in)) )
    {
      perror("TCP bind");
      prec->ipsts = aliveIPSTS_INOPERABLE;
      monitor_field(prec, (void *) &prec->ipsts);
      return NULL;
    }
  
  socklen = sizeof(struct sockaddr_in);
  if( getsockname( tcp_sockfd, (struct sockaddr *) &l_addr, &socklen) )
    {
      perror("TCP getsockname");
      prec->ipsts = aliveIPSTS_INOPERABLE;
      monitor_field(prec, (void *) &prec->ipsts);
      return NULL;
    }    
  // wait to use result until we know listen() works

  if( listen(tcp_sockfd, 5) )
    {
      perror("TCP listen");
      prec->ipsts = aliveIPSTS_INOPERABLE;
      monitor_field(prec, (void *) &prec->ipsts);
      return NULL;
    }

  prec->iport = ntohs(l_addr.sin_port);
  monitor_field(prec, (void *) &prec->iport);
  prec->ipsts = aliveIPSTS_OPERABLE;
  monitor_field(prec, (void *) &prec->ipsts);

  // request remote read
  prpvt->flags |= ((uint16_t) 1);
  
  while(1)
    {
      struct sockaddr r_addr;
      osiSocklen_t r_len = sizeof( r_addr);

      client_sockfd = epicsSocketAccept( tcp_sockfd, &r_addr, &r_len);
      if (client_sockfd == INVALID_SOCKET)
        continue;

      // fault flag can't happen, but just in case
      if( prec->isup || prpvt->fault_flag || !prpvt->ready_flag || 
          ( ((struct sockaddr_in *)&r_addr)->sin_addr.s_addr !=
            prpvt->h_addr.sin_addr.s_addr) )
        {
          epicsSocketDestroy(client_sockfd);
          continue;
        }

      // type and buffer_length should swap positions in the next protocol
      
      ev_number = 0;
      buffer_length = 10;  // from version + type + buffer_length + ev_number
#if defined (vxWorks)
      memset( &bootparams, 0, sizeof( BOOT_PARAMS) );
      bootStringToStruct( sysBootLine, &bootparams);
      // don't check to see if it returns EOS, as it's zeroed out

      // bootparams.unitNum, bootparams.procNum, bootparams.flags
      buffer_length += 12; 
      buffer_length += 12; // 8-bit lengths below
      buffer_length += ((uint8_t) strlen(bootparams.bootDev) );
      buffer_length += ((uint8_t) strlen(bootparams.hostName) );
      buffer_length += ((uint8_t) strlen(bootparams.bootFile) );
      buffer_length += ((uint8_t) strlen(bootparams.ead) );
      buffer_length += ((uint8_t) strlen(bootparams.bad) );
      buffer_length += ((uint8_t) strlen(bootparams.had) );
      buffer_length += ((uint8_t) strlen(bootparams.gad) );
      buffer_length += ((uint8_t) strlen(bootparams.usr) );
      buffer_length += ((uint8_t) strlen(bootparams.passwd) );
      buffer_length += ((uint8_t) strlen(bootparams.targetName) );
      buffer_length += ((uint8_t) strlen(bootparams.startupScript) );
      buffer_length += ((uint8_t) strlen(bootparams.other) );
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
      buffer_length += 1; // 8-bit string length
      if( user != NULL)
        buffer_length += ((uint8_t) strlen(user) );
      buffer_length += 1; // 8-bit string length
      if( group != NULL)
        buffer_length += ((uint8_t) strlen(group) );
      buffer_length += 1; // 8-bit string length
      if( hostname != NULL)
        buffer_length += ((uint8_t) strlen(hostname) );
#endif
#if defined (_WIN32)
      {
        uint32_t size;

        buffer_length += 2; // two variable lengths

        size = 80;
        if (GetUserNameA(user_buffer, &size))
          {
            user = user_buffer;
            buffer_length += ((uint8_t)strlen(user));
          }
        else
          user = NULL;

        size = 20;
        if (GetComputerNameA(machine_buffer, &size))
          {
            machine = machine_buffer;
            buffer_length += ((uint8_t)strlen(machine));
          }
        else
          machine = NULL;
      }
#endif

      for( i = 0; i < ENV_CNT; i++)
        {
          if( prpvt->envdef[i][0] == '\0')
            envdef_len[i][0] = 0;
          else
            {
              ev_number++;
              buffer_length += 3; // 8-bit key & 16-bit value string lengths
              envdef_len[i][0] = strlen(prpvt->envdef[i]);
              buffer_length += envdef_len[i][0];
              q = getenv(prpvt->envdef[i]);
              if( q == NULL)
                envdef_len[i][1] = 0;
              else
                {
                  envdef_len[i][1] = strlen(q);
                  // if size is greater that 16-bit max, truncate to zero
                  if( envdef_len[i][1] > 65535)
                    envdef_len[i][1] = 0;
                  buffer_length += envdef_len[i][1];
                }
            }
        }
      for( i = 0; i < ENV_CNT; i++)
        {
          if( prpvt->env[i][0] == '\0')
            env_len[i][0] = 0;
          else
            {
              ev_number++;
              buffer_length += 3; // 8-bit key & 16-bit value string lengths
              env_len[i][0] = strlen(prpvt->env[i]);
              buffer_length += env_len[i][0];
              q = getenv(prpvt->env[i]);
              if( q == NULL)
                env_len[i][1] = 0;
              else
                {
                  env_len[i][1] = strlen(q);
                  // if size is greater that 16-bit max, truncate to zero
                  if( env_len[i][1] > 65535)
                    env_len[i][1] = 0;
                  buffer_length += env_len[i][1];
                }
            }
        }
      
      /* Start send procedure */
      
      if( (send_buffer = malloc(buffer_length * sizeof(char))) == NULL)
        {
          errlogSevPrintf( errlogMajor, "alive record: Can't malloc() "
                           "memory to send TCP data.\n");
          epicsSocketDestroy( client_sockfd);
          continue;
        }
      sptr = send_buffer;
      
      // TCP protocol version
      sptr = buffer_adder_16( sptr, PROTOCOL_VERSION);
      // IOC type, currently 1 = vxworks, 2 = linux, 3 = darwin
      sptr = buffer_adder_16( sptr, type);

      sptr = buffer_adder_32( sptr, buffer_length);
      sptr = buffer_adder_16( sptr, ev_number);

      for( i = 0; i < ENV_CNT; i++)
        {
          if( envdef_len[i][0] == 0)
            continue;

          sptr = buffer_adder_8string( sptr, prpvt->envdef[i],
                                       envdef_len[i][0]);

          if( envdef_len[i][1] == 0)
            sptr = buffer_adder_16( sptr, 0);
          else
            {
              q = getenv(prpvt->envdef[i]);
              // protect against variable disappearing
              sptr = buffer_adder_16string( sptr, q == NULL ? "" : q,
                                           envdef_len[i][1]);
            }
            
        }
      for( i = 0; i < ENV_CNT; i++)
        {
          if( env_len[i][0] == 0)
            continue;

          sptr = buffer_adder_8string( sptr, prpvt->env[i],
                                       env_len[i][0]);

          if( env_len[i][1] == 0)
            sptr = buffer_adder_16( sptr, 0);
          else
            {
              q = getenv(prpvt->env[i]);
              sptr = buffer_adder_16string( sptr, q == NULL ? "" : q,
                                           env_len[i][1]);
            }

        }

#ifdef vxWorks
      sptr = buffer_adder_8string_autolen( sptr, bootparams.bootDev);
      sptr = buffer_adder_32( sptr, bootparams.unitNum);
      sptr = buffer_adder_32( sptr, bootparams.procNum);
      sptr = buffer_adder_8string_autolen( sptr, bootparams.hostName);
      sptr = buffer_adder_8string_autolen( sptr, bootparams.bootFile);
      sptr = buffer_adder_8string_autolen( sptr, bootparams.ead);
      sptr = buffer_adder_8string_autolen( sptr, bootparams.bad);
      sptr = buffer_adder_8string_autolen( sptr, bootparams.had);
      sptr = buffer_adder_8string_autolen( sptr, bootparams.gad);
      sptr = buffer_adder_8string_autolen( sptr, bootparams.usr);
      sptr = buffer_adder_8string_autolen( sptr, bootparams.passwd);
      sptr = buffer_adder_32( sptr, bootparams.flags);
      sptr = buffer_adder_8string_autolen( sptr, bootparams.targetName);
      sptr = buffer_adder_8string_autolen( sptr, bootparams.startupScript);
      sptr = buffer_adder_8string_autolen( sptr, bootparams.other);
#endif
#if defined (linux) || defined (darwin)
      sptr = buffer_adder_8string_nullcheck( sptr, user);
      sptr = buffer_adder_8string_nullcheck( sptr, group);
      sptr = buffer_adder_8string_nullcheck( sptr, hostname);
#endif
#if defined (_WIN32)
      sptr = buffer_adder_8string_nullcheck( sptr, user);
      sptr = buffer_adder_8string_nullcheck( sptr, machine);
#endif
      ret = sender( client_sockfd, send_buffer, buffer_length);
      epicsSocketDestroy( client_sockfd);
      free(send_buffer);
      if( ret) // error, so don't unset flags
        {
          errlogSevPrintf( errlogMajor, "alive record: Can't send TCP data "
                           "(send errno=%d).\n", ret);
          continue;
        }
      
      // turn off request flag
      prpvt->flags &= ~((uint16_t) 1);
      // if itrigger was set, unset it
      if( prec->itrig)
        {
          prec->itrig = 0; 

          monitor_field(prec, (void *) &prec->itrig);
        }
    }

  
  return NULL;
}


static void *ioc_alive_send(void *data)
{
  aliveRecord *prec = (aliveRecord *) data;
  struct rpvtStruct *prpvt;

  // 200 is fine, ioc name length is limited to 100 in init
  char buffer[200];  

  uint32_t message;

  epicsTimeStamp now;

  char *p;
  int len;

  prpvt = prec->rpvt;
  
  epicsThreadSleep( 0.1);

  while(1)
    {
      if( prpvt->fault_flag || !prpvt->ready_flag || 
          (prec->hrtbt == aliveHRTBT_OFF ) )
        {
          epicsThreadSleep( (double) prec->hprd);
          continue;
        }

      prec->val++;

      p = buffer;

      // magic signature, to help weed out probes
      message = htonl(prec->hmag);
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

      // period
      message = htons( prec->hprd);
      *((uint16_t *) p) = message;
      p += 2;

      // flags
      message = htons( prpvt->flags);
      *((uint16_t *) p) = message;
      p += 2;

      // return port
      message = htons( prec->iport);
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

      epicsThreadSleep( (double) prec->hprd);
    }

  return NULL;
}

static long init_record(void *precord, int pass)
{
  aliveRecord *prec;
  struct rpvtStruct *prpvt;

  epicsTimeStamp start_time;

  int i, flag;
  char *p;
  

  prec = precord;

  if( pass == 0) 
    {
      // last element of struct rpvtStruct keeps coming out as 
      // zero length array, so pad it a bit to do a kludge fix
      prec->rpvt = calloc(1, sizeof(struct rpvtStruct) + 16);

      return 0;
    }

  prec->val = 0;
  
  i = sprintf( prec->ver, "%d-%d-%d", ALIVE_VERSION, ALIVE_REVISION,
               ALIVE_MODIFICATION );
  if( ALIVE_DEV_VERSION > 0)
    sprintf( &prec->ver[i], "-dev%d", ALIVE_DEV_VERSION );

  prpvt = prec->rpvt;

  // save value to local structure, set to zero until initialized.
  prpvt->orig_port = prec->iport;
  prec->iport = 0;

  // if period is set to 0, use default of 15
  if( !prec->hprd)
    prec->hprd = 15;

  prpvt->fault_flag = 0;
  prpvt->ready_flag = 0;
  prpvt->flags = 0;  // first bit set when port initialized

  epicsTimeGetCurrent(&start_time);
  prpvt->incarnation = (uint32_t) start_time.secPastEpoch;

  for( i = 0; i < ENV_CNT; i++)
    {
      flag = sscanf( *(&prec->evd1 + i), "%s", prpvt->envdef[i]);
      if( flag != 1)
        prpvt->envdef[i][0] = '\0';
    }
  for( i = 0; i < ENV_CNT; i++)
    {
      flag = sscanf( *(&prec->ev1 + i), "%s", prpvt->env[i]);
      if( flag != 1)
        prpvt->env[i][0] = '\0';
    }

  if( (prpvt->socket = epicsSocketCreate(AF_INET, SOCK_DGRAM, 0)) == 
      INVALID_SOCKET)
    {
      errlogSevPrintf( errlogFatal, "%s", "alive record: "
                       "Won't start, can't open DGRAM socket.\n");
      prpvt->fault_flag = 1;
      return 1;
    }

  
#ifdef vxWorks
  bzero( (void *) &(prpvt->h_addr), sizeof(struct sockaddr_in) );
#else
  memset( (void *) &(prpvt->h_addr), 0, sizeof(struct sockaddr_in) );
#endif
  prpvt->h_addr.sin_family = AF_INET;
  prpvt->h_addr.sin_port = htons(prec->rport);
  if(!hostToIPAddr(prec->rhost, &(prpvt->h_addr.sin_addr) ) )
    {
      prpvt->ready_flag = 1;
      strcpy( prec->raddr, inet_ntoa( prpvt->h_addr.sin_addr) );
    }
  else
    {
      strcpy( prec->raddr, "invalid RHOST" );
    }
  
  flag = sscanf( prec->iocnm, "%s", prpvt->ioc_name);
  if( flag == 1)
    {
      strcpy( prec->nmvar, "----");
    }
  else
    {
      p = getenv(prec->nmvar);
      if( p == NULL)
        {
          errlogSevPrintf( errlogFatal, "%s%s%s", 
                           "alive record: Won't start, can't get ",
                           prec->nmvar," name environment variable.\n");
          prpvt->fault_flag = 1;
          return 1;
        }
      // condition is sanity check, as the output buffer is static
      if( strlen( p) > 39)
        {
          errlogSevPrintf( errlogFatal, "%s%s%s", 
                           "alive record: Won't start, environment variable ",
                           prec->nmvar, " too long.\n");
          prpvt->fault_flag = 1;
          return 1;
        }
      strncpy(prpvt->ioc_name, p, 39);
      prpvt->ioc_name[39] = '\0';
      strncpy(prec->iocnm, prpvt->ioc_name, 40);
    }

  prpvt->listen_thread = 
    epicsThreadCreate("alive_tcp_listen_thread", 51, 
                      epicsThreadGetStackSize(epicsThreadStackMedium),
                      (EPICSTHREADFUNC) ioc_alive_listen, (void *) precord);
  prpvt->send_thread = 
    epicsThreadCreate("alive_udp_send_thread", 50, 
                      epicsThreadGetStackSize(epicsThreadStackMedium),
                      (EPICSTHREADFUNC) ioc_alive_send, (void *) precord);

  return 0;
}

static long process(aliveRecord *prec)
{
  prec->pact = TRUE;
  prec->udf = FALSE;

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
      if(!hostToIPAddr(prec->rhost, &(prpvt->h_addr.sin_addr) ) )
        {
          prpvt->ready_flag = 1;
          strcpy( prec->raddr, inet_ntoa( prpvt->h_addr.sin_addr) );
        }
      else
        {
          prpvt->ready_flag = 0;
          strcpy( prec->raddr, "invalid RHOST" );
        }
      db_post_events(prec,&prec->raddr,DBE_VALUE);
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
      // falls through to trigger reread
      break;
    case(aliveRecordEV1):
    case(aliveRecordEV2):
    case(aliveRecordEV3):
    case(aliveRecordEV4):
    case(aliveRecordEV5):
    case(aliveRecordEV6):
    case(aliveRecordEV7):
    case(aliveRecordEV8):
    case(aliveRecordEV9):
    case(aliveRecordEV10):
    case(aliveRecordEV11):
    case(aliveRecordEV12):
    case(aliveRecordEV13):
    case(aliveRecordEV14):
    case(aliveRecordEV15):
    case(aliveRecordEV16):
      relIndex = fieldIndex - aliveRecordEV1;
      f = sscanf( *(&prec->ev1 + relIndex), "%s", prpvt->env[relIndex]);
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

static void monitor_field(aliveRecord *prec, void *field)
{
    unsigned short  monitor_mask;

    monitor_mask = recGblResetAlarms(prec);
    monitor_mask |= DBE_VALUE;
    db_post_events(prec,field,monitor_mask);

    return;
}

