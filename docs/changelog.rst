.. _changelog:

#########
Changelog
#########

********************
v0.4.10 (unreleased)
********************

*******************
v0.4.9 (2025-12-16)
*******************

Bug fixes
=========
- Fix the artifacts paths for internal conda channel deployment. By `Kyle Brindley`_.

*******************
v0.4.8 (2025-12-16)
*******************

Bug fixes
=========
- Preliminary Windows support and add Windows CI tests (:merge:`144`). By `Kyle Brindley`_.

Internal Changes
================
- Parametrize the system tests against command-line provided abaqus commands instead of repeat calls to pytest. Cuts a
  few minutes off of the full test suite execution time (:issue:`138`, :merge:`140`). By `Kyle Brindley`_.
- Test against Abaqus 2025 (:issue:`137`, :merge:`141`). By `Kyle Brindley`_.
- Limit Abaqus system test parameterization to tests that require Abaqus (:merge:`142`). By `Kyle Brindley`_.
- Convert to a ``noarch`` conda package with OS-specific dependencies (:merge:`145`). By `Kyle Brindley`_.

Enhancements
============
- Update default Abaqus version when no abaqus command is provided from 2024 to 2025 (:issue:`137`, :merge:`141`). By
  `Kyle Brindley`_.

*******************
v0.4.7 (2025-12-08)
*******************

Internal Changes
================
- Updated build and environment files to use python greater than or equal to 3.10 as 3.9 is end of life
  (:issue:`131`, :merge:`136`).  By `Prabhu Khalsa`_.
- Clean up conda-recipe for minimum build and runtime requirements (:merge:`138`). By `Kyle Brindley`_.
- Build the CI environments on the fly for each CI job (:issue:`123`, :merge:`139`). By `Kyle Brindley`_.

*******************
v0.4.6 (2025-09-29)
*******************

Internal Changes
================
- Revert build settings from github settings accidentally introduced (:issue:`132`, :merge:`134`). By `Prabhu Khalsa`_.

*******************
v0.4.5 (2025-09-23)
*******************

Internal Changes
================
- Update package to use numpy version 1 or version 2 (:issue:`129`, :merge:`132`). By `Prabhu Khalsa`_.

*******************
v0.4.4 (2025-09-22)
*******************

Bug fixes
=========
- Fix incorrect dataset dimensions and labels (:issue:`120`, :merge:`124`). By `Prabhu Khalsa`_.
- Replacing incrementNumber, which can repeat, with actual frame number (:issue:`126`, :merge:`127`). By `Prabhu
  Khalsa`_.
- Fix writing of history output data (:issue:`127`, :merge:`128`). By `Prabhu Khalsa`_.
- Fix naming of faces including handling cases with unknown names (:issue:`128`, :merge:`130`). By `Prabhu Khalsa`_.

Documentation
=============
- Add GitHub shields and GitHub-Pages URL to README and HTML documentation (:issue:`116`, :merge:`123`). By `Kyle
  Brindley`_.

Internal Changes
================
- Update package to use numpy greater than or equal to version 2 (:issue:`125`, :merge:`126`). By `Prabhu Khalsa`_.

*******************
v0.4.3 (2025-06-30)
*******************

Internal Changes
================
- Update environment for automated Gitlab releases and add missing logic for conda-build package job identity in
  downstream deployment pipelines. By `Kyle Brindley`_.

*******************
v0.4.2 (2025-06-30)
*******************

Bug fixes
=========
- Add field output name if component labels dimension is empty (:issue:`118`, :merge:`118`). By `Prabhu Khalsa`_.

Documentation
=============
- Grid style landing page with quick links to most used portions of documenttaion. By `Kyle Brindley`_.
- Adding zenodo information into documentation (:issue:`112`, :merge:`117`). By `Prabhu Khalsa`_.

Internal Changes
================
- Updating recipe files per suggestions from conda forge linter (:issue:`112`, :merge:`117`). By `Prabhu Khalsa`_.
- Enforce more complete flake8 checks in CI jobs. By `Kyle Brindley`_.
- Release using Gitlab-CI tag pipelines. By `Kyle Brindley`_.
- Draft GitHub-Actions workflows. By `Kyle Brindley`_.
- Use a tag release pipeline compatible internal deployment job (:issue:`122`, :merge:`122`). By `Kyle Brindley`_.

*******************
v0.4.1 (2025-06-25)
*******************

Bug fixes
=========
- Fixed writing of Mises data to match correct dimensions (:issue:`101`, :merge:`99`). By `Prabhu Khalsa`_.
- Fixed typo in for loop iterator (:issue:`98`, :merge:`98`). By `Prabhu Khalsa`_.
- Process elements differently for efficiency. Fix the code that writes variable length datasets into the hdf5 file
  (:issue:`102`, :merge:`100`). By `Prabhu Khalsa`_.

