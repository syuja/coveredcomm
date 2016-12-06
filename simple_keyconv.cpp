/*****
compile :  g++ -std=c++11 simple_keyconv.cpp

CHANGE:
PRODUCEPATHS <== TO ADD LOCATIONS OF SSH KEYS
getBinBack <== in the main loop to only print one key

Additional Functions are Located in keyconv.cpp

Scans computer for ssh keys.
Saves them in a vector of vectors
Converts them to Character then to Int
Finally produces bits of a key

******/

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <bitset>
#include <cstdlib>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>


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

/**********
FUNCTIONS:
**********/
//FINDING AND SAVING KEYS
void producePaths();                      //produce path guesses
void findSSHFile();                       //find .ssh, push path into allpaths
void dirFound(vector<string> &possibleP); //remove paths that don't exist
void fill2DAllKeys();                     //put the key into
vector<string> readKey(string keypath);   //read private key, push key allkeys


//CONVERT TO BINARY
void keyToChar(vector<string> key);      //char by char 2d vector
void charToInt();                        //convert char_key to int
string printBinary(int x);              //returns integer as binary string
void getBinBack(vector<string> binaryKey);

//PRINT
void printVector2(vector<vector<string> > vec);

int main(){
  //guesses of .ssh directory
  producePaths();
  //tests guesses of .ssh directories
  findSSHFile();

  //store keys from .ssh directories
  fill2DAllKeys();

  /******************************
      Convert Line to char by char
      "---BEGIN PRIVATE KEY" ==> ["-","-","-","B","E"...]
      then char by char to int by int
      ["-","-","-","B","E"...] ==> [16, 16, 16, 68, 90....]
  ******************************/

  for(int k = 0; k < allKeys.size(); ++k){
    //char by char
    keyToChar(allKeys[k]);
    //int by int
    charToInt();

    //convert int to binary and store in 3D array ALLBINARYKEYS
    //1D is for Key, 2D is for line in key, 3D is for character in that line
    //oneline is 1 line of a key
    vector<string> oneline;
    //bkey stanza is 1 whole key
    vector<vector <string> > bkey_stanza;
    for(int j = 0; j < int_key.size(); ++j){
      oneline.clear();
      for(int i = 0; i < int_key[j].size(); ++i){
        //print Binary converts 67 ==> 0101010101
        oneline.push_back(printBinary(int_key[j][i]));
        }
        bkey_stanza.push_back(oneline);
      }
      ALLBINARYKEYS.push_back(bkey_stanza);
    }

    //HACK: print one keys
    for(int j = 0; j < ALLBINARYKEYS[0].size(); ++j){
      getBinBack(ALLBINARYKEYS[0][j]);
    }
 cout << "exit" << endl;
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
  string path3 = "/home/syuja/.ssh";

  possibleSSHPaths.push_back(path1);
  possibleSSHPaths.push_back(path2);
  possibleSSHPaths.push_back(path3);

	vector<string> temp_possiblePaths;
  //erase entries that don't exist
  dirFound(possibleSSHPaths);
  if(possibleSSHPaths.empty()){
		cerr << "Paths Do Not Exist" << endl;
	  //print possible paths
	  for(int i = 0; i < temp_possiblePaths.size(); ++i){
	  	cerr << "Path Doesn't Exist: " << temp_possiblePaths[i] << endl;
	  }
	  cerr << "Please Add More Paths to producePaths(): possibleSSHPaths" << endl;
	  exit(1);
  }
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
  if(allKeyPaths.empty()){
  	cerr << "findSSHFile(): no ssh file found" << endl;
  	cerr << "Please 1) add more possibleSSHPaths to ProducePaths()\n" <<
  	"or 2) create an ssh key in one of the possibleSSHPaths" << endl;
  	exit(1);
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

      //push it into allKeys
      allKeys.push_back(temp);
  }
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
  rotate(binary.begin(), binary.begin()+1, binary.end());

  return binary;
}


void getBinBack(vector<string> binaryKey){
  for(int i = 0; i < binaryKey.size(); ++i){
    cout << binaryKey[i] << endl;
  }
}



//PRINT
void printVector2(vector<vector<string> > vec){
  cout << "entered printVector2" << endl;
  if(vec.empty()){
  	cout << "allkeys is empty!!!!" << endl;
  	return;
  }
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
