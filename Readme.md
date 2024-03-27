# Title

## 实时性措施

### 实时线程

1. `Preepmt RT`
2. 内存页锁 `mlockall(MCL_CURRENT | MCL_FUTURE)`
3. 调度、优先级设定 `SCHED_FIFO`

    ```c++
    /* Set scheduler policy and priority of pthread */
    ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    if (ret) {
        s_logger->critical("pthread setschedpolicy failed: {}", ret);
        return false;
    }
    param.sched_priority = 99;
    ret = pthread_attr_setschedparam(&attr, &param);
    if (ret) {
        s_logger->critical(
            "pthread setschedparam failed, priority: {}, ret: {}",
            param.sched_priority, ret);
        return false;
    }
    /* Use scheduling parameters of attr */
    ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    if (ret) {
        s_logger->critical("pthread setinheritsched failed: {}", ret);
        return false;
    }
    ```

### cpu_dma_latency

1. 设置cpu_dma_latency (**要注意不要在写入后关闭文件描述符**)

    ```c++
    /* 消除系统时钟偏移函数，取自cyclic_test */
    struct stat s;
    int ret;
    if (stat("/dev/cpu_dma_latency", &s) == 0) {
        latency_target_fd_ = open("/dev/cpu_dma_latency", O_RDWR);
        if (latency_target_fd_ == -1) {
            s_logger->error("open /dev/cpu_dma_latency failed");
            return false;
        }
        ret = write(latency_target_fd_, &latency_target_value_, 4);
        if (ret <= 0) {
            s_logger->error("# error setting cpu_dma_latency to {}!: {}\n",
                            latency_target_value_, strerror(errno));
            close(latency_target_fd_);
            return false;
        }
        //! 不可以在这里关闭!!, 不然就直接无效了
        // close(latency_target_fd_);
        s_logger->info("# /dev/cpu_dma_latency {} set to {}us\n",
                    latency_target_fd_, latency_target_value_);
        return true;
    } else {
        return false;
    }

    ```

### CPU 隔离(利用 LINUX 启动项):

1. 将 CPU2,CPU3（从 0 计数, 2 和 3 应该同一个物理核）隔离在系统调度范围外 `isolcpus=2-3`
2. 减少本地 CPU 时钟中断 (Full Dynamic Tick) `nohz_full=2-3`
3. 从引导选择的CPU上卸载RCU回调处理 (Offload RCU callback) `rcu_nocbs=2-3`

    ```bash
    sudo vim /etc/default/grub

    # 添加这些启动项
    GRUB_CMDLINE_LINUX_DEFAULT="quiet splash isolcpus=2-3 nohz_full=2-3 rcu_nocbs=2-3"
    GRUB_CMDLINE_LINUX=""
    ```

4. 在程序中, 设置实时 rt 线程的线程亲和性为已隔离的CPU编号

    ```c++

    /* Set cpu affinity */
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(2, &mask);
    CPU_SET(3, &mask);
    ret = pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &mask);
    if (ret) {
        s_logger->critical("pthread_attr_setaffinity_np failed: {}", ret);
        return false;
    }

    ```



### 其他配置

1. 中断CPU隔离, 启动项增加 `irqaffinity=0-1`

2. 禁用irqbanlance

    ```bash
    systemctl stop irqbalance.service
    systemctl disable irqbalance.service
    ```

    必要的话直接卸载irqbalance。

    ```bash
    apt-get remove irqbalance
    ```

    x86平台还可添加参数acpi_irq_nobalance禁用ACPI irqbalance. `acpi_irq_nobalance noirqbalance`

3. intel 核显配置[x86]

    主要针对intel CPU的核显，配置intel核显驱动模块i915，防止集成显卡更改电源状态，内核参数如下。

    ```bash
    GRUB_CMDLINE_LINUX="i915.enable_rc6=0 i915.enable_dc=0 i915.disable_power_well=0  i915.enable_execlists=0 i915.powersave=0"
    ```

4. 禁用调频

    `cpufreq.off=1` 禁用调频，调频影响实时性详见后文。

