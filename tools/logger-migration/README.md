# Logger Migration Tools

This directory contains tools and guides for migrating libraries from fixed custom logger dependencies to optional logger support.

## Contents

- **MIGRATION_PLAN_OPTIONAL_LOGGER.md** - Comprehensive step-by-step migration guide
- **QUICK_LOGGER_MIGRATION.md** - Quick reference with code snippets and commands
- **migrate_logger.py** - Python script to analyze and assist with migration

## Purpose

These tools help transform libraries that require a specific Logger implementation into libraries that can work with or without the custom logger, making them more widely usable.

## Quick Start

```bash
# Analyze a library
python migrate_logger.py --lib-name MyLibrary --lib-path ../../MyLibrary --analyze-only

# Run full migration assistant
python migrate_logger.py --lib-name MyLibrary --lib-path ../../MyLibrary
```

The migration preserves backward compatibility while adding flexibility for users who don't have the custom Logger library.