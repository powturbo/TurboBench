Changelog
=========

v0.1.20 - 2022-XX-XX
--------------------

- Project was contributed by SecureAuth to the OWASP CBAS Project in October 2022.
- `pysap/SAPCredv2.py`: Added subject fields instead of commonName for LPS-enabled credentials ([\#35](https://github.com/OWASP/pysap/issues/35)). Thanks [@rstenet](https://github.com/rstenet)!
- `pysap/SAPCredv2.py`: Add support for cipher format version 1 with 3DES ([\#35](https://github.com/OWASP/pysap/issues/35) and [\#37](https://github.com/OWASP/pysap/pull/37)). Thanks [@rstenet](https://github.com/rstenet)!
- `pysap/SAPHDB.py`: Added missing `StatementContextOption` values (see [\#22](https://github.com/SecureAuthCorp/SAP-Dissection-plug-in-for-Wireshark/issues/22)).


v0.1.19 - 2021-04-29
--------------------

- Using Scapy version 2.4.4.
- `pysap/SAPSSFS.py`: New module for SAP Secure Store in File System file format.
- `bin/pysaphdbuserstore`: New script for interacting with `hdbuserstore` SSFS files.
- `requirements-examples.txt`: Renamed to match `setup.py`'s extra. 
- `pysap/SAPHDB.py`: Implementation of GSS-based auth method with Kerberos 5.
- `pysap/SAPHDB.py`: Handling of Session Cookie values when found in the `CONNECT` response.
- `pysap/SAPRouter.py`: Add support to route string in `SAPRouterNativeProxy` ([\#33](https://github.com/OWASP/pysap/pull/33)). Thanks [@gloomicious](https://github.com/gloomicious)!
- `examples/router_fingerprints.json`: New fingerprints for SAP Router version 7450.34.25.5091. Thanks [@jvis](https://twitter.com/jvis)!
- `examples/router_portfw.py`: Add support to route string. ([\#33](https://github.com/OWASP/pysap/pull/33)) Thanks [@gloomicious](https://github.com/gloomicious)!
- `tests/sapssfs_test.py`: Basic unit tests for the SSFS file format.


v0.1.18 - 2020-07-15
--------------------

- Using Sphinx 1.8.5 for documentation.
- Replaced AppVeyor and Travis builds with GitHub Actions.
- Added `cryptography` as required library instead of optional requirement.
- Replaced the use of deprecated `optparse` module for `argparse` across all tools and examples.
- Fixed some PEP8 warnings across modules and example scripts.
- `pysap/SAPCAR.py`: Added signature manifest file type.
- `pysap/SAPHDB.py`: New module for SAP HANA SQL Command Network protocol packets, authentication methods and connection classes.
- `pysap/SAPNI.py`: Disconnect clients in SAPNIServer if socket errors are catched. 
- `pysap/utils/crypto`: Added implementation of SCRAM algorithms for use in HDB authentication.
- `examples/hdb_auth.py`: New example script to illustrate the use of the different authentication methods in HDB.
- `examples/hdb_discovery.py`: New example script to perform discovery of HANA database tenants. 
- `examples/diag_login_brute_force.py`: Handling valid users (e.g. no dialog users) vs valid passwords. Thanks fabhap!
- `examples/diag_login_brute_force.py`: Fixed discovery with right message match ("Client does not exist").
- `examples/default_sap_credentials`: Added a couple of default credentials from trial versions.
- `tests/crypto_test.py`: Testing output of SCRAM algorithms.
- `tests/saphdb_test.py`: Basic unit tests for the HDB protocol.
- `tests/sapni_test.py`: Arranged and fixed a couple of tests that were failing on macOS and Windows.

v0.1.17 - 2019-11-05
--------------------

- Using Scapy version 2.4.3.
- Added documentation of projects using `pysap`.
- Added documentation of all example scripts.
- `bin/pysapcar`: Add cli option to determine output directory when extracting archives with pysapcar ([\#24](https://github.com/OWASP/pysap/pull/24)) Thanks [@okuuva](https://github.com/okuuva)!
- `pysap/SAPDiag.py`: Added support bits for SAP GUI 7.50 and SAP NW 7.52 SP01.
- `pysap/SAPCAR.py`: Fixed crafting of archive files by defaulting length fields to zero ([\#22](https://github.com/OWASP/pysap/issues/22)) Thanks [@okuuva](https://github.com/okuuva)!
- `pysap/SAPMS.py`: Added the `DPInfo[1-3]` packets for handling specific Message Server `ADM` packets relaying Dispatcher/WP info: `[1-3]` because of tight SAP kernel version dependency. ([\#31](https://github.com/OWASP/pysap/pull/31)) Thanks Mathieu ([@gelim](https://github.com/gelim)) and Dmitry ([@_chipik](https://twitter.com/_chipik))!
- `pysap/SAPRFC.py`: Enhanced with mainly `SAPCPIC*` and `SAPRFXPG*` new packets. ([\#31](https://github.com/OWASP/pysap/pull/31)) Thanks Mathieu ([@gelim](https://github.com/gelim)) and Dmitry ([@_chipik](https://twitter.com/_chipik))!
- `pysap/SAPRouter.py`: Fixed padding on SAPRouter client info packets.
- `examples/rfc_monitor.py`: Renamed the script to `gw_monitor.py`.
- `examples/router_scanner.py`: Add capability to provide a comma separated list of targets/ports to scan.
- `examples/list_sap_parameters`: Updated recommended values and added new parameters.
- `examples/ms_dump_info.py`: Added `NOTEQUAL` check type.
- `examples/router_fingerprints.json`: Added fingerprints for SAP Router 7.49 and 7.45 kernels.


v0.1.16 - 2018-06-19
--------------------

- Using Scapy version 2.4.0.
- Using Sphinx 1.7.4 for documentation.
- Added missing `pysap.utils` package in source/binary packages.
- Use flake8 to find syntax errors and undefined names in Travis ([\#20](https://github.com/OWASP/pysap/pull/20)) Thanks [@cclauss](https://github.com/cclauss)!
- `pysap/SAPCAR.py`: Improved SAPCAR files parsing, adding proper structure names based on VSI documentation.


v0.1.15 - 2018-03-27
--------------------

- Version released at Troopers'18
- Added initial support for handling IGS (Internet Graphic Server) packets along with example scripts to play with them ([\#19](https://github.com/OWASP/pysap/pull/19)) Thanks [@iggy38](https://github.com/iggy38)!
- Added initial support for handling PSE and SSO Credential format files.
- `bin/pysapgenpse`: New binary tool for working with PSE and SSO credential files.
- `bin/pysapcar`: Added options for creating a new archive and appending files to an existing one.
- `pysap/SAPCredv2.py`: New module for SSO Credential files definitions and decryption logic.
- `pysap/SAPDiagItems.py`, `extra/parsesupportbits.py`: Corrected order of support data bit fields ([\#18](https://github.com/OWASP/pysap/pull/18)). Thanks [@hnzlmnn](https://github.com/hnzlmnn)!
- `pysap/SAPIGS.py`: New module for IGS packets layer ([\#19](https://github.com/OWASP/pysap/pull/19)).
- `pysap/SAPLPS.py`: New module for LPS definitions and INT/DP API decryption logic.
- `pysap/SAPMS.py`: Added Message Server Domain field, MS J2EE Cluster/Header/Service packets. Thanks Albert Zedlitz!
- `pysap/SAPPSE.py`: New module for PSE files definitions and decryption logic.
- `pysap/utils.py`: Moved utils classes into a package.
- `examples/diag_login_brute_force.py`: Detect invalid clients while logins ([\#17](https://github.com/OWASP/pysap/pull/17)). Thanks [@hnzlmnn](https://github.com/hnzlmnn)!
- `examples/dlmanager_decrypt.py`: Replaced use of PyCrypto with Cryptography library.
- `examples/igs_*.py`: New example scripts to interact with IGS services ([\#19](https://github.com/OWASP/pysap/pull/19)).
- `examples/rfc_monitor.py`: Added `noop` command in the monitor and version command line option.
- `examples/ms_*.py`: Added command line option to specify Message Server Domain.
- `extra/pse2john.py`: New extra script to extract crypto material in John the Ripper format.


v0.1.14 - 2017-10-04
--------------------

- Added initial support for handling SNC frames and some of their fields.
- `pysap/SAPNC.py`: Added fields for SNCFrames and helper wrapper/unwrapper functions.
- `pysap/SAPRouter.py`: Fixed route request for more than one SAP Router when using the native proxy.
- `pysap/SAPRouter.py`: Allow route strings to use lowercase separator chars (e.g. "/h/host/s/port").
- `pysap/SAPRouter.py`: Allow a `SAPRoutedStreamSocket` to bypass the NI layer if no route was specified but talk mode
  was set to raw ([\#10](https://github.com/OWASP/pysap/pull/10)).
- `pysap/SAPRouter.py`: Enhanced the version retrieve routine by not failing when an error is returned by the server
  ([\#11](https://github.com/OWASP/pysap/issues/11)). Thanks [@gelim](https://github.com/gelim)!
- `pysap/SAPRouter.py`: Fixed missing eyecatcher in control messages ([\#10](https://github.com/OWASP/pysap/pull/10)).
  Thanks [@gelim](https://github.com/gelim)!.
- `examples/diag_login_screen_info.py`: Script was improved by better printing technical information and
  outputting login screen text items (language, input fields, login text) ([\#14](https://github.com/OWASP/pysap/pull/14)).
  Thanks [@gelim](https://github.com/gelim)!.
- `examples/diag_login_brute_force.py`: Script was improved to handle currently logged in users
  ([\#16](https://github.com/OWASP/pysap/pull/16)). Thanks [@hnzlmnn](https://github.com/hnzlmnn)!
- `example/router/admin.py`: Improved client list table display and fixed timestamps ([\#12](https://github.com/OWASP/pysap/issues/12)).
  Thanks [@gelim](https://github.com/gelim)!.
- `examples/ms_dump_param.py`: New example script to list Message Server parameters and check them against a list of
  expected values ([\#15](https://github.com/OWASP/pysap/pull/15)). Thanks [@iggy38](https://github.com/iggy38)!
- `examples/ms_dos_exploit.py`: New example script to check for [CVE-2017-5997](https://erpscan.com/advisories/erpscan-16-038-sap-message-server-http-remote-dos/)
  DoS vulnerability on Message Server and fixed at [SAP Note 2358972](https://launchpad.support.sap.com/#/notes/2358972)
  ([\#10](https://github.com/OWASP/pysap/pull/10)). Thanks [@vah13](https://github.com/vah13) and [@gelim](https://github.com/gelim)!


v0.1.13 - 2017-02-16
--------------------

- Documentation now includes graphical representation of the main packets of each protocol.
- Example scripts now accept route strings without requiring remote host option.
- `pysap/SAPRouter.py`: Fixed route request for more than one SAP Router.
- `pysap/SAPEnqueue.py`: `SAPEnqueueStreamSocket` now can connect to an Enqueue server through a SAPRouter.
- `examples/router_niping.py`: New example script that implements a very basic
  version of the `niping` tool. It works on client or server mode.
- `examples/enqueue_monitor.py`: The script now accepts route strings for connecting through a SAPRouter.


v0.1.12.1 - 2016-12-19
----------------------

- Minor release.
- Source build didn't included header files and thus builds from pip source were failing.


v0.1.12 - 2016-12-16
--------------------

- Using Scapy version 2.3.3.
- Minor fixes and code arrangements.
- Building and testing in OSX with Travis and Windows with Appveyor.
- `pysap/SAPDiagItems.py`: Added default support bits from SAP GUI 7.40 version.
- `examples/diag_capturer.py`: Added option to display available capture interfaces.
- `examples/enqueue_dos_exploit.py`: New example script to check for [CVE-2016-4015](https://erpscan.com/advisories/erpscan-16-019-sap-netweaver-enqueue-server-dos-vulnerability/)
  DoS vulnerability on Standalone Enqueue Server and fixed at [SAP Note 2258784](https://launchpad.support.sap.com/#/notes/0002258784)
  ([\#6](https://github.com/OWASP/pysap/pull/6)). Thanks [@vah13](https://github.com/vah13)!
- `examples/router_fingerprints.json`: Added fingerprints from SAP Router releases 745.


v0.1.11 - 2016-10-12
--------------------

- Some documentation improvements. Hosting the documentation in [Read the Docs](https://pysap.readthedocs.io/en/latest/).
- ChangeLog file in Markdown format for better documentation.
- Improvements over the `pysapcar` tool, general fixes and added handling of multi-block SAP CAR archive files. Thanks
  [Hans-Christian Esperer](https://github.com/hce) for the feedback!
- `pysap/SAPDiag.py`: Splitted `SBA`/`SFE`/`SLC` fields.
- `pysap/SAPDiag.py`: Renamed error flag to error number.
- `examples/diag_capturer.py`: Fix imports for running diag capturer example script in Windows.
- `examples/diag_login_brute_force.py`: Added SOLMAN [default](https://www.troopers.de/media/filer_public/37/34/3734ebb3-989c-4750-9d48-ea478674991a/an_easy_way_into_your_sap_systems_v30.pdf)
  [credentials](https://launchpad.support.sap.com/#/notes/2293011) and improved reading credentials file.


v0.1.10 - 2016-03-25
--------------------

- Version released at Troopers'16
- Added support for handling SAP SAR file formats.
- `pysap/SAPCAR.py`: New module for handling SAP SAR file formats.
- `extra/dlmanager_decrypt.py`: Example PoC for [decrypting](https://www.coresecurity.com/advisories/sap-download-manager-password-weak-encryption)
  [SAP Download Manager](https://support.sap.com/software/download-manager.html) stored passwords.
- `examples/dlmanager_infector.py`: Example script to open a SAP SAR archive file and infect it by adding files with
  arbitrary filenames (e.g. including absolute or relative paths). It can be also used as a
  [mitmproxy](https://mitmproxy.org/) script for on-the-fly infecting SAR files being downloaded.


v0.1.9 - 2016-02-08
-------------------

- Using Scapy version 2.3.2.
- Replaced epydoc with Sphinx for documentation.
- Minor enhancement and code arrangements.
- `pysap/SAPDiagItems.py`: Added new support bits from SAP GUI 7.30p9 and 7.40 versions.
- `pysap/utils.py`: Removed custom fields now available on Scapy.
- `examples/router_fingerprints.json`: Added some fingerprints from SAP Router releases 720.32 and 742.


v0.1.8 - 2015-10-29
-------------------

- Enabled travis containers for more quick builds.
- Minor documentation and README improvements.
- `pysap/SAPRouter.py`: Documented some version numbers from old releases ([\#3](https://github.com/OWASP/pysap/pull/3)). Thanks [invisiblethreat](https://github.com/invisiblethreat)!
- `pysapcompress/vpa108csulzh.cpp`: Improved the fix for CVE-2015-2278 by properly initializing arrays ([\#4](https://github.com/OWASP/pysap/pull/4)). Thanks [ret5ret](https://github.com/ret5ret)!
- `examples/diag_render_login_screen.py`: Fail gracefully if `wx` is not found.
- `examples/router_password_check.py`: Updating the `fau_timer` library in use and failing gracefully if it's not found.
- `examples/router_fingerprint.py`: New example script for performing fingerprint over SAP Router versions.
  This is experimental and the database included only contains a few SAP Router versions, but it might work.


v0.1.7 - 2015-05-13
-------------------

- Fixed vulnerabilities in `LZC` and `LZH` compression libraries ([CVE-2015-2282 and CVE-2015-2278](https://www.coresecurity.com/advisories/sap-lzc-lzh-compression-multiple-vulnerabilities)).
  Added test cases for checking proper fixes.
- `pysap/SAPRouter.py`: Moved SAP Router native proxy implementation to the SAP Router module so it can be reused.
- `examples/router_portfw.py`: Using the native proxy implementation in SAP Router module.


v0.1.6 - 2015-03-25
-------------------

- Requirements now handled by setuptools.
- Test building with clang on travis.
- Cleared installation docs.
- `pysap/SAPNI.py`: Made clients an instance variable in `SAPNIServer`.
- `pysap/SAPRouter.py`: Added unknown field.
- `examples/diag_dos_exploit.py`: Small fix.
- `examples/router_admin.py`: Small fix on response handling.
- `examples/router_portfw.py`: Added support for specify talk mode when requesting routes.
- `examples/router_scanner.py`: Added support for specify talk mode when requesting routes.


v0.1.5 - 2015-01-16
-------------------

- Updated to use scapy v2.3.1.
- Code is more Python3-friendly.
- Added travis script for running tests.
- General minor fixes and code improvements.
- Added test suites for SAPNI, SAPDiag and SAPRouter modules.
- Added support for routing via SAP Router in almost all example scripts.
- `pysap/SAPDiag.py`: Added support for message info and Diag error packets.
- `pysap/SAPDiag.py`: Diag item lookup now support looking up multiple items, and string lookups.
- `pysap/SAPDiagClient.py`: Added support for specifying support bits when connecting, support for routing via SAP Router.
- `pysap/SAPDiagClient.py`: If no terminal is supplied, use a random looking IP by default to avoid identification
  ([SAP Note 1497445](https://launchpad.support.sap.com/#/notes/1497445)).
- `pysap/SAPDiagItems.py`: Fixes on some atom items for old versions.
- `pysap/SAPEnqueue.py`: Added trace max file size field.
- `pysap/SAPNI.py`: Added helpers for creating new connections.
- `pysap/SAPNI.py`: `SAPNIProxy` implemented using a Worker thread.
- `pysap/SAPNI.py`: `SAPNIServer` implemented using `SAPNIStreamSocket`.
- `pysap/SAPRouter.py`: Added route hop conversion helpers.
- `pysap/SAPRouter.py`: Added info client and info server packets.
- `pysap/SAPRouter.py`: Added `SAPRoutedStreamSocket`.
- `pysap/utils.py`: Reimplemented `MutablePacketField` with evaluators.
- `pysapcompress/pysapcompress.cpp`: Improved routines and added handling of some error conditions.
- `examples/diag_capturer.py`: New example script for dumping Diag login credentials by sniffing or reading a pcap file.
- `examples/enqueue_monitor.py`: Added command for get replication info and command for checking trace pattern endless
  loop vulnerability ([CVE-2014-0995](https://www.coresecurity.com/advisories/sap-netweaver-enqueue-server-trace-pattern-denial-service-vulnerability)).
- `examples/router_admin.py`: Parsing of info request responses.
- `examples/router_password_check`: New example script for testing if a SAP Router is vulnerable to a timing attack on
  the password check ([CVE-2014-0984](https://www.coresecurity.com/advisories/sap-router-password-timing-attack)).
- `requirements-optional.txt`: Added optional requirements.


v0.1.4 - 2014-03-25
-------------------

- Version released at Troopers'14.
- Changelog now in GNU format.
- Changed setup from distutils to setuptools.
- Added some unit tests.
- Arranged most of the code according to PEP8.
- `pysap/SAPDiagItems.py`: Fixed some support bits and added new ones found in SAP GUI version 7.30.
- `pysap/SAPDiagItems.py`: Added new Diag Items: `WindowsSize`.
- `pysap/SAPEnqueue.py`: New packet classes. Crafting of Enqueue Server packets: Connection Admin and Server Admin.
- `pysap/SAPNI.py`: Fixed handling of `NI_PING` keep-alive requests.
- `pysap/SAPNI.py`: Added logging namespace `sapni` for all NI layer activity.
- `pysap/SAPMS.py`: New packet classes. Crafting of Message Server packets.
- `pysap/SAPRouter.py`: New packet classes. Crafting of SAP Router packets: Route, Admin, Control and Error Information.
- `pysap/SAPSNC.py`: New packet class. Container for SNC Frame packets.
- `pysapcompress/pysapcompress.cpp`: Splitted exception class in two: `CompressError` and `DecompressError`.
- `examples/ms_change_param.py`: Added example for retrieving or changing a parameter value using MS Admin `set_param`
  commands.
- `examples/ms_dump_info.py`: New example script for retrieving information using MS Admin dump commands.
- `examples/ms_impersonator.py`: New example script for impersonating an application server connected to a Message
  Server service instance.
- `examples/ms_listener.py`: New example script for connecting to a Message Server and listening for messages coming
  from the server.
- `examples/ms_messager.py`: New example script for sending a message to a connected client through the Message Server.
- `examples/ms_monitor.py`: New example script for monitoring the Message Server service (`msmon` tool on steroids).
- `examples/ms_observer.py`: New example script for connecting to a Message Server service and observe clients
  connecting to it (`msprot` tool).
- `examples/router_admin.py`: New example script for performing administrative tasks on a SAP Route. Includes
  undocumented commands.
- `examples/router_portfw.py`: New example script for routing native connections through SAP Router.
- `examples/router_scanner.py`: New example script for scanning internal hosts using SAP Router.


v0.1.3 - 2013-08-28
-------------------

- Added general documentation and setup.py command to build it using epydoc.
- `pysap/SAPNI.py`: Refactored the SAP Diag Proxy and Server modules to a base NI implementation.
- `pysapcompress/pysapcompress.cpp`: Added handling of error return codes.
- `examples/diag_interceptor.py`: Refactored to use the new `NIProxy` implementation. Fixed some hanging issues.
  Thanks Florian Grunow for the feedback!
- `examples/diag_login_brute_force.py`: Handling of license errors.


v0.1.2 - 2012-09-27
-------------------

- Version released at Brucon'12.
- `pysap/SAPNI.py`, `pysap/SAPDiag.py`: Network Interface packet class moved to a new module. Binding of the
  SAPNI/protocol layer is performed now by each script to allow the use of different protocols with SAPNI.
- `pysap/SAPNI.py`: Added a NI Stream Socket class for using it instead of the base Stream Socket.
- `pysap/SAPDiagItems.py`: Added new Diag Atom types, as used in NW 7.01 and early versions.
- `examples/diag_rogue_server.py`: Minor fixes.
- `examples/diag_render_login_screen.py`: Minor fixes.
- `examples/diag_login_brute_force.py`: Added multi-thread support.


v0.1.1 - 2012-07-29
-------------------

- Initial version released at Defcon 20.
