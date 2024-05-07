./createzip.sh

echo -e "PROVIDED CHECKER \n --------------- \n"
./checker_os_p3.sh os_p3_100531523_100495775_100541510.zip
rm os_p3_100531523_100495775_100541510.zip
rm nota.txt
rm authors.txt
echo -e "######################## \n \n \n"

read -p "Run team test? y(yes), n(no) : " answer

if [$answer = 'y']; then

    echo -e "TEAM TEST \n  --------------- \n"
    ./test_1_test.sh

fi