Enhancements
============
- Write initial outline for VTKHDF format to eventually opening natively in Paraview (:issue:`97`, :merge:`97`).
  By `Prabhu Khalsa`_.

Documentation
=============
- Add default action expansion to SCons builder docstrings. By `Kyle Brindley`_.

Internal Changes
================
- Allow for multiple command line entries for instance, step, history-region, history, and field output (:issue:`100`,
  :merge:`112`). By `Prabhu Khalsa`_.
- Allow for negative indexing of frame numbers and multiple frame numbers on the command line (:issue:`99`,
  :merge:`111`). By `Prabhu Khalsa`_.
- Create top level parts, instances, and assemblies groups in hdf5 file (:issue:`103`, :merge:`101`).
  By `Prabhu Khalsa`_.
- If element belongs to instance don't write list of instances under mesh (:issue:`106`, :merge:`104`).
  By `Prabhu Khalsa`_.
- Write node_sets and element_sets as their own groups instead of as a "dimension" of mesh (:issue:`105`,
  :merge:`105`).  By `Prabhu Khalsa`_.
- Write xarray datasets for mesh, history outputs, and field outputx (:issue:`107`, :merge:`108`).  By `Prabhu Khalsa`_.
- Decide that this project _must_ be upstream of WAVES, remove WAVES developer/runtime dependency TODO notes and replace
  with notes to never use WAVES as a depedency. By `Kyle Brindley`_.

*******************
v0.4.0 (2025-04-30)
*******************

Enhancements
============
- Add command line option to specify odb format (:issue:`67`, :merge:`64`). By `Prabhu Khalsa`_.
- Write odb and jobData as hdf5 attributes instead of datasets (:issue:`76`, :merge:`74`). By `Prabhu Khalsa`_.
- Create mesh group in hdf5 file with data in extract format (:issue:`69`, :merge:`77`). By `Prabhu Khalsa`_.
- Create HistoryOutputs group in hdf5 file with data in extract format (:issue:`84`, :merge:`83`). By `Prabhu Khalsa`_.
- Create FieldOutputs group in hdf5 file with data in extract format (:issue:`89`, :merge:`88`). By `Prabhu Khalsa`_.
- Change odb-format command line option to format option (:issue:`90`, :merge:`89`). By `Prabhu Khalsa`_.
- Re-factor code to write history and field output at the same time as it's being processed
  (:issue:`91`, :merge:`90`). By `Prabhu Khalsa`_.
- Re-factor code to write field output data in a more efficient manner (:issue:`92`, :merge:`92`). By `Prabhu Khalsa`_.

Internal Changes
================
- Create top level instance groups in hdf5 file (:issue:`68`, :merge:`65`). By `Prabhu Khalsa`_.
- Refactor how nodes are stored for new extract format (:issue:`70`, :merge:`67`). By `Prabhu Khalsa`_.
- Stop passing around log_file and command_line_options in favor of class member (:issue:`71`, :merge:`68`).
  By `Prabhu Khalsa`_.
- Rename variables named 'set' since it's a reserved word (:issue:`72`, :merge:`69`). By `Prabhu Khalsa`_.
- Remove duplicate code in command line options class (:issue:`73`, :merge:`70`). By `Prabhu Khalsa`_.
- Wrapping write and create attribute in a try/catch statement (:issue:`77`, :merge:`75`). By `Prabhu Khalsa`_.
- Refactor how elements are stored for new extract format (:issue:`75`, :merge:`73`). By `Prabhu Khalsa`_.
- Write section point data under mesh group in extract format (:issue:`81`, :merge:`80`). By `Prabhu Khalsa`_.
- Adjust some write functions to detect empty data before creating an empty hdf5 group (:issue:`82`,
  :merge:`81`). By `Prabhu Khalsa`_.
- Using section assignment name for name of hdf5 group (:issue:`85`, :merge:`84`). By `Prabhu Khalsa`_.
- Changing how field output is stored and written (:issue:`86`, :merge:`85`). By `Prabhu Khalsa`_.
- Add checks to prevent writing of empty groups (:issue:`88`, :merge:`86`). By `Prabhu Khalsa`_.
- Re-factor loops through maps to use named tuples instead of iterators (:issue:`87`, :merge:`87`). By `Prabhu Khalsa`_.
- Check for command line input before writing history/field output data by instance (:issue:`93`, :merge:`91`).
  By `Prabhu Khalsa`_.
- Remove function that writes xarray attributes (:issue:`94`, :merge:`93`). By `Prabhu Khalsa`_.
- Store and clear memory differently for variable length datasets (:issue:`95`, :merge:`94`). By `Prabhu Khalsa`_.
- Added some verbose and debug log messages (:issue:`96`, :merge:`95`). By `Prabhu Khalsa`_.

