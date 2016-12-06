/******
Compile:
g++ -std=c++11 bin2key.cpp -o bin2key
Input:
 cat bin_file.txt | ./bin2key

*******/
#include<iostream>
#include<string>
#include<vector>
#include <bitset>

  using namespace std;

#define BYTESIZE 8

string printASCII(string binary );
void getKeyBack(vector<string> binaryKey);

int main(){
    vector<string> binaryKey;
    string temp;
    //read from stdin
    while(getline(cin, temp)){
      //push back into the vector
      temp.pop_back(); //remove the extra one at the end
      binaryKey.push_back(temp);
    }
    //call getKeyBack
    binaryKey.pop_back();

    cout << "calling getKeyBack" << endl;
    getKeyBack(binaryKey);
    return 0;
}
//transform text file into a key and print it!!
string printASCII(string binary ){

  unsigned long x  = bitset<BYTESIZE>(binary).to_ulong();
  char castchar[2];
  castchar[0]= (char) x; castchar[1] = '\0';
  string hello = string(castchar);
  return hello;
}

//call printASCII on each character
void getKeyBack(vector<string> binaryKey){

  for(int i = 0; i < binaryKey.size(); ++i){
    cout << printASCII(binaryKey[i]);
  }
}
