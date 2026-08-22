# ps14 - Anti-Tamper Game Engine

**Kernel-Level Sync & Repair System for PC Game Protection**

---

## 🎯 PROJECT STATUS

**Current Phase:** 1 - Core Protection Engine (User Mode)  
**Progress:** 100% Complete  
**Last Updated:** August 23, 2026  
**Mode:** ACTIVE (Building)

---

## 📊 PHASE TRACKER

### ✅ Phase 0: Project Setup & Architecture (Week 1) - COMPLETED
- [x] Create folder structure
- [x] Write ARCHITECTURE.md
- [x] Set up CMake build system
- [x] Create initial stub files
- [x] Add .gitignore
- [x] Create build scripts (build.ps1, build.bat)
- [x] Implement shared library (config, logger, hash, network, thread)
- [x] Commit to git
- [x] Push to remote repository
- [ ] Add LICENSE file
- [ ] Add API_SPEC.md
- [ ] Add DEPLOYMENT.md

### ✅ Phase 1: Core Protection Engine - User Mode (Weeks 2-4) - COMPLETED
- [x] Memory monitoring system (memory_scanner.c, page_auditor.c, hash_database.c)
- [x] Integrity verification system (file_verifier.c, code_signature.c, checksum_db.c)
- [x] Basic authentication gateway (auth_client.c, session_manager.c, token_validator.c, state_sync.c)
- [x] State synchronization system (state_sync.c)
- [x] Repair system (memory_repair.c, file_restorer.c, state_roller.c, backup_manager.c, repair.h)
- [x] Unit tests (test_memory_monitor.c, test_integrity_check.c, test_auth_gateway.c, test_repair_system.c)

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
│   │   ├── memory_monitor/         # Memory auditing (IMPLEMENTED)
│   │   │   ├── memory_scanner.c    # Memory scanning logic
│   │   │   ├── page_auditor.c      # Memory page auditing
│   │   │   └── hash_database.c    # Hash storage and lookup
│   │   ├── integrity_check/        # File/process integrity (IMPLEMENTED)
│   │   │   ├── file_verifier.c     # File hash verification
│   │   │   ├── code_signature.c    # PE signature verification
│   │   │   └── checksum_db.c       # Checksum database
│   │   ├── auth_gateway/           # Authentication system (IMPLEMENTED)
│   │   │   ├── auth_client.c       # Client-side authentication
│   │   │   ├── session_manager.c   # Session lifecycle management
│   │   │   ├── token_validator.c   # Token generation/validation
│   │   │   └── state_sync.c        # State synchronization
│   │   ├── repair_system/          # State repair logic (IMPLEMENTED)
│   │   │   ├── memory_repair.c     # Memory restoration
│   │   │   ├── file_restorer.c     # File restoration
│   │   │   ├── state_roller.c      # State rollback
│   │   │   └── backup_manager.c    # Backup management
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
├── tests/                          # Test suites (IMPLEMENTED)
│   ├── unit/
│   │   ├── main.c                 # Test runner
│   │   ├── test_memory_monitor.c  # Memory monitor tests
│   │   ├── test_integrity_check.c  # Integrity checker tests
│   │   ├── test_auth_gateway.c    # Auth gateway tests
│   │   └── test_repair_system.c   # Repair system tests
│   └── integration/
│       ├── main.c
│       ├── test_client_server.c
│       └── test_memory_protection.c
├── include/                        # Header files
│   └── ps14/
│       ├── ps14.h                 # Main header (IMPLEMENTED)
│       ├── config.h               # Configuration (IMPLEMENTED)
│       ├── logger.h               # Logger (IMPLEMENTED)
│       ├── hash.h                 # Hash functions (IMPLEMENTED)
│       ├── memory.h               # Memory monitor (IMPLEMENTED)
│       ├── integrity.h            # Integrity checker (IMPLEMENTED)
│       ├── auth.h                 # Authentication (IMPLEMENTED)
│       ├── repair.h               # Repair system (IMPLEMENTED)
│       ├── network.h              # Network (IMPLEMENTED)
│       └── thread.h               # Thread pool (IMPLEMENTED)
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

### ✅ Implemented (Phase 0 & 1)

**Phase 0 - Infrastructure:**
- Project folder structure
- CMake build system
- Main header file (ps14.h) with platform detection and types
- Logger system with file/console output
- Hash algorithms (CRC32, MD5, SHA-256)
- Thread pool for async operations
- Basic network socket functions
- Configuration system
- Build scripts for Windows (PowerShell and Batch)
- .gitignore for Windows development
- All module stubs in place

**Phase 1 - Core Protection Engine:**
- Memory monitoring system (async auditing, page scanning, hash database)
- Integrity verification (file hashing, code signature validation, checksum database)
- Authentication gateway (client-server connection, session management, token validation, state synchronization)
- Repair system (memory repair, file restoration, state rollback, backup management)
- Unit tests for all Phase 1 modules

### 🚧 Planned Features
- Kernel-mode driver
- Advanced detection (debuggers, hooks)
- Buffer overflow protection
- Shadow page table detection
- Integ
