#include <ThreadManager.h>


ThreadManager::ThreadManager(){

}
void ThreadManager::operator += (Thread * t){
    threads.push_back(t);
}
void ThreadManager::start () // This function will call a number of threads. We know threads is a vector, so we need to use
{// iterators  
for ( auto x = threads.begin() ; x!=threads.end() ; x++) 
x[0]->start(); // So we call the function start to implement multi-threading
}
void ThreadManager::barrier (){
for ( auto x = threads.begin() ; x!=threads.end() ; x++) 
x[0]->wait();  //Join all of them with the for loop 
}
ThreadManager::~ThreadManager(){
}
