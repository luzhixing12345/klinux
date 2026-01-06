
# cgroup

通过下面这条命令来查看当前系统使用的 Cgroups V1 还是 V2

```bash
stat -fc %T /sys/fs/cgroup/
```

如果输出是cgroup2fs 那就是 V2, 如果输出是tmpfs 那就是 V1; 现代一些的 linux kernel 默认采用的都是 v2 的 cgroup

## 启用 cgroup v2

如果当前系统未启用 Cgroup V2,也可以通过修改内核 cmdline 引导参数在你的 Linux 发行版上手动启用 cgroup v2

如果你的发行版使用 GRUB,则应在 /etc/default/grub 下的 GRUB_CMDLINE_LINUX 中添加 systemd.unified_cgroup_hierarchy=1

```bash
GRUB_CMDLINE_LINUX="quiet splash systemd.unified_cgroup_hierarchy=1"
```

```bash
sudo update-grub
```

重启后查看,不出意外切换到 cgroups v2 了

## 基本使用

创建 sub-cgroup: 只需要创建一个子目录

## 删除 cgroup

初始状态下,只有 root cgroup,所有进程都属于这个 cgroup

一个 cgroup 可以有多个子 cgroup,形成一个树形结构; 每个 cgroup 都有一个可读写的接口文件 `cgroup.procs`

- 读该文件会列出这个 cgroup 内的所有 PID,每行一个;
- PID 并未排序;
- 同一 PID 可能出现多次:进程先移出再移入该 cgroup,或读文件期间 PID 被重用了,都可能发生这种情况。

## 参考

- [arthurchiao cgroupv2-zh](https://arthurchiao.art/blog/cgroupv2-zh/)
- [lixueduan 08-cgroup-v2](https://www.lixueduan.com/posts/linux/08-cgroup-v2/)