#!/bin/bash

# Check if the script is run as root
if [ "$(id -u)" -ne "0" ]; then
    echo "This script must be run as root" 1>&2
    exit 1
fi

# Define the directory where the kernel images are stored
KERNEL_DIR="/boot"

# Get the current running kernel version
CURRENT_KERNEL=$(uname -r)

# List available kernel versions and assign a number to each
echo "Available kernel versions:"
kernels=($(ls ${KERNEL_DIR}/vmlinuz-*))
count=0
for kernel in "${kernels[@]}"; do
    kernel_version=$(basename "$kernel" | sed 's/vmlinuz-//')

    if [[ "$kernel_version" == "$CURRENT_KERNEL" ]]; then
        # 白色加粗 + 箭头标识当前内核
        printf "[%d]: \e[1;37m%s ← (current)\e[0m\n" "$count" "$kernel_version"
    else
        echo "[$count]: $kernel_version"
    fi
    ((count++))
done
echo ""

# Prompt the user to select a kernel version by number
read -p "Enter the number of the kernel version you want to switch to: " kernel_number

# Check if the input is a number and within the range
if ! [[ "$kernel_number" =~ ^[0-9]+$ ]] || [ "$kernel_number" -lt "0" ] || [ "$kernel_number" -ge "$count" ]; then
    echo "Invalid selection"
    exit 1
fi

# Get the kernel version based on the number
kernel_version=$(echo ${kernels[$kernel_number]} | sed 's/.*\/vmlinuz-//')
echo "Switching to kernel version: $kernel_version"

# Check if the selected kernel version exists
if [ ! -e "$KERNEL_DIR/vmlinuz-$kernel_version" ]; then
    echo "Kernel version $kernel_version does not exist"
    exit 1
fi

# Extract the menu entry for the default kernel
MID=`awk '/Advanced options.*/{print $(NF-1)}' /boot/grub/grub.cfg`
MID="${MID//\'/}"

KID=`awk -v kern="with Linux $kernel_version" '$0 ~ kern && !/recovery/ { print $(NF - 1) }' /boot/grub/grub.cfg`
KID="${KID//\'/}"

# Update GRUB configuration
sed -i "s/GRUB_DEFAULT=.*/GRUB_DEFAULT=\"$MID>$KID\"/" /etc/default/grub
update-grub

echo -e "Linux kernel is now [\e[32;1m$kernel_version\e[0m], Please reboot machine"
