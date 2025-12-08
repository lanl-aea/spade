###########
User Manual
###########

|PROJECT| is a wrapper around an Abaqus :cite:`abaqus` c++ executable and the Abaqus c++ API.

***********
Quick Start
***********

.. include:: README.txt
   :start-after: user-start-do-not-remove
   :end-before: user-end-do-not-remove

.. _tutorials:

*******
Example
*******

1. Fetch an Abaqus tutorial ODB file

   .. code-block::

      $ /apps/abaqus/Commands/abq2025 fetch -job viewer_tutorial.odb

2. Extract to H5 with |PROJECT|

   .. code-block::

      $ spade extract viewer_tutorial.odb
      $ ls viewer_tutorial*.*
      viewer_tutorial.h5  viewer_tutorial.odb  viewer_tutorial.spade.log

************
Installation
************

This project builds and deploys as a `Conda`_ package :cite:`conda,conda-gettingstarted`.

.. include:: README.txt
   :start-after: installation-start-do-not-remove
   :end-before: installation-end-do-not-remove
