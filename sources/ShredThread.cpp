#include <ShredThread.h>

ShredThread::ShredThread (FileSpooler * p_fileSpooler,  char * p_key_file_name,  char * p_iv_file_name, const char * p_file_name,uint16_t block_size,MultiHeadQueue  <sb_block_index_t> * p_multiHeadQueue,char p_shred_name,bool truncate):Thread(), Shred(p_file_name,block_size,truncate)
{
        srcFileSpooler = p_fileSpooler;
        key_file_name = p_key_file_name;
        iv_file_name = p_iv_file_name;
        multiHeadQueue = p_multiHeadQueue;
        shred_name = p_shred_name;
}
void ShredThread::mainThreadBody(){}
ShredThread::~ShredThread(){}

EncryptShredThread::EncryptShredThread (FileSpooler * p_fileSpooler,  char * p_key_file_name,  char * p_iv_file_name,const char * p_file_name,uint16_t block_size,Lottery * p_lottery,MultiHeadQueue  <sb_block_index_t> * p_multiHeadQueue,char p_shred_name,bool truncate) : ShredThread(p_fileSpooler, p_key_file_name, p_iv_file_name,p_file_name,block_size,p_multiHeadQueue,p_shred_name,truncate)
{
    lottery = p_lottery;
}
void EncryptShredThread::mainThreadBody()
{ // I am inside the program which we need to assure its logic is right. It is the core of the program 
// It will simply take each block , encrypt it and then give a ticket not to be somewhere we do not know.
CryptoPP::byte key[ CryptoPP::AES::DEFAULT_KEYLENGTH]; // I need to open the key 
      memset( key, 0x00, CryptoPP::AES::DEFAULT_KEYLENGTH); // Intalization
        CryptoPP::byte iv[ CryptoPP::AES::BLOCKSIZE]; // IV
            memset( iv, 0x00, CryptoPP::AES::BLOCKSIZE);

    ifstream kf,f;
    kf.open(key_file_name,ios::in); //we need to open the key and iv
    if ( kf.is_open())
    {
        kf.read (reinterpret_cast<char*>(key),sizeof(key));
        kf.close();
    }
     f.open(iv_file_name.c_str(),ios::in);
    if ( f.is_open()) // We can figure out why it is inevitable to generate in the Multi_thread_shred_manager
    {
        f.read (reinterpret_cast<char*>(iv),sizeof(iv));
        f.close();
    }

   
     //std::cout << sizeof(iv)<< " "<< iv << std::endl; 
    // std::cout << sizeof(key) << "  "<< key<< std::endl; 

long l = lottery->withdraw(); // I am taking the ticket randomly
sb_block_index_t e; // Yes this is the struct which will help me to give to each block a source.
e.shred= shred_name; // I need to know the shred name A or B or C or D and so on

for ( int i = 0;l!=-1 ;i++) // This loop does the following: I need to have a block; encrypt it 
{Block* b=(*srcFileSpooler)[l];
b->encrypt(key,iv); // it is here yes
*(this) << *b;// append it using the operator overloading. With this, I mean in this class or function
e.block= l; // Now I would like to destinguish between blocks. So giving the e.block relating to the l ( the ticket from withdraw)
//of the lottery
//std::cout << e.block<<std::endl;
e.shred_block = i; // Which block in the shred
//std::cout << e.shred_block<<std::endl;
//  cstd::cout << e.shred << std::endl; 
multiHeadQueue->enqueue(e); // Then enqueue. Meaning that Put the index it in a vector::data in the enqueue( where 
// multiheadqueue class inherits the vector class)
//delete(b); 
l = lottery->withdraw();// Again the withdraw
//How does withdraw work?? Basically, imagine we have 10 numbers, so the lottery withdraw possiblity to give to us any number from 
//0 to 9 . Take a number then 0 to 8 .. Take a number from 0 to 7 and so on... till -1 here I tell it stop! 
}
  
}
EncryptShredThread::~EncryptShredThread()
{

}

DecryptShredThread::DecryptShredThread (FileSpooler * p_fileSpooler,  char * p_key_file_name,  char * p_iv_file_name,const char * p_file_name,uint16_t block_size,MultiHeadQueue  <sb_block_index_t> * p_multiHeadQueue,char p_shred_name,bool truncate): ShredThread(p_fileSpooler, p_key_file_name, p_iv_file_name,p_file_name,block_size,p_multiHeadQueue,p_shred_name,truncate)
{
    

}
Block * DecryptShredThread::operator [] (int index)
{
    return (*fileSpooler)[index];
}

void DecryptShredThread::mainThreadBody()
{ // It is nearly the same as the encryptedshredthread function
    AutoSeededRandomPool prng;
    CryptoPP::byte key[ CryptoPP::AES::DEFAULT_KEYLENGTH];
     CryptoPP::byte iv[ CryptoPP::AES::BLOCKSIZE ];
    memset( key, 0x00, CryptoPP::AES::DEFAULT_KEYLENGTH);
    memset( iv, 0x00, CryptoPP::AES::BLOCKSIZE );
    // I need to open both of them. 
    //Here is why I cannot generate the iv in the encryptedmainbody. This decrypted will choose which one?? Of course
    //The last one because they overwrite each other; this is why we need to generate in the threadmanager
    ifstream f;
    f.open(key_file_name,ios::in);
    if ( f.is_open())
    {
        f.read (reinterpret_cast<char*>(key),sizeof(key));
        f.close();
    }
    f.open(iv_file_name.c_str(),ios::in);
    if ( f.is_open())
    {
        f.read (reinterpret_cast<char*>(iv),sizeof(iv));
        f.close();
    }

//std::cout << sizeof(iv)<< " "<< iv << std::endl; 
   //  std::cout << sizeof(key) << "  "<< key<< std::endl; 

sb_block_index_t e;
e.shred=shred_name;

while(true){// I have infinite loop, which will stop if the dequeue is empty or the block return is null
bool c = false;
 c=multiHeadQueue->dequeue(e,[](sb_block_index_t &e1, sb_block_index_t &e2)->bool{ // This lambda function helps me to compare
 //between two index. Because if there is not. It is empty, so false!
    if(e1.shred == e2.shred) return true; 
    else return false;
});

if(!c) break; 
//std::cout << e.shred_block<< std::endl;
Block* b = (*this)[e.shred_block]; // I need to get the function, and its ticket by dequeuing
if ( b == NULL) break; // If null, so break to be out of the loop
//std::cout << b << std::endl; 
b->decrypt(key,iv);// Then decrypt this block
srcFileSpooler->writeBlockAt(b,e.block);// And write the block and store it 
delete(b); // and return the b to NULL

}


}
DecryptShredThread::~DecryptShredThread()
{
    
}
