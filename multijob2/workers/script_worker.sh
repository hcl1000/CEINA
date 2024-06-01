LOG_FILE_NAME_SUFFIX="$1"
make && ./build/worker_cont123 -- -m "00:00:00:00:00:01" -s 1.1.1.1 -d 2.2.2.2 -p 1 -f "$LOG_FILE_NAME_SUFFIX"  | tee log.txt


