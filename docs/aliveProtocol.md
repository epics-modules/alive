---
layout: default
title: Message Protocol
nav_order: 6
---

# alive Message Protocol
{: .no_toc}

## Table of contents
{: .no_toc .text-delta }

- TOC
{:toc}

This page describes the current protocol, version __5__. All messages sent use network order (big-endian).

{: .note }
> The time values sent are EPICS time values, which are relative to
> 1990. Converting them to standard Linux time values means adding
> a value of 631152000 (20 years of seconds) to each.

Heartbeat Message
-----------------

This is the UDP message sent for each heartbeat of the alive record. The minimum size for a message payload is 30 bytes, being fixed fields of 28 bytes with a null-terminated string. All values are unsigned.

| Offset (bytes) |       Field     |
|:--------------:|:---------------:|
|      0 - 3     |      Magic      |
|      4 - 5     |     Version     |
|      6 - 9     |   Incarnation   |
|     10 - 13    |   Current Time  |
|     14 - 17    | Heartbeat Value |
|     18 - 19    |      Period     |
|     20 - 21    |      Flags      |
|     22 - 23    |   Return Port   |
|     24 - 27    |  User Message   |
|     28 - ...   | IOC Name (len X)|
|     28 + X     |        0        |

* __Magic Number__ (32-bit)  The value of this field comes from the __HMAG__ field. It is used by the remote server to delete messages received that don't start with this number.  
* __Version of Protocol__ (16-bit)  The value of this field is the current version of the protocol for this record. The remote server can handle or ignore a particular version as it sees fit. If the version number does not match the one that this document describes, the fields after this one will most likely differ in some way.  
* __Incarnation__ (32-bit)  This value is a unique number for this particular boot of the IOC. It's also the boot time as measured by the IOC, which should be unique for each boot, assuming that the EPICS time is correct when the record initialization happens.  
* __Current Time__ (32-bit)  This is the current time as measured by the IOC.  
* __Heartbeat Value__ (32-bit)  This is the current value of the __VAL__ field.  
* __Period__ (16-bit)  This is the heartbeat period used by the alive record, which is to be used for determining the operational status of the IOC, and comes from __HPRD__.  
* __Flags__ (16-bit)  This value holds bit flags for the server.   
    - __Bit 0:__ Server should read the environment variables, as there are updated values or __ITRIG__ was set. This will be cleared after a successful read.
    - __Bit 1:__ Server is not allowed to read the environment variables, and will be blocked if tried. This is set by __ISUP__, and this bit overrides bit 0.
    
* __Return Port__ (16-bit)  This is the return TCP port number to use for reading IOC information, from __IPORT__.  
* __User Message__ (32-bit)  Whatever is in __MSG__ will be included here.  
* __IOC Name__ (variable length 8-bit)  This is the value of the environment variable *IOC*, and is null-terminated.  

- - - - - -

Information Request Message
---------------------------
    
This is the message that is read from the TCP port __IPORT__ on the IOC. When the port is opened, the IOC will write this message and then immediately close the port. There is no way to write a message to the IOC this way.

{: .warning }
> If the suppression __ISUP__ field is set to "On", the IOC will
> immediately close any connection to this port.

| Offset (bytes) |       Field     |
|:--------------:|:---------------:|
|      0 - 1     |    Version      |
|      2 - 3     |    IOC Type     |
|      4 - 7     | Message Length  |
|      8 - 9     | Variable Count  |

* __Version of Protocol__ (16-bit)  The value of this field is the current version of the protocol for this record. The remote server can handle a previous version or ignore them as it sees fit.  
* __IOC Type__ (16-bit)  Stores type of IOC. This value also determines the type of extra information that is at the end of the message.  
    - __0)__ Generic: No extra information.
    - __1)__ VxWorks: The boot parameters are sent.
    - __2)__ Linux: The user and group IDs of the process as well as the hostname are sent.
    - __3)__ Darwin: The user and group IDs of the process as well as the hostname are sent.
    - __4)__ Windows: The login name and machine name are sent.
    
* __Message Length__ (32-bit)  This is the length of the entire message.  
* __Variable Count__ (16-bit)  This is the number of environment variables sent. Only values for non-empty __EVDxx__ and __EVxx__ fields are sent.

At this point of the message, byte 10, the locations become variable due to the variable nature of the data. The environment variables are sent as multiple records, the number being __Variable Count__.

| Offset (bytes) |         Field       |
|:--------------:|:-------------------:|
|        0       |   Name Length (X)   |
|      1 - X     |    Variable Name    |
|    X+1 - X+2   | Variable Length (Y) |
|    X+3 - X+Y+2 |   Variable Value    |

* __Name Length__ (8-bit)  This is the length of the environment variable name.  
* __Variable Name__ (variable length 8-bit)  This is the name of the environment variable (cannot be an empty string).  
* __Value Length__ (16-bit)  This is the length of the environment variable value. If the value was over 16-bits in size (64kb), an empty string is returned.  
* __Variable Value__ (variable length 8-bit)  This is the value of the environment variable. If the variable did not exist on the IOC, this is an empty string (size 0).

If the value of __IOC Type__ is non-zero, there may be extra data at this point. Currently both supported types do include data, so the extra information presented below is present for vxWorks, Linux, and Darwin.

- - - - - -

OS-Specific Data
----------------

### vxWorks

For vxWorks, the extra information is the boot parameters. The data is either in a string or a number. A string is represented by an 8-bit string length, followed by the string itself. The number is a 32-bit number.
    
* __Field Order__
    * Boot Device (str)       
    * Unit Number (int)       
    * Processor Number (int)  
    * Boot Host Name (str)    
    * Boot File (str)         
    * Address (str)           
    * Backplane Address (str) 
    * Boot Host Address (str) 
    * Gateway Address (str)   
    * User Name (str)         
    * User Password (str)     
    * Flags (int)             
    * Target Name (str)       
    * Startup Script (str)    
    * Other (str)   

### Linux and Darwin

For Linux and Darwin, the extra information is the user and group IDs of the IOC process, as well as the hostname of the host computer. The data are represented by an 8-bit string length, followed by the string itself.
    
* __Field Order__
    * User ID (str)  
    * Group ID (str) 
    * Hostname (str) 

### Windows

For Windows, the extra information is the login name of the IOC process, as well as the machine name of the host computer. The data are represented by an 8-bit string length, followed by the string itself.
    
* __Field Order__
    * Login name (str)   
    * Machine name (str) 
