.. _changelog:

#########
Changelog
#########

*******************
v0.3.2 (unreleased)
*******************

*******************
v0.3.1 (2024-05-28)
*******************

Bug fixes
=========
- Return a non-zero exit code on errors in internal c++ implementation (:issue:`28`, :merge:`26`). By `Kyle Brindley`_.

Internal Changes
================
- Limit Conda package files (:issue:`26`, :merge:`21`). By `Kyle Brindley`_.
- Force recompile during system tests (:issue:`25`, :merge:`22`). By `Kyle Brindley`_.
- Match c++ CLI usage to implementation (:issue:`24`, :merge:`23`). By `Kyle Brindley`_.
- Track Abaqus hotfix version (:merge:`24`). By `Kyle Brindley`_.
- Exit with an error on bad c++ CLI options. Should not occur during production use because c++ CLI options are
  constructed internally by Python wrapper (:issue:`28`, :merge:`26`). By `Kyle Brindley`_.
- Common c++ exception handling and exit messages (:issue:`29`, :merge:`27`). By `Kyle Brindley`_.
- Build, test, and package against multiple versions of hdf5 (:issue:`31`, :merge:`28`). By `Kyle Brindley`_.
- Add unit tests for utility functions (:issue:`33`, :merge:`29`). By `Kyle Brindley`_.

Enhancements
============
- Apply file overwrite option to both extracted H5 file and log file (:issue:`9`, :merge:`25`). By `Kyle Brindley`_.
- Stream STDOUT and STDERR instead of printing at the end of execution (:issue:`28`, :merge:`26`). By `Kyle Brindley`_.

*******************
v0.3.0 (2024-05-24)
*******************

Breaking changes
================
- Accept arbitrary abaqus command paths instead of a version with hardcoded prefix path assumptions (:issue:`8`,
  :merge:`19`). By `Kyle Brindley`_.

*******************
v0.2.1 (2024-05-24)
*******************

New Features
============
- Add experimental, untested scons extensions module. Unit tests pending :issue:`23`. System tests pending :issue:`22`
  (:issue:`16`, :merge:`16`). By `Kyle Brindley`_.

Documentation
=============
- Add experimental, external API documentation (:issue:`16`, :merge:`16`). By `Kyle Brindley`_.

Internal Changes
================
- Add SCons extensions unit tests (:issue:`22`, :merge:`18`). By `Kyle Brindley`_.

*******************
v0.2.0 (2024-05-24)
*******************

Breaking changes
================
- Separate subcommand to drive data extraction (:issue:`17`, :merge:`15`). By `Kyle Brindley`_.

New Features
============
- Bundle HTML and man page documentation with package (:issue:`14`, :merge:`10`). By `Kyle Brindley`_.

Bug fixes
=========
- Fix entry point for CLI (:issue:`14`, :merge:`10`). By `Kyle Brindley`_.

Internal Changes
================
- Add pytest packages to CI environment. By `Kyle Brindley`_.
- Add pytests for unit and system tests. Drive test suite exclusively with pytest. Add a real data extract system test
  (:issue:`17`, :merge:`15`). By `Kyle Brindley`_.

*******************
v0.1.0 (2024-05-24)
*******************

Breaking changes
================

New Features
============

Bug fixes
=========

Documentation
=============
- Add documentation template (:issue:`6`, :merge:`7`). By `Kyle Brindley`_.
- Add web-hosted documentation to CI build (:issue:`6`, :merge:`7`). By `Kyle Brindley`_.

Internal Changes
================

Enhancements
============
