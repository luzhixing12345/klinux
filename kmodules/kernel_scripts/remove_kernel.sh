#!/bin/bash

# Check if the script is run as root
if [ "$(id -u)" -ne "0" ]; then
    echo "This script must be run as root" 1>&2
    exit 1
fi

# Define the directory where the kernel images are stored
KERNEL_DIR="/boot"

# List available kernel versions and assign a number to each
echo "Available kernel versions:"
kernels=($(ls ${KERNEL_DIR}/vmlinuz-*))
count=0
for kernel in "${kernels[@]}"; do
    kernel_version=$(echo $kernel | sed 's/.*\/vmlinuz-//')
    echo "[$count]: $kernel_version"
    ((count++))
done
echo ""

# Prompt the user to select a kernel version by number
read -p "Enter the number of the kernel version you want to delete: " kernel_number

# Check if the input is a number and within the range
if ! [[ "$kernel_number" =~ ^[0-9]+$ ]] || [ "$kernel_number" -lt "0" ] || [ "$kernel_number" -ge "$count" ]; then
    echo "Invalid selection"
    exit 1
fi

# Get the kernel version based on the number
kernel_version=$(echo ${kernels[$kernel_number]} | sed 's/.*\/vmlinuz-//')
read -p "remove kernel version: $kernel_version (y/n)? " response
if [[ "$response" == "n" || "$response" == "N" ]]; then
    echo "Operation cancelled."
    exit 1
fi

ls /boot/ | grep ${kernel_version} | sudo xargs -I {} rm /boot/{}
sudo rm -rf /lib/modules/${kernel_version}
sudo rm -rf /usr/src/linux-headers-${kernel_version}
sudo update-grub

echo -e "\e[32mKernel $kernel_version removed successfully.\e[0m"