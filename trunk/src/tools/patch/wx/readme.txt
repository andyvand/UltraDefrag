This directory contains files which need to be copied to wxWidgets sources
in order to compile libraries suitable for UltraDefrag. All the things are
automated by the patch-tools.cmd script. Run it to patch wxWidgets by these
files:

intl.cpp - contains correct definition of Javanese language:
           http://trac.wxwidgets.org/changeset/67426

setup.h  - configures wxWidgets for use with UltraDefrag
