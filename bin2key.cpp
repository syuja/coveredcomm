/************
  Compile :  make bin2key
  Run: ./bin2key [saved_file]
  or make runClient

  Requires the Saved File
  Converts the Saved Received Binary Message into the Original SSH Key
************/

#include <iostream>
#include <string>
#include <vector>
#include <bitset>
#include <fstream>
  using namespace std;

vector<string> CLEAN_BINARYKEY;

#define BYTESIZE 8

/********************
*FUNCTIONS:
********************/
string printASCII(string binary );
void getKeyBack(vector<string> binaryKey);

int main(int argc, char** argv){
  //read file into CLEAN_BINARYKEY

  //open the file
  string FILE = argv[1];
  ifstream file;
  file.open(FILE);

  string line;

  while(getline(file,line)){
    //already in multiples of 8 (see client.cpp writeBinary)
    line.pop_back();
    CLEAN_BINARYKEY.push_back(line);
  }
  getKeyBack(CLEAN_BINARYKEY);

  return 0;
}

/********************
*CONVERT
********************/
//converts "1010101" ==> char
string printASCII(string binary ){
  //if(binary == "0000101") cout << " newline " << endl;
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
