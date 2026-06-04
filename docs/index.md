---
layout: default
title: Overview
nav_order: 1
---

# alive Module
{: .no_toc}

## Table of contents
{: .no_toc .text-delta }

- TOC
{:toc}

Introduction
------------

The __alive__ module provides a heartbeat-based system for monitoring whether EPICS IOCs are running. It consists of a custom EPICS record type -- the [alive record](aliveRecord.html) -- that sends periodic UDP heartbeat messages to a central server and responds to TCP information requests.

The system has two main components:

- **alive record** -- Runs inside each IOC. Sends UDP heartbeats to a configured server and listens for TCP queries about environment variables and system information.
- **alive server** -- A separate program (outside of EPICS) that collects heartbeats, tracks IOC status, and queries IOCs for additional information. The [alive server design](aliveServer.html) page describes how a server should process the data. The author's server implementation, __alived__, can be found [here](https://github.com/epics-alive-server/alived).

{: .important }
> The alive module is not useful on its own, as it needs a remote
> server to send heartbeat information.

Key design characteristics:

- IOCs self-report to the server; the server doesn't need to know which IOCs to monitor in advance.
- Heartbeats are simple UDP packets, avoiding EPICS subnet boundary issues.
- The server queries IOC information over TCP, keeping the server independent of EPICS libraries.
- The [message protocol](aliveProtocol.html) is documented for server implementers.

See the [User Guide](aliveUserGuide.html) for database files, build instructions, display files, and troubleshooting.
