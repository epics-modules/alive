
      /*
#define BOOT_DEV_LEN            40      // max chars in device name
#define BOOT_HOST_LEN           20      // max chars in host name
#define BOOT_ADDR_LEN           30      // max chars in net addr
#define BOOT_TARGET_ADDR_LEN    50      // IP address + mask + lease times
#define BOOT_ADDR_LEN           30      // max chars in net addr
#define BOOT_FILE_LEN           160     // max chars in file name
#define BOOT_USR_LEN            20      // max chars in user name
#define BOOT_PASSWORD_LEN       20      // max chars in password
#define BOOT_OTHER_LEN          80      // max chars in "other" field

#define BOOT_FIELD_LEN          160     // max chars in any boot field

typedef struct                          // BOOT_PARAMS
    {
    char bootDev [BOOT_DEV_LEN];        // boot device code
    char hostName [BOOT_HOST_LEN];      // name of host
    char targetName [BOOT_HOST_LEN];    // name of target
    char ead [BOOT_TARGET_ADDR_LEN];    // ethernet internet addr
    char bad [BOOT_TARGET_ADDR_LEN];    // backplane internet addr
    char had [BOOT_ADDR_LEN];           // host internet addr
    char gad [BOOT_ADDR_LEN];           // gateway internet addr
    char bootFile [BOOT_FILE_LEN];      // name of boot file
    char startupScript [BOOT_FILE_LEN]; // name of startup script file
    char usr [BOOT_USR_LEN];            // user name
    char passwd [BOOT_PASSWORD_LEN];    // password 
    char other [BOOT_OTHER_LEN];        // available for applications
    int  procNum;                       // processor number
    int  flags;                         // configuration flags
    int  unitNum;                       // network device unit number
    } BOOT_PARAMS;
*/
