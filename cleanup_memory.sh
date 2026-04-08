#!/bin/bash
# Memory Cleanup Script for Terrain/Chunk System
# This script helps clean up memory by unloading unused chunk meshes and garbage collecting

echo "=== Memory Cleanup Script ==="
echo "Date: $(date)"
echo ""

# Configuration
MAX_CHUNKS_IN_MEMORY=512
UNLOAD_DISTANCE=12
CLEANUP_INTERVAL=300  # seconds

echo "Configuration:"
echo "  Max chunks in memory: $MAX_CHUNKS_IN_MEMORY"
echo "  Unload distance: $UNLOAD_DISTANCE"
echo "  Cleanup interval: ${CLEANUP_INTERVAL}s"
echo ""

# Function to check if process is running
check_process() {
    if pgrep -f "terrain_game" > /dev/null; then
        echo "[INFO] Game process is running"
        return 0
    else
        echo "[INFO] No game process found"
        return 1
    fi
}

# Function to get current chunk count from log
get_chunk_count() {
    if [ -f "terrain.log" ]; then
        tail -100 terrain.log | grep -oP "Loaded chunks: \K\d+" | tail -1
    else
        echo "0"
    fi
}

# Function to analyze memory usage
analyze_memory() {
    echo "=== Memory Analysis ==="
    
    # Check system memory
    if command -v free &> /dev/null; then
        echo "System Memory:"
        free -h | grep -E "Mem|Swap"
    fi
    
    # Check process memory if running
    if check_process; then
        PID=$(pgrep -f "terrain_game" | head -1)
        if [ -n "$PID" ]; then
            echo ""
            echo "Process Memory (PID: $PID):"
            ps -o pid,rss,vsz,%mem,%cpu,cmd -p $PID 2>/dev/null || echo "Could not get process info"
        fi
    fi
    
    # Check chunk count from logs
    CHUNK_COUNT=$(get_chunk_count)
    echo ""
    echo "Chunks loaded (from log): $CHUNK_COUNT"
    
    if [ "$CHUNK_COUNT" -gt "$MAX_CHUNKS_IN_MEMORY" ] 2>/dev/null; then
        echo "[WARNING] Chunk count exceeds maximum!"
    fi
    
    echo ""
}

# Function to trigger garbage collection via signal
trigger_gc() {
    echo "=== Triggering Garbage Collection ==="
    
    if check_process; then
        PID=$(pgrep -f "terrain_game" | head -1)
        if [ -n "$PID" ]; then
            echo "Sending SIGUSR1 to process $PID to trigger GC..."
            kill -USR1 $PID 2>/dev/null && echo "[OK] Signal sent" || echo "[ERROR] Failed to send signal"
        fi
    else
        echo "[INFO] No process to signal"
    fi
    
    echo ""
}

# Function to clean up old log files
cleanup_logs() {
    echo "=== Cleaning Up Log Files ==="
    
    # Remove old rotated logs
    find . -name "terrain.log.*" -mtime +7 -delete 2>/dev/null
    echo "Removed old rotated logs (>7 days)"
    
    # Truncate large log files
    if [ -f "terrain.log" ]; then
        LOG_SIZE=$(stat -f%z terrain.log 2>/dev/null || stat -c%s terrain.log 2>/dev/null || echo "0")
        MAX_LOG_SIZE=$((10 * 1024 * 1024))  # 10MB
        
        if [ "$LOG_SIZE" -gt "$MAX_LOG_SIZE" ] 2>/dev/null; then
            echo "Log file is large (${LOG_SIZE} bytes), keeping last 1000 lines..."
            tail -1000 terrain.log > terrain.log.tmp && mv terrain.log.tmp terrain.log
            echo "[OK] Log truncated"
        else
            echo "Log file size OK (${LOG_SIZE} bytes)"
        fi
    fi
    
    echo ""
}

# Function to clean up temporary files
cleanup_temp() {
    echo "=== Cleaning Up Temporary Files ==="
    
    # Remove temporary mesh files
    find . -name "*.tmp" -mmin +60 -delete 2>/dev/null
    echo "Removed temp files older than 60 minutes"
    
    # Remove core dumps
    find . -name "core.*" -delete 2>/dev/null
    echo "Removed core dump files"
    
    echo ""
}

# Function to display recommendations
show_recommendations() {
    echo "=== Recommendations ==="
    
    CHUNK_COUNT=$(get_chunk_count)
    
    if [ "$CHUNK_COUNT" -gt "$MAX_CHUNKS_IN_MEMORY" ] 2>/dev/null; then
        echo "1. Consider reducing render distance in game settings"
        echo "2. Current chunk count ($CHUNK_COUNT) exceeds recommended max ($MAX_CHUNKS_IN_MEMORY)"
    fi
    
    if command -v free &> /dev/null; then
        MEM_AVAILABLE=$(free -m | awk '/^Mem:/ {print $7}')
        if [ "$MEM_AVAILABLE" -lt 512 ] 2>/dev/null; then
            echo "3. Low system memory available (${MEM_AVAILABLE}MB) - close other applications"
        fi
    fi
    
    echo "4. Run this script periodically during long gaming sessions"
    echo "5. Restart the game if experiencing severe memory issues"
    
    echo ""
}

# Main execution
main() {
    echo "Starting memory cleanup..."
    echo ""
    
    analyze_memory
    cleanup_logs
    cleanup_temp
    show_recommendations
    
    echo "=== Cleanup Complete ==="
    echo "Time: $(date)"
}

# Parse command line arguments
case "${1:-}" in
    --analyze|-a)
        analyze_memory
        ;;
    --gc|--trigger-gc)
        trigger_gc
        ;;
    --logs)
        cleanup_logs
        ;;
    --temp)
        cleanup_temp
        ;;
    --help|-h)
        echo "Usage: $0 [OPTION]"
        echo ""
        echo "Options:"
        echo "  --analyze, -a     Show memory analysis only"
        echo "  --gc              Trigger garbage collection in running process"
        echo "  --logs            Clean up log files only"
        echo "  --temp            Clean up temporary files only"
        echo "  --help, -h        Show this help message"
        echo ""
        echo "Without options, runs full cleanup routine."
        ;;
    *)
        main
        ;;
esac

exit 0
