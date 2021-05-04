# efyj

A tool to easily run evluation or optimization of
[DEXi](https://kt.ijs.si/MarkoBohanec/dexi.html) file (Multi-Attribute
Decision Making).

* Copyright © 2014-2021 INRA
* The software is released under the MIT license. See the COPYING file.

## Requirements

On Unix (Linux, OS X)

* libexpat (≥ 2)
* cmake (≥ 3.10)
* gcc ≥ 7.0 or clang ≥ 7.0 (need `c++17` support)
* python ≥ 3.0 (optional)
* R ≥ 4.0 (optional)

For recent Debian and Ubuntu derivatives (remove clang to only use gcc):

````bash
apt-get install build-essential cmake libexpat1-dev g++ python r-base r-base-dev
````

On Windows

* [Visual Studio 2017 or newer](https://visualstudio.microsoft.com/fr/downloads/) for command line interface and python package
* cmake (≥ 3.10)
* [vcpkg](https://github.com/microsoft/vcpkg)
* [vcpkg expat](https://github.com/microsoft/vcpkg/tree/master/ports/expat)
* [python](https://www.python.org/downloads/windows/) ≥ 3.0 (optional)
* [R](https://www.r-project.org/) ≥ 4.0 (optional)
* [Rtools](https://cran.r-project.org/bin/windows/Rtools/) ≥ 4.0 (optional)

````
git clone https://github.com/microsoft/vcpkg
.\vcpkg\bootstrap-vcpkg.bat
.\vcpkg\vcpkg integrate install
.\vcpkg\vcpkg install
````

## Python package installation

First, on Windows, make sure you have installed Visual Studio 2017, [vcpkg](https://github.com/microsoft/vcpkg) and [vcpkg expat](https://github.com/microsoft/vcpkg/tree/master/ports/expat). On Unix (Linux, OS X), install python and python-dev package. 

Then, clone this repository and pip install. Note the `--recursive` option which is needed for both the pybind11 and fmt submodule:

````bash
git clone --recursive https://github.com/quesnel/efyj.git
pip install ./efyj
````

## R package installation

On Windows, make sur you have installed R, Rtools. Then starts  `rtools bash` from the start menu and type the following commands:

````bash
pacman -Sy
pacman -S mingw-w64-i686-expat
pacman -S mingw-w64-i86_64-expat
````

On Unix (Linux and OS X), install R and R development files from the package tools of your distribution (`apt`, `packman`, `yaourt`, `portage` etc.).

Then, for both Windows and Unix,  clone this repository and install the package. Note the `--recursive` option which is needed for both the pybind11 and fmt submodule:

````bash
git clone --recursive https://github.com/quesnel/efyj.git
````

If you have already cloned efyj, you need to update your copy. Use the following commands:

````bash
cd efyj			# move to the right directory path
git fetch origin
git submodule update --init
git reset --hard origin/master
````

Under a R terminal or Rstudio, type the following command (adapt the `setwd` command to the correct efyj clone):

````R
install.packages("Rcpp")
library(Rcpp)
detach("package:Rcpp", unload = TRUE)
install.packages("devtools")
library(devtools)
setwd("C:/Users/XXXXXXXX/efyj/refyj")	# use the right directory path
load_all(".")
devtools::test()
````

## Command line usage

````bash
efyj -m file.dxi -o file.csv [-p] [-a]
````

The `file.csv` contains the options of the model. Four first one
columns contains fields: site, fields, department, year. Next columns
are basic attributes. Finally, the last column contains the result of
the model dexi.

For example, if a DEXi file `test.dxi` have one aggregate attribute
`root` and three basic attributes `a`, `b` and `c`, the CSV file is:

	$ cat file.csv
	site;fields;department;year;a;b;c;observed
	xxxx;yyyy;1532;2010;ok;favourable;host;0-5
	...

The '-p' option starts a prediction algorithm, '-a' option starts am
adjustment algorithm.
