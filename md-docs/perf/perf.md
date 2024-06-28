
# perf

```bash
cd tools/perf
```

```bash
sudo apt install libtraceevent-dev

## 如果有 conda 环境退出, 因为编译过程依赖了 libpython
## 要么手动加上 export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:<lib path>
# conda deactivate

make -j`nproc`
```

编译的时候会提示缺少什么依赖

![20240626153915](https://raw.githubusercontent.com/learner-lu/picbed/master/20240626153915.png)

编译之后需要手动 cp 到一个全局路径中

```bash
sudo cp perf /usr/local/bin
```

## 使用

假设有如下程序 `a.c`, 矩阵乘

```c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#define N 1024

double A[N][N];
double B[N][N];
double C[N][N];


int main(){
    /* init srand */
    srand((unsigned)time(NULL));

    /* init matrix */
    for(int i = 0 ; i < N ; ++i){
        for(int j = 0 ; j < N ; ++j){
            A[i][j] = rand()/32767.0;
            B[i][j] = rand()/32767.0;
            C[i][j] = 0;
        }
    }

    /* mult matrix */
    for(int i = 0 ; i < N ; ++i){
        for(int j = 0 ; j < N ; ++j){
            for(int k = 0 ; k < N ; ++k){
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }

    return 0;
}
```

```bash
gcc -g a.c -o a
perf stat ./a
```

![20240627144243](https://raw.githubusercontent.com/learner-lu/picbed/master/20240627144243.png)

- **task-clock:u**: 程序在用户模式下运行的总时间,以毫秒为单位.这里表示程序运行了 3683.33 毫秒.
- **context-switches:u**: 程序执行期间发生的上下文切换次数.这里为 0,表示在程序运行期间没有发生上下文切换.
- **cpu-migrations:u**: 程序执行期间 CPU 迁移(从一个 CPU 核迁移到另一个 CPU 核)的次数.这里为 0,表示没有发生 CPU 迁移.
- **page-faults:u**: 程序执行期间发生的页面错误次数.这里为 576 次.
- **cycles:u**: 程序执行期间的 CPU 周期数.这里为 16,400,517,033 个周期.
- **instructions:u**: 程序执行期间的指令数.这里为 44,199,451,790 条指令.
- **insn per cycle**: 每个周期执行的指令数.这里为 2.70,表示每个 CPU 周期平均执行了 2.70 条指令.
- **branches:u**: 程序执行期间的分支指令数.这里为 1,111,320,947 条分支指令.
- **branch-misses:u**: 程序执行期间未命中的分支预测数.这里为 1,126,808 次,约占总分支数的 0.10%.

TMA 指 Topdown Microarchitecture Analysis

- **tma_backend_bound**: 后端受限的时间比例,表示 CPU 在等待内存、缓存等资源的时间.这里为 17.7%.
- **tma_bad_speculation**: 错误推测的时间比例,表示由于错误的分支预测导致的时间浪费.这里为 70.9%,这是一个很高的比例,表明大量时间浪费在错误的分支预测上.
- **tma_frontend_bound**: 前端受限的时间比例,表示 CPU 等待获取指令的时间.这里为 0.0%.
- **tma_retiring**: 正常执行指令的时间比例,表示 CPU 实际在执行指令的时间.这里为 11.4%.

其他信息

- **seconds time elapsed**: 程序执行的实际时间.这里为 3.684550651 秒.
- **user**: 程序在用户模式下的执行时间.这里为 3.683721000 秒.
- **sys**: 程序在内核模式下的执行时间.这里为 0.000000000 秒.

从输出中可以看出:
- 程序的执行时间主要在用户模式下.
- 程序没有上下文切换和 CPU 迁移.
- 发生了一些页面错误.
- 每个周期执行了较多的指令,指令周期比(IPC)为 2.70,表明 CPU 效率较高.
- 错误分支预测的比例较高(70.9%),这可能会影响性能.

总体而言,程序的性能受到了错误分支预测的较大影响,可能需要优化分支预测或调整代码结构以减少错误分支预测的发生.

## 参考

- [Perf的安装与简单使用](https://blog.csdn.net/qq_48201696/article/details/126381924)