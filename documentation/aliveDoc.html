<!DOCTYPE html>
<html>
  <head>
    <meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1">
    <title>aliveDocs</title>
  </head>
  <body>

    <h1 align="center">alive Module Documentation</h1>

    <p align="center" style="font-size: 1.4em;margin-bottom:1em">Dohn Arms</p>

    <a href="aliveReleaseNotes.html">Release Notes</a>

    <h2>Record</h2>

    <dl>
      <dt><a href="aliveRecord.html">aliveRecord.html</a></dt>
      <dd>
        The <strong>alive</strong> record is used for determining
        where the heartbeat information is sent, as well as the extra
        environment variables that may be requested remotely.
      </dd>
      <dt><a href="aliveServer.html">aliveServer.html</a></dt>
      <dd>
        This record requires a separate server program outside of the
        scope of EPICS for it to be useful.  It is intended for it to
        be straightforward to create such a server implementation, and
        this linked document describes how one can implement one for
        receiving and processing the heartbeat meassages, as well as
        making inquiries to the alive record about the extra
        information.  The author's version of such a
        server, <b>alived</b>, can be found
      <a href="https://github.com/epics-alive-server/alived">here</a>.
      </dd>
    </dl>

    <h2>EPICS Database</h2>

    <dl>
      <dt>alive.db</dt>
      <dd>
        <p>
          This database is basically a straight implementation of the
          alive record itself.  It creates a record of the name
          "$(P)alive".
        </p>
        <p>
          The name of the IOC sent back to the server is by default the
          value of the "IOC" environment variable, but can be directly
          specified by setting IOCNM to the name wanted.
        </p>
        <p>
          The <strong>RHOST</strong> is defined using "$(RHOST)".  The
          rest of the fields that can be specified have defaults in
          them that can be overridden.  The default for "$(RPORT)" is
          "5678", and for "$(HPRD)" is "15".  If multiple IOC are to
          run on the same computer, then "$(IPORT)" for each IOC
          should have a different value, or all "$(IPORT)" values
          should be set to zero to allow the system to determine it.
          Only the first nine of the default environment variable
          fields, "$(EVD1)" to "$(EVD9)", have values, being "ARCH",
          "TOP", "EPICS_BASE", "SUPPORT", "ENGINEER", "LOCATION",
          "GROUP", "STY", and "PROCSERV_INFO".
        </p>
      </dd>
      <dt>aliveMSGCalc.db</dt>
      <dd>
        <p>
          This database consists of a calcout record that will
          periodically set the <strong>MSG</strong> value of the IOC's
          alive record, based on status PV values of
          other <i>synApps</i> modules.  The <strong>MSG</strong>
          usage is undefined, but was intended for sending warnings if
          something bad happens, as a sequence of error
          bits. Currently, only the status of the autosave is being
          used.  If the first bit of the <strong>MSG</strong> is set,
          then autosave is dead.
        </p>
        <p>
          This is not included directly in <i>alive.db</i> since the
          alive module might be deployed without the
          other <i>synApps</i> modules.  It might also become locally
          customized.
        </p>
        <p>
          There is an associated <i>aliveMSGCalc_local.req</i> file.
          It does not use the normal <i>_settings.req</i> convention
          (which in recent synApps versions causes it to be
          automatically loaded), as it's intended for manually
          overriding the global defaults only if that is wanted.
          Otherwise, changing the calculation parameters would need to
          be done for every IOC instead of at the master file
          location.
        </p>
      </dd>
    </dl>
    

    <h2>MEDM display files</h2>


    <dl>
      <dt>aliveRecord.adl</dt>
      <dd>
        <p>
          This is the medm ADL file for an alive record.  All the
          fields, except for the environment variables to be sent, are
          exposed on this screen.
        </p>
      </dd>
      <dt>aliveRecordEnvVars.adl</dt>
      <dd>
        <p>
          This is the medm ADL file for the environment variables to
          be sent by the alive record.  The top values are the
          unchangeable defaults, while the bottom values can be
          changed at any time.
        </p>
      </dd>
    </dl>

    <h2>How to build and use</h2>

    <ul>
      <li>
        Edit <code>configure/RELEASE</code> to specify the paths
        to <strong>EPICS base</strong>.  There are no dependencies on
        any other modules.
      </li>
      <li>
        Run Gnu Make to build.
      </li>
      <li>
        Note that the <em>alive</em> module is not useful on its own,
        as it needs a remote server to send heartbeat information.
      </li>
    </ul>

    <h2>Checking to see if record is talking to a remote server</h2>

    <ul>
      <li>
        Configure an alive record to talk to a remote server daemon
        that understands how to talk to this type of record.
      </li>
      <li>
        Make sure that RADDR has the correct IPv4 address in it, as
        converted from RHOST, and that RPORT is correct.  RHOST and
        RPORT can't be changed on a running IOC, but AHOST and APORT
        allow this and can be used for testing, with the corresponding
        address in AADR.
      </li>
      <li>
        Make sure that IPSTS is set by the record to "Operable",
        otherwise the information port is not working.
      </li>
      <li>
        Make sure that the HRTBT field is set to "On" (the default).
      </li>
      <li>
        Make sure that the ISUP field is set to "Off" (the default).
      </li>
      <li>
        Check the RRSTS field to check that the read status of the
        remote server (or ARSTS for an auxiliary server) is "Idle".
        If so, then set ITRIG field to "Trigger"; otherwise the record
        is already in the process of requesting a remote read.
      </li>
      <li>
        If RRSTS (or ARSTS) field eventually changes to "Overdue",
        then the remote server is either not receiving the heartbeat
        correctly, or is not reading or able to read the TCP port at
        value IPORT; if it eventually changes to "Idle", then the
        connection with remote server is working correctly.
      </li>
    </ul>

  </body>
</html>
