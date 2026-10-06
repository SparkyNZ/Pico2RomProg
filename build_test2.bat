c:\cc65\bin\ca65 test2.asm -l test2.lst
if errorlevel 1 exit /b %errorlevel%

c:\cc65\bin\ld65 -C rom64k.cfg test2.o -o test2.bin
if errorlevel 1 exit /b %errorlevel%

REM openocd -s C:\pico\openocd\share\openocd\scripts -f interface/cmsis-dap.cfg -f target/rp2350.cfg -c "adapter speed 5000" -c "program test2.bin bin 0x10080000 verify reset exit"

REM -- NO VERIFICATION
REM openocd -s C:\pico\openocd\share\openocd\scripts -f interface/cmsis-dap.cfg -f target/rp2350.cfg -c "adapter speed 5000" -c "program test2.bin bin 0x10080000 reset exit"

REM -- VERIFICATION
REM openocd -s C:\pico\openocd\share\openocd\scripts -f interface/cmsis-dap.cfg -f target/rp2350.cfg -c "adapter speed 5000" -c "program test2.bin bin 0x10080000" -c "program verify_on.bin bin 0x10088000 reset exit"


REM openocd -f interface/cmsis-dap.cfg -f target/rp2350.cfg -c "init; reset halt; program test2.bin 0x10080000 verify; reset run; exit"
openocd -f interface/cmsis-dap.cfg -c "adapter speed 5000" -f target/rp2350.cfg -c "init; reset halt; program test2.bin 0x10080000; reset run; exit"