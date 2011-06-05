rem By Plasma Works
rem Suite 300-413, 2700 Woodlands Vlg. Blvd., Flagstaff AZ  86001
rem Puzzle Pits is Copyright (C) 1996 by Abe Pralle
@echo off
echo.
echo Puzzle Pits v96.5 Installation
echo.
if not exist pits.exe goto changecurdrive
if not %1\pits==\pits goto skiplecture

echo USAGE:  install path
echo.
echo         A new directory called PITS will be created using the path
echo         you specify.  For example, typing:
echo           install c:
echo         will create a new directory on the c: drive.
echo         Typing: 
echo           install c:\games
echo         will create a directory called PITS inside the GAMES directory.
echo.
goto exitinstall

:skiplecture
echo You have specified that you wish to install the game to 
echo "%1\pits"
echo.

if not exist %1\nul goto notfound
choice /c:yn Is this correct?
if errorlevel 2 goto exitinstall

echo.
mkdir %1\pits
mkdir %1\pits\levels
copy *.exe %1\pits
copy *.dat %1\pits
copy *.fnt %1\pits
copy *.wav %1\pits
copy levels\*.* %1\pits\levels

echo.
echo Puzzle Pits installation complete!
echo Change the current drive to the drive you installed the game on,
echo and type:
echo   cd %1\pits
echo   pits
echo to start the game!
echo.
goto end

:changecurdrive
echo You must make the floppy drive the current drive before running
echo INSTALL.  If your disk is in drive A, type:
echo   a:
echo and then re-run INSTALL.  If your disk is in drive B, type:
echo   b:
echo and then re-run INSTALL.
echo.
goto exitinstall

:notfound
echo I cannot find the directory %1.  Please verify that it 
echo exists and try again.

:exitinstall
echo Installation Aborted
echo.

:end
