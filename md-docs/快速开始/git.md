
# git

git 基本用法这里不再赘述, 主要介绍一些少见的和 kernel 社区有关的 git 用法

建议安装两个 Vscode 插件: [Git History](https://marketplace.visualstudio.com/items?itemName=donjayamanne.githistory) 和 [GitLens](https://marketplace.visualstudio.com/items?itemName=eamodio.gitlens), 很好用, 可以节省记忆命令的时间, 对比起来也很直观

## 检查某一个修改

要查看某一个特定提交(commit)的所有修改

```bash
git show <commit-hash>
```

> hash 不需要全部的 40 位, 正常来说前 6 位就可以区分

![20240701101448](https://raw.githubusercontent.com/learner-lu/picbed/master/20240701101448.png)

如果只想看修改了哪些文件名

```bash
$ git diff-tree --no-commit-id --name-only -r bd94d86f
arch/x86/kernel/tsc_sync.c
```

## 将 commit 打包为 patch

首席按确定你要打包的 commit 范围.你可以使用 `git log` 命令查看 commit 历史

然后使用 `format-patch` 来进行打包, 比如打包最后一次提交的 commit

```bash
git format-patch -1
```

打包某一个特定的 commit 可以使用

```bash
git format-patch -1 <commit-hash>
```

如果你想打包一段特定范围内的 commit,可以使用以下方式:

```bash
git format-patch <start-commit>..<end-commit>
```

> 例如,要打包从 `commit1` 到 `commit2` 的所有 commit,可以使用:
> 
> ```bash
> git format-patch commit1..commit2
> ```

**打包多个 commit 时会为每一次 commit 都创建一个 patch**, 如果想要将多个 commit 合并打包为一个 patch 可以使用

```bash
git format-patch -2 --stdout > a.patch
```

如果你想将生成的 patch 文件保存到特定目录,可以使用 `-o` 选项.例如,将 patch 文件保存到 patches 目录:

```bash
git format-patch -2 -o patches
```

## 下载 patch

```bash
sudo apt install patatt b4
```

所有的 patch 都可以在 [kernel lore](https://lore.kernel.org/lkml/) 或者 [kernel patchwork](https://patchwork.kernel.org/project/linux-trace-kernel/list/) 找到

```bash
b4 am <url>
```

## 合并 patch

```bash
git-apply --whitespace=error-all <patch>
```

```bash
patch -p1 < ./xxx.mbx
```

```bash
git clean -fd
```

## 发送邮件

```bash
sudo apt update
sudo apt install git-email
```

[how to configure and use git send email to work with gmail to email patches to](https://stackoverflow.com/questions/68238912/how-to-configure-and-use-git-send-email-to-work-with-gmail-to-email-patches-to)

[how to solve unable to initialize smtp properly when using using git send ema](https://stackoverflow.com/questions/28038662/how-to-solve-unable-to-initialize-smtp-properly-when-using-using-git-send-ema)

## 参考

- [如何从linux社区下载和合入内核patch?](https://blog.csdn.net/pengdonglin137/article/details/131148344)
- [kernel patch](https://unix.stackexchange.com/questions/80519/how-do-i-get-a-linux-kernel-patch-set-from-the-mailing-list)