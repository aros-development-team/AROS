
/****** bsdsocket.library/getdtablesize *************************************

    NAME
        getdtablesize - get socket descriptor table size

    SYNOPSIS

        nfds = getdtablesize()
        D0

        long getdtablesize(void);

    FUNCTION
        Return value of maximum  number of open socket  descriptors.
        Larger  socket  descriptor  table  can   be  allocated  with
        SocketBaseTagList() call.

    SEE ALSO
        SocketBaseTagList()

*****************************************************************************
*
*/

/****** bsdsocket.library/syslog ********************************************

    NAME
        syslog, vsyslog - write message to AmiTCP/IP log.

    SYNOPSIS
        #include <syslog.h>

        vsyslog(level, format, ap)
                D0     A0      A1

        void syslog(unsigned long level, char * format, ...); 

        void vsyslog(unsigned long level, char * format, ap); 

    FUNCTION
        Writes the message given as format string and arguments
        (printf-style) both to the log file and to the console.
        The message is prepended with the name of the calling
        application, if the name is known by AmiTCP (the standard
        autoinitiazer module in the net.lib passes the name of the
        application to AmiTCP).

        The level is selected from an ordered list:

            LOG_EMERG           A panic condition.

            LOG_ALERT           A condition that should be
                                corrected immediately, such as a 
                                corrupted system database.

            LOG_CRIT            Critical conditions, such  as  hard
                                device errors.

            LOG_ERR             Errors.

            LOG_WARNING         Warning messages.

            LOG_NOTICE          Conditions that are not error  con-
                                ditions,  but that may require spe-
                                cial handling.

            LOG_INFO            Informational messages.

            LOG_DEBUG           Messages that  contain  information
                                normally of use only when debugging
                                a program.

    INPUTS
        Level     - indicates the type of the message. The levels
                    are defined in sys/syslog.h and listed above.

        format    - This is a printf-style format string.

        arguments - as in printf().

        ap        - pointer to an array of arguments.

    RESULT
        Returns no value.

    EXAMPLES
        To log a message at priority LOG_INFO, it would make the
        following call to syslog:

            syslog(LOG_INFO,  "Connection from host %s",
                   CallingHost);

    NOTES
        In contrast to the previous releases of the AmiTCP/IP, the
        integer arguments are expected to be 32 bits wide, thus
        eliminating the need to specify the 'l' size modifier for the
        number formatters.
 
        This function is callable from interrupts.

    BUGS
        Because there is a limited number of internal messages used
        by the logging system, some log messages may get lost if a
        high priority task or interrupt handler sends many messages
        in succession. If this happens, the next log message tells
        the fact. 

    SEE ALSO
        net.lib/syslog for syslog utility functions (openlog(),
        closelog() and setlogmask()),
        C-library printf() documentation

*****************************************************************************
*
*/
