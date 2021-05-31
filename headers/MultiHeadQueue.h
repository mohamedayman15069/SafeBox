#ifndef MULTIHEADQUEUE_H
#define MULTIHEADQUEUE_H

#include <includes.h>
#include <defines.h>
#include <vector>
#include <thread>
#include <fstream>

// The implemented functions are besically trying to make the text harder to be decrypted. How? We are using a random way to the blocks. Giving each block a ticket and
// storing the blocks' identities in a queue. After then, I need to encrypt this queue and then decrypt it.
using namespace std;

template <typename T>
class MultiHeadQueue: private std::vector<T>{
    
    private:
        mutex mtx;
    public:
        MultiHeadQueue ():vector<T>(){
        }
        void enqueue(T & t)
        {
            mtx.lock();
            vector<T>::push_back(t);
   mtx.unlock();

        }
        bool dequeue(T & t,std::function <bool (T&,T&)> check)
        { // The LAMBDA FUNCTION
            mtx.lock();
            T e; //  I need to check if we find any identity of these blocks in the queue or not
            for (auto x = vector<T>::begin() ;x != vector<T>::end();x++)
            {
if(check(x[0],t)){ // IF YES
t =x[0];//SO GIve it the identity (TICKET)
vector<T>::erase(x);//DELETE IT FROM THE QUEUE.. I do not need it anymore

            mtx.unlock(); //mutex not to enter the threadings each other
return true;
            }
}
            mtx.unlock();
            return false;
        }
        void dump (char * filename,char * p_key_file,char * p_iv_file) //Send an email
        {// DUMP is basically.. Takes the identies and then encrypt them
            mtx.lock();
ifstream a; 
a.open(p_key_file,ios::in);  // I need to open the key and iv
CryptoPP::byte  key[CryptoPP::AES::DEFAULT_KEYLENGTH];
CryptoPP::byte* iv = new byte[CryptoPP::AES::BLOCKSIZE]; 
 if ( a.is_open())
    {
        a.read (reinterpret_cast<char*>(key),CryptoPP::AES::DEFAULT_KEYLENGTH);
        a.close();
    }
a.open(p_iv_file,ios::in); 
 if ( a.is_open())
    {// READ IT
        a.read (reinterpret_cast<char*>(iv),CryptoPP::AES::BLOCKSIZE);
        a.close();
    }

        uint32_t read_size= vector<T>::size()*sizeof(T); //THEN; I need to know the size of the queue, which is the size of the vector and each one has size of struct
   std::string ciphertext; //Typical as the block No thing difference
    CryptoPP::AES::Encryption aesEncryption(key, CryptoPP::AES::DEFAULT_KEYLENGTH);
    CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption( aesEncryption, iv );
    CryptoPP::StreamTransformationFilter stfEncryptor(cbcEncryption, new CryptoPP::StringSink( ciphertext ) );
    stfEncryptor.Put( reinterpret_cast<const unsigned char*>( vector<T>::data() ),read_size );// BE careful the vector data is point of integar, so it needs casting
    stfEncryptor.MessageEnd(); // I need to get all of them
    memcpy (vector<T>::data(),ciphertext.c_str(),ciphertext.size()); // copy all of them inside the vector<T>::data


            ofstream f;
            f.open(filename,ios::out|ios::trunc);
            if ( f.is_open())
          { //And overwrite in the queue file
               f.write(reinterpret_cast<char*>(vector<T>::data()),vector<T>::size()*sizeof(T));
f.close();
}


mtx.unlock(); 

        }
        void load (char * filename,char * p_key_file,char * p_iv_file)
        {
            mtx.lock();
            ifstream f;

            f.open(filename,ios::in);
            if ( f.is_open())
            { // I need to know the size of the whole file
                f.seekg(0,f.end);
                long sz = f.tellg();
                f.seekg(0,f.beg);
                T * buffer = (T *) calloc(sz/sizeof(T),sizeof(T));
                f.read (reinterpret_cast<char*>(buffer),sz);
                f.close();
while(sz%CryptoPP::AES::BLOCKSIZE!=0){ //This is while loop to handle the padding by ourselves. 
   sz++;    // ++ till get the number as the encrypted to solve the padding

}
ifstream fi;
uint32_t read_size=vector<T>::size()*sizeof(T);
CryptoPP::byte key[CryptoPP::AES::DEFAULT_KEYLENGTH];
CryptoPP::byte * iv  = new byte [CryptoPP::AES::BLOCKSIZE];
f.open(p_key_file,ios::in);
fi.open(p_iv_file,ios::in);// Open both of them and read them in the key and iv
f.read(reinterpret_cast<char*>(key),CryptoPP::AES::DEFAULT_KEYLENGTH);
fi.read( reinterpret_cast<char*>(iv),CryptoPP::AES::BLOCKSIZE); 
std::string decryptedtext;
    CryptoPP::AES::Decryption aesDecryption(key, CryptoPP::AES::DEFAULT_KEYLENGTH);
    CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption( aesDecryption, iv);
    CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink( decryptedtext ),CryptoPP::StreamTransformationFilter::ZEROS_PADDING );
//Here is the thing. I need to delete the padding
    stfDecryptor.Put( reinterpret_cast<const unsigned char*>( buffer ), sz);
    stfDecryptor.MessageEnd();
    memmove(buffer,decryptedtext.c_str(),sz); // And put the decrypted in the buffer 
              sz/=sizeof(T);
                for ( int i = 0 ; i < sz ; i++)
                    vector<T>::push_back(buffer[i]);
                free (buffer);
            }
            mtx.unlock();
        }
        ~MultiHeadQueue(){}
};



#endif
