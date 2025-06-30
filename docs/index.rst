#########
|project|
#########

.. include:: project_brief.txt

.. raw:: latex

   \part{User Manual}

.. toctree::
   :hidden:
   :maxdepth: 2
   :caption: User Manual

   installation
   user
   external_api
   cli

.. raw:: latex

   \part{Developer Manual}

.. toctree::
   :hidden:
   :maxdepth: 1
   :caption: Developer Manual

   internal_api
   devops

.. raw:: latex

   \part{Reference}

.. toctree::
   :hidden:
   :maxdepth: 1
   :caption: Reference

   license
   citation
   release_philosophy
   changelog
   zreferences
   README

.. only:: html

   .. include:: README.txt
      :start-after: badges-start-do-not-remove
      :end-before: badges-end-do-not-remove

   |

   .. grid:: 1 2 2 2
      :gutter: 2
      :margin: 2

      .. grid-item-card:: :octicon:`download` Installation
         :link: installation
         :link-type: ref

         Installation with conda-forge or PyPI

      .. grid-item-card:: :octicon:`mortar-board` Tutorials
         :link: tutorials
         :link-type: ref

         Tutorials using the command line interface

      .. grid-item-card:: :octicon:`code-square` API
         :link: external_api
         :link-type: ref

         Public application program interface (API)

      .. grid-item-card:: :octicon:`command-palette` CLI
         :link: cli
         :link-type: ref

         Public command line interface (CLI)

