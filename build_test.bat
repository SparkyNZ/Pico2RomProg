c:\cc65\bin\ca65 test.asm -l test.lst
if errorlevel 1 exit /b %errorlevel%

c:\cc65\bin\ld65 -C rom64k.cfg test.o -o test.bin
if errorlevel 1 exit /b %errorlevel%

openocd -s C:\pico\openocd\share\openocd\scripts -f interface/cmsis-dap.cfg -f target/rp2350.cfg -c "adapter speed 5000" -c "program test.bin bin 0x10080000 verify reset exit"