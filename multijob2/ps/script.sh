FILE_NAME_SUFFIX="$1"
make && ./build/ps -- -m "00:00:00:00:00:01" -s 1.1.1.1 -d 2.2.2.2 -f "$FILE_NAME_SUFFIX"