5. hugepages

    使用大页有利于降低缺页异常和提高tlb命中率、以减少内存分配的频率，这里设置一次分配 1GB 的 RAM 块。
    ```bash
    GRUB_CMDLINE_LINUX="hugepages=1024"
    ```

    防止基于任务处理器相关性移动缓存
    ```bash
    GRUB_CMDLINE_LINUX="numa_balancing=disable"
    ```

6. nmi_watchdog[x86]

    NMI watchdog是Linux的开发者为了debugging而添加的特性，但也能用来检测和恢复Linux kernel hang，现代多核x86体系都能支持NMI watchdog。

    NMI（Non Maskable Interrupt）即不可屏蔽中断，之所以要使用NMI，是因为NMI watchdog的监视目标是整个内核，而内核可能发生在关中断同时陷入死循环的错误，此时只有NMI能拯救它。

    Linux中有两种NMI watchdog，分别是I/O APIC watchdog（nmi_watchdog=1）和Local APIC watchdog（nmi_watchdog=2）。它们的触发机制不同，但触发NMI之后的操作是几乎一样的。一旦开启了I/O APIC watchdog（nmi_watchdog=1），那么每个CPU对应的Local APIC的LINT0线都关联到NMI，这样每个CPU将周期性地接到NMI，接到中断的CPU立即处理NMI，用来悄悄监视系统的运行。如果系统正常，它啥事都不做，仅仅是更改 一些时间计数；如果系统不正常（默认5秒没有任何普通外部中断），那它就闲不住了，会立马跳出来，且中止之前程序的运行。该出手时就出手。

    避免周期中断的NMI watchdog影响xenomai实时性需要关闭NMI watchdog，传递内核参数`nmi_watchdog=0`.
    禁用超线程，传递内核参数`noht`.

7. nosoftlockup

    linux内核参数，禁用 soft-lockup检测器在进程在 CPU 上执行的时间超过 softlockup 阈值（默认 120 秒）时禁用回溯跟踪日志记录。 `nosoftlockup`

8. CPU特性[x86]

    intel处理器相关内核参数：

    `nosmap`

    `nohalt`
    告诉内核在空闲时,不要使用省电功能PAL_HALT_LIGHT。 这增加了功耗。但它减少了中断唤醒延迟，这可以提高某些环境下的性能，例如联网服务器或实时系统。

    `mce=ignore_ce`,忽略machine checkerrors (MCE).

    `idle=poll,`不要使用HLT在空闲循环中进行节电，而是轮询以重新安排事件。 这将使CPU消耗更多的功率，但对于在多处理器基准测试中获得稍微更好的性能可能很有用。 它还使使用性能计数器的某些性能分析更加准确。

    `clocksource=tsc tsc=reliable`,指定tsc作为系统clocksource.

    `intel_idle.max_cstate=0` 禁用intel_idle并回退到acpi_idle.

    `processor.max_cstate=0 intel.max_cstate=0 processor_idle.max_cstate=0` 限制睡眠状态c-state。

    `hpet=disable clocksource=tsc tsc=reliable` 系统clocksource相关配置。

### 最终启动命令参数

只限制rt线程在cpu2:

```bash

GRUB_CMDLINE_LINUX="audit=0 isolcpus=2 nohz_full=2 rcu_nocbs=2 irqaffinity=0,1 
    rcu_nocb_poll=1024 rcupdate.rcu_cpu_stall_suppress=1 acpi_irq_nobalance 
    numa_balancing=disable cpufreq.off=1 nosmap nosmt  nohalt  noirqbalance hugepages=1024  
    i915.enable_rc6=0 i915.enable_dc=0 i915.disable_power_well=0  i915.enable_execlists=0  
    nmi_watchdog=0 nosoftlockup processor.max_cstate=0 intel.max_cstate=0 
    processor_idle.max_cstate=0 intel_idle.max_cstate=0 clocksource=tsc tsc=reliable 
    nmi_watchdog=0 noht nosoftlockup intel_pstate=disable idle=poll nohalt nosmap  
    mce=ignore_ce hpet=disable clocksource=tsc tsc=reliable"

```