#!/bin/bash

set -e


BOOTPATH='bootloader'
KERNELPATH='kernel'

cd $BOOTPATH 
make clean
make
cd ..

cd ${KERNELPATH}
make clean

if [ $# -eq 1 -a "$1" = "b" ];then
    cp PDE_bochs PDE_entrys
else
    cp PDE_qemu PDE_entrys
fi

make
cd ..

cp boot.img_bak boot.img
dd if=${BOOTPATH}/boot.bin of=boot.img bs=512 count=1 conv=notrunc
echo '970410'|sudo -S mount boot.img /media/
echo -e '\n'
sudo cp ${BOOTPATH}/loader.bin /media/
sudo cp ${KERNELPATH}/kernel.bin /media/
sync
sudo umount /media/

if [ $# -eq 1 -a "$1" = "b" ];then
    bochs
else
    qemu-system-x86_64 -enable-kvm -m 2048M -fda boot.img
fi