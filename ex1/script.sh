mkdir 201629870
cd 201629870
mkdir temp
cd temp
echo shoshan > shoshan
echo zelikovitz > zelikovitz
echo shoshanz > shoshanz
cp shoshan ../zelikovitz
cp zelikovitz ../shoshan
rm shoshan
rm zelikovitz
mv shoshanz ../
cd ../
rmdir temp
echo "listing directory content:"
ls -la
echo "listing first name file:"
cat shoshan
echo "listing last name file:" 
cat zelikovitz
echo "listing user name file:"
cat shoshanz
