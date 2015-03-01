The patch-tools.cmd script copies the following files to wxWidgets sources
to make 'em compatible with UltraDefrag:

  intl.cpp     - contains the correct definition of the Javanese language:
                 http://trac.wxwidgets.org/changeset/67426

  menuitem.cpp - adds menu item width adjustment to wxMenuItem::SetItemLabel

  setup.h      - configures wxWidgets for use with UltraDefrag

Don't forget to recompile the library whenever they get changed.