#include <ShredManager.h>
#include <ThreadManager.h>
#include <MultiHeadQueue.h>
ShredManager::ShredManager () // added this in phase 2
{
}

ShredManager::ShredManager(char * p_file_name, uint16_t p_block_size, uint16_t p_shred_count,bool truncate)
{
    shred_count = p_shred_count;
    shreds = (Shred ** ) calloc (shred_count,sizeof(Shred*));
    for ( char i = 0 ; i  < shred_count; i++)
    {
        string fname = p_file_name;
        fname.insert(fname.find('.'),1,i+'A');
        if (truncate)
            shreds[i] = new Shred(fname.c_str(),p_block_size,truncate);
        else shreds[i] = new Shred(fname.c_str(),(p_block_size+16)&~15,truncate);
    }
}
bool ShredManager::encrypt (FileSpooler * fileSpooler, const char * key_file_name, const char * iv_file_name)
{
    AutoSeededRandomPool prng;
    CryptoPP::byte key[ CryptoPP::AES::DEFAULT_KEYLENGTH ], iv[ CryptoPP::AES::BLOCKSIZE ];
    memset( key, 0x00, CryptoPP::AES::DEFAULT_KEYLENGTH );
    memset( iv, 0x00, CryptoPP::AES::BLOCKSIZE );
//std::cout << key << std::endl; 
    ifstream kf;
    kf.open(key_file_name,ios::in);
    if ( kf.is_open())
    {
        kf.read (reinterpret_cast<char*>(key),sizeof(key));
        kf.close();
    }
    prng.GenerateBlock(iv,sizeof(iv));
    Block * block = fileSpooler->getNextBlock();
    for (int i = 0 ;block != NULL; i ++)
    {
        block->encrypt(key,iv);
        *(shreds[i%shred_count]) << *block;
        delete (block);
        block = fileSpooler->getNextBlock();
    }
    ofstream f;
    f.open(iv_file_name,ios::out|ios::trunc);
    if ( f.is_open())
    {
        f.write (reinterpret_cast<const char*>(iv),sizeof(iv));
        f.close();
    }
    return true;
}
bool ShredManager::decrypt (FileSpooler * fileSpooler, const char * key_file_name, const char * iv_file_name)
{
    AutoSeededRandomPool prng;
    CryptoPP::byte key[ CryptoPP::AES::DEFAULT_KEYLENGTH ], iv[ CryptoPP::AES::BLOCKSIZE ];
    memset( key, 0x00, CryptoPP::AES::DEFAULT_KEYLENGTH );
    memset( iv, 0x00, CryptoPP::AES::BLOCKSIZE );
    ifstream f;
    f.open(key_file_name,ios::in);
    if ( f.is_open())
    {
        f.read (reinterpret_cast<char*>(key),sizeof(key));
        f.close();
    }

    f.open(iv_file_name,ios::in);
    if ( f.is_open())
    {
        f.read (reinterpret_cast<char*>(iv),sizeof(iv));
        f.close();
    }

    Block * block = NULL;
    for (int i = 0 ; i == 0 || block != NULL; i ++)
    {
        block = shreds[i%shred_count]->getNextBlock();
        if ( block == NULL) break;
        block->decrypt(key,iv);
        //block->print();
        fileSpooler->appendBlock(block);
        delete (block);
    }
    return false;
}
ShredManager::~ShredManager()
{
    for ( int i = 0 ; i  < shred_count; i++)
        delete(shreds[i]);
    free(shreds);
}
//*************

