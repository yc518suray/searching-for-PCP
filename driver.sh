#TO USE: ./driver.sh [ORDER] [Compression Factor] --[OPT1] [para1] --[OPT2] [para2]

order=$1
compress=$2

# added options & parameters

option1=$3
letter1=$4
option2=$5
letter2=$6

CYAN_COLOR='\033[1;36m'
NO_COLOR='\033[0m'

# input options

if [[ -z "$option1" || -z "$letter1" ]]; then
	option1=""
else
	option1="${option1#--}"
	letter1="${letter1^^}"

	if [[ -z "$option2" || -z "$letter2" ]]; then
		option2=""
	else
		option2="${option2#--}"
		letter2="${letter2^^}"
	fi
fi

# option1 tells start or not, option2 tells stop or not

if [ "$option1" = "stop" ]; then
	temp=$option1
	option1=$option2
	option2=$temp

	temp=$letter1
	letter1=$letter2
	letter2=$temp
fi

len=$(($order / $compress))

[ $order -eq $order 2>/dev/null ] || exit 1
#[ $numproc -eq $numproc 2>/dev/null ] || exit 1

# ---------- Generation ---------- #
if [ -z "$option1" ]; then #start Generation (default)

mkdir results 2> /dev/null
mkdir results/$order 2> /dev/null

echo Generating Candidates...

start=`date +%s`
./bin/generate_hybrid $order $compress
end=`date +%s`

runtime1=$((end-start))
echo $runtime1 seconds elapsed

fi #finish Generation

if [ "$option2" = "stop" ] && [ "$letter2" = "G" ];then
	echo -e "exit after finishing ${CYAN_COLOR}generation${NO_COLOR} ... GOODBYE!"
	exit 0
fi

# ---------- Matching ---------- #
if [[ -z "$option1" || ( "$option1" = "start" &&  "$letter1" = "M" ) ]];then #start Matching

option1=""

echo Matching Candidates...

start=`date +%s`

sort results/$order/$order-unique-filtered-a_1 | uniq > results/$order/$order-candidates-a.sorted_1
sort results/$order/$order-unique-filtered-b_1 | uniq > results/$order/$order-candidates-b.sorted_1

./bin/match_pairs $order $len 1
end=`date +%s`

runtime2=$((end-start))
echo $runtime2 seconds elapsed

mv results/$order/$order-pairs-found_1 results/$order/$order-pairs-found-1

total=$((runtime1 + runtime2))

echo $total seconds total

epochtime=$(date +%s)
datetime=$(date +"%Y-%m-%d")

cp results/$order/$order-pairs-found-1 results/history/$order-$compress-$datetime-$epochtime 2> /dev/null
cp results/$order/$order-pairs-found-1 results/$order-pairs-found 2> /dev/null

fi #finish Matching

if [ "$option2" = "stop" ] && [ "$letter2" = "M" ];then
	echo -e "exit after finishing ${CYAN_COLOR}matching${NO_COLOR} ... GOODBYE!"
	exit 0
fi

# ---------- Uncompression ---------- #
# 平行化修改
if [[ -z "$option1" || ( "$option1" = "start" &&  "$letter1" = "U" ) ]];then #start Uncompression

option1=""

if [ $compress -gt 1 ]
then

# --- 平行化開始 ---
# 定義平行化參數
NUM_PROCS=7      # <-- 請根據你的 CPU 核心數設定
NEWCOMPRESS=1    # <-- 最終解壓縮的目標因子 (通常是 1)
INPUT_FILE="results/$order-pairs-found"

echo "Calculating line counts and splitting input file..."

if [ ! -f "$INPUT_FILE" ]; then
    echo "Error: Input file $INPUT_FILE not found for uncompression."
    exit 1
fi

TOTAL_LINES=$(tr -d '\r' < "$INPUT_FILE" | wc -l)

# 為了確保每個檔案大小平均，我們需要重新執行 uncompress.sh 中的計算邏輯
START_LINE=1
TEMP_INPUT_PREFIX="results/$order/uncomp_input_"
rm -f ${TEMP_INPUT_PREFIX}* # 清理舊的分割檔案

for i in $(seq 1 $NUM_PROCS); do
    # 重新計算 CHUNK_SIZE (與 uncompress.sh 邏輯一致)
    CHUNK_SIZE_BASE=$((TOTAL_LINES / NUM_PROCS))
    REMAINDER=$((TOTAL_LINES % NUM_PROCS))
    
    if [ "$i" -le "$REMAINDER" ]; then
        CHUNK_SIZE=$((CHUNK_SIZE_BASE + 1))
    else
        CHUNK_SIZE=$CHUNK_SIZE_BASE
    fi
    
    # 使用 AWK 抽取該區塊的行並輸出到獨立檔案
    if [ "$CHUNK_SIZE" -gt 0 ]; then
        END_LINE=$((START_LINE + CHUNK_SIZE - 1))
        
        SPLIT_OUTPUT_FILE="${TEMP_INPUT_PREFIX}$i"
        
        # AWK 是一種高性能的行數選擇工具
        awk "NR >= $START_LINE && NR <= $END_LINE" $INPUT_FILE > "$SPLIT_OUTPUT_FILE"
        echo "Created file $SPLIT_OUTPUT_FILE: Lines $START_LINE to $END_LINE"

        # 更新下一輪的起始行
        START_LINE=$((END_LINE + 1))
    fi
done

echo "Finished splitting file into $NUM_PROCS parts."

# --- END 檔案切割 ---

echo "Launching $NUM_PROCS parallel uncompression tasks..."
start_uncomp=`date +%s` 
mkdir -p logs 2> /dev/null

# 迴圈啟動所有工作進程 (ProcID 從 1 到 NUM_PROCS)
for i in $(seq 1 $NUM_PROCS); do
	./uncompress.sh $order $compress $NEWCOMPRESS $i > logs/uncompress_proc_$i.log 2>&1 &
done

# 等待所有背景任務完成
wait 
end_uncomp=`date +%s`
runtime3=$((end_uncomp - start_uncomp))
echo $runtime3 seconds elapsed for parallel uncompression.

# cp results/$order/$order-pairs-found-0 results/history/$order-1-$datetime-$epochtime 2> /dev/null
# cp results/$order/$order-pairs-found-0 results/$order-pairs-found 2> /dev/null
# rm results/$order/$order-pairs-found-0

else
	echo "compression factor = 1, no need to uncompress"

fi
fi #finish Uncompression

if [ "$option2" = "stop" ] && [ "$letter2" = "U" ];then
	echo -e "exit after finishing ${CYAN_COLOR}uncompression${NO_COLOR} ... GOODBYE!"
	exit 0
fi

# ---------- Equivalence Filtering ---------- #
if [[ -z "$option1" || ( "$option1" = "start" &&  "$letter1" = "E" ) ]];then #start Equivalence Filtering

echo Filtering Equivalences...

start=`date +%s`
./bin/cache_filter $order $order
end=`date +%s`

runtime3=$((end-start))

echo $runtime3 seconds elapsed

epochtime=$(date +%s)
datetime=$(date +"%Y-%m-%d")

cp results/$order-unique-pairs-found results/history/$order-1-$datetime-$epochtime 2> /dev/null

echo "Filtering Equivalences done, go see the results!"
fi #finish Equivalence Filtering
