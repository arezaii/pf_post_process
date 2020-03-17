pfio
=======

*Major update:*
Switch from PyArray_FromDims to PyArray_SimpleNew in order to be compatible with Numpy >1.18.

Thanks much for Mr. Ahmad Rezaii!

------------------------------

*pfio* is a Python C extension for reading and writing ParFlow binary (.pfb) files. This repo contains files to install *pfio* as a Python module.

#### Development Team
+ Hoang Tran (hoangtran@mines.edu)
+ Jun Zhang	(junzhang55@email.arizona.edu)
+ Laura Condon (lecondon@email.arizona.edu)
+ Reed Maxwell (rmaxwell@mines.edu)


Installation
--------------------
For Python2
```
python setup.py install

```

For Python3
```
python3 setup.py install

``````

Originality
--------------------
Most of the c code *pfio.c* is adapted from [ParFlow](https://github.com/parflow/parflow). Specifically, these codes were used:
+ printdatabox.c
+ readdatabox.c
+ tools_io.c 

Usage
--------------------

To read .pfb file

```
import pfio
data = pfio.pfread(filename)

```

To write a new .pfb file

```
import pfio
pfio.pfwrite(data,filename,X0,Y0,Z0,DX,DY,DZ)
```


License
--------------------
Copyright (C) 2019 Hoang Tran

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation version 3 of the License

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>

Acknowledgments
--------------------
+ ParFlow utilities codes by Mr. Steven Smith (LLNL)