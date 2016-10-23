
## Key Finder, Encoder and Decoder:  
--- 

### Summary of Functions:   
#### Setup Functions:  
  1. producePaths(): creates strings guessing where the .ssh file is located   
  2. findSSHFile, dirFound check producePaths output to verify that the .ssh file exists   
  3. fill2DAllKeys finds private ssh keys starting with "PRIVATE KEY", reads and saves them to `allKeys`   
  
#### Converting Functions:  
  1. keyToChar converts key into vector of vector line-by-line and char by char   
  2. charToInt converts keyToChar output into integers   
  3. finally, ints are converted into binary string "00111010" and placed into `ALLBINARYKEYS`   
    - vector of vector of vector  
      - first vector is by key
      - second is by line  
      - third is by string   
      
#### Get ASCII Key Back:  
getKeyBack function returns the ascii key from the binary representation.  


#### Utility Function: 
isBitOn(int pos, string Binary or int) : takes a position and a binary string or integer and returns 1 if the bit is 1 at that position  

