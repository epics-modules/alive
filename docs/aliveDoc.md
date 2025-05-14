---
layout: default
title: Overview
nav_order: 2
---


alive Module Documentation
==========================

[Release Notes](aliveReleaseNotes.html)

Record
------

[aliveRecord.html](aliveRecord.html)

> The __alive__ record is used for determining where the heartbeat information is sent, as well as the extra environment variables that may be requested remotely.  

[aliveServer.html](aliveServer.html)  

>This record requires a separate server program outside of the scope of EPICS for it to be useful. It is intended for it to be straightforward to create such a server implementation, and this linked document describes how one can implement one for receiving and processing the heartbeat meassages, as well as making inquiries to the alive record about the extra information. The author's version of such a server, __alived__, can be found [here](https://github.com/epics-alive-server/alived).  

EPICS Database
--------------

alive.db  

>This database is basically a straight implementation of the alive record itself. It creates a record of the name "$(P)alive".
>
>The name of the IOC sent back to the server is by default the value of the "IOC" environment variable, but can be directly specified by setting IOCNM to the name wanted.
>
>The __RHOST__ is defined using "$(RHOST)". The rest of the fields that can be specified have defaults in them that can be overridden. The default for "$(RPORT)" is "5678", and for "$(HPRD)" is "15". If multiple IOC are to run on the same computer, then "$(IPORT)" for each IOC should have a different value, or all "$(IPORT)" values should be set to zero to allow the system to determine it. Only the first nine of the default environment variable fields, "$(EVD1)" to "$(EVD9)", have values, being "ARCH", "TOP", "EPICS\_BASE", "SUPPORT", "ENGINEER", "LOCATION", "GROUP", "STY", and "PROCSERV\_INFO".

aliveMSGCalc.db  

>This database consists of a calcout record that will periodically set the __MSG__ value of the IOC's alive record, based on status PV values of other *synApps* modules. The __MSG__ usage is undefined, but was intended for sending warnings if something bad happens, as a sequence of error bits. Currently, only the status of the autosave is being used. If the first bit of the __MSG__ is set, then autosave is dead.
>
>This is not included directly in *alive.db* since the alive module might be deployed without the other *synApps* modules. It might also become locally customized.
>
>There is an associated *aliveMSGCalc\_local.req* file. It does not use the normal *\_settings.req* convention (which in recent synApps versions causes it to be automatically loaded), as it's intended for manually overriding the global defaults only if that is wanted. Otherwise, changing the calculation parameters would need to be done for every IOC instead of at the master file location.

MEDM display files
------------------

aliveRecord.adl  

>This is the medm ADL file for an alive record. All the fields, except for the environment variables to be sent, are exposed on this screen.

aliveRecordEnvVars.adl  

>This is the medm ADL file for the environment variables to be sent by the alive record. The top values are the unchangeable defaults, while the bottom values can be changed at any time.

How to build and use
--------------------

- Edit `configure/RELEASE` to specify the paths to __EPICS base__. There are no dependencies on any other modules.
- Run Gnu Make to build.
- Note that the *alive* module is not useful on its own, as it needs a remote server to send heartbeat information.
 
Checking to see if record is talking to a remote server
-------------------------------------------------------

- Configure an alive record to talk to a remote server daemon that understands how to talk to this type of record.
- Make sure that RADDR has the correct IPv4 address in it, as converted from RHOST, and that RPORT is correct. RHOST and RPORT can't be changed on a running IOC, but AHOST and APORT allow this and can be used for testing, with the corresponding address in AADR.
- Make sure that IPSTS is set by the record to "Operable", otherwise the information port is not working.
- Make sure that the HRTBT field is set to "On" (the default).
- Make sure that the ISUP field is set to "Off" (the default).
- Check the RRSTS field to check that the read status of the remote server (or ARSTS for an auxiliary server) is "Idle". If so, then set ITRIG field to "Trigger"; otherwise the record is already in the process of requesting a remote read.
- If RRSTS (or ARSTS) field eventually changes to "Overdue", then the remote server is either not receiving the heartbeat correctly, or is not reading or able to read the TCP port at value IPORT; if it eventually changes to "Idle", then the connection with remote server is working correctly.
