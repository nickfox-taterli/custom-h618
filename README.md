# H616 自定义板启动镜像制作笔记

没有显示功能,没有音频功能,只有USB,以太网,WiFi+蓝牙,GPIO.因此释放了所有其他资源占用,并且rootfs也再做了一遍,主要是这样干净得什么都没有,毕竟只是我自己用,但是硬件和OPI 2W很相近(只支持1.5G版本),所以开放一下.

## 1. 编译 TF-A (ARM Trusted Firmware)

### 环境准备
```bash
git clone https://github.com/nickfox-taterli/custom-h618
cd arm-trusted-firmware/
```

### 编译命令
```bash
make CROSS_COMPILE=aarch64-linux-gnu- PLAT=sun50i_h616 DEBUG=1 bl31
```

### 生成文件
```
build/sun50i_h616/debug/bl31.bin
```

## 2. 编译 U-Boot

### 编译步骤
```bash
make CROSS_COMPILE=aarch64-linux-gnu- BL31=../arm-trusted-firmware/build/sun50i_h616/debug/bl31.bin sun50i-h618-taterli_defconfig
make CROSS_COMPILE=aarch64-linux-gnu- BL31=../arm-trusted-firmware/build/sun50i_h616/debug/bl31.bin
```

### 生成文件
```
u-boot-sunxi-with-spl.bin
```

## 3. 烧录引导程序

### TF卡烧录
```bash
dd if=u-boot-sunxi-with-spl.bin of=/dev/sdb bs=8K seek=1
```

### SPI Flash烧录(需在目标板执行)
```bash
apt-get install mtd-utils
flashcp -v u-boot-sunxi-with-spl.bin /dev/mtd0
```

## 4. 编译 Linux 内核

### 配置与编译
```bash
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- sun50iw9-taterli_defconfig
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- Image dtbs modules -j$(nproc)
```

### 关键输出文件
```
arch/arm64/boot/Image
arch/arm64/boot/dts/allwinner/sun50i-h618-headless.dtb
```

## 5. 构建根文件系统

### 使用 debootstrap 创建基础系统
```bash
debootstrap --arch=arm64 --variant=buildd --foreign bookworm rootfs http://mirrors.tuna.tsinghua.edu.cn/debian
cp /usr/bin/qemu-aarch64-static rootfs/usr/bin/
chroot rootfs debootstrap/debootstrap --second-stage
```

### 系统配置

#### 软件源配置 `/etc/apt/sources.list`
```bash
deb http://mirrors.tuna.tsinghua.edu.cn/debian/ bookworm main contrib non-free non-free-firmware
deb http://mirrors.tuna.tsinghua.edu.cn/debian/ bookworm-updates main contrib non-free non-free-firmware
deb http://mirrors.tuna.tsinghua.edu.cn/debian-security bookworm-security main contrib non-free non-free-firmware
```

#### 安装基础软件包
```bash
chroot rootfs apt update && chroot rootfs apt install -y \
    openssh-server systemd-sysv \
    network-manager wpasupplicant \
    build-essential git \
    python3-dev python3-pip \
    bluez rfkill
chroot rootfs passwd root
```

### 网络配置

#### 启用网络服务
```bash
chroot rootfs systemctl enable systemd-networkd
chroot rootfs systemctl enable systemd-resolved
```

#### DHCP 配置文件 `/etc/systemd/network/10-eth0.network`
```ini
[Match]
Name=eth0

[Network]
DHCP=yes
```

### 本地化设置

#### `/etc/locale.gen`
取消注释：
```
zh_CN.UTF-8 UTF-8
```

#### `/etc/default/locale`