*******************
v0.3.7 (2025-02-06)
*******************

Bug fixes
=========
- Replace forward slash with vertical bar in history output names (:issue:`65`, :merge:`61`). By `Prabhu Khalsa`_.

Internal Changes
================
- Create replace_slashes function and call it where ever it seems possible for a slash to be in a group name
  (:issue:`66`, :merge:`62`). By `Prabhu Khalsa`_.

*******************
v0.3.6 (2025-02-05)
*******************

Bug fixes
=========
- Wrapped all create functions used for h5 file with try/catch (:issue:`62`, :merge:`57`). By `Prabhu Khalsa`_.

Enhancements
============
- Change soft links for node data to hard links (:issue:`64`, :merge:`59`). By `Prabhu Khalsa`_.

Internal Changes
================
- Pass log file to all internal write functions (:issue:`63`, :merge:`58`). By `Prabhu Khalsa`_.

*******************
v0.3.5 (2025-01-31)
*******************

Bug fixes
=========
- Check to see if string dataset already exists in write_element before attempting to write to it (:issue:`60`,
  :merge:`54`). By `Prabhu Khalsa`_.

Enhancements
============
- Use hard links instead of string dataset for elements (:issue:`61`, :merge:`55`). By `Prabhu Khalsa`_.

*******************
v0.3.4 (2024-12-10)
*******************

Enhancements
============
- Provide Abaqus 2024 support (:issue:`57`, :merge:`52`). By `Kyle Brindley`_.

Internal Changes
================
- Removed Proxy Settings (:issue:`53`, :merge:`47`). By `Sergio Cordova`_.
- Update CI environment glibc (sysroot) version for Abaqus 2024 requirements (:issue:`59`, :merge:`51`). By `Kyle
  Brindley`_.
- Update minimum glibc/sysroot version runtime requirement for Abaqus 2024. Add Abaqus 2024 to the system test matrix
  (:issue:`57`, :merge:`52`). By `Kyle Brindley`_.

*******************
v0.3.3 (2024-12-06)
*******************

Enhancements
============
- Print minimal program flow when verbose is requested. Unify logging for verbose and debugging output (:issue:`39`,
  :merge:`35`). By `Kyle Brindley`_.
- Print more useful output when docs subcommand fails to open a web browser (:issue:`48`, :merge:`43`). By `Kyle
  Brindley`_.

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
- Execute spade from the calling working directory as the most likely location for execute permission (:issue:`51`,
  :merge:`46`). By `Kyle Brindley`_.
- Run system tests from a local working directory to avoid failing on no-execute permissions on /tmp (:issue:`51`,
  :merge:`46`). By `Kyle Brindley`_.
- Pass through Abaqus commands in system tests and test against a matrix of Abaqus versions (:issue:`54`, :merge:`48`).
  By `Kyle Brindley`_.
- Check style guide against both flake8 and black autoformatter (:issue:`52`, :merge:`49`). By `Kyle Brindley`_.

*******************
v0.3.2 (2024-05-30)
*******************

Enhancements
============
- Always compile in a unique, user-writable directory. Has the side effect of always compiling, but this only takes ~20
  seconds and avoids race conditions during parallel execution (:issue:`37`, :merge:`33`). By `Kyle Brindley`_.

Documentation
=============
- Add minimal user example that mirrors internal system test (:issue:`34`, :merge:`30`). By `Kyle Brindley`_.

Internal Changes
================
- Add unit tests for utility functions (:issue:`33`, :merge:`29`). By `Kyle Brindley`_.
- Separate spade source and build trees (:issue:`21`, :merge:`31`). By `Kyle Brindley`_.
- Store the SCons signatures file(s) with the associated build directory (:issue:`36`, :merge:`32`). By `Kyle
  Brindley`_.

*******************
v0.3.1 (2024-05-28)
*******************

Bug fixes
=========
- Return a non-zero exit code on errors in internal c++ implementation (:issue:`28`, :merge:`26`). By `Kyle Brindley`_.

Enhancements
============
- Apply file overwrite option to both extracted H5 file and log file (:issue:`9`, :merge:`25`). By `Kyle Brindley`_.
- Stream STDOUT and STDERR instead of printing at the end of execution (:issue:`28`, :merge:`26`). By `Kyle Brindley`_.

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

Enhancements
============

Documentation
=============
- Add documentation template (:issue:`6`, :merge:`7`). By `Kyle Brindley`_.
- Add web-hosted documentation to CI build (:issue:`6`, :merge:`7`). By `Kyle Brindley`_.

Internal Changes
================
