
# autonuma

在 NUMA 系统中,内存被分为多个节点,每个节点与一组 CPU 物理上靠近。访问同一节点上的内存(本地访问)的延迟和带宽优于访问其他节点上的内存(远程访问).如果内存分配不当,会导致性能下降。

因此我们希望, 操作系统可以根据不同 NUMA 节点的 CPU 和内存访问模式自动选择最优的策略, 将内存迁移到同 CPU 的 NUMA node 中以获取最大性能

**AutoNUMA**(Automatic Non-Uniform Memory Access)是 Linux 内核的一项特性,用于在 NUMA(非一致性内存访问)架构下自动优化内存的分配和访问,以提高多核系统的性能。它通过监测进程的内存访问模式,动态调整内存和 CPU 的绑定策略,从而减少远程内存访问的开销。它目标是:
1. **动态内存迁移**:将数据从远程内存节点迁移到本地节点,以减少远程访问的延迟。
2. **CPU 任务调度**:将任务迁移到靠近其内存的节点上的 CPU,以优化内存访问。
3. **自动化和透明化**:无需应用程序开发人员进行额外配置,系统内核自动完成优化。

## 工作机制

AutoNUMA 的工作主要分为以下三个阶段:

1. **内存访问跟踪**:
   任务扫描器定期扫描任务地址空间的一部分, 并在内存页面上设置 `PROT_NONE` 权限(即不允许访问).
   
2. **分析访问模式**:
   当应用程序访问这些页面时,触发 NUMA 页面提示错误(numa hint fault),内核记录下访问该页面的 CPU 和内存节点。根据记录的数据,判断页面和任务的访问频率与位置关系。
   
3. **优化迁移**:
   - 如果页面频繁被某个节点的 CPU 访问,内核会将页面迁移到该节点。
   - 如果任务频繁访问远程节点的内存,内核会尝试将任务迁v  移到内存所在节点的 CPU 上。

## 迁移与回收

内核使用称为"NUMA 提示错误"的技术来检测是否需要迁移给定页面。任务地址空间的范围会定期取消映射,以便后续访问该范围内的页面将触发页面错误。当发生页面错误时,内存管理子系统可以使用触发页面错误的CPU的位置来确定页面是否需要迁移到包含该CPU的节点。完全没有故障表明页面正在变冷,并且可能在回收期间迁移到慢速层节点。随着工作负载的运行和访问模式的变化,页面在热和冷之间转换,并相应地在快速和慢速 NUMA 节点之间迁移。

内存回收由"水印"系统驱动,该系统尝试保留至少最少数量的可用空闲页面。当请求分配时,内核会将进行分配的节点中的空闲页面数量与区域水印阈值进行比较。如果分配后节点中的空闲页数低于水印指定的阈值,则 kswapd 唤醒内核线程以异步扫描并从节点回收页面。这允许在节点中的内存压力导致分配阻塞和直接回收发生之前抢先释放内存。

内核中的区域水印根据主机的内存配置文件静态调整大小。内存较少的系统的区域水印较低,而较大系统的水印则较高。直观上,这种缩放是有道理的。如果您的计算机具有大量内存,则回收可能应该比内存很少的计算机更早触发,因为预期应用程序会在拥有更多内存的系统上更积极地请求内存。然而,静态阈值也有缺点。在分层内存系统的上下文中,如果节点的阈值太低,则快速节点可能无法足够积极地回收,并且将没有可用空间来提升来自慢速层节点的热页。

## 内核配置

可以通过以下方式查看和调整 AutoNUMA 的设置:

```bash
echo 1 > /proc/sys/kernel/numa_balancing  # 启用
echo 0 > /proc/sys/kernel/numa_balancing  # 禁用
```

## 参考

- [autoNUMA学习记录-20230302](https://blog.csdn.net/u010503464/article/details/129307084)
- [Automatic Non-Uniform Memory Access (NUMA) Balancing](https://documentation.suse.com/sles/12-SP5/html/SLES-all/cha-tuning-numactl.html)
- [Automatic NUMA Balancing](https://docs.redhat.com/en/documentation/red_hat_enterprise_linux/7/html/virtualization_tuning_and_optimization_guide/sect-virtualization_tuning_optimization_guide-numa-auto_numa_balancing#sect-Virtualization_Tuning_Optimization_Guide-NUMA-Auto_NUMA_Balancing)
- [NUMA rebalancing on tiered-memory systems](https://lwn.net/Articles/893024/)