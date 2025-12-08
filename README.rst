.. target-start-do-not-remove

.. _`spade`: https://re-git.lanl.gov/aea/python-projects/spade
.. _`AEA Gitlab Group`: https://re-git.lanl.gov/aea
.. _`Gitlab CI/CD`: https://docs.gitlab.com/ee/ci/
.. _`AEA Compute Environment`: https://re-git.lanl.gov/aea/developer-operations/aea_compute_environment
.. _`AEA Conda channel`: https://aea.re-pages.lanl.gov/developer-operations/aea_compute_environment/aea_compute_environment.html#aea-conda-channel
.. _`Bash rsync`: https://re-git.lanl.gov/aea/developer-operations/aea_compute_environment
.. _Conda: https://docs.conda.io/en/latest/
.. _Conda installation: https://docs.conda.io/projects/conda/en/latest/user-guide/install/index.html
.. _Conda environment management: https://docs.conda.io/projects/conda/en/latest/user-guide/tasks/manage-environments.html

.. _`Prabhu Khalsa`: pkhalsa@lanl.gov
.. _`Kyle Brindley`: kbrindley@lanl.gov
.. _`Sergio Cordova`: sergioc@lanl.gov

.. target-end-do-not-remove

####################################################
Serialized Proprietary Abaqus Data Extractor (SPADE)
####################################################

.. badges-start-do-not-remove

.. |github-pages| image:: https://img.shields.io/github/actions/workflow/status/lanl-aea/spade/pages.yml?branch=main&label=GitHub-Pages
   :target: https://lanl-aea.github.io/spade/

.. |github-release| image:: https://img.shields.io/github/v/release/lanl-aea/spade?label=GitHub-Release
   :target: https://github.com/lanl-aea/spade/releases

.. |zenodo| image:: https://zenodo.org/badge/DOI/10.5281/zenodo.15747904.svg
   :target: https://doi.org/10.5281/zenodo.15747904

|github-pages| |github-release| |zenodo|

.. badges-end-do-not-remove

.. inclusion-marker-do-not-remove

***********
Description
***********

.. description-start-do-not-remove

Serialized Proprietary Abaqus Data Extractor (SPADE) extracts data from an Abaqus ODB File into serialized open source data formats.

.. description-end-do-not-remove

Documentation
=============

* GitHub: https://lanl-aea.github.io/spade/index.html
* LANL: https://aea.re-pages.lanl.gov/python-projects/spade/

Author Info
===========

* `Prabhu Khalsa`_
* `Kyle Brindley`_

***********
Quick Start
***********

.. user-start-do-not-remove

Currently, this project is only available on the `AEA Conda channel`_ and in an `AEA Compute Environment`_.

1. Load the AEA compute environment

   .. code-block::

      $ module use /projects/aea_compute/aea-conda
      $ module load aea-nightly

2. View the CLI usage

   .. code-block::

      $ spade -h

.. user-end-do-not-remove

************
Installation
************

.. installation-start-do-not-remove

Currently, this project is only available on the `AEA Conda channel`_ and in an `AEA Compute Environment`_.

SPADE can be installed in a `Conda`_ environment with the `Conda`_ package manager. See the `Conda installation`_
and `Conda environment management`_ documentation for more details about using `Conda`_.

.. code-block::

   $ conda install --channel /projects/aea_compute/aea-conda --channel conda-forge spade

.. installation-end-do-not-remove

****************
Copyright Notice
****************

.. copyright-start-do-not-remove

Copyright (c) 2023-2025, Triad National Security, LLC. All rights reserved.

This program was produced under U.S. Government contract 89233218CNA000001 for Los Alamos National Laboratory (LANL),
which is operated by Triad National Security, LLC for the U.S.  Department of Energy/National Nuclear Security
Administration. All rights in the program are reserved by Triad National Security, LLC, and the U.S. Department of
Energy/National Nuclear Security Administration. The Government is granted for itself and others acting on its behalf a
nonexclusive, paid-up, irrevocable worldwide license in this material to reproduce, prepare derivative works, distribute
copies to the public, perform publicly and display publicly, and to permit others to do so.

.. copyright-end-do-not-remove

**********************
Developer Instructions
**********************

Cloning the Repository
======================

.. cloning-the-repo-start-do-not-remove

Cloning the repository is very easy, simply refer to the sample session below. Keep in mind that you get to choose the
location of your local `spade`_ clone. Here we use ``/projects/roppenheimer/repos`` as an example.

.. code-block:: bash

    [roppenheimer@sstelmo repos]$ git clone ssh://git@re-git.lanl.gov:10022/aea/python-projects/spade.git

.. cloning-the-repo-end-do-not-remove

Compute Environment
===================

.. compute-env-start-do-not-remove

You can create a local environment with the Conda package manager as

.. code-block::

   [roppenheimer@mymachine spade]$ conda env create --file environment.yml --name spade-env
   [roppenheimer@mymachine spade]$ conda activate spade-env

.. compute-env-end-do-not-remove

Testing
=======

.. testing-start-do-not-remove

This project now performs CI testing on AEA compute servers. The up-to-date test commands can be found in the
``.gitlab-ci.yml`` file. The full regression suite includes the documentation builds, Python 3 unit tests, and the
system tests.

.. code-block::

    $ pwd
    /home/roppenheimer/repos/spade
    $ scons regression

There is also a separate style guide check run as

.. code-block::

    $ scons flake8 black-check

If the CI job fails, you can run the following and commit the changes

.. code-block::

    $ scons black-format

It is also possible to run ``flake8`` and ``black`` directly, but the SCons aliases include the canonical options for
the continuous integration jobs.

The full list of available aliases can be found as ``scons -h``.

.. testing-end-do-not-remove
