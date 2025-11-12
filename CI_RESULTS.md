# Viewing CI Build Results

This document explains how to view and access CI build results for the PCB project.

## GitHub Actions UI

### View Latest Builds

1. **Via GitHub Web:**
   - Go to: https://github.com/technobauble/pcb/actions
   - Click on the latest workflow run
   - You'll see all jobs (4 build configs + code quality + summary)

2. **View Job Summaries (NEW!):**
   - Each job now writes to the **Job Summary** visible in the GitHub UI
   - Look for the "Summary" tab in each job
   - The summary includes:
     - ✅/❌ Status indicators
     - Build configuration details
     - Test results
     - Error logs (if failures occur)
     - Code quality metrics

### Build Summary Report

The **Build Summary** job provides an overall report:
- Status of all 4 build configurations
- Code quality check results
- Links to downloadable artifacts

## Downloadable Artifacts

GitHub Actions saves build artifacts that you can download:

### Available Artifacts

1. **Build Logs** (for each configuration):
   - `configure.log` - Configure script output
   - `build.log` - Compiler output
   - `test.log` - Test execution log
   - Unit test logs (if failures occur)
   - Retention: 7 days

2. **Code Quality Reports**:
   - `cppcheck-report.txt` - Static analysis results
   - Retention: 30 days

3. **Built Binary**:
   - `pcb-linux-gtk` - Compiled PCB binary (GTK GUI)
   - Retention: 7 days

### Download Artifacts

**Via Web UI:**
1. Go to the workflow run
2. Scroll to the bottom of the page
3. Look for "Artifacts" section
4. Click to download (ZIP format)

**Via GitHub CLI:**
```bash
# List artifacts for a run
gh run view <run-id>

# Download all artifacts
gh run download <run-id>

# Download specific artifact
gh run download <run-id> -n build-logs-GTK-GUI
```

## Build Status in README

The README.md includes a status badge:
[![Build Status](https://github.com/technobauble/pcb/workflows/Build%20and%20Test/badge.svg)](https://github.com/technobauble/pcb/actions)

- ✅ Green = All builds passing
- ❌ Red = Some builds failing
- 🟡 Yellow = Builds in progress

Click the badge to jump directly to Actions page.

## Job Outputs

Each build step now includes detailed output:

### Success Output Example
```
✅ autogen.sh completed successfully
✅ Configure completed for: GTK GUI
**Configure flags:** `--with-gui=gtk --disable-doc ...`
✅ Build completed successfully
✅ Tests passed
```

### Failure Output Example
```
❌ Build Failed: GTK GUI

### Unit Test Log
[Last 50 lines of error output]

### Test Suite Log
[Last 50 lines of test failures]
```

## Monitoring Builds

### Get Notified of Failures

1. **Watch the repository** (via GitHub)
   - Click "Watch" → "Custom" → "Actions"
   - You'll receive emails on failures

2. **Check Status via CLI:**
   ```bash
   # Latest run status
   gh run list --branch main --limit 1

   # Watch a running build
   gh run watch
   ```

3. **RSS Feed:**
   - Subscribe to: `https://github.com/technobauble/pcb/commits/main.atom`

## Understanding Build Matrix

The CI tests 4 build configurations:

| Configuration | Purpose | Key Options |
|--------------|---------|-------------|
| **GTK GUI** | Standard GUI build | `--with-gui=gtk` |
| **GTK + OpenGL** | GPU-accelerated GUI | `--enable-gl` |
| **Lesstif GUI** | Legacy X11 GUI | `--with-gui=lesstif` |
| **Batch (Headless)** | Non-interactive mode | `--with-gui=batch` |

All configurations:
- Disable documentation building (`--disable-doc`)
- Disable D-Bus (`--disable-dbus`)
- Disable desktop database updates

## What Gets Logged

### Logged to Files (downloadable):
- Complete configure output
- Complete build output (all compiler messages)
- Complete test output
- Test suite logs on failure

### Shown in Job Summary:
- Build status for each step
- Configuration details
- Error summaries (last 50 lines)
- Code quality metrics

### Shown in Annotations:
- Compiler warnings
- Test failures
- Code quality warnings (register keywords, CVS artifacts)

## Troubleshooting

### "Where are my test results?"
- Check the **Job Summary** tab in the build job
- Download the `build-logs-*` artifact for full logs

### "Build succeeded but summary shows failure"
- The summary job only fails if builds fail
- Warnings don't cause summary to fail

### "Can't find artifacts"
- Artifacts are only kept for 7-30 days
- Re-run the workflow to regenerate them

### "Want more detail"
- Click into individual job steps
- Each step shows complete stdout/stderr
- Download logs for offline analysis

## Alternative: Local Build Logging

You can replicate CI logging locally:

```bash
# Using Docker (recommended)
docker-compose up -d
docker-compose exec pcb-dev bash

# Inside container
./autogen.sh 2>&1 | tee autogen.log
./configure --disable-doc 2>&1 | tee configure.log
make -j$(nproc) 2>&1 | tee build.log
make check 2>&1 | tee test.log
```

All logs are saved to `.log` files for later review.

## Future Enhancements

Potential additions:
- [ ] GitHub Pages for historical build reports
- [ ] Test coverage reports
- [ ] Performance benchmarking over time
- [ ] Automated changelog generation
- [ ] Release artifact publishing

## Questions?

If you need different information in the CI reports, the workflow can be customized. See `.github/workflows/build.yml` for the configuration.
