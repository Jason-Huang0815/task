# Git 场景手册（GitHub）

`README.md` 第 7 节是零基础教程；本文是贴在工位上的速查表。

---

## 1. 一分钟速查

| 目的 | 命令 |
| --- | --- |
| 看状态 | `git status` |
| 看改动 | `git diff` |
| 暂存全部 | `git add -A` |
| 提交 | `git commit -m "feat(x): ..."` |
| 拉取并变基 | `git pull --rebase` |
| 推送 | `git push` |
| 新建分支 | `git switch -c feature/xxx` |
| 切分支 | `git switch main` |
| 看历史 | `git log --oneline --graph --decorate -20` |
| 看远程 | `git remote -v` |

---

## 2. 场景

### 2.1 第一次推送到 GitHub

```bash
git init -b main
git add .
git commit -m "chore: 初始化项目骨架"
git remote add origin https://github.com/<user>/task.git
git branch -M main
git push -u origin main
```

### 2.2 队友先推了，我推不上去

```bash
git pull --rebase
# 解决冲突（见 2.3）
git push
```

### 2.3 解决冲突

```bash
git status                 # 看哪些文件冲突
# 打开文件，保留正确内容，删掉 <<<<<<< ======= >>>>>>>
git add <file>
git rebase --continue
```

想放弃：`git rebase --abort`。

### 2.4 提交到一半发现分支错了

```bash
git stash              # 暂存改动
git switch -c feature/right-branch
git stash pop
```

### 2.5 刚提交就发现写错了

```bash
git reset --soft HEAD~1   # 只撤 commit，改动还在
# 改完重新 add/commit
```

已经 push 了则用 `git revert <hash>`，不要改写公共历史。

### 2.6 误删文件想找回

```bash
git restore <file>        # 从暂存区/HEAD 恢复
git reflog                # 找到丢失的提交
git switch -c rescue <hash>
```

### 2.7 忘记拉取就改了同一文件

```bash
git stash
git pull --rebase
git stash pop
# 手动合并冲突
```

### 2.8 仓库里混进了 build/

```bash
git rm -r --cached build
git commit -m "chore: 移除误提交的构建目录"
```

如果历史里已经有大文件，需要 `git filter-repo` 清理，先找老师确认。

---

## 3. GitHub 设置

- **协作者**：Settings → Collaborators → Add people。
- **保护 main**：Settings → Branches → Add rule → Require PR + 1 approval。
- **PAT**：Settings → Developer settings → Personal access tokens → Fine-grained → 仓库 Contents 读写。
- **SSH**（可选）：`ssh-keygen -t ed25519 -C "邮箱"`，把 `~/.ssh/id_ed25519.pub` 加到 GitHub → SSH keys。

---

## 4. 不要做的事

1. 提交 `build/`、`sdkconfig`、`.idea/`、`managed_components/`。
2. `git push --force` 到 `main`。
3. 把 token、Wi-Fi 密码、身份证号写进仓库。
4. 用 `git add .` 之前不看 `git status`。
