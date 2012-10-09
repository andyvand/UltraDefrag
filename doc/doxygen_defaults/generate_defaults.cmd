@echo off
::
:: This utility will generate the default header, footer and style sheet for
:: HTML, CSS and LaTeX
:: of a default doxygen configuration without any additional customization
::
:: Three versions will be created:
::    1) regular ... no search and no treeview
::    2) search .... search only
::    3) tree ...... treeview only
::
echo.

doxygen --version >default_Current_Version.txt
set /p DoxyCurVer=<default_Current_Version.txt

mkdir default_%DoxyCurVer%

del /f /q default_%DoxyCurVer%\default_*.*

cd default_%DoxyCurVer%

doxygen -g default_Doxyfile

echo Generating Default HTML files with Search...
echo PROJECT_NAME=Default >>default_Doxyfile
doxygen -w html default_header_search.html default_footer_search.html default_doxygen_search.css default_Doxyfile
doxygen -w latex default_header_search.tex default_footer_search.tex default_doxygen_search.sty default_Doxyfile

echo Generating Default HTML files...
echo SEARCHENGINE=NO >>default_Doxyfile
doxygen -w html default_header_regular.html default_footer_regular.html default_doxygen_regular.css default_Doxyfile
doxygen -w latex default_header_regular.tex default_footer_regular.tex default_doxygen_regular.sty default_Doxyfile

echo Generating Default HTML files with Treeview...
echo GENERATE_TREEVIEW=YES >>default_Doxyfile
doxygen -w html default_header_tree.html default_footer_tree.html default_doxygen_tree.css default_Doxyfile
doxygen -w latex default_header_tree.tex default_footer_tree.tex default_doxygen_tree.sty default_Doxyfile

echo.
pause
