---
title: Common UNIX Printing System 1.1.21rc2
layout: post
permalink: /blog/:year-:month-:day-:title.html
---

<P>The second release candidate for version 1.1.21 of the Common
- The scheduler used a select() timeout of INT_MAX
- Updated the cupsaddsmb program to use the new Windows
- The gziptoany filter did not produce copies for raw
- The cupsLangGet() function now uses nl_langinfo(),
- Added a ReloadTimeout directive to control how long
- Added a note to the default cupsd.conf file which
- The IPP backend incorrectly used the local port when
- The cups-lpd mini-daemon wasn't using the right
- Updated the new httpDecode64_2() and httpEncode64_2()
- String options with quotes in their values were not
- Configure script changes for GNU/Hurd (Issue #838)
- The lppasswd program was not installed properly by GNU
- Updated the cups-lpd man page (Issue #843)
- Fixed a typo in the cupsd man page (Issue #833)
- The USB backend now defaults to using the newer
- The config.h file did not define the HAVE_USERSEC_H
- The lp and lpr commands now report the temporary
- Added ServerTokens directive to control the Server
- Added new httpDecode64_2(), httpEncode64_2(), and
- The cupsGetFile() and cupsPutFile() code did not
- The httpSeparate() function did not decode all
- The cupstestppd program now checks for invalid Duplex
- Updated the printer name error message to indicate
- The scheduler didn't handle HTTP GET form data
- The pstops filter now makes sure that the prolog code
- The pstops filter now handles print files that
- Miscellaneous build fixes for NetBSD (Issue #788)
- Added support for quoted system group names (Issue #784)
- Added "version" option to IPP backend to workaround
- Added Spanish translation of web interface (Issue #772,
- The LPD backend now uses geteuid() instead of getuid()
- The IPP backend did not report the printer state if
- The printer state was not updated for "STATE: foo,bar"
- Added new CUPS API convenience functions which accept
- The scheduler did not correctly throttle the browse
- The scheduler did not pass the correct CUPS_ENCRYPTION
- The lpq command showed 11st, 12nd, and 13rd instead of
- "make install" didn't work on some platforms due to an
- Changed some calls to snprintf() in the scheduler to