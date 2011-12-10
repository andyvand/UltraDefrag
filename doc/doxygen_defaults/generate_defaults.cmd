@echo off
::
:: This utility will generate the default HTML header, footer and CSS style sheet
:: for a default doxygen configuration without any additional customization
::
:: Three versions will be created:
::    1) regular ... no search and treeview
::    2) search .... search only
::    3) tree ...... treeview only
::
echo.

del /f /q default_*.*

doxygen -g default_Doxyfile

echo Generating Default HTML files with Search...
echo PROJECT_NAME=Default >>default_Doxyfile
doxygen -w html default_header_search.html default_footer_search.html default_doxygen_search.css default_Doxyfile

echo Generating Default HTML files...
echo SEARCHENGINE=NO >>default_Doxyfile
doxygen -w html default_header_regular.html default_footer_regular.html default_doxygen_regular.css default_Doxyfile

echo Generating Default HTML files with Treeview...
echo GENERATE_TREEVIEW=YES >>default_Doxyfile
doxygen -w html default_header_tree.html default_footer_tree.html default_doxygen_tree.css default_Doxyfile

echo.
pause
