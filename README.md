# Building

## OpenWrt components
```
cd openwrt
cp config-arm-mrvl-4.4 .config
make oldconfig
make -j1 BOARD=arm-mrvl-4.4 OPENWRT_EXTRA_BOARD_SUFFIX=_mrvl_4.4
```

## Kernel
The example below uses the OpenWrt cross compilation environment. If you already have an arm cross compiler installed, modify the CROSS_COMPILE path accordingly.

```
cd linux-4.4
cp ../openwrt/target/linux/switch-arm-mrvl-4.4/config .config
make CROSS_COMPILE=../openwrt/staging_dir_arm_mrvl_4.4/bin/arm-unknown-linux-uclibcgnueabihf- ARCH=arm oldconfig
make CROSS_COMPILE=../openwrt/staging_dir_arm_mrvl_4.4/bin/arm-unknown-linux-uclibcgnueabihf- ARCH=arm prepare
touch rootlist
make CROSS_COMPILE=../openwrt/staging_dir_arm_mrvl_4.4/bin/arm-unknown-linux-uclibcgnueabihf- ARCH=arm vmlinux
```
