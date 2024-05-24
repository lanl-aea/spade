.. _releasephilosophy:

##################
Release Philosophy
##################

This section discusses topics related to |PROJECT| releases and version numbering.

Version Numbers
===============

The |PROJECT| project follows the `PEP-440`_ standard for version numbering, as
implemented by `setuptools_scm`_. The production release version number uses the
three component ("major.minor.micro") scheme. The production version numbers
correspond to Git tags in the project `repository`_ which point to a static
release of the |PROJECT| project.

The developer (a.k.a. dev or aea-beta) version number follows the production release
number with an anticipated micro version bump ("major.minor.micro+1") and the
appended ".dev" local version number. Because the developer version is
constantly updated against development work, the version number found in the
deployed release contains additional information. During deployment, the
developer version number is appended with the Git information from the most
recent build as described by the default versioning scheme for
`setuptools_scm`_.

************
Major Number
************

The major number is expected to increment infrequently. Incrementing the major
number requires a version change announcement from the |PROJECT| development
team. This project will follow `semantic versioning`_ after the version 1.0.0
release.

************
Minor Number
************

.. warning::

   Until the version 1.0.0 release, minor increments may also indicate breaking changes.

The minor number is updated for the following reasons:

* New modules or major features
* Features spanning multiple module dependencies
* UX modifications

Minor version number increments should be made after
a decision from the |PROJECT| development team.

************
Micro Number
************

The micro version number indicates the following changes:

* Bug fixes
* Existing feature enhancements
* UI modifications

Micro releases are made at the discretion of the |project| lead developer and
the development team.

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
3. Commit changes and submit a merge request to the ``main`` branch at the upstream `repository`_.
4. Solicit feedback and make any required changes.
5. Immediately prior to merge, add the new version tag to the most recent commit.

   .. code-block::

      $ git tag 0.4.1
      $ git push origin release-0-4-1 --tags

6. Merge the release branch to ``main``
7. Create a new release for the new tag: https://re-git.lanl.gov/aea/python-projects/spade/-/releases
