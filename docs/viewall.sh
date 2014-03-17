# Please personalize this script for your system

DEV=/dev/fd0H720

for i in `qltools $DEV -s`
do
	echo -n $i:
	read yn

	if [ $yn = y ]
	then
		qtool $DEV $i | more
	fi
done