```
LANG=zh_CN.UTF-8
LANGUAGE="zh_CN.UTF-8"
LC_CTYPE="zh_CN.UTF-8"
LC_NUMERIC="zh_CN.UTF-8"
LC_TIME="zh_CN.UTF-8"
LC_COLLATE="zh_CN.UTF-8"
LC_MONETARY="zh_CN.UTF-8"
LC_MESSAGES="zh_CN.UTF-8"
LC_PAPER="zh_CN.UTF-8"
LC_NAME="zh_CN.UTF-8"
LC_ADDRESS="zh_CN.UTF-8"
LC_TELEPHONE="zh_CN.UTF-8"
LC_MEASUREMENT="zh_CN.UTF-8"
LC_IDENTIFICATION="zh_CN.UTF-8"
LC_ALL="zh_CN.UTF-8"
```

生成本地化信息

```bash
chroot rootfs dpkg-reconfigure locales
chroot rootfs timedatectl set-timezone Asia/Shanghai
```

## 6. 安装内核模块和头文件

```bash
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- INSTALL_MOD_PATH=../rootfs modules_install
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- INSTALL_HDR_PATH=../rootfs/usr headers_install
```

## 7. 准备启动文件

### 复制内核文件
```bash
cp arch/arm64/boot/Image ../rootfs/boot/
cp arch/arm64/boot/dts/allwinner/sun50i-h618-headless.dtb ../rootfs/boot/
```

### 创建 U-Boot 启动脚本

#### `boot.cmd` 内容
```bash
setenv bootargs console=ttyS0,115200 earlyprintk root=/dev/mmcblk1p1 rootwait
load mmc 0:1 0x40080000 /boot/Image
load mmc 0:1 0x4fa00000 /boot/sun50i-h618-headless.dtb
booti 0x40080000 - 0x4fa00000
```

#### 生成 boot.scr
```bash
mkimage -C none -A arm -T script -d boot.cmd boot.scr
cp boot.scr ../rootfs/boot/
```

## 8. 制作系统镜像

### 分区与格式化
```bash
fdisk /dev/sdb << EOF
o
n
p
1
2048
+2G
w
EOF

mkfs.ext4 /dev/sdb1 -L rootfs
```

### 复制文件系统
```bash
mount /dev/sdb1 /mnt
cp -a ../rootfs/* /mnt
umount /mnt
```

## 9. 无线网络配置(目标板执行)

### 复制固件文件(在主机执行)
```bash
sudo cp -r firmware/* rootfs/lib/firmware/
```

### 加载无线模块
```bash
modprobe uwe5622_bsp_sdio
modprobe sprdwl_ng
modprobe sprdbt_tty
```

### WiFi 连接配置
```bash
cat > /etc/wpa_supplicant.conf <<EOF
network={
    ssid="YOUR_SSID"
    psk="YOUR_PASSWORD"
    key_mgmt=WPA-PSK
}
EOF
wpa_supplicant -B -i wlan0 -c /etc/wpa_supplicant.conf
dhclient wlan0
```

### 编译蓝牙工具
```bash
cd hcitools/
make CROSS_COMPILE=aarch64-linux-gnu-
```

### 安装到根文件系统
```bash
cp output/hciattach_opi /path/to/rootfs/usr/bin/
```

### 3. 蓝牙配置与启用
```bash
# 使用编译的蓝牙工具
hciattach_opi -n -s 1500000 /dev/ttyBT0 sprd

# 检查蓝牙状态
hciconfig -a
hciconfig hci0 up

# 扫描附近设备
hcitool scan
```

## 10. 最终清理

```bash
rm rootfs/usr/bin/qemu-aarch64-static
rm -rf rootfs/var/lib/apt/lists/*
```

## 注意事项

U-Boot 如果要在TF卡,分区位置得往后推一推.

## 附录：文件结构说明

```
.
├── arm-trusted-firmware/    # TF-A 源码
├── firmware/                # 硬件固件文件
├── hcitools/               # 蓝牙工具源码
├── linux/                  # 内核源码
├── u-boot/                 # U-Boot 源码
├── rootfs/                 # 根文件系统
│   ├── boot/
│   │   ├── Image
│   │   ├── sun50i-h618-headless.dtb
│   │   └── boot.scr
│   └── lib/
│       └── firmware/       # 硬件固件
└── tmp/                    # 临时文件
```
