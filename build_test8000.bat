c:\cc65\bin\ca65 test8000.asm -l test8000.lst
if errorlevel 1 exit /b %errorlevel%

c:\cc65\bin\ld65 -C rom64k.cfg test8000.o -o test8000.bin
if errorlevel 1 exit /b %errorlevel%

openocd -f interface/cmsis-dap.cfg -f target/rp2350.cfg -c "adapter speed 5000" -c "program test8000.bin 0x10080000 verify reset exit"