# Serialized Proprietary Abaqus Data Extractor (SPADE)
Extract data from an Abaqus ODB File into an HDF5 file

To see available options
```spade -h```

Developer:
* Prabhu Khalsa <prabhu@lanl.gov>

---

---


## Running this code 

### On Local Machine
``module use --append /projects/aea_compute/modulefiles``
``module load aea-release``  
``spade <options>``

---

---

## Available options
This section provides a complete list of the command line options available.

### Options
---
``-h, help`` -- show this help message and exit
``-v, verbose`` -- turn on verbose logging
``-o, output-file`` -- name of output file (default: <odb file name>.h5)
``-t, output-file-type`` -- type of file to store output (default: h5)
``-f, force-overwrite`` -- if output file already exists, then over write the file
``--step`` -- get information from specified step (default: all)
``--frame`` -- get information from specified frame (default: all)
``--frame-value`` -- get information from specified frame (default: all)
``--field`` -- get information from specified field (default: all)
``--history`` -- get information from specified history value (default: all)
``--history-region`` -- get information from specified history region (default: all)
``--instance`` -- get information from specified instance (default: all)

---

---

## Compiling
### development environment
``module use /projects/aea_compute/modulefiles``
``module load odb-extract-env``

### Compiling
To compile this code:
``scons``

### Removing executables
To remove all executables after make:
``scons --clean``

---

---

## Examples

### SPADE

``spade model.odb``

``spade -v model.odb``

``spade -o hdf5_model.h5 model.odb``

``spade --step step1 model.odb``

``spade --frame 0 model.odb``

``spade --instance part1 model.odb``

---

---
