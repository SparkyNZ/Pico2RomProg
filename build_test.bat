c:\cc65\bin\ca65 test.asm -l test.lst
if errorlevel 1 exit /b %errorlevel%

c:\cc65\bin\ld65 -C rom64k.cfg test.o -o test.bin
if errorlevel 1 exit /b %errorlevel%

REM openocd -s C:\pico\openocd\share\openocd\scripts -f interface/cmsis-dap.cfg -f target/rp2350.cfg -c "adapter speed 5000" -c "program test.bin bin 0x10080000 verify reset exit"

REM -- NO VERIFICATION
REM openocd -s C:\pico\openocd\share\openocd\scripts -f interface/cmsis-dap.cfg -f target/rp2350.cfg -c "adapter speed 5000" -c "program test.bin bin 0x10080000 reset exit"

REM -- VERIFICATION
REM openocd -s C:\pico\openocd\share\openocd\scripts -f interface/cmsis-dap.cfg -f target/rp2350.cfg -c "adapter speed 5000" -c "program test.bin bin 0x10080000" -c "program verify_on.bin bin 0x10088000 reset exit"


openocd -f interface/cmsis-dap.cfg -f target/rp2350.cfg -c "init; reset halt; program test.bin 0x10080000 verify; reset run; exit"