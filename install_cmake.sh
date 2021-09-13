#!/bin/bash
chmod +x cmake-3.21.2-linux-x86_64.sh
yes | ./cmake-3.21.2-linux-x86_64.sh
ln -s /opt/cmake/cmake-3.21.2-linux-x86_64/bin/ccmake /usr/bin/ccmake
ln -s /opt/cmake/cmake-3.21.2-linux-x86_64/bin/cmake /usr/bin/cmake
ln -s /opt/cmake/cmake-3.21.2-linux-x86_64/bin/cmake-gui /usr/bin/cmake-gui
ln -s /opt/cmake/cmake-3.21.2-linux-x86_64/bin/cpack /usr/bin/cpack
ln -s /opt/cmake/cmake-3.21.2-linux-x86_64/bin/ctest /usr/bin/ctest
exit 0
