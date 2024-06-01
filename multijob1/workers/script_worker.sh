LOG_FILE_NAME_SUFFIX="$1"
SCHEME="$2" # 1: cont1 / 12: cont12
make && ./build/worker -- -m "00:00:00:00:00:00" -s 1.1.1.1 -d 2.2.2.2 -f "$LOG_FILE_NAME_SUFFIX" -c "$SCHEME" | tee log.txt
#usage1 : ./script_worker.sh [LOG_FILE_NAME] COMM1  -> QA / COMM model
#usage2 : ./script_worker.sh [LOG_FILE_NAME] COMM12 -> QA+JA / COMM model
#usage3 : ./script_worker.sh [LOG_FILE_NAME] COMP1  -> QA / COMP model
#usage4 : ./script_worker.sh [LOG_FILE_NAME] COMP12 -> QA+JA / COMP model