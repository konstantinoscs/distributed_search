#!/bin/bash
#find total number of words
total_words=$(awk '$2=="search"{print $2}' FS=" : " log/* | wc -l)
#store all words in array words
words=($(awk '$2=="search"{print $3}' FS=" : " log/*))
#eliminate duplicates and store them in uniq
uniq=($(printf "%s\n" "${words[@]}" | sort -u)); 
#echo "${uniq[@]}"
declare -A wordc

#initialize array with 0
for i in "${uniq[@]}"
do :
  wordc[$i]=0
done

#count all word appearances across all logs
for i in "${uniq[@]}"
do :
	((wordc[$i]+=$(awk -F' : ' -v word=$i -v ctr=${wordc[$i]} '$3==word {ctr+=NF-3;} END {print ctr}' log/*)))
done

#find min and max
minw=(${!wordc[@]})
maxw=(${!wordc[@]})
min=${wordc[$minw]}
max=${wordc[$maxw]}

for i in "${!wordc[@]}"
do :
  	if (( ${wordc[$i]} > $max )); then
    	max=${wordc[$i]}
		maxw=$i
	fi
	if(( ${wordc[$i]} < $min )); then
    	min=${wordc[$i]}
		minw=$i
	fi
	#echo $i ${wordc[$i]}
done

echo "Total number of keywords searched: $total_words"
echo "Keyword most frequently found: $maxw [$max]" 
echo "Keyword least frequently found: $minw [$min]" 
