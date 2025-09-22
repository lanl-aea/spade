.. _releasephilosophy:

##################
Release Philosophy
##################

This section discusses topics related to |PROJECT| releases and version numbering.

***************
Version Numbers
***************

The |PROJECT| project follows the `PEP-440`_ standard for version numbering, as
implemented by `setuptools_scm`_. The production release version number uses the
three component ("major.minor.micro") scheme. The production version numbers
correspond to Git tags in the project `repository`_ which point to a static
release of the |PROJECT| project.

Because the deployed release of the developer version is constantly updated
against development work, the version number found in the developer version
contains additional information. During deployment, the developer version number
is appended with the git information from the most recent build. This
information contains the most recent git tag ("major.minor.micro.dev") followed
by the number of commits since the last production release and a short hash.

Major Number
============

The major number is expected to increment infrequently. After the first major release, it is recommended that the major
version number only increments for major breaking changes.

Minor Number
============

.. warning::

   Until the version 1.0.0 release, minor increments may also indicate breaking changes.

The minor number is updated for the following reasons:

* New features
* Major internal implementation changes
* Non-breaking interface updates

Minor version number increments should be made after a decision from the |PROJECT| development team.

Micro Number
============

The micro version number indicates the following changes:

* Bug fixes
* Minor internal implementation changes

Micro releases are made at the discretion of the |project| lead developer and the development team.

.. _releasebranchreq:

***************************
Release Branch Requirements
***************************

All version requires a manual update to the release number on a dedicated release commit. Versions are built from Git
tags for the otherwise automated `setuptools_scm`_ version number tool. Tags may be added directly to a commit on the
``main`` branch, but a release branch is encouraged.

Steps needed for a release include:

1. Create a release branch, e.g. ``release-0-4-1``.
2. Modify ``docs/changelog.rst`` to move version number for release MR commit and add description as relevant.
3. Check and update the ``CITATION.bib`` and ``CITATION.cff`` files to use the new version number and release date.
4. Commit changes and submit a merge request to the ``main`` branch at the upstream `repository`_.
5. Solicit feedback and make any required changes.
6. Merge the release branch to ``main``.
7. Create a new tag on the main branch from the CLI or web-interface:
   https://re-git.lanl.gov/aea/python-projects/spade/-/tags.