MultithreadedShredManager::MultithreadedShredManager(char * p_file_name, uint16_t p_block_size, uint16_t p_shred_count,bool p_truncate) : ShredManager()
{
    file_name = p_file_name;
    shred_count = p_shred_count;
    block_size = p_block_size;
    truncate = p_truncate;
    shreds = (Shred ** ) calloc (shred_count,sizeof(Shred*));
}
bool MultithreadedShredManager::encrypt (FileSpooler * p_fileSpooler, char * key_file_name,  char * iv_file_name, char * q_file_name)
{// This function is basically will do the following:
// I need at first to have on iv as required. 
//SO THIS PLACE is most suitable one WHY? SINCE IT IS THE Centeralized function to generate one iv and write it in the iv file name
// Then, this function will not be called again. Why not in the mainthreadbody as the doctor wrote? 
// MY logic says that mainbodythread will be called inside the for loop and it is inevitable. 
// Since this function depends on one shred only, so I will call it three times if I requested three shreds. 
    ThreadManager threadmanager; 
    Lottery lottery(p_fileSpooler->getBlockCount()); // The lottery is important to have a random ticket.
    MultiHeadQueue<sb_block_index_t> multiHeadQueue; // Queue is important to make everything randomizied
    AutoSeededRandomPool prng; // 
  CryptoPP::byte iv[ CryptoPP::AES::BLOCKSIZE];
    memset( iv, 0x00, CryptoPP::AES::BLOCKSIZE);
    prng.GenerateBlock(iv,sizeof(iv)); // Here is where I generate a random block
     ofstream f;
    f.open(iv_file_name,ios::out|ios::trunc); //
    if ( f.is_open())
    {
        f.write (reinterpret_cast<const char*>(iv),sizeof(iv)); //Write it in the file respecting to what the doctor wrote.
        //I can reopen them again, but the iv is safe now.
        f.close();
    }
   for ( char i =0 ; i<shred_count ; i++){   // Here is most important part. 
   // I need to call the EncryptShredThread function to each thread. Here is why I need to use the for loop.
              string fname = file_name;
        fname.insert(fname.find('.'),1,i+'A');// Changing the names of the shred files not to overwrite
       //std:: cout << iv <<std::endl;
        char pname = ('A'+i);
        if( truncate) // This if condition to enter into the first one.
shreds[i]= new EncryptShredThread(p_fileSpooler,key_file_name,iv_file_name,(char*)(fname.c_str()),block_size,&lottery,&multiHeadQueue,pname,truncate);
else  shreds[i]= new EncryptShredThread(p_fileSpooler,(key_file_name),iv_file_name,((char*)(fname.c_str())),(block_size+16)&~15,&lottery,&multiHeadQueue,pname,truncate);
threadmanager += dynamic_cast <Thread *> (shreds[i]); // I would like to make to each shread a thread right? So threads are vectors
// and we simply push_back to make the following happen ,, which ones?? start and barrier
   }
threadmanager.start(); // Here I say to the threads," Each one has an own stack and perform the code"
threadmanager.barrier(); // I need to wait and join not to crash the program
multiHeadQueue.dump(q_file_name,key_file_name,iv_file_name); // I need dump for the queue. The header decomunted line will 
//explain everything
    return true;    
}

bool MultithreadedShredManager::decrypt (FileSpooler * p_fileSpooler, char * key_file_name,  char * iv_file_name, char * q_file_name)
{// The decrypted is nearly the same as the encrypted but with a slight difference

ThreadManager thr;
MultiHeadQueue<sb_block_index_t> multiHeadQueue;
multiHeadQueue.load(q_file_name,key_file_name,iv_file_name);
// I need first to call the load function from the queue to decrypt the queue( having the ticket to each block: index, shred name
// and so on)
for(char i=0; i<shred_count; i++){
        string fname = file_name;
    fname.insert(fname.find('.'),1,i+'A');// SAME; I need to get them to call it in the decrypt. To get the right path

    char pname = ('A'+i);    
    if(truncate)
    shreds[i]= new DecryptShredThread(p_fileSpooler,key_file_name,iv_file_name,(char*)fname.c_str(),block_size,&multiHeadQueue,pname,truncate) ;
else shreds[i]= new DecryptShredThread(p_fileSpooler,key_file_name,iv_file_name,(char*)fname.c_str(),(block_size+16)&~15,&multiHeadQueue,pname,truncate) ;

thr+= dynamic_cast <Thread *> (shreds[i]); //SAME HERE for threads ( vectors as I mentioned in the encrypted multithread shred manager
//function)

}
thr.start(); //You need to start 
thr.barrier();//JOIN NOT TO CRASH

    return true;
}

MultithreadedShredManager::~MultithreadedShredManager()
{

}
