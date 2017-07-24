/************
  Compile :  make key
  Run: ./key [number of ports to use] [paths to private SSH keys]

  It accepts multiple paths, but will use the first functional one.

  Functions:
  Converts SSH Private Key into Binary.
  Printing it for transmission (trailing 0 to aid transmission) by blink.cpp.
******/

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <bitset>
#include <algorithm>
  using namespace std;

/********************
*GLOBAL VARIABLES:
********************/
#define BYTESIZE 8                  //for conversion
vector <string> KEYPATHS;
int NUM_PORTS = 8;                  //default
vector <vector <string>> BINARYKEY; //holds final key in binary multiples

/********************
*FUNCTIONS:
********************/
//Finding, Validate and Save Key Paths
void parseArgs(int arg_count, char** argv);
void checkPaths();

//Reading and Converting the Key to Binary
void readConvertKey();
string convertToBinary(char c);

//Transmit the Message in Multiples of NUM_PORTS
void transmitMssg();
string transmitLine(vector<string> binaryLine);


//UTILITY:
template<typename T>
void printVector(vector<T> vec);

template<typename T>
void printVecVector(vector<T> vec);



int main(int argc, char** argv){
  //parse arguments: NUM_PORTS and KeyPaths
  parseArgs(argc, argv);

  //checks given path is a private ssh key file
  checkPaths();

  //reads line by line converting char ==> int ==> binary string
  readConvertKey();

  //prints in multiples according to NUM_PORTS
  transmitMssg();

  return 0;
}


/********************
*PARSE and VALIDATE ARGS
********************/
//validates input and saves globally
void parseArgs(int arg_count, char** argv){
  //need at least 3 arguments: NUM_PORTS and 1 path
  if(arg_count < 3){
    cerr << "Error keyconv.cpp: parseArgs not enough Arguments" << endl;
    exit(1);
  }

  //saves the number of ports
  NUM_PORTS = stoi(argv[1]);

  //multiple of 8?
  if(NUM_PORTS % 8 !=0){
    cerr << "Error keyconv.cpp: parseArgs NUM_PORTS not multiple of 8" << endl;
    exit(1);
  }

  //save paths
  for(int i = 2; i < arg_count; ++i){
    KEYPATHS.push_back(argv[i]);
  }
}

//check provided paths
void checkPaths(){
  //remove invalid paths
  auto it = KEYPATHS.begin();
  //using stat for speed
  struct stat buf;
  //read first line
  string firstline;

  while(it != KEYPATHS.end()){
    //1. can it be opened?
    if(stat(it->c_str(), &buf) == -1){
      //file not found
      it = KEYPATHS.erase(it);
      continue;
    }

    //2. contains a private key
    ifstream sshkey_file(it->c_str());
    firstline.clear();
    //get first line
    getline(sshkey_file, firstline);

    //first line must be contain "---PRIVATE KEY---"
    if(firstline.find("PRIVATE KEY", 0) == string::npos){
      //not a private key
      sshkey_file.close();
      it = KEYPATHS.erase(it);
      continue;
    }

    sshkey_file.close();
    ++it;
  }

  //if no valid paths provided, then exit
  if(KEYPATHS.empty()){
    cerr << "Error keyconv.cpp: checkPaths no valid SSH Key Paths provided" << endl;
    exit(1);
  }
}

/********************
*READ and CONVERT KEY
********************/
void readConvertKey(){
  //open first file containing a key
  ifstream sshkey_file(KEYPATHS.front().c_str());
  string line;
  vector<string> binaryLine;

  //read a line
  while(getline(sshkey_file, line)){
    //for each letter
    for(char c : line){
      binaryLine.push_back(convertToBinary(c));
    }
    //push newline since getline discards it
    binaryLine.push_back(convertToBinary('\n'));
    //save it in binary
    BINARYKEY.push_back(binaryLine);
    //clear prev values
    line.clear();
    binaryLine.clear();
  }
}

//char ==> int ==> binary string
string convertToBinary(char x){
  //convert char to int
  int temp_int = (int) x;

  //convert int to binary
  string binary = bitset<BYTESIZE>(x).to_string();
  binary[0] = '1';
  //rotate for easy conversion by client
  rotate(binary.begin(), binary.begin()+1, binary.end());

  return binary;
}

/********************
*TRANSMIT
********************/
void transmitMssg(){
    for(int line_num = 0; line_num < BINARYKEY.size(); ++line_num){
      //calls helper
      string temp = transmitLine(BINARYKEY[line_num]);
      cout << temp;
    }
}

//transmitMssg helper
string transmitLine(vector<string> binaryLine){
  int limit = NUM_PORTS/8;
  string tmp = "";

  //places as many characters as possible for transmission at once!
  int pos = 0;
  while(pos <= binaryLine.size() - limit){
    for(int i = pos; i < limit+pos; ++i){
      tmp += binaryLine[i];
    }
    tmp += '\n';
    pos = pos + limit;
  }

  int tmp_pos = pos;
  while(pos < binaryLine.size()){
    tmp+= binaryLine[pos];
    ++pos;
  }

  if(tmp_pos < binaryLine.size()) tmp+= '\n';

  return tmp;
}

/********************
*UTILITY
********************/
//prints a vector
template<typename T>
void printVector(vector<T> vec){
  for( auto it = vec.begin(); it != vec.end(); ++it)
    cout << *it << endl;
}

//print vector of vector
template<typename T>
void printVecVector(vector<T> vec){
  for(int i = 0; i < vec.size(); ++i){
    //send to printVector
    printVector(vec[i]);
  }
}
