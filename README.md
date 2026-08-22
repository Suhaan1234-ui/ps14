# ps14 - Anti-Tamper Game Engine

**Kernel-Level Sync & Repair System for PC Game Protection**

---

## 🎯 PROJECT STATUS

**Current Phase:** 0 - Project Setup & Architecture  
**Progress:** 50% Complete  
**Last Updated:** August 23, 2026  
**Mode:** ACTIVE (Building)

---

## 📊 PHASE TRACKER

### ✅ Phase 0: Project Setup & Architecture (Week 1)
- [x] Create folder structure
- [x] Write ARCHITECTURE.md
- [x] Set up CMake build system
- [x] Create initial stub files
- [x] Add .gitignore
- [x] Create build scripts (build.ps1, build.bat)
- [ ] Add LICENSE file
- [ ] Add API_SPEC.md
- [ ] Add DEPLOYMENT.md

### ⏳ Phase 1: Core Protection Engine - User Mode (Weeks 2-4)
- [ ] Memory monitoring system
- [ ] Integrity verification system
- [ ] Basic authentication gateway
- [ ] State synchronization system
- [ ] Unit tests

### ⏳ Phase 2: Advanced Detection Systems (Weeks 5-6)
- [ ] Asynchronous page auditing
- [ ] Reverse engineering detection
- [ ] Buffer overflow protection
- [ ] Integration tests

### ⏳ Phase 3: Kernel-Mode Components (Weeks 7-8)
- [ ] Kernel driver skeleton
- [ ] Memory protection from kernel
- [ ] Process and thread monitoring
- [ ] Shadow page table detection

### ⏳ Phase 4: Integration & Testing (Weeks 9-10)
- [ ] Client integration
- [ ] Server integration
- [ ] Testing framework
- [ ] Performance optimization

### ⏳ Phase 5: Deployment & Documentation (Week 11)
- [ ] Build automation (CI/CD)
- [ ] Installation system
- [ ] Complete documentation
- [ ] Final testing

---

## 🏗️ PROJECT STRUCTURE

```
ps14/
├── docs/                           # Documentation
│   ├── ARCHITECTURE.md             # System design (COMPLETED)
│   ├── API_SPEC.md                 # API specifications (TODO)
│   └── DEPLOYMENT.md               # Deployment guide (TODO)
├── src/
│   ├── core/                       # Core protection engine
│   │   ├── memory_monitor/         # Memory auditing (STUB)
│   │   │   ├── memory_scanner.c
│   │   │   ├── page_auditor.c
│   │   │   └── hash_database.c
│   │   ├── integrity_check/        # File/process integrity (STUB)
│   │   │   ├── file_verifier.c
│   │   │   ├── code_signature.c
│   │   │   └── checksum_db.c
│   │   ├── auth_gateway/           # Authentication system (STUB)
│   │   │   ├── auth_client.c
│   │   │   ├── session_manager.c
│   │   │   ├── token_validator.c
│   │   │   └── state_sync.c
│   │   ├── repair_system/          # State repair logic (STUB)
│   │   │   ├── memory_repair.c
│   │   │   ├── file_restorer.c
│   │   │   ├── state_roller.c
│   │   │   └── backup_manager.c
│   │   └── core.c                  # Main DLL entry point
│   ├── client/                     # Game client wrapper (STUB)
│   │   ├── launcher.c
│   │   ├── config_manager.c
│   │   ├── network_client.c
│   │   └── api_hooks.c
│   ├── server/                     # Server-side components (STUB)
│   │   ├── auth_server.c
│   │   ├── session_store.c
│   │   ├── ban_manager.c
│   │   ├── state_validator.c
│   │   ├── network_handler.c
│   │   └── main.c
│   ├── kernel/                     # Kernel-mode driver (STUB)
│   │   ├── driver.c
│   │   ├── memory_guard.c
│   │   ├── process_watcher.c
│   │   ├── callback_handler.c
│   │   └── shadow_pt_detector.c
│   └── shared/                     # Shared libraries (IMPLEMENTED)
│       ├── config.c
│       ├── logger.c
│       ├── hash.c
│       ├── network.c
│       └── thread_pool.c
├── tests/                          # Test suites (STUB)
│   ├── unit/
│   │   ├── main.c
│   │   ├── test_memory_monitor.c
│   │   ├── test_integrity_check.c
│   │   ├── test_auth_gateway.c
│   │   └── test_repair_system.c
│   └── integration/
│       ├── main.c
│       ├── test_client_server.c
│       └── test_memory_protection.c
├── include/                        # Header files (PARTIAL)
│   └── ps14/
│       ├── ps14.h                 # Main header (IMPLEMENTED)
│       ├── config.h               # Configuration (TODO)
│       ├── logger.h               # Logger (IMPLEMENTED)
│       ├── hash.h                 # Hash functions (IMPLEMENTED)
│       ├── memory.h               # Memory monitor (IMPLEMENTED)
│       ├── thread.h               # Thread pool (IMPLEMENTED)
│       └── network.h              # Network (IMPLEMENTED)
├── lib/                            # Third-party libraries
├── tools/                          # Build and utility scripts
│   ├── build.ps1                   # PowerShell build script (IMPLEMENTED)
│   └── build.bat                   # Batch build script (IMPLEMENTED)
├── samples/                        # Sample game integration
├── README.md                       # This file (live progress)
├── CMakeLists.txt                  # Build configuration (IMPLEMENTED)
└── .gitignore                      # Git ignore rules (IMPLEMENTED)
```

