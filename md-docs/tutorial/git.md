
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
git format-patch -s --subject-prefix='PATCH' -1
```

如果是 v2 版本可以使用

```bash
git format-patch -s --subject-prefix='PATCH' -1 -v2
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

合并一个 patch 可以用三种方式, `git apply`, `git am` 和 `patch`, 它们略有区别

git apply 不会创建新的 commit, 只是接受 `git diff` 的输出然后将其应用于当前目录

git am 会**直接创建一个新的 commit**, 接受的文件通常是通过 git format-patch 创建的 `mailbox` 类型的文件

可以使用如下命令合并一个 patch

```bash
git-apply <patch>
```

通常来说合并的时候会出现一些关于 whitespace 的 warning, 只是因为它检测到了 trailing whitespace 或者空行, 但是这并不会影响合并的正确性, 可以使用如下的命令忽略掉所有的 warning

```bash
git config --global apply.whitespace nowarn
```

> [what does 1 line adds whitespace errors mean when applying a patch](https://stackoverflow.com/questions/12396622/what-does-1-line-adds-whitespace-errors-mean-when-applying-a-patch)

笔者个人更推荐使用 patch 来进行合并

```bash
patch -p1 < ./xxx.mbx
```

## 查看两个文件夹的差异

一个常见的需求是一个项目基于某一个版本的内核修改, 但是并没有留下明确的 commit 可以拆分出来, 因此需要对于当前代码和源代码的差异, 需要对比两个文件夹的差别

假设项目基于 linux-6.6 代码修改, 则需要先下载 linux-6.6 的源码并初始化一个 git 仓库, 假设为 linux-6.6/

要对比的项目位于 code/project-linux

```bash
diff -Nur linux-6.6 code/project-linux > changes.patch
```

> [!NOTE]
> **顺序不可以颠倒!**, `linux-6.6` 为对比基准文件夹; `code/project-linux` 为对比的目标文件夹
> 
> -r 表示递归对比子文件夹, -u 表示生成 unified diff 格式
>
> -N 表示处理新增文件或文件夹(在 `linux-6.6` 中不存在而在 `linux-6.6-host` 中新增的文件会被包含在补丁中)

此时便可以提取出修改的部分了, 要将 `changes.patch` 应用到 `linux-6.6` 文件夹,可以使用以下命令:

```bash
patch -p1 -d linux-6.6 < changes.patch
```

## 版本切换

切换到某一个大版本

```bash
git checkout v5.15
```

清除当前所有暂存区的文件并回退到 HEAD

```bash
git reset --hard
```

回退到 HEAD 之前一个版本

```bash
git reset --hard HEAD^
```

清除暂存区中所有新建的文件/文件夹

```bash
git clean -fd
```

如果还希望保留所有暂存区的修改, 可以使用 `--soft`

```bash
git reset --soft <commit-hash>
```

## 发送邮件

> 初次配置邮件发送需要一个相对繁琐的过程, 希望读者有耐心配置完成并成功发送你的第一封邮件

大部分从软件源安装的 git 没有安装发送 email 的功能, 需要先安装一下

```bash
sudo apt update
sudo apt install git-email
```

然后需要配置一下 git 发送邮件使用 STMP 服务, 打开 `~/.gitconfig`, 填写对应的 gmail 邮箱信息

```txt
[sendemail]
  smtpServer = smtp.gmail.com
  smtpUser = luzhixing12345@gmail.com
  smtpPass = xxx # 先不填
  smtpServerPort = 587
  smtpEncryption = tls
  chainreplyto = false
[credential]
  helper = store
```

**其中 passwd 先不要填写你的邮箱对应的密码**, 因为现在谷歌使用 2FA(2-factor auth)的方式进行身份认证, 所以即使使用邮箱密码也没有办法正常登录. 其他的部分直接照抄即可, user 改为你的邮箱地址

进入 [google account](https://myaccount.google.com/security) 开启 2FA 认证

![20240702104333](https://raw.githubusercontent.com/learner-lu/picbed/master/20240702104333.png)

使用 google 账号为 app 创建单独的密码: [appasswords](https://myaccount.google.com/apppasswords)

> 设备名字叫做 `git send-email`

![20240702104500](https://raw.githubusercontent.com/learner-lu/picbed/master/20240702104500.png)

创建完成之后会返回给你一个 16 位的密码, 完善 `.gitconfig` 的 passwd, **注意复制之后去掉中间的空格, 密码只有 16 位**

```txt
[sendemail]
  smtpServer = smtp.gmail.com
  smtpUser = luzhixing12345@gmail.com
  smtpPass = **YOUR-2FA-PASSWD**       <--- 将 16 位数字填写到这里
  smtpServerPort = 587
  smtpEncryption = tls
  chainreplyto = false
[credential]
  helper = store
```

至此就完成了发送邮件的基础配置, 现在来尝试编写一封邮件, 新建一个 `email.txt`, 写入如下内容

```txt
Subject: Hello world

nice to meet you by email, my friend

Thanks,
Lu, zhixing
```

**注意发送的邮件需要一定的格式**(mailbox): 开头的 `Subject` 必须有, 为邮件标题内容. 正文内容需要在 Subject 之后 **换行** 编写.~~不要问我为什么知道, 因为我被坑了~~, 结尾最好加上你的名字, 以及 "Thanks", "Best wishes" 等

现在先给自己发一封邮件吧

```bash
git send-email --to=luzhixing12345@gmail.com email.txt
```

可以成功收到邮件

![20240702110008](https://raw.githubusercontent.com/learner-lu/picbed/master/20240702110008.png)

如果不行可以试试在邮箱设置中启用 IMAP

![20240702110105](https://raw.githubusercontent.com/learner-lu/picbed/master/20240702110105.png)

[kernel lore](https://lore.kernel.org/) 的官方邮件列表下面也给出了如何回复某一个帖子的方式, 比如对于这个 [issue](https://lore.kernel.org/linux-cxl/6a07125f-e720-404c-b2f9-e55f3f166e85@fujitsu.com/), 可以划到最下面看到回复的方式

```bash
git send-email \
    --in-reply-to=6a07125f-e720-404c-b2f9-e55f3f166e85@fujitsu.com \
    --to=lizhijian@fujitsu.com \
    --cc=akpm@linux-foundation.org \
    --cc=dan.j.williams@intel.com \
    --cc=david@redhat.com \
    --cc=linux-cxl@vger.kernel.org \
    --cc=linux-mm@kvack.org \
    --cc=osalvador@suse.de \
    --cc=y-goto@fujitsu.com \
    /path/to/YOUR_REPLY
```

其中 `/path/to/YOUR_REPLY` 就是你回复的文件内容, 如果要回复一个讨论的纯文本内容的邮件那么直接按照格式编写并发送即可

如果要编写带代码部分的 patch 回复那么需要先使用 `git format-patch` 创建一个 patch 然后发送该 patch 即可, 创建的 patch 默认就是 mailbox 格式的, 默认的正文内容就是你的 commit message, 可以修改为你想要编辑的邮件内容

## 查看 tag

要查看某个特定的提交(commit)属于 Linux 内核的哪个版本

- git describe 命令可以帮助你确定某个 commit 距离最近的标签(版本)有多远

  ```bash
  git describe --contains <commit-hash>
  ```

- git tag --contains 命令列出所有包含指定 commit 的标签.你可以使用以下命令来查看一个 commit 是在哪些标签(版本)中存在的

  ```bash
  git tag --contains <commit-hash>
  ```

## 查看 log

要在 Git 的提交日志中查找包含特定信息的所有提交记录

- 查找提交消息中包含特定关键词的所有提交

  ```bash
  git log --grep="fix bug"
  ```

- 查看 Git 仓库中某个人或某个邮箱的提交

  ```bash
  git log --author="作者的名字"
  ```

  查看提交中的变更可以添加 `-p`

  查看更详细的内容可以结合 `--pretty` 选项

- 如果需要按时间顺序, 即从过去到现在, 可以使用 --reverse, 不过搜索时间会比较长

  例如查看 MGLRU 的修改

  ```bash
  git log --author="yuzhao@google.com" --grep="MGLRU" --reverse
  ```


## bisect

`git bisect` 是 Git 中的一个非常有用的命令,用于在代码仓库中查找引入特定 bug 或问题的提交.它通过二分查找法有效地缩小问题的范围,帮助开发者快速定位到导致问题的具体提交.以下是 `git bisect` 的基本工作原理和使用步骤:

1. **标记已知的良好和有问题的提交**:你需要告诉 Git 一个已知的良好(没有问题)的提交和一个有问题的(包含 bug 的)提交.
2. **二分查找**:Git 会在这两个提交之间选择一个中间的提交,并切换到这个中间提交.
3. **测试中间提交**:你需要在这个中间提交上运行你的测试或检查代码,来判断这个提交是"良好"还是"有问题".
4. **根据测试结果继续二分查找**:根据测试结果,Git 会在剩下的提交中选择另一个中间提交进行测试.这个过程会持续进行,直到找到引入 bug 的具体提交.

5. **启动 bisect**:首先,启动 `git bisect` 模式.
    ```bash
    git bisect start
    ```
6. **标记有问题的提交**:
    ```bash
    git bisect bad [有问题的提交]
    ```
    如果当前工作目录已经在有问题的提交上,可以省略提交 hash:
    ```bash
    git bisect bad
    ```
7. **标记已知良好的提交**:
    ```bash
    git bisect good [良好的提交]
    ```
8. **Git 会自动切换到一个中间提交**.你需要在这个提交上运行测试或检查代码,判断这个提交是"良好"还是"有问题".
    - 如果这个提交是有问题的:
        ```bash
        git bisect bad
        ```
    - 如果这个提交是良好的:
        ```bash
        git bisect good
        ```
9. **重复步骤 4**,直到找到引入 bug 的具体提交.

10. **结束 bisect**:一旦找到了引入 bug 的提交,结束 bisect 模式:
    ```bash
    git bisect reset
    ```

假设你知道当前的提交有问题,并且你知道一个早期的提交是好的:

```bash
# 启动 bisect 模式
git bisect start

# 标记当前提交有问题
git bisect bad

# 标记一个已知良好的提交
git bisect good abc1234

# 根据提示,测试中间的提交并标记结果
# 假设中间提交是良好的
git bisect good

# Git 会继续选择中间提交,重复测试和标记,直到找到引入 bug 的提交

# 结束 bisect 模式
git bisect reset
```

通过 `git bisect`,可以快速有效地定位引入问题的提交,极大地节省了调试时间.

## reset

使用 `git reset HEAD^` 后,你会将当前分支的 HEAD 重置到它的上一个提交.这样会导致你丢失之后的所有提交.如果你想回到最开头的位置(即最初的提交),你有几种方法可以实现:

1. **使用具体的提交 ID**:如果你知道最初提交的哈希值,可以使用以下命令:
    ```bash
    git reset --hard <commit-hash>
    ```
   你可以通过 `git log` 找到最初提交的哈希值.

2. **使用 `--soft` 和 `--hard` 参数**:
   - `--soft` 只会重置 HEAD 指向的提交,不会改变工作目录和暂存区的内容.
   - `--hard` 会重置 HEAD 并清理工作目录和暂存区,使其与指定的提交完全一致.

3. **回到最初的提交**:
   - 查找最初的提交哈希值:
     ```bash
     git rev-list --max-parents=0 HEAD
     ```
   - 然后重置到该提交:
     ```bash
     git reset --hard <first-commit-hash>
     ```

4. **使用 `reflog` 回到之前的状态**:
   如果你在使用 `git reset HEAD^` 之前有提交过其他的更改,你可以通过 `git reflog` 来查看 HEAD 的历史位置,然后重置到那个位置.例如:
   ```bash
   git reflog
   ```
   找到你想要回到的那个位置的哈希值,然后执行:
   ```bash
   git reset --hard <commit-hash>
   ```

举个例子,假设你的最初提交哈希值是 `a1b2c3d4`,那么你可以执行:
```bash
git reset --hard a1b2c3d4
```

这样,你就回到了最初的位置,丢弃了所有在此之后的提交和更改.请注意,使用 `--hard` 选项会丢失所有未提交的更改和提交,因此请确保在执行此操作之前备份任何重要的更改.

## Git error "object file ... is empty"?

```bash
~/workspace$ git status
error: object file .git/objects/77/3ce9d2d96bcdb909c1b32e5549a5965f1d30ed is empty
error: object file .git/objects/77/3ce9d2d96bcdb909c1b32e5549a5965f1d30ed is empty
fatal: bad object HEAD
```

[how can i fix the git error object file is empty/12371337#12371337](https://stackoverflow.com/questions/11706215/how-can-i-fix-the-git-error-object-file-is-empty/12371337#12371337)

先试一下

```bash
git pull
```

```bash
find . -type f -empty -delete -print
git fsck --full
tail -n 2 .git/logs/refs/heads/main
git show hash
```

## 查看最后一次提交

```bash
git reset --soft HEAD^
git restore --staged .
```

## revert

要撤回某个较早的提交,但保留其后的提交不变,可以使用 Git 的 revert 命令.revert 会生成一个新的提交,该提交会撤销指定的旧提交的更改,而不会修改提交历史

```bash
git log --oneline
git revert <commit-hash>
```

这会生成一个新的提交,撤销该提交中的所有更改,但保留后续的提交记录

如果该提交对多个文件进行了更改,git revert 会在所有这些文件上应用反向更改

## 参考

- [如何从linux社区下载和合入内核patch?](https://blog.csdn.net/pengdonglin137/article/details/131148344)
- [kernel patch](https://unix.stackexchange.com/questions/80519/how-do-i-get-a-linux-kernel-patch-set-from-the-mailing-list)
- [how to configure and use git send email to work with gmail to email patches to](https://stackoverflow.com/questions/68238912/how-to-configure-and-use-git-send-email-to-work-with-gmail-to-email-patches-to)
- [how to solve unable to initialize smtp properly when using using git send ema](https://stackoverflow.com/questions/28038662/how-to-solve-unable-to-initialize-smtp-properly-when-using-using-git-send-ema)
- [Setup-git-send-email](https://gist.github.com/winksaville/dd69a860d0d05298d1945ceff048ce46)
- [lifeislife 手把手教你向开源社区提Patch](https://lifeislife.cn/2022/11/20/%E6%89%8B%E6%8A%8A%E6%89%8B%E6%95%99%E4%BD%A0%E5%90%91%E5%BC%80%E6%BA%90%E7%A4%BE%E5%8C%BA%E6%8F%90Patch/)
- [lifeislife 如何使用git-send-mail给开源社区提交Patch](https://lifeislife.cn/2022/09/28/%E5%A6%82%E4%BD%95%E4%BD%BF%E7%94%A8git-send-mail%E7%BB%99%E5%BC%80%E6%BA%90%E7%A4%BE%E5%8C%BA%E6%8F%90%E4%BA%A4Patch/)