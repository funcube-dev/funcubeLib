@echo off
setlocal

set filetype=%1
set outfile=%2

set workdir=%~dp0%

set svnexe=svn.exe

if not exist "%svnexe%" goto :no_svn

for /f "tokens=4" %%i in ('%svnexe% info %workdir%^|find "Last Changed Rev"') do set svn_rev=%%i
for /f "tokens=4" %%i in ('%svnexe% info %workdir%^|find "Last Changed Date"') do set svn_date=%%i
for /f "tokens=5" %%i in ('%svnexe% info %workdir%^|find "Last Changed Date"') do set svn_time=%%i

if "%svn_rev%"=="" goto :empty_value

goto yes_svn

:no_svn
echo USING DEFAULTS, Subversion not found: %svnexe%
goto :set_defaults

:empty_value
echo USING DEFAULTS, Subversion found: %svnexe% but no revision obtained
goto :set_defaults
:set_defaults

set svn_rev=6666
set svn_date=1970-01-01
set svn_time=00:00:00

:yes_svn
if %filetype%==header goto write_header
if %filetype%==assembly goto write_assembly

goto end

:write_header
echo Writing header rev=%svn_rev% date=%svn_date% time=%svn_time% file=%outfile%

echo #pragma once > %outfile%
echo // Changes will be overwritten, build generated file >> %outfile%
echo #define SVN_REV %svn_rev% >> %outfile%
echo #define SVN_REV_STR "%svn_rev%" >> %outfile%
echo #define SVN_DATE_STR "%svn_date%" >> %outfile%
echo #define SVN_TIME_STR "%svn_time%" >> %outfile%
goto end

:write_assembly
echo Writing assembly rev=%svn_rev% date=%svn_date% time=%svn_time% file=%outfile%

echo using System; > %outfile%
echo // Changes will be overwritten, build generated file >> %outfile%
echo public class VersionNo >> %outfile%
echo { >> %outfile%
echo     public const string SVN_REV = "%svn_rev%"; >> %outfile%
echo     public const string SVN_DATE = "%svn_date%"; >> %outfile%
echo     public const string SVN_TIME = "%svn_time%"; >> %outfile%
echo } >> %outfile%
goto end

:end