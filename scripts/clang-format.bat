@echo off
setlocal
REM Format Flipman sources, CTest, or supplied files/directories with the shared driver.
python "%~dp0format_sources.py" %*
exit /b %errorlevel%
