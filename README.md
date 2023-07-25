# Odb Extract
Extract data from an Abaqus ODB File into and HDF5 file

To see available options
```odb_extract -h```

Developer:
* Prabhu Khalsa <prabhu@lanl.gov>

---

---


## Running this code 

### On Local Machine
``module use --append /projects/aea_compute/modulefiles``
``module load aea-release``  
``odb_extract <options>``

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

### Compiling
To compile this code:
``scons``

### Removing executables
To remove all executables after make:
``scons --clean``

---

---

## Examples

### Odb Extract

``odb_extract model.odb``

``odb_extract -v model.odb``

``odb_extract -o hdf5_model.h5 model.odb``

``odb_extract --step step1 model.odb``

``odb_extract --frame 0 model.odb``

``odb_extract --instance part1 model.odb``

---

---