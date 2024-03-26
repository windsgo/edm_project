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
2. 减少本地 CPU 时钟中断 `nohz_full=2-3`

    ```bash
    sudo vim /etc/default/grub

    # 添加这些启动项
    GRUB_CMDLINE_LINUX_DEFAULT="quiet splash isolcpus=2-3 nohz_full=2-3 rcu_nocbs=2-3"
    GRUB_CMDLINE_LINUX=""
    ```

3. 在程序中, 设置实时 rt 线程的线程亲和性为已隔离的CPU编号

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

4. 中断CPU隔离 *TODO*

    目前的测试结果是不明显，而且每次开机要重新设置。
