git submodule update --init

cd submodules/STM32CubeL4
git submodule update --init Drivers/CMSIS/Device/ST/STM32L4xx

cd ../..
cd submodules/FreeRTOS
git submodule update --init FreeRTOS/Source
