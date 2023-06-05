---
layout: default
title: Release Notes
nav_order: 4
---


alive Module Release Notes
==========================

Release 1-4-1
-------------

 Documentation updated for github pages


Release 1-4-0
-------------

 Added __RRSTS__ and __ARSTS__ to show the read status from the main and auxiliary remote servers. These can be used to check for a connection problem between the servers and the record.

 Altered __ITRIG__ field to work solely as a trigger and not imply a status change with its value. Its value will immediately return to "Idle" after being set to "Trigger".

Release 1-3-1
-------------

 Added mentions about the new github repository for the BCDA alive server, [alived](https://github.com/epics-alive-server/alived).

Release 1-3-0
-------------

 Added __AHOST__, __APORT__, and __AADR__ in order to allow duplicate heartbeats to be sent to an auxiliary server. This can be used for testing purposes or server backup.

 Made __RHOST__, __RPORT__, and __HMAG__ all read-only to help make sure that record doesn't stop sending correct heartbeats to the server.

 Removed __NMVAR__, as it is redundant to simply using an environment variable to directly set __IOCNM__.

Release 1-2-1
-------------

 Added *CONSOLE\_INFO* as __EVDxx__ default.

 Fixed compilation issue for non-gcc compilers.

Release 1-2-0
-------------

 Allow __RHOST__ to be a name, and not just a numeric IP address. Added __RADDR__ to contain the resulting IP address found, or state that the __RHOST__ value is invalid.

 Changed the name of the __ENVxx__ fields to __EVxx__.

 Added 16 __EVDxx__ fields, similar to __EVxx__ fields, which are default environment variables that are unchangeable after boot.

 Moved the default environment variables in *alive.db* from the renamed __ENVxx__ fields to the new __EVDxx__ fields. Added *STY* and *PROCSERV\_INFO* as __EVDxx__ defaults.

Release 1-1-1
-------------

 Minor configuration file changes.

Release 1-1-0
-------------

 Added a field __IOCNM__ which shows the IOC name that is sent to the remote server. If this value is set directly, then this is the value used, otherwise it is found using the __NMVAR__ field.

 Added a field __NMVAR__ to define the environment variable that defines the IOC's name, with it defaulting to IOC.

 Added a new database, *aliveMSGCalc.db*, which creates a calcout that writes into alive record's __MSG__ field automatically, based on available synApps error messages. Currently, it is only using an autosave error PV as input.

Release 1-0-1
-------------

 Changed number of ENVxx PV fields from 10 to 16.

 Windows build fixes from other BCDA staff added.

Release 1-0-0
-------------

 This is the first release of the synApps alive module. It was developed using EPICS base 3.14.12, but earlier 3.14 releases should probably work.
