/*
 * Notes about these functions, from https://community.nxp.com/thread/389140 
 *
To retarget Redlib's printf(), you need to provide your own implementations of the function __sys_write():
int __sys_write(int iFileHandle, char *pcBuffer, int iLength)
Function returns number of unwritten bytes if error, otherwise 0 for success

Similarly if you want to retarget scanf(), you need to provide your own implementations of the function __sys_readc():
int __sys_readc(void)
Function returns character read

Note that these two functions effectively map directly onto the underlying "semihosting" operations.
*/



int sendchar(int x); // Function prototype
int getkey();        // Function prototype


// Function returns number of unwritten bytes if error, otherwise 0 for success
int __sys_write(int iFileHandle, char *pcBuffer, int iLength) {
  int nChars = 0;
  for (/* Empty */; iLength != 0; --iLength) {
    if (sendchar(*pcBuffer++) < 0) {
      //return -1;
      return (iLength - nChars);
    }
    ++nChars;
  }
  //return nChars;
  return 0;
}

// Function returns the character read
int __sys_readc(void) {
  return sendchar(getkey()); // Echo each character first
  //return getkey();         // Don't echo each character first
}
