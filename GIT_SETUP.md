# Git Setup and Push Guide

This guide walks you through testing, committing, and pushing your CUBIT Streaming System to GitHub.

## Step 1: Pre-Push Testing

Before pushing to Git, run the automated test suite:

```bash
cd /Users/say/Desktop/Streaming_System/cubit-streaming-system
./scripts/pre_push_test.sh
```

This will:
- Verify all files are present
- Test that the project builds
- Check documentation is complete
- Ensure no build artifacts are included
- Validate configuration files

**Expected Output**: "All tests passed! Ready to commit and push to Git"

## Step 2: Initialize Git Repository

If not already initialized:

```bash
cd /Users/say/Desktop/Streaming_System/cubit-streaming-system
git init
```

This creates a `.git` directory in your project.

## Step 3: Review What Will Be Committed

Check which files will be added:

```bash
git status
```

You should see:
- Green: All your source files, docs, scripts
- Red (if any): Build artifacts (these should be in .gitignore)

Review the .gitignore:

```bash
cat .gitignore
```

Make sure `build/` is listed.

## Step 4: Stage All Files

Add all files to staging:

```bash
git add .
```

Verify what's staged:

```bash
git status
```

You should see all files in green (staged for commit).

## Step 5: Create Initial Commit

Commit with a meaningful message:

```bash
git commit -m "Initial commit: Low-latency video streaming system v1.0.1

- Multi-threaded video capture and encoding
- Hardware-accelerated encoding (NVENC) with CPU fallback
- UDP streaming with packet fragmentation
- Client registration mechanism
- Adaptive bitrate control
- GPU monitoring via NVML
- Docker support with GPU passthrough
- Configuration file support

Developed during internship at CUBIT (Center for Understanding Biology
using Imaging Technology) at Stony Brook University."
```

Verify commit was created:

```bash
git log --oneline
```

## Step 6: Create GitHub Repository

### Option A: Via GitHub Website

1. Go to https://github.com
2. Click the "+" icon (top right) → "New repository"
3. **Repository name**: `cubit-streaming-system` (or your choice)
4. **Description**: "Low-latency video streaming system for biomedical microscope cameras"
5. **Visibility**:
   - Public (if you want it on your resume/portfolio)
   - Private (if for personal use only)
6. **DO NOT** initialize with README, .gitignore, or license (we already have these)
7. Click "Create repository"

### Option B: Via GitHub CLI (if installed)

```bash
gh repo create cubit-streaming-system --public --source=. --remote=origin --push
```

This automatically creates repo, adds remote, and pushes.

## Step 7: Add Remote Repository

Copy the repository URL from GitHub (should look like):
- HTTPS: `https://github.com/YOUR_USERNAME/cubit-streaming-system.git`
- SSH: `git@github.com:YOUR_USERNAME/cubit-streaming-system.git`

Add as remote:

```bash
# Using HTTPS
git remote add origin https://github.com/YOUR_USERNAME/cubit-streaming-system.git

# OR using SSH (if you have SSH keys set up)
git remote add origin git@github.com:YOUR_USERNAME/cubit-streaming-system.git
```

Verify remote was added:

```bash
git remote -v
```

## Step 8: Push to GitHub

Push your code:

```bash
# First time push (sets upstream)
git push -u origin main

# If your default branch is 'master' instead of 'main'
git push -u origin master
```

If you get an error about branch names, set main as default:

```bash
git branch -M main
git push -u origin main
```

**Expected Output**: Progress messages showing upload, then "Branch 'main' set up to track remote branch 'main' from 'origin'."

## Step 9: Verify on GitHub

1. Go to your repository URL: `https://github.com/YOUR_USERNAME/cubit-streaming-system`
2. You should see:
   - All your files
   - README.md rendered on the main page
   - Commit history
   - File count (~35 files)

## Step 10: Add Repository Topics (Optional)

On GitHub, click "⚙️ Settings" or "Add topics" on your repo homepage:

Suggested topics:
- `video-streaming`
- `low-latency`
- `nvenc`
- `ffmpeg`
- `v4l2`
- `biomedical-imaging`
- `gpu-acceleration`
- `cpp17`
- `docker`
- `stony-brook-university`

## Future Commits

After making changes:

```bash
# See what changed
git status
git diff

# Stage changes
git add .

# Commit
git commit -m "Brief description of changes"

# Push
git push
```

## Common Issues and Solutions

### Issue: "Permission denied (publickey)"

**Solution**: You're using SSH but haven't set up keys. Either:

1. Switch to HTTPS:
   ```bash
   git remote set-url origin https://github.com/YOUR_USERNAME/cubit-streaming-system.git
   ```

2. Or set up SSH keys: https://docs.github.com/en/authentication/connecting-to-github-with-ssh

### Issue: "Updates were rejected because the remote contains work"

**Solution**: Someone else pushed, or you edited on GitHub. Pull first:
```bash
git pull origin main --rebase
git push
```

### Issue: Large files warning

**Solution**: GitHub has a 100MB file limit. Check:
```bash
find . -type f -size +50M
```

If found, add to .gitignore and remove from git:
```bash
git rm --cached path/to/large/file
echo "path/to/large/file" >> .gitignore
```

### Issue: Build artifacts committed

**Solution**: Remove from git, add to .gitignore:
```bash
git rm -r --cached build/
echo "build/" >> .gitignore
git commit -m "Remove build artifacts"
git push
```

## Adding a README Badge (Optional)

Add a status badge to your README:

```markdown
# CUBIT Video Streaming System

![Build Status](https://img.shields.io/badge/build-passing-brightgreen)
![C++17](https://img.shields.io/badge/C++-17-blue)
![License](https://img.shields.io/badge/license-MIT-green)

...
```

## Creating Releases (Optional)

Tag important versions:

```bash
# Create annotated tag
git tag -a v1.0.1 -m "Version 1.0.1: Added client registration and packet fragmentation"

# Push tag to GitHub
git push origin v1.0.1
```

On GitHub, go to "Releases" → "Draft a new release" → Select tag v1.0.1 → Publish

## Repository Settings (Recommended)

On GitHub, go to Settings:

1. **Description**: "Low-latency video streaming system for biomedical microscope cameras"
2. **Website**: (your portfolio URL if you have one)
3. **Topics**: Add relevant topics as mentioned above
4. **Include in the home page**: Check this to feature on your profile
5. **Releases**: Enable releases
6. **Issues**: Enable if you want bug tracking

## Complete Checklist

Before considering it "done":

- All tests pass (./scripts/pre_push_test.sh)
- Git repository initialized
- All files committed
- Remote repository created on GitHub
- Code pushed successfully
- README renders correctly on GitHub
- Repository has description and topics
- License file included
- .gitignore prevents build artifacts
- Repository is public (if using for portfolio)
- URL added to resume/portfolio

## Portfolio Use

When adding to your resume or portfolio:

**GitHub URL**: `https://github.com/YOUR_USERNAME/cubit-streaming-system`

**Description Example**:
> **Low-Latency Video Streaming System** (C++, FFmpeg, CUDA)
>
> Developed during internship at CUBIT, Stony Brook University. Multi-threaded video streaming system achieving 50-70ms end-to-end latency for biomedical microscope cameras. Features hardware-accelerated encoding (NVENC), adaptive bitrate control, and Docker containerization. Supports 500+ concurrent clients via UDP multicast.
>
> Tech: C++17, FFmpeg, NVENC, V4L2, CUDA, Docker, CMake
>
> [GitHub](https://github.com/YOUR_USERNAME/cubit-streaming-system)

---

**Questions?** Check GitHub Docs: https://docs.github.com/en/get-started
