#!/usr/bin/env python3
"""
Logger Migration Helper Script

This script helps automate the migration from fixed custom logger to optional logger support.
It performs safe replacements and creates backups.

Usage:
    python migrate_logger.py --lib-name MyLibrary --lib-path ../MyLibrary
"""

import os
import re
import argparse
import shutil
from datetime import datetime

def create_backup(lib_path):
    """Create a backup of the library"""
    backup_name = f"{os.path.basename(lib_path)}_backup_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
    backup_path = os.path.join(os.path.dirname(lib_path), backup_name)
    shutil.copytree(lib_path, backup_path)
    print(f"✓ Created backup at: {backup_path}")
    return backup_path

def get_macro_prefix(lib_name):
    """Generate macro prefix from library name"""
    # Convert CamelCase to UPPER_SNAKE
    prefix = re.sub('([a-z0-9])([A-Z])', r'\1_\2', lib_name).upper()
    return prefix

def create_logging_header(lib_name):
    """Generate the logging configuration header content"""
    prefix = get_macro_prefix(lib_name)
    return f'''// Logging configuration using LogInterface
#include <LogInterface.h>

#define {prefix}_LOG_TAG "{lib_name}"
#define {prefix}_LOG_E(...) LOG_ERROR({prefix}_LOG_TAG, __VA_ARGS__)
#define {prefix}_LOG_W(...) LOG_WARN({prefix}_LOG_TAG, __VA_ARGS__)
#define {prefix}_LOG_I(...) LOG_INFO({prefix}_LOG_TAG, __VA_ARGS__)
#define {prefix}_LOG_D(...) LOG_DEBUG({prefix}_LOG_TAG, __VA_ARGS__)
#define {prefix}_LOG_V(...) LOG_VERBOSE({prefix}_LOG_TAG, __VA_ARGS__)
'''

def find_main_header(lib_path, lib_name):
    """Find the main header file"""
    src_path = os.path.join(lib_path, 'src')
    candidates = [
        f"{lib_name}.h",
        f"{lib_name.lower()}.h",
        f"{get_macro_prefix(lib_name).lower()}.h"
    ]
    
    for candidate in candidates:
        header_path = os.path.join(src_path, candidate)
        if os.path.exists(header_path):
            return header_path
    
    # If not found, list all .h files
    headers = [f for f in os.listdir(src_path) if f.endswith('.h')]
    if headers:
        print(f"Could not find main header. Found: {headers}")
        return None
    return None

def analyze_logger_usage(lib_path):
    """Analyze current logger usage in the library"""
    src_path = os.path.join(lib_path, 'src')
    patterns = {
        'LOG_ERROR': [],
        'LOG_WARN': [],
        'LOG_INFO': [],
        'LOG_DEBUG': [],
        'LOG_VERBOSE': [],
        'Logger includes': [],
        'getLogger calls': []
    }
    
    for root, dirs, files in os.walk(src_path):
        for file in files:
            if file.endswith(('.cpp', '.h', '.c')):
                file_path = os.path.join(root, file)
                with open(file_path, 'r') as f:
                    content = f.read()
                    
                # Find logger patterns
                for pattern in ['LOG_ERROR', 'LOG_WARN', 'LOG_INFO', 'LOG_DEBUG', 'LOG_VERBOSE']:
                    matches = re.findall(f'{pattern}\\s*\\([^)]+\\)', content)
                    if matches:
                        patterns[pattern].append((file, len(matches)))
                
                # Find includes
                if '#include "Logger' in content or '#include <Logger' in content:
                    patterns['Logger includes'].append(file)
                
                # Find getLogger calls
                if 'getLogger()' in content:
                    patterns['getLogger calls'].append(file)
    
    return patterns

def generate_migration_report(lib_name, patterns):
    """Generate a migration report"""
    report = f"""# Logger Migration Report for {lib_name}

## Analysis Results

"""
    
    for pattern, occurrences in patterns.items():
        if occurrences:
            report += f"### {pattern}\n"
            if isinstance(occurrences[0], tuple):
                for file, count in occurrences:
                    report += f"- {file}: {count} occurrences\n"
            else:
                for file in occurrences:
                    report += f"- {file}\n"
            report += "\n"
    
    prefix = get_macro_prefix(lib_name)
    report += f"""
## Suggested Replacements

- LOG_ERROR(TAG, ...) → {prefix}_LOG_E(...)
- LOG_WARN(TAG, ...) → {prefix}_LOG_W(...)
- LOG_INFO(TAG, ...) → {prefix}_LOG_I(...)
- LOG_DEBUG(TAG, ...) → {prefix}_LOG_D(...)
- LOG_VERBOSE(TAG, ...) → {prefix}_LOG_V(...)

## Build Configuration

To use with custom logger:
```ini
build_flags = -D {prefix}_USE_CUSTOM_LOGGER
```

## Next Steps

1. Add logging configuration to main header
2. Replace logger calls with new macros
3. Remove Logger includes from source files
4. Test both configurations
5. Update documentation
"""
    
    return report

def main():
    parser = argparse.ArgumentParser(description='Logger Migration Helper')
    parser.add_argument('--lib-name', required=True, help='Library name (e.g., MyLibrary)')
    parser.add_argument('--lib-path', required=True, help='Path to library')
    parser.add_argument('--no-backup', action='store_true', help='Skip backup creation')
    parser.add_argument('--analyze-only', action='store_true', help='Only analyze, do not modify')
    
    args = parser.parse_args()
    
    if not os.path.exists(args.lib_path):
        print(f"Error: Library path '{args.lib_path}' does not exist")
        return 1
    
    print(f"\n🔧 Logger Migration Helper for {args.lib_name}")
    print("=" * 50)
    
    # Create backup
    if not args.no_backup and not args.analyze_only:
        backup_path = create_backup(args.lib_path)
    
    # Analyze current usage
    print("\n📊 Analyzing logger usage...")
    patterns = analyze_logger_usage(args.lib_path)
    
    # Generate report
    report = generate_migration_report(args.lib_name, patterns)
    report_path = os.path.join(args.lib_path, 'LOGGER_MIGRATION_REPORT.md')
    with open(report_path, 'w') as f:
        f.write(report)
    print(f"✓ Generated report: {report_path}")
    
    if args.analyze_only:
        print("\n✅ Analysis complete (--analyze-only mode)")
        return 0
    
    # Find main header
    print("\n🔍 Finding main header file...")
    main_header = find_main_header(args.lib_path, args.lib_name)
    if not main_header:
        print("❌ Could not find main header file. Please add logging configuration manually.")
    else:
        print(f"✓ Found main header: {main_header}")
        
        # Generate logging configuration
        logging_config = create_logging_header(args.lib_name)
        config_path = os.path.join(args.lib_path, 'logging_config.txt')
        with open(config_path, 'w') as f:
            f.write(logging_config)
        print(f"✓ Generated logging configuration: {config_path}")
        print("\n📋 Add this to your main header file before the class definition:")
        print("-" * 50)
        print(logging_config)
        print("-" * 50)
    
    print("\n✅ Migration preparation complete!")
    print("\n📚 Next steps:")
    print("1. Review the migration report")
    print("2. Add logging configuration to your main header")
    print("3. Use search & replace to update logger calls")
    print("4. Remove Logger includes from source files")
    print("5. Test both configurations")
    
    return 0

if __name__ == '__main__':
    exit(main())