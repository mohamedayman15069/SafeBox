# SafeBox
## Introduction
This repository contains code to encrypt and decrypt plain text using Advanced Encryption Standard (AES) and Cipher Block Chaining (CBC) mode. The purpose of this README is to provide an overview of the project and explain how to use it.

## Project Discussion 
### Encryption
To encrypt a plain text, follow these steps:

1. Call the class constructor of CBC_Mode with key and iv.
2. Read the key file and store the contents in a dynamic array of bytes. Generate a random iv.
3. Cipher the plain text by putting it in a StringSource function.
4. Use StreamTransformationFilter to pipe everything to StringSink.
5. Put the ciphered text inside the buffer.
6. Use MessageEnd to finalize everything.
7. Use a "for loop" to encrypt all blocks, and use a Spooler for this loop.
8. In each iteration of the loop, take a block and put it to the array of shreds.
9. Use Shred[i] = newShred(parameters) to create the constructor of shredManager.

### Decryption
To decrypt the ciphered text, follow these steps:

1. Decrypt the block using the Decrypt Block function.
2. In shredManager, take the shred[0]->appendblock(), decrypt it and use the same loop until b points to null.

### MultiThreading
To speed up the process, we have developed a multithreading feature. In this phase, we create various threads to handle importing or exporting file blocks to/from the various shreds at the same time. We also ensure the process is more secure by spreading the blocks to the various shreds in a random way. Each block is given an identity that includes the shred name, block number inside the shred, and the random block number in general compared to other block numbers.

The SafeBoxImport function creates two objects from the classes FileSpooler and MultiShredManager, which are used to open the target file, get its blocks, and call the encrypt function. Reading the blocks from the file, each thread (which is a shred) gets a random number from a LotteryScheduler object and accesses the corresponding block. Threads work on encrypting the accessed blocks and placing them into the shreds. We guarantee that blocks in the shreds are completely ordered randomly.

To know where each block relates to which location in the main file for decryption purposes, we created the multiheaderqueue. By each thread accessing a block, encrypting it, and putting it into a certain shred, data like the block number, shred number, and name are stored into a struct and saved in that queue. We also solved the problem of padding by using a while loop in the decrypted queue to get the number of encrypted files if the number of read_size(sz)% BLOCKSIZE != 0. We used ZERO PADDING to delete the padding idea in the decrypted function.

## Conclusion
This project provides a comprehensive solution to encrypt plain text using AES and CBC mode. The implementation is secure, efficient, and easy to use. The multithreading feature improves the processing time and the randomization feature adds an extra layer of security to the encryption process.