---

## 🚀 GETTING STARTED

### Prerequisites
- Windows 10/11 (64-bit)
- Visual Studio 2022 with C++ and WDK components
- Windows SDK
- Windows Driver Kit (WDK) - for kernel driver
- CMake 3.20+

### Quick Start

```bash
# Clone the repository
git clone https://github.com/Suhaan1234-ui/ps14.git
cd ps14

# Build using CMake
mkdir build
cd build
cmake ..
cmake --build .

# Or use the build scripts
# Windows (PowerShell):
./tools/build.ps1

# Windows (Batch):
./tools/build.bat
```

### Build Options

```bash
# Build in Debug mode
./tools/build.bat debug

# Build with clean
./tools/build.bat clean

# Build with verbose output
./tools/build.bat verbose

# Build kernel driver (requires WDK)
./tools/build.bat kernel

# Combine options
./tools/build.bat clean debug verbose
```

---

## 📚 DOCUMENTATION

- [Architecture Document](docs/ARCHITECTURE.md) - ✅ Complete system design
- [API Specification](docs/API_SPEC.md) - 📋 To be written
- [Deployment Guide](docs/DEPLOYMENT.md) - 📋 To be written

---

## 🛠️ FEATURES

### ✅ Implemented (Phase 0)
- Project folder structure
- CMake build system
- Main header file (ps14.h)
- Logger system with file/console output
- Hash algorithms (CRC32, MD5, SHA-256)
- Thread pool for async operations
- Basic network socket functions
- Configuration system
- Build scripts for Windows

### 🚧 In Development
- Memory monitoring system
- Integrity verification
- Authentication gateway
- State synchronization
- Repair system

### ⏳ Planned
- Kernel-mode driver
- Advanced detection (debuggers, hooks)
- Buffer overflow protection
- Shadow page table detection

---

## 📝 CHANGELOG

### August 23, 2026
- ✅ Project plan approved
- ✅ Switched to ACTIVE mode
- ✅ Created complete folder structure
- ✅ Added .gitignore
- ✅ Created ARCHITECTURE.md with full system design
- ✅ Created CMakeLists.txt build configuration
- ✅ Implemented shared library (config, logger, hash, network, thread)
- ✅ Created build scripts (build.ps1, build.bat)
- ✅ Added placeholder files for all modules
- 🚧 Ready for git commit

---

## 📊 FILE COUNT

| Type | Count | Status |
|------|-------|--------|
| Header files (.h) | 4 | Partially implemented |
| Source files (.c) | 35+ | Partially implemented |
| Documentation | 1 | ARCHITECTURE.md complete |
| Build scripts | 2 | PowerShell & Batch |
| Configuration | 1 | .gitignore |
| Total | 43+ | Phase 0 complete |

---

## 🤝 CONTRIBUTING

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

---

## 📜 LICENSE

[MIT License](LICENSE) - To be added

---

## 🆘 SUPPORT

For issues, questions, or support:
- Open an issue on GitHub: https://github.com/Suhaan1234-ui/ps14/issues
- Check the [Documentation](docs/)

---

## 🏆 NEXT STEPS

1. **Immediate:**
   - Commit Phase 0 to git
   - Push to remote repository
   - Test basic build

2. **Phase 1:**
   - Implement memory monitoring
   - Implement integrity checking
   - Implement authentication gateway
   - Implement repair system

3. **Phase 2:**
   - Add async page auditing
   - Add debugger detection
   - Add buffer overflow protection

---

**Generated by Mistral Vibe**
**Co-Authored-By: Mistral Vibe <vibe@mistral.ai>**
READEO
