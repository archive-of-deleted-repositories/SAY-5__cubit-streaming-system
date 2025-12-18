# Push to GitHub - Exact Commands for SAY-5

Your GitHub: https://github.com/SAY-5

## Step 1: Create Repository on GitHub

1. Go to: **https://github.com/new**
2. Fill in:
   - **Repository name**: `cubit-streaming-system`
   - **Description**: `Low-latency video streaming system for biomedical microscope cameras`
   - **Visibility**: ✅ **Public** (recommended for portfolio)
   - **❌ DO NOT** check "Add a README file"
   - **❌ DO NOT** check "Add .gitignore"
   - **❌ DO NOT** choose a license
3. Click **"Create repository"**

## Step 2: Push Your Code

Copy and paste these commands in your terminal:

```bash
# Navigate to project directory
cd /Users/say/Desktop/Streaming_System/cubit-streaming-system

# Add GitHub remote
git remote add origin https://github.com/SAY-5/cubit-streaming-system.git

# Push to GitHub
git push -u origin main
```

**Expected Output:**
```
Enumerating objects: 50, done.
Counting objects: 100% (50/50), done.
Delta compression using up to 8 threads
Compressing objects: 100% (45/45), done.
Writing objects: 100% (50/50), 45.67 KiB | 4.57 MiB/s, done.
Total 50 (delta 5), reused 0 (delta 0), pack-reused 0
To https://github.com/SAY-5/cubit-streaming-system.git
 * [new branch]      main -> main
Branch 'main' set up to track remote branch 'main' from 'origin'.
```

## Step 3: Verify on GitHub

Your repository will be at:
👉 **https://github.com/SAY-5/cubit-streaming-system**

You should see:
- ✓ All 37 files
- ✓ README.md displayed on homepage
- ✓ Commit: "Initial commit: Low-latency video streaming system v1.0.1"
- ✓ 4,556 lines of code

## Step 4: Add Repository Topics (Recommended)

On your GitHub repo page:

1. Click **"⚙️"** next to "About" section
2. Add these topics:
   ```
   video-streaming
   low-latency
   cpp
   cpp17
   ffmpeg
   nvenc
   docker
   biomedical-imaging
   gpu-acceleration
   real-time
   cuda
   v4l2
   stony-brook
   ```
3. Set description: `Low-latency video streaming system for biomedical microscope cameras`
4. Click **"Save changes"**

## Step 5: Update Your GitHub Profile

### Add to Pinned Repositories

1. Go to your profile: https://github.com/SAY-5
2. Click "Customize your pins"
3. Select `cubit-streaming-system`
4. This will feature it on your profile page!

### Add to README (if you have one)

Create or update your profile README at `SAY-5/SAY-5/README.md`:

```markdown
## 🔥 Featured Projects

### [Low-Latency Video Streaming System](https://github.com/SAY-5/cubit-streaming-system)
Multi-threaded video streaming system achieving 50-70ms end-to-end latency.
Built during internship at CUBIT, Stony Brook University.

**Tech:** C++17 • FFmpeg • NVENC • CUDA • Docker • CMake

**Features:**
- Hardware-accelerated encoding (NVENC) with CPU fallback
- Adaptive bitrate control based on GPU utilization
- Supports 500+ concurrent clients via UDP multicast
- Docker containerization with GPU passthrough
```

## For Your Resume

**GitHub URL:**
```
https://github.com/SAY-5/cubit-streaming-system
```

**Project Description:**

> **Low-Latency Video Streaming System** (C++17, FFmpeg, CUDA, Docker)
>
> Developed multi-threaded video streaming system during internship at CUBIT,
> Stony Brook University. Achieved 50-70ms end-to-end latency for real-time
> biomedical microscope imaging. Features hardware-accelerated encoding (NVENC),
> adaptive bitrate control, and Docker containerization. Supports 500+ concurrent
> client connections.
>
> **Tech Stack:** C++17, FFmpeg, NVENC, V4L2, CUDA, NVML, Docker, CMake
>
> [View on GitHub →](https://github.com/SAY-5/cubit-streaming-system)

## LinkedIn Post Template

```
🎓 Excited to share my latest project from my internship at CUBIT
(Center for Understanding Biology using Imaging Technology) at
Stony Brook University!

I developed a low-latency video streaming system for biomedical
microscope cameras, achieving sub-70ms latency for real-time remote viewing.

🚀 Key Achievements:
✅ Multi-threaded C++ pipeline with hardware-accelerated encoding (NVENC)
✅ Adaptive bitrate control based on real-time GPU utilization
✅ Docker containerization with GPU passthrough
✅ Supports 500+ concurrent client connections
✅ Full documentation and production-ready code

💻 Tech Stack:
C++17, FFmpeg, CUDA, NVML, Docker, CMake

This project gave me hands-on experience with:
• Systems programming & multi-threading
• Video encoding & GPU acceleration
• Network protocols & optimization
• Production deployment & containerization

Check it out on GitHub: https://github.com/SAY-5/cubit-streaming-system

#SoftwareEngineering #CPlusPlus #VideoStreaming #GPU #Docker
#StonyBrookUniversity #CUBIT #SystemsProgramming
```

## Future Updates

When you make changes:

```bash
cd /Users/say/Desktop/Streaming_System/cubit-streaming-system

# Check what changed
git status

# Add changes
git add .

# Commit with message
git commit -m "Your change description"

# Push to GitHub
git push
```

## Troubleshooting

### If you get "Permission denied"

Make sure you're using HTTPS (not SSH):

```bash
git remote set-url origin https://github.com/SAY-5/cubit-streaming-system.git
git push -u origin main
```

### If prompted for credentials

GitHub will ask for:
- **Username:** SAY-5
- **Password:** Use a **Personal Access Token** (not your GitHub password)

To create a token:
1. GitHub → Settings → Developer settings → Personal access tokens → Tokens (classic)
2. Generate new token
3. Select scopes: `repo` (full control)
4. Copy token and use it as password

---

**You're all set! 🎉**

Your repository URL: https://github.com/SAY-5/cubit-streaming-system
