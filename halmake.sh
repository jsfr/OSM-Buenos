#!/bin/bash
make
cd tests/
make
cd ..
util/tfstool delete fyams.harddisk shell
util/tfstool write fyams.harddisk tests/HAL9000 shell
fyams-sim buenos initprog=[fyamsHDD]shell
exit 0