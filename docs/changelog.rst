.. _changelog:

#########
Changelog
#########

*******************
v0.3.3 (unreleased)
*******************

Documentation
=============
- Do not show Python internal types in CLI help/usage. By `Kyle Brindley`_.
- Convert to sphinx-book-theme (:issue:`50`, :merge:`45`). By `Kyle Brindley`_.

Internal Changes
================
- Re-enable threaded system tests (:issue:`19`, :merge:`36`). By `Kyle Brindley`_.
- Add more system tests by running simulations for Abaqus fetched input files. Make local system test directories
  persistent for easier debugging (:issue:`41`, :merge:`38`). By `Kyle Brindley`_.
- Always clean the system test working directories and force overwrite the extracted file to ensure up-to-date test
  results (:issue:`44`, :merge:`39`). By `Kyle Brindley`_.
- Use a common downstream pipeline to deploy to the internal conda channel (:issue:`733`, :merge:`904`). By `Kyle
  Brindley`_.

Enhancements
============
- Print minimal program flow when verbose is requested. Unify logging for verbose and debugging output (:issue:`39`,
  :merge:`35`). By `Kyle Brindley`_.
- Print more useful output when docs subcommand fails to open a web browser (:issue:`48`, :merge:`43`). By `Kyle
  Brindley`_.

*******************
v0.3.2 (2024-05-30)
*******************

Documentation
=============
- Add minimal user example that mirrors internal system test (:issue:`34`, :merge:`30`). By `Kyle Brindley`_.

Internal Changes
================
- Add unit tests for utility functions (:issue:`33`, :merge:`29`). By `Kyle Brindley`_.
- Separate spade source and build trees (:issue:`21`, :merge:`31`). By `Kyle Brindley`_.
- Store the SCons signatures file(s) with the associated build directory (:issue:`36`, :merge:`32`). By `Kyle
  Brindley`_.

Enhancements
============
- Always compile in a unique, user-writable directory. Has the side effect of always compiling, but this only takes ~20
  seconds and avoids race conditions during parallel execution (:issue:`37`, :merge:`33`). By `Kyle Brindley`_.

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
