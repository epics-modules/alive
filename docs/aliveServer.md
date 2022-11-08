---
layout: default
title: Alive Server
nav_order: 5
---


alive Server Design
===================

Introduction
------------

The alive record in itself is not very useful, as it needs a server that collects the heartbeat UDP packets sent by the alive records on IOCs. The server has to process the messages and keep a database in order to make use of the information.

The typical intended configuration is for many IOCs sending heartbeats to a single server. This of course means there is a single point of failure, being the server. If this is a problem, since the record doesn't allow for multiple packet recipients (although it could in theory), multiple records could be on an IOC with different server targets (and different local TCP ports), allowing for server redundancy.

This document describes how the data can be used. It is based on how the author has designed a server, [alived](https://github.com/epics-alive-server/alived).

- - - - - -

Heartbeat Processing
--------------------

 The first thing is to make sure that the alive record is sending heartbeats UDP packets to the server (from __RHOST__) at the expected UDP port (from __RPORT__), and at the expected rate determined from the __HPRD__ period.

 UDP packets will arrive at the server port from IOCs at this point. UDP packets are by their nature unreliable, with some getting dropped or delayed (so packets may arrive out of order), so the packet handling has to allow for this.

 The IP address of the sending IOC is not included in the heartbeat. This is because there might be several active network interfaces, which make it not clear which one will be used for sending. When receiving the UDP packet, the IP address of the sender is given, which needs to be used for the IOC IP address. The IP address alone can't identify an IOC, as multiple IOCs can exist on one machine, which is why the *IOC* environment variable is used for identification.

 The following shows how the UDP message's fields can be used (they are shown in order), as well as the record field if one corresponds to that value. A data structure for each IOC should be made, and all the data structures should be made into a searchable construct (binary tree/list/etc.) where the key is the IOC name. Below, the values that should be recorded into a data structure are noted.

__Magic Number__ (32-bit) - __HMAG__   This value is used by the server to weed out UDP packets sent to it that it does not expect or want. A server typically would allow only one number, but at its discretion could handle multiple magic numbers (different ones for different IOCs) or just accept all packets.  
__Version of Protocol__ (16-bit)  This value signifies the version of the message protocol, and determines the fields that follow. A server can support multiple versions, or ignore all versions but one (forcing the IOC to match the version). The current value is __5__.  
__Incarnation__ (32-bit)  This value should be recorded, and it serves two purposes. It is the recorded EPICS time when the record was initialized, hence it's a boot time (as measured by the IOC). It also acts as a unique identifier for the IOC session, and a change means that a boot happened and this is now a new session. If this value changes, one should reinitialize the IOC's data record as if it was new (as things may have been changed between boots).  
__Current Time__ (32-bit)  This value should be recorded and is the current EPICS time as measured by the IOC.   
__Heartbeat Value__ (32-bit) - __VAL__   This value should be recorded. It increases by one each time the remote alive record processes. If a packet with a lower heartbeat value arrives, it should be ignored as it came out of order.  
__Period__ (16-bit)  This value should be recorded and is the period used for sending heartbeats, which can be used for determining failure.  
__Flags__ (16-bit)  This value holds bit flags for the server, and they need to be acted on. Currently, the bits only deal with a TCP callback to the alive record.  
- __Bit 0 (Read):__ Set when __ITRIG__ is set, or when a record field is updated. This means that the record wants the server to do a TCP callback to read its extra information. After a successful read, this will be cleared. If the server does not want to implement TCP callbacks, then this bit can be ignored.
- __Bit 1 (Blocked):__ Set when __ISUP__ is set. This means that the server can't make a callback to the alive record. This bit overrides bit 0 (which operates independently). If an attempt to make a TCP callback is made by the server, it will be rejected. An IOC behind a firewall that does not allow TCP return traffic should have this permanently enabled to keep the server from endlessly trying to make a callback.
 
__Return Port__ (16-bit) - __IPORT__   This is the port number to use for making a TCP callback to the alive record. If implementing callbacks, it should either be recorded or simply passed along to the callback routine. If the IOC was not able to create the callback port, a value of 0 will be here.   
__User Message__ (32-bit) - __MSG__   This value has no set action. This value should either be recorded and/or acted on if used as a server flag. Multiple values or flags could be combined in this value, and might need to be split out.  
__IOC Name__ (variable length 8-bit)  This value should be recorded as the searchable key for the IOC data structure. It's the value of the *IOC* environment variable at the server.  

- - - - - -


Failure Detection and Up/Down Times
-----------------------------------

 When an IOC is turned off or crashes, there is no immediate detection of failure. This determination depends on the rate of heartbeats and the number of missing heartbeats. For a __HPRD__ rate of 15 seconds, a failure declared after four missing heartbeats would be a minute. This is fairly conservative, and if you are certain that the network between the IOCs and the server doesn't drop many packets, the packet number can be reduced; the __HPRD__ rate could also be increased, although that means more processing at the server.

 The time value to use for determination of failure should be how long it has been since the last accepted heartbeat (as they can be out of order) was received, with the reception time being locally measured by the server, not the IOC's current time. There might seem to be some redundancy of measuring server local time when the IOC sends its local time, but this allows you to sense any packet delivery lag or any systematic difference in time (like from time zone differences). One also has to remember that the EPICS time sent back from IOCs has a negative offset from Linux time of 631152000 seconds (20 years).

 A failure can be actively detected and acted on directly by the server, or the server can simply collect data and let polling clients determine the failures themselves (which allows for varying failure times and __HPRD__ rates).

 The up time for an active IOC is the time since the last heartbeat plus the difference between the IOC's last current time and its incarnation time.

 The down time for a failed IOC is the time since the last heartbeat.

- - - - - -


4. Callback Processing
----------------------

 The TCP callback is used to get static information from the alive record. If the IOC was not able to create the callback port, the value of the Return Port will be 0, and a callback can't be made.

 The server can make a callback at any time, although it typically should be done when a new incarnation is seen in the heartbeat message or when the Read (Bit 0) flag is set in the heartbeat message. Also, if the Blocked (Bit 1) flag is set in the heartbeat message, the callback will not work as the record will not accept connections; if the server tries a callback to an alive record that is sending heartbeats to a different server, it will also fail.

 The information returned is static in nature (and the server doesn't really need it for running), so it should be recorded in a data structure, attached to the IOC data entry.

- - - - - -
